#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <math.h>
#define RWS 500
struct sockaddr_in theirAddr;
int file_length; //in terms of bytes
int my_seq;
int rec_seq;
int handshake_complete,last_packet_size,last_packet_num;
FILE* fp;
int SocketUDP;
double for_cmp;
double time_out=1.0;
int heartbeat=0;
struct timeval send_time;
struct timeval curr_time;
struct frame
{
	int data_seq_num;
	int complete;
	char data[1015]; //for packet size of 1KB
};
struct frame* frames;
/*void reliablyReceive(unsigned short int myUDPport, char* destinationFile) {

}*/
void* toSender(void* unusedParam)
{
	struct timespec sleepFor;
	sleepFor.tv_sec = 0;
	sleepFor.tv_nsec = 50 * 1000 * 1000; //50 ms
        char sendBuf[1024];
	
        
	
	while(1)
	{
		if(heartbeat==1)
          {
		nanosleep(&sleepFor, 0);
                memset(sendBuf,'\0', sizeof(sendBuf));
	sendBuf[0]= 'H';
        
	
	if(sendto(SocketUDP, sendBuf, 1024, 0, (struct sockaddr*)&theirAddr,sizeof(theirAddr)) < 0)
    	perror("sendto()");
          }
	}
}



void handle_ack(char* recvBuf)
{
         printf("get here1\n");
	int ack_num;
	memcpy(&ack_num,recvBuf+5,4);
	ack_num = ntohl(ack_num);
        printf("ack_num is %d,my_seq is %d\n",ack_num,my_seq);
	if(ack_num == my_seq-1)
	{
		handshake_complete = 1;
		frames = malloc(ceil(file_length/1015.0)*sizeof(struct frame));
		last_packet_num = ceil(file_length/1015.0)-1; // -1 for data seq num from 0
		last_packet_size = file_length%1015;
		if(last_packet_size == 0)
			last_packet_size = 1015;
		printf("handle ack last packet num  %d...last size %d\n",last_packet_num,last_packet_size);

	}
}
void send_synack()
{
	char sendBuf[1024];
	int my_seq_n = htonl(my_seq);
	int rec_seq_n = htonl(rec_seq);
        
	memset(sendBuf,'\0', sizeof(sendBuf));
	sendBuf[0]= 'T';
        
	memcpy(sendBuf+sizeof(char),&my_seq_n,sizeof(int)); //add own syn no
    memcpy(sendBuf+sizeof(char)+sizeof(int),&rec_seq_n,sizeof(int)); //add ack no
	if(sendto(SocketUDP, sendBuf, 1024, 0, (struct sockaddr*)&theirAddr,sizeof(theirAddr)) < 0)
    	perror("sendto()");
        
}
void handle_syn(char* recvBuf)
{
	int seq_num;
	int file_size;
	memcpy(&seq_num,recvBuf+1,4);
    memcpy(&file_size,recvBuf+9,4);
    rec_seq = ntohl(seq_num);
    file_length = ntohl(file_size);
	printf("handle syn seq %d...file_size %d\n",rec_seq,file_length);
	send_synack();
	
}
void send_ack(int single_ack, int cum_ack)
{
        printf("s_a is %d,c_a is %d\n",single_ack,cum_ack);
	single_ack = htonl(single_ack);
	cum_ack = htonl(cum_ack);
	char sendBuf[1024];
	memset(sendBuf,'\0', sizeof(sendBuf));
	sendBuf[0] ='D'; // create a D packet
	memcpy(sendBuf+sizeof(char),&single_ack,sizeof(int)); //add own syn no
    memcpy(sendBuf+sizeof(char)+sizeof(int),&cum_ack,sizeof(int)); //add ack no
	if(sendto(SocketUDP, sendBuf, 1024, 0, (struct sockaddr*)&theirAddr,sizeof(theirAddr)) < 0)
    	perror("sendto()");
	
}
void flush_to_file()
{
	int i;
	for(i=0; i< last_packet_num; i++)
	{ 
		fwrite(frames[i].data,1,1015,fp);
	}
	fwrite(frames[last_packet_num].data,1,last_packet_size,fp);
	fflush(fp);
}
void listener()
{
	char fromAddr[100];
	socklen_t theirAddrLen;
	unsigned char recvBuf[1024];// 1024 packet size
	my_seq =10; // some random number
	rec_seq = -1; //nothing received
	handshake_complete =0; //false
	int single_ack = -1;
	int cum_ack = -1;
	time_t start, end_time, wait_time;
	volatile int done =0;
	int NFE = 0;
	int bytesRecvd;
        
	while(1)
   	{
       	memset(recvBuf,'\0',sizeof(recvBuf));
        theirAddrLen = sizeof(theirAddr);
        if ((bytesRecvd = recvfrom(SocketUDP, recvBuf, 1024 , 0,
           (struct sockaddr*)&theirAddr, &theirAddrLen)) == -1)
        {
        	perror("connectivity listener: recvfrom failed");
            exit(1);
        }
		printf("the char is  %c\n",recvBuf[0]);

		if(recvBuf[0] == 'F')//its a finish packet
		{
			break;
		}
		if(recvBuf[0] == 'S')//its a syn handshake packet
		{
			handle_syn(recvBuf);
                        my_seq++;
		}
		if(recvBuf[0]=='A')//its an ack handshake packet
        {
        	handle_ack(recvBuf);
        }
		if(recvBuf[0]=='D')//its a data packet
		{
                        heartbeat=1;
                        printf("get here2\n");
			if(handshake_complete == 0)
				send_synack(); //try to complete handshake but process data packet as well
			//unpack the data do here so u have data, seq of packet
			int packet_seq;
			memcpy(&packet_seq,recvBuf+1,4);
			packet_seq = ntohl(packet_seq);
			if((packet_seq-NFE < RWS) && (packet_seq >= NFE) && (frames[packet_seq].complete !=1)) //packet within sliding window
			{
				if(packet_seq == last_packet_num) //last packet
				{
					memcpy(frames[packet_seq].data,recvBuf+9,last_packet_size);
				}
				else
					memcpy(frames[packet_seq].data,recvBuf+9,1015);
				frames[packet_seq].complete =1;
				int i;
				for(i=NFE;i<(NFE+RWS);i++)
				{
					if(frames[i].complete != 1)
						break;
				}
				NFE = i; // update sw
				cum_ack = NFE -1;
                                
				if(cum_ack == last_packet_num)
				{
					flush_to_file();
                                        if(done==0)
                                        gettimeofday(&send_time,0);
					done = 1;
                                        
					start = time(NULL);
					wait_time = 1;
					end_time = start + wait_time;
                                        
				}
			}
			single_ack = packet_seq;
                     
			send_ack(single_ack,cum_ack); //send ack in all cases
			if(done ==1)
			{
                                gettimeofday(&curr_time,0);
                                for_cmp=curr_time.tv_sec-send_time.tv_sec+((curr_time.tv_usec-send_time.tv_usec)/1000000.0);
				//start = time(NULL);
				if(for_cmp > time_out)
					break;
			}
		}
	}
}
int main(int argc, char** argv)
{
	unsigned short int udpPort;
	
	if(argc != 3)
	{
		fprintf(stderr, "usage: %s UDP_port filename_to_write\n\n", argv[0]);
		exit(1);
	}
	
	udpPort = (unsigned short int)atoi(argv[1]);
	if((SocketUDP=socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
    	perror("socket");
        exit(1);
    }
    struct sockaddr_in bindAddr;       
   	memset(&bindAddr, 0, sizeof(bindAddr));
    bindAddr.sin_family = AF_INET;
    bindAddr.sin_port = htons(udpPort);
	bindAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(SocketUDP, (struct sockaddr*)&bindAddr, sizeof(struct sockaddr_in)) < 0)
    {
    	perror("bind");
        close(SocketUDP);
        exit(1);
    }
	fp = fopen(argv[2], "wb");
	if (fp == NULL)
		exit(EXIT_FAILURE);

	pthread_t listenerThread;
    pthread_create(&listenerThread, 0, toSender, (void*)0);
	//reliablyReceive(udpPort, argv[2]); */
	listener();
	close(SocketUDP);
}
