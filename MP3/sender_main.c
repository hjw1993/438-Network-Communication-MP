#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#define max_packet_size 1024
#define window_size 500


int senderSocket;
struct timeval RTT;
struct timeval current_RTT;
struct timeval send_time;
struct timeval curr_time;


struct sockaddr_in destAddr;
double time_out=0.01;
int i_time_out;
int num_packets;
int ack_received;
int ack_seq;
struct sliding_window
{
int size;//sliding window size
int fpac;// the first packet in the sliding window
int lpac;// the last packet in the sliding window
}sw;
int handshake_complete=0;


// htonl to network order, ntohl to mormal
handshake(int bytesToTransfer,char data_packets[][max_packet_size-9])

{
  char* sendBuf = malloc(max_packet_size);
  int bytesRecvd;
  
  unsigned char recvBuf[1024];
  int syn_num=0;
  int ack_num;
  int S_complete=0;
  int i;
  double for_cmp;
  int zero=0;
  int seq_num=0;


 
  struct sockaddr_in theirAddr;
  socklen_t theirAddrLen;
  //send the syn
  
  strcpy(sendBuf,"S");
  printf("the send message is %c,the syn_message is %d\n",sendBuf[0],syn_num);
  syn_num= htonl(syn_num);
  
  memcpy(sendBuf+1,&syn_num,sizeof(int));
  memcpy(sendBuf+5,&zero,sizeof(int));
  printf("the total bytes is %d\n",bytesToTransfer);
  bytesToTransfer=htonl(bytesToTransfer);
  
  memcpy(sendBuf+9,&bytesToTransfer,sizeof(int));
  
  gettimeofday(&send_time,0);//if sliding window changed,update current time
if(sendto(senderSocket, sendBuf,max_packet_size, 0,(struct sockaddr*)&destAddr, sizeof(destAddr)) < 0)
perror("sendto()");
  printf("already sent\n");
  
  
  while(handshake_complete==0)
  {
    theirAddrLen = sizeof(theirAddr);
		if ((bytesRecvd = recvfrom(senderSocket, recvBuf, 1024 , 0, 
					(struct sockaddr*)&theirAddr, &theirAddrLen)) == -1)
		{
			perror("connectivity listener: recvfrom failed");
			exit(1);
		}
    // if receiving
    if(bytesRecvd)
    printf("recv sth, the type is %c\n",recvBuf[0]);
    if(recvBuf[0]=='T')
    {
      
      S_complete=1;
      memcpy(&ack_num,recvBuf+5,4);
      ack_num=ntohl(ack_num);
      printf(" the recvd message is %c,  the ack is %d,the syn_num is %d\n",recvBuf[0],ack_num,syn_num);
      if (ack_num==syn_num)//send the ack to receiver
      {
        printf("send ack k\n");
        memcpy(&syn_num,recvBuf+1,4);
        syn_num=ntohl(syn_num);
        memset(sendBuf,&zero,max_packet_size);
        ack_num=htonl(syn_num);
        strcpy(sendBuf,"A");
        memcpy(sendBuf+1,&zero,4);
        memcpy(sendBuf+5,&ack_num,4);
        gettimeofday(&send_time,0);
        if(sendto(senderSocket, sendBuf,max_packet_size, 0,(struct sockaddr*)&destAddr, sizeof(destAddr)) < 0)
        perror("sendto()");
        handshake_complete=1;
      }
    }
    

    
    
    


  }//while loop








}











