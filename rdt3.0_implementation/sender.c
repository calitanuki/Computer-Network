#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
//#include <signal.h>
//#include <setjmp.h>
#include <sys/select.h>
//static void sig_alrm(int);
//static jmp_buf env_alrm;

/*************** modified a little content to make it work 
 *************** now only remains problems with error check
 ***************/

int main(int argc, char *argv[])
{
		struct timeval time;
		fd_set filefd;      //use for select
		int rest;			//rest of time
        int sockfd;			//use for socket
		int n=0;
    	int fd;				//use for openfile
		char data[30];
		char seqdata[31];
		char checksum;
        char sendline[50];
        char recvline[10];
		char a;  //length
		int i;
		char recv1;
		char recv2;
		char recv3;
		char recv4;
		int count=0;
		fd = open (argv[3], O_RDONLY );
        struct sockaddr_in serverAddress;
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);

        bzero(&serverAddress, sizeof(serverAddress));
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(atoi(argv[2]));
			
        inet_pton(AF_INET, argv[1], &serverAddress.sin_addr);
	int server_len = sizeof(serverAddress);
		
		//if(signal(SIGALRM,sig_alrm)==SIG_ERR)
		//		printf("signal alarm error\n");
		//if(setjmp(env_alrm)!=0)
		//		printf("read timeout");
		bzero(data, sizeof(data));
        while(1)
		{
				//format size(1byte)+seq(1byte)+data(30bytes)+checksum(1byte)
				if ((a=read( fd, data, sizeof(data))) <= 0) 		break;
				//wait for call 0
				printf("**********STATE: wait for call 0\n");
				bzero(seqdata,sizeof(seqdata));
				checksum = a+2;
				//sprintf(seqdata, "%c%s", '0', data);
				seqdata[0]='0';
				for(i=1;i<=a;i++)	seqdata[i]=data[i-1];	
				printf("seqdata = %s\n", seqdata);
				//get checksum
				for(i=0;i<=a;i++)	checksum+=seqdata[i];
				checksum=~checksum;
				while(1)
				{
						FD_ZERO(&filefd);
						FD_SET(sockfd,&filefd);
						time.tv_sec=0;
						time.tv_usec=500000;
						printf("checksum value=%d\n",checksum);
						//sndpkt=make_pkt(0,data,checksum)
						bzero(sendline,sizeof(sendline));
						sendline[0]=a+2;
						for(i=0;i<=a;i++)	sendline[i+1]=seqdata[i];
						sendline[a+2]=checksum;
						printf("packet length = %d , sendline = %s\n",a+2,sendline);
						//udt_send(sndpkt)
						sendto(sockfd,sendline,a+3,0,(struct sockaddr *) &serverAddress, sizeof(serverAddress));
						printf("sndpkt=%s\n",sendline);
						//wait for ACK 0
						printf("**********STATE: wait for ACK 0\n");
						while((rest=select(sockfd+1,&filefd,NULL,NULL,&time))>0)
						{	
								//rdt_rcv(rcvpkt)
								FD_ZERO(&filefd);
								FD_SET(sockfd,&filefd);
								time.tv_sec=0;
								time.tv_usec=500000;
								if((n=recvfrom(sockfd,recvline,4,0,(struct sockaddr*) &serverAddress, &server_len))<0)
										printf("read error\n");
								recvline[n]=0;
								recv1=recvline[0];  //recvline datasize
								recv2=recvline[1];  //recvline seq#
								recv3=recvline[2];  //recvline A
								recv4=recvline[3];  //recvline checksum	
						}
						printf("recv1=%d\n",recv1);
						printf("recv2=%d\n",recv2);
						printf("recv3=%c\n",recv3);
						printf("recv4=%d\n",recv4);
						if(((recv1+recv2+recv3+recv4)==-1)&&recv2=='0')
						{		
								count++;
								printf("success packet %d\n",count);
								break;
						}
						else if( recv2 != '0' )	printf("timeout: packet loss in state 0\n");
						else if( (recv1+recv2+recv3+recv4) != -1 )	printf("timeout: checksum error in state 0\n");
						printf("rest0=%d\n",rest);
				}
				//if(fgets(data,30,fd)==NULL)		break;
				if ((a=read( fd, data, sizeof(data))) <= 0) 		break;
				//format size(1byte)+seq(1byte)+data(30bytes)+checksum(1byte)
				//wait for call 1
				printf("**********STATE: wait for call 1\n");
				bzero(seqdata,sizeof(seqdata));
				checksum=a+2;//32
				//sprintf(seqdata,"%c%s",'1',data);
				seqdata[0]='1';
				for(i=1;i<=a;i++)	seqdata[i]=data[i-1];
				printf("seqdata = %s\n", seqdata);
				//get checksum
				for(i=0;i<=a;i++)	checksum+=seqdata[i];
				checksum=~checksum;
				while(1)
				{
						FD_ZERO(&filefd);
						FD_SET(sockfd,&filefd);
						time.tv_sec=0;
						time.tv_usec=500000;
						printf("checksum value=%d\n",checksum);
						//sndpkt=make_pkt(0,data,checksum)
						bzero(sendline,sizeof(sendline));
						sendline[0]=a+2;
						for(i=0;i<=a;i++)	sendline[i+1]=seqdata[i];
						sendline[a+2]=checksum;
						printf("packet length = %d , sendline = %s\n",a+2,sendline);
						//udt_send(sndpkt)
						sendto(sockfd,sendline,a+3,0,(struct sockaddr *) &serverAddress, sizeof(serverAddress));
						printf("sndpkt=%s\n",sendline);
						//wait for ACK 1
						printf("**********STATE: wait for ACK 1\n");
						while((rest=select(sockfd+1,&filefd,NULL,NULL,&time))>0)
						{
								//rdt_rcv(rcvpkt)
								FD_ZERO(&filefd);
								FD_SET(sockfd,&filefd);
								time.tv_sec=0;
								time.tv_usec=500000;
								if((n=recvfrom(sockfd,recvline,4,0,(struct sockaddr *) &serverAddress, &server_len))<0)
								printf("read error\n");
								recvline[n]=0;
								recv1=recvline[0];  //recvline datasize
								recv2=recvline[1];  //recvline seq#
								recv3=recvline[2];  //recvline A
								recv4=recvline[3];  //recvline checksum
						}
						printf("recv1=%d\n",recv1);
						printf("recv2=%d\n",recv2);
						printf("recv3=%c\n",recv3);
						printf("recv4=%d\n",recv4);
						if(((recv1+recv2+recv3+recv4)==-1)&&recv2=='1')
						{		
								count++;
								printf("success packet %d\n",count);
								break;
						}
						else if( recv2 != '1' )	printf("timeout: packet loss in state 1\n");
						else if( (recv1+recv2+recv3+recv4) != -1 )	printf("timeout: checksum error in state 1\n");
				}
				
        }
		close(sockfd);
        close(fd);
        return 0;
}
/*static void sig_alrm(int signo)
{
	longjmp(env_alrm,1);
}
*/
