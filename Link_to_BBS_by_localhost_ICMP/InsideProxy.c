#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <linux/icmp.h>
#include <linux/ip.h>
#include <pthread.h>

//Global socket
//TCP with localhost:5000
int listenfd, localfd;
socklen_t length;
struct sockaddr_in serverAddress, clientAddress;

//ICMP
int proxyfd;
struct sockaddr_in proxyAddress;

//construct the icmp header
struct icmphdr *icmphd;

void *graphic()
{
	char recvline[1200];
	int n;
	int i;
	char screen[1030];
	while(1)
	{
        usleep(300000);
		n=recvfrom(proxyfd,recvline,sizeof(recvline),0,NULL,NULL);
		//filter the ping
		if(recvline[28]=='p'&&recvline[29]=='t'&&recvline[30]=='t')
		{
			for(i=31;i<n;i++)
			{
				screen[i-31]=recvline[i];
			}
			if(write(localfd,screen,n-31)<0)
				printf("write to localhost error\n");
            bzero(recvline,sizeof(recvline));
		    bzero(screen,sizeof(screen));				
		}
	}
}

int main(int argc, char *argv[])
{
    //TCP with localhost:5000
    char buffer[1000];   //use for data
	char header[8];     //use for icmp header
    char packet[1200];   //use for the whole packet
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&serverAddress, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(5000);
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(listenfd, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
    listen(listenfd, 1);

	//read from LOCAL   sendto Outside proxy
	int n;
    length = sizeof(clientAddress);
    localfd = accept(listenfd, (struct sockaddr *) &clientAddress, &length);
    printf("connection accept\n");
	
	
    //ICMP
	//short check=0;
	proxyfd = socket(AF_INET,SOCK_RAW,IPPROTO_ICMP);
	bzero(&proxyAddress, sizeof(proxyAddress));
	proxyAddress.sin_family = AF_INET;
	inet_pton(AF_INET,"192.168.0.2", &proxyAddress.sin_addr);
	printf("icmp set complete\n");	
	
	if(localfd>0)
	{
		icmphd = (struct icmphdr *)(header);
		icmphd->type = 8;
		icmphd->code = 0;
		icmphd->checksum = 0;
		memcpy(packet,icmphd,sizeof(struct icmphdr));
		memcpy(packet+sizeof(struct icmphdr),"p1",2);
		sendto(proxyfd,packet,sizeof(packet),0,(struct sockaddr *) &proxyAddress, sizeof(proxyAddress));
		
	}
	//nonblocking set
	int val = 1;
    ioctl(localfd, FIONBIO, &val);
	
	int pval = 1;
    ioctl(proxyfd, FIONBIO, &pval);	
	
	//create thread (get message from other clients)
    pthread_t tid;
    int err;
    err=pthread_create(&tid,NULL,graphic,NULL);
    if(err!=0)	printf("can't create thread\n");
	
	//ICMP sequence number initials to 0
	//read from localhost send to outside proxy
    while(1){
        usleep(300000);
		if((n=read(localfd,buffer,sizeof(buffer)))>0){
			icmphd = (struct icmphdr *)(header);
			icmphd->type = 8;
			icmphd->code = 0;
			icmphd->checksum = 0;
			//icmphd->un.echo.id = (short)(getpid()&0xffff);
			memcpy(packet,icmphd,sizeof(struct icmphdr));
			memcpy(packet+sizeof(struct icmphdr),buffer,n);
			sendto(proxyfd,packet,n+8,0,(struct sockaddr *) &proxyAddress, sizeof(proxyAddress));
			bzero(buffer,sizeof(buffer));
			
		}
    }
	
	//when finished, close all of the descriptor
	close(listenfd);
	close(localfd);
	close(proxyfd);

    return 0;
}

