#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <netinet/in.h>
#include <linux/icmp.h>
#include <sys/ioctl.h>

// outside proxy

// define the IP_pkt
// get the ICMP pkt from inside proxy
// dismantle the pkt
// know where to connect, then connect to PTT server
// get pkt form PTT
// construct an ICMP pkt
// send back to inside proxy

// *in IP (ICMP) pkt, source add = inside proxy
// *                  destination add = PTT server
// *time to live = 255

int notice = 0, back = 0;
int count = 0;
char recvbuffer[1028], recvdata[1000];  // recvbuffer for echorequest, recvdata for data part of IP pkt
char pttbuffer[1000], pttdata[1028];    // pttbuffer for pkt ptt server sent, pttdata for sending back to ICMP
int m;
 
void* TCP_PTT( void* arg){
    
    // as client, TCP connection to the ptt server
    int ptt_sockfd;
    struct sockaddr_in serverAddress;    
    
    ptt_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&serverAddress, sizeof(serverAddress));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(23);     
    inet_pton(AF_INET, "140.112.172.11", &serverAddress.sin_addr);
    connect(ptt_sockfd, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
    printf("connect to ptt!\n");

    int val = 1, i;
    ioctl(ptt_sockfd, FIONBIO, &val);    

    while(1){
        usleep(300000);
        m = read( ptt_sockfd, pttbuffer + 3, sizeof(pttbuffer)-3);         // need to add "PTT" so minus 3 (1008 - 3)
        pttbuffer[0] = 'p';
        pttbuffer[1] = 't';
        pttbuffer[2] = 't';
        back = 1;
        
        // send the data to ptt server
        if( notice == 1 ){
            write( ptt_sockfd, recvdata, sizeof(recvdata));
            printf("write to ptt server\n");
            memset(recvbuffer, '\0', sizeof(recvbuffer));
            notice = 0;
        }
    }
    close(ptt_sockfd);
    return (void*)0;
}

int main( int argc, char* argv[] ){

    int sockfd;
    int first = 1;    
    char *icmpbuf;        // may not be this size!!    
    icmpbuf = malloc(sizeof(struct icmphdr));
    struct sockaddr_in serverAddress, clientAddress; 
    struct icmphdr *icmphd; 
    pthread_t td;  void *tret;       
    
    // create row socket, wait for inside proxy connection??
    if( (sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0 )
    {   perror("SOCK_ROW error\n");    exit(1);   }
	
	// set the serverAddress
    bzero(&serverAddress, sizeof(serverAddress));   
    serverAddress.sin_family = AF_INET;
    //serverAddress.sin_port = htons(8001);
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);

    // start to bind the ICMP connection
    if( bind(sockfd, (struct sockaddr *) &serverAddress , sizeof(serverAddress)) < 0 )
	{	perror("Server-bind error.\n");      exit(1);    }    
	
	// no need to listen and accept	
    int client_len = sizeof(clientAddress);
    printf("localhost connection is ready!\n");

    int y;
    y = recvfrom( sockfd, recvbuffer, sizeof(recvbuffer), 0, (struct sockaddr *) &clientAddress, &client_len );
    printf( "recvbuffer = (%c %c), y = %d\n", recvbuffer[28],recvbuffer[29],y);
    if( recvbuffer[28] == 'p' && recvbuffer[29] == '1' && first == 1 ){
         //create TCP connection, should use thread
         //run TCP_PTT, for sending request to PTT, get info back from PTT
        if( (pthread_create(&td, NULL, TCP_PTT, NULL)) != 0 )
        {   perror("thread create fail\n");    exit(1);   }                   
        first = 0;
    }
    
    int val = 1;
    ioctl(sockfd, FIONBIO, &val);    
    int n, i;
    while(1) { 
        usleep(300000);
        // read from icmp, get the IP pkt
        n = recvfrom( sockfd, recvbuffer, sizeof(recvbuffer), 0, (struct sockaddr *) &clientAddress, &client_len );
        if( n > 0 ){
            printf("read from icmp packet succeed\n");
            // dismantle the IP pkt, distract the data part
            memset(recvdata, '\0', sizeof(recvdata));
            for( i = 0; i < n; i++ ) recvdata[i] = recvbuffer[i+28];                  // copy the data part out
            notice = 1;
        }
        
        // send echo reply back
        if( first == 0)  {
            icmphd = (struct icmphdr*) icmpbuf;
            icmphd->type = 0;
            icmphd->code = 0;
            icmphd->checksum = (short)0;
            icmphd->un.echo.id = (short)getpid();

            memcpy(pttdata, icmphd, sizeof(struct icmphdr));                             // copy icmpheader to pkt
            memcpy(pttdata + sizeof(struct icmphdr), pttbuffer, sizeof(pttbuffer));       // copy tcp_data to pkt
            sendto( sockfd, pttdata, sizeof(pttdata), 0,(struct sockaddr *) &clientAddress, sizeof(clientAddress));
            memset(pttbuffer, '\0', sizeof(pttbuffer));
        }
    }
    // until the process finishes, also wait until the thread stops
    if( (pthread_join(td, &tret)) != 0 )
    {   perror("thread join fail\n");       exit(1);    }       
    close(sockfd); 
    free(icmpbuf);
    return 0;
}