void reliablyTransfer(char* hostname, unsigned short int hostUDPport, char* filename, unsigned long long int bytesToTransfer) {
        long int size;//read the size of the file
        int sequence_num;
        struct sockaddr_in theirAddr;
	socklen_t theirAddrLen;
        int ack_num;
        int seq_num_recv;
        int seq_num_recv1;
        int seq_num_send;
        int seq_num;
        int window_move;
        double for_cmp;
        int bytesRecvd;
        unsigned char recvBuf[1024];
        time_out=0.01;//initialize the time_out value
        FILE* to_send;
        to_send=fopen(filename,"rb");
        i_time_out=5;
        int wait_time=0;
        int last_packet_bytes;
        int zero=0;
        //initialize the send sliding window
        sw.size=window_size;
        sw.fpac=0;
        sw.lpac=sw.size-1;
        int i;

        //sender address
        senderSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if(senderSocket < 0)
		perror("socket()");
        struct sockaddr_in srcAddr;
	memset(&srcAddr, 0, sizeof(srcAddr));
	srcAddr.sin_family = AF_INET;
	srcAddr.sin_port = htons( 8000);
        srcAddr.sin_addr.s_addr=htonl(INADDR_ANY);
	if(bind(senderSocket, (struct sockaddr*)&srcAddr, sizeof(srcAddr)) < 0)
		perror("bind()");

	//receiver information:address
	
        char tempaddr[100];
	sprintf(tempaddr,"%s",hostname);
	memset(&destAddr, 0, sizeof(destAddr));
	destAddr.sin_family = AF_INET;
	destAddr.sin_port = htons(hostUDPport);
	inet_pton(AF_INET, tempaddr, &destAddr.sin_addr);
       //printf("the bytes is %d\n",bytesToTransfer);
        

       // printf("size is %d\n",sizeof(short int));
        num_packets=bytesToTransfer/(max_packet_size-9);//how many packets to send
        last_packet_bytes= (bytesToTransfer%(max_packet_size-9)); 
        if(last_packet_bytes)//if there is a last packet, then num+=1
        num_packets+=1;
        char data_packets[num_packets][max_packet_size-9];// header 28 bytes, and 9 bytes for seq ack "D"  
        int recv_mark[num_packets];
        printf("the num_packets is %d,the last packet bytes is %d\n",num_packets,last_packet_bytes);
        // store the bytes in buffers
        for(i=0; i< num_packets;i++)
        {
         if(i!=num_packets-1)
         fread(data_packets[i],sizeof(char),max_packet_size-9,to_send);
         else
         fread(data_packets[i],sizeof(char),last_packet_bytes,to_send);
        }

        if (sw.lpac>num_packets)
        sw.lpac=num_packets-1;
        char* sendBuf = malloc(max_packet_size);
       
        char temp_buf[1200];
    
    handshake(bytesToTransfer,data_packets);
    for(i=0;i<=sw.lpac;i++)
        {
         memset(sendBuf,0,sizeof(sendBuf));
         strcpy(sendBuf,"D");
         seq_num=htonl(i);
         memcpy(sendBuf+1,&seq_num, sizeof(int));//sequence number
         memcpy(sendBuf+5,&zero,sizeof(int));
         memcpy(sendBuf+9,data_packets[i], sizeof(data_packets[i]));//data
                           
         if(sendto(senderSocket, sendBuf,max_packet_size, 0,(struct sockaddr*)&destAddr, sizeof      (destAddr)) < 0)
         perror("sendto()");  

       
        }
    /*gettimeofday(&send_time,0);//if sliding window changed,update current time 
    //send first 4 packets without ack receiving
    for(i=sw.fpac;i<=sw.lpac;i++)
    {
    memset(sendBuf,0,sizeof(sendBuf));
    strcpy(sendBuf,"D");
    memcpy(sendBuf+1,&i, sizeof(int));//sequence number
    
    memcpy(sendBuf+9,data_packets[i], sizeof(data_packets[i]));//data
                           
    if(sendto(senderSocket, sendBuf,max_packet_size, 0,(struct sockaddr*)&destAddr, sizeof(destAddr)) < 0)
    perror("sendto()");
    }*/
  
  // keep listening for ACK messages (after the handshake)
  while(1)
   {
   //printf("entered the loop\n");
  theirAddrLen = sizeof(theirAddr);
		if ((bytesRecvd = recvfrom(senderSocket, recvBuf, 1024 , 0, 
					(struct sockaddr*)&theirAddr, &theirAddrLen)) == -1)
		{
			perror("connectivity listener: recvfrom failed");
			exit(1);
		}
   
   if(!strncmp(recvBuf, "D", 1))
    {
          seq_num_recv=0;
          seq_num_recv1=0;
          window_move=-1;
          memcpy(&seq_num_recv1,recvBuf+1,4);//get the seq number
          //printf("seq11111 is %d\n",seq_num_recv1);
          seq_num_recv=ntohl(seq_num_recv1);
          
         
          
          memcpy(&ack_num,recvBuf+5,4);//get the ack number
          ack_seq=ntohl(ack_num);
          //printf("the ack is %d\n",ack_seq);
           //recv_mark[seq_num_recv]=1;//mark the packets that already recvd
          if(ack_seq==num_packets-1)
           break;
          //printf("the ack is %d,the 1st is %d\n",ack_seq,sw.fpac);
          
                if((ack_seq>=sw.fpac)&&(ack_seq<=sw.lpac))
			{
                        window_move=ack_seq-sw.fpac;
			sw.fpac=ack_seq+1;
			sw.lpac=sw.fpac+sw.size-1;
                        if(sw.lpac>(num_packets-1))
                        sw.lpac=num_packets-1;
                        //printf("the 1st is %d\n",sw.fpac);
			}
		 if(window_move>=0)
			{
                         
                         for(i=window_move;i>=0;i--)
                          {
                           
                           printf("the window_move is %d\n",window_move);
                           printf("the ack num is %d\n",ack_seq);
                           //printf("send new packet\n");
                           printf("the fpac is %d\n",sw.fpac);
                           seq_num_send=sw.lpac-i;
                           seq_num_send= htonl(seq_num_send);
                           memset(sendBuf,0,sizeof(sendBuf));
                           strcpy(sendBuf,"D");
                           seq_num=htonl(sw.lpac-i);
                           printf("the seq is %d\n",sw.lpac-i);
			   memcpy(sendBuf+1,&seq_num, sizeof(int));//sequence number
                           memcpy(sendBuf+5,&zero, sizeof(int));
                           memcpy(sendBuf+9,data_packets[sw.lpac-i], sizeof(data_packets[sw.lpac-i]));//data
                           
                           if(sendto(senderSocket, sendBuf,max_packet_size, 0,(struct sockaddr*)&destAddr, sizeof(destAddr)) < 0)
			perror("sendto()");
                          }
                          wait_time=0;
gettimeofday(&send_time,0);//if sliding window changed,update current time 
			}
     }
      gettimeofday(&curr_time,0);
     // if there is a time_out
     wait_time+=1;
     //printf("the time is %lds,%ldus\n",curr_time.tv_sec,curr_time.tv_usec);
     //for_cmp=curr_time.tv_sec-send_time.tv_sec+((curr_time.tv_usec-send_time.tv_usec)/100000.0);
     //printf("time_out is %lf,the for cmp is %lf\n",time_out,for_cmp);
      printf("time_out is %d,the wait time  is %d\n",i_time_out,wait_time);
     if (wait_time>i_time_out)
      {       printf("resend the data\n");
              
             //resend all the packet in sliding window
             for(i=sw.fpac;i<=sw.lpac;i++)
                          {
                           if(recv_mark[i]==0)// if receiver doesn't have the packet
                             {
                           
                           memset(sendBuf,0,sizeof(sendBuf));
                           strcpy(sendBuf,"D");
                           seq_num=htonl(i);
			   memcpy(sendBuf+1,&seq_num, sizeof(int));//sequence number
                           memcpy(sendBuf+5,&zero, sizeof(int));
                           memcpy(sendBuf+9,data_packets[i], sizeof(data_packets[i]));//data packet
                           
                           if(sendto(senderSocket, sendBuf,max_packet_size, 0,(struct sockaddr*)&destAddr, sizeof(destAddr)) < 0)
			perror("sendto()");
                             }
                          }
                          gettimeofday(&send_time,0);//resending data,update current
                          wait_time=0;
             // update the time_out
        time_out+=time_out;
        i_time_out+=5;
        if(time_out>0.20)
        time_out=0.20;  
        if(i_time_out>30)
         i_time_out=30;    

      }
      curr_time.tv_sec=0;
      curr_time.tv_usec=0;

 }
}






int main(int argc, char** argv)
{
	unsigned short int udpPort;
	unsigned long long int numBytes;
	char* sendBuf = malloc(max_packet_size);
	if(argc != 5)
	{
		fprintf(stderr, "usage: %s receiver_hostname receiver_port filename_to_xfer bytes_to_xfer\n\n", argv[0]);
		exit(1);
	}
	udpPort = (unsigned short int)atoi(argv[2]);
	numBytes = atoll(argv[4]);
	
	reliablyTransfer(argv[1], udpPort, argv[3], numBytes);
        
                           sendBuf[0]='F';
                           if(sendto(senderSocket, sendBuf,max_packet_size, 0,(struct sockaddr*)&destAddr, sizeof(destAddr)) < 0)
			perror("sendto()");
       printf("done\n");
} 
