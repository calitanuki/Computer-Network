#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <netdb.h>
#include <netinet/in.h>

char compute_checksum ( char leng ,char no, char SEN_checksum, char* datum , int n ){
    char cs = 0, value;
    
    int i;
    for( i = 0; i < n; i++ )	cs = cs + datum[i];
    
    printf("leng = %d, no = %d, SEN = %d, cs = %d\n", leng, no, SEN_checksum, cs);
    value = cs + no + leng + SEN_checksum ;
    printf("value = %d\n", value);
    return value;
}

int main(int argc, char* argv[]){
    
    int sockfd, tmp = 1, count = 0;
    struct sockaddr_in serverAddress, clientAddress;    
    
    // create the listen socket
	if((sockfd = socket( AF_INET, SOCK_DGRAM, 0 )) < 0)
    {   fprintf(stderr,"Server-socket error.\n");   exit(1);    }
	// add is already used
	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(int)) == -1)
    {   fprintf(stderr,"Server-setsockopt() error.");exit(1);	}	
	
	// set the serverAddress
    bzero(&serverAddress, sizeof(serverAddress));   
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(atoi(argv[1]));
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);

    // start to bind (to listensocket)
    if(bind(sockfd, (struct sockaddr *) &serverAddress , sizeof(serverAddress)) < 0)
	{	fprintf(stderr,"Server-bind error.\n");      exit(1);    }

    // no need to listen and accept	
    int client_len = sizeof(clientAddress);
    printf("receiver is ready!\n");
    
    int n, oncethru = 0, fd;
    char buf[33], data[30], sendback[4];
    char checksum, seq_no, REC_checksum, REC_len = 3;
    
    fd = open( "data_trans", O_CREAT | O_RDWR | O_APPEND, 0666 );
    
    while(1){
            while(1)    {   // wait for 0 from below
		printf( "STATE: in state 0, wait for pkt 0\n");
                n = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *) &clientAddress, &client_len); 
		char SEN_len = n - 1;
		printf("in 0 n = %d\n", n);
		seq_no = buf[1];
		printf("wait for seq 0, SEQ = %c\n", seq_no);
                memset( data, '\0', sizeof(data) );
                //strncpy ( data, buf + 2, 30);    // extract data
		int i;
		for(i = 0 ; i < (n - 3) ; i ++ )	data[i] = buf[i + 2];
                REC_checksum = compute_checksum ( SEN_len, seq_no, buf[n - 1], data, n - 3);
                // checksum
                
		if( n > 0) {
		    printf("recv no problem in while 0\n");
                    if ( seq_no == '0' &&  REC_checksum == -1 )  { // check checksum (corrupt)
			count ++;
                        printf("receive pkt 0 successfully\n");
                        
                        printf("buffer_pkt (%d) ontent: %s\n", count, buf);
                        write( fd, data, n - 3);// combine data (write to a new file)
                        
                        memset( sendback, '\0', sizeof(sendback) );
                        checksum = ~(3 + '0' + 'A');
			sendback[0] = REC_len;	sendback[1] = seq_no;	sendback[2] = 'A'; sendback[3] = checksum;
                        //sprintf( sendback, "%c%cA%c", REC_len, seq_no, checksum);
                        sendto( sockfd, sendback, strlen(sendback), 0, (struct sockaddr *) &clientAddress, client_len);
                        printf("send ACK 0 to sender\n");
                        oncethru = 1;
                        break;
                    }   else{
                            if( oncethru == 1 ){    // not the first round
				if( seq_no == '1' )	printf("wait for 0, but receive same pkt 1\n");
				else if( REC_checksum != -1)	printf("checksum error in state 0, cs = %d\n", REC_checksum);
                                printf("receive same pkt or checksum fails\n"); // receive same then drop it 
                                sendto( sockfd, sendback, strlen(sendback), 0, (struct sockaddr *) &clientAddress, client_len);  
                                printf("send same ACK 0 to sender\n");
                                continue;                    
                            }
                            // first round has problem => wait for time out
                    }
                }   else {  fprintf(stderr, "recv-error in receiver 0\n");    exit(1);    }
            }
            
            while(1)    {   // wait for 1 from below
		printf( "STATE: in state 1, wait for pkt 1\n");
                n = recvfrom(sockfd, buf, 33, 0, (struct sockaddr *) &clientAddress, &client_len); 
		char SEN_len = n - 1;
		printf("in 1 n = %d\n", n);
                seq_no = buf[1];
		printf( "wait for seq 1, SEQ = %c\n", seq_no);
                // checksum

                memset( data, '\0', sizeof(data) );
                //strncpy ( data, buf + 2, 30);    // extract data
		int i;
		for ( i = 0; i < (n - 3); i ++ )	data[i] = buf[i + 2];
                REC_checksum = compute_checksum ( SEN_len, seq_no, buf[n - 1], data, n - 3);  
                              
                if( n > 0) {              
		    printf("recv no promblem in while 1\n");
                    if ( seq_no == '1' && REC_checksum == -1 )    { // move to next state 0
			count ++;
                        printf("receive pkt 1 successfully\n");
                        
                        printf("buffer_pkt (%d) content: %s\n", count , buf);
                        write( fd, data, n - 3);// combine data (write to a new file)
                        
                        memset ( sendback, '\0', sizeof(sendback) );
                        checksum = ~(3 + '1' + 'A');
			sendback[0] = REC_len;	sendback[1] = seq_no;	 sendback[2] = 'A';	sendback[3] = checksum;
                        //sprintf( sendback, "%c%cA%c", REC_len, seq_no, checksum);
                        sendto( sockfd, sendback, strlen(sendback), 0, (struct sockaddr *) &clientAddress, client_len);
                        printf("send ACK 1 to sender\n");    
                        break;
                    }   else{
			//receive same pkt or checksum err, drop it
			if( seq_no == '0' )	printf("wait for 1, but receive same pkt 0\n");
			else if ( REC_checksum != -1 )	printf("checksum error in state 1, cs = %d\n, REC_checksum");
                        sendto( sockfd, sendback, strlen(sendback), 0, (struct sockaddr *) &clientAddress, client_len);  
                        printf("send same ACK 1 to sender\n");
                        continue;
                    }
                }   else {  fprintf(stderr, "recv-error in receiver 1\n");  exit(1);    }
            }
    }
    close(sockfd);    
    close(fd);
    return 0;  
}
