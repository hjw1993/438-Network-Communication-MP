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
#include <pthread.h>
#include <ctype.h>
extern struct 
{
   int connection;
   int cost;
}neighbors[256];


extern struct lsp_received
{
  int neighbor[100];
  int cost[100];
  int seq_num;
}lsp_received[256];


extern struct 
{  
  int destination;
  int nexthop;
  int cost;
  int before_dest;
}forwarding[256];

extern struct 
{  
  int validation;
  int destination;
  int nexthop;
  int cost;
  int before_dest;
}tentative[256];
extern int chosen;//for updating shortest path
extern int seq;
extern int globalMyID;
extern int call_count;
extern FILE* log_events;
//last time you heard from each node. TODO: you will want to monitor this
//in order to realize when a neighbor has gotten cut off from you.
extern struct timeval globalLastHeartbeat[256];
extern struct timeval cut_neighborlfc[256];
extern struct timeval listen_neighbor[256];
//our all-purpose UDP socket, to be bound to 10.1.1.globalMyID, port 7777
extern int globalSocketUDP;
//pre-filled for sending to 10.1.1.0 - 255, port 7777
extern struct sockaddr_in globalNodeAddrs[256];
extern int cost[256];
//Yes, this is terrible. It's also terrible that, in Linux, a socket
//can't receive broadcast packets unless it's bound to INADDR_ANY,
//which we can't do in this assignment.
extern int updating_flag;
void hackyBroadcast(const char* buf, int length)
{
	int i;
	for(i=0;i<256;i++)
		if(i != globalMyID) //(although with a real broadcast you would also get the packet yourself)
          sendto(globalSocketUDP, buf, length, 0,
				  (struct sockaddr*)&globalNodeAddrs[i], sizeof(globalNodeAddrs[i]));
}





void lsp_sending(unsigned char*buf, int length,int no_send)
{
	int i;
        //printf("lsp_sending is %s", buf);
	for(i=0;i<256;i++)
		if(neighbors[i].connection==1&&i!=no_send) //(although with a real broadcast you would also get the packet yourself)
          sendto(globalSocketUDP, buf, length, 0,
				  (struct sockaddr*)&globalNodeAddrs[i], sizeof(globalNodeAddrs[i]));
}



void* announceToNeighbors(void* unusedParam)
{
	struct timespec sleepFor;
	sleepFor.tv_sec = 0;
	sleepFor.tv_nsec = 300 * 1000 * 1000; //100 ms
	while(1)
	{
		hackyBroadcast("HEREIAM", 7);
		nanosleep(&sleepFor, 0);
	}
}






void lsp_send( )
{
	struct timespec sleepFor;
	sleepFor.tv_sec = 0;
        int i=0;
        char lsp_for_send[1000];
        char str[10];
        
        
       //printf("get here\n");
	sleepFor.tv_nsec = 300 * 1000 * 1000; //300 ms
	   
                
                lsp_for_send[0]='\0';
                sprintf(str,"%d",globalMyID);
                strcpy(lsp_for_send,"lsp");
                strcat(lsp_for_send,str);
                strcat(lsp_for_send,";");//originator
                for(i=0;i<256;i++)//originator;neighborID-cost:neighborID-cost:....;seq
                {
                 if(neighbors[i].connection==1)
                    {
                     if(cost[i]!=0)
                     {
                       neighbors[i].cost=cost[i];
                     }
                     else
                     {
                       neighbors[i].cost=1;
                     }
                     sprintf(str,"%d",i);
                     strcat(lsp_for_send,str);
                     strcat(lsp_for_send,"-");
                     sprintf(str,"%d",neighbors[i].cost);
                     strcat(lsp_for_send,str);
                     strcat(lsp_for_send,":");
                     }
                }
                strcat(lsp_for_send,";");
                sprintf(str,"%d",seq);
                strcat(lsp_for_send,str);
                strcat(lsp_for_send,";");
                nanosleep(&sleepFor, 0);
                
               // printf("the messages is %s\n",lsp_for_send);
		lsp_sending(lsp_for_send, strlen(lsp_for_send),globalMyID);
                seq++;
		
	
}


void ten_to_confirm( )
{
 int i;
 int cost_temp=60000;
 
 for(i=0;i<256;i++)
{
  if(tentative[i].validation==1)
   {
     if(tentative[i].cost<cost_temp)
      {
      cost_temp=tentative[i].cost;
      chosen=i; 
      }//shorter path



   }// if it is in the table
}//find the least cost
  forwarding[chosen].destination=tentative[chosen].destination;
  forwarding[chosen].nexthop=tentative[chosen].nexthop;
  forwarding[chosen].cost=tentative[chosen].cost;
  forwarding[chosen].before_dest=tentative[chosen].before_dest;
  
 // printf("the chosen is %d\n",chosen);
   /* if(globalMyID==0)
   {
    printf("the confirmed information is\n");
   for (i=0;i<256;i++)
     {
      if(forwarding[i].cost!=0)
         {
          printf(" the vector is (%d,%d,%d)\n",i,forwarding[i].cost,forwarding[i].nexthop);

         }


     }
   }*/



}


void updating_tentative( )
{
 int i;
 int neighbor_id;
 tentative[chosen].validation=2;
 //printf("get here");
/*for(i=0;i<100;i++)
{
if(lsp_received[255].cost[i]!=0)
printf("node255 neighbor is %d,the cost is %d\n",lsp_received[255].neighbor[i],lsp_received[255].cost[i]);

}*/
for(i=0;i<100;i++)
      {
       if(lsp_received[chosen].cost[i]!=0)
           {
             neighbor_id=lsp_received[chosen].neighbor[i];
             if(tentative[neighbor_id].validation==0&&neighbor_id!=globalMyID)
              {
                 tentative[neighbor_id].validation=1;
                 tentative[neighbor_id].destination=neighbor_id;
                 tentative[neighbor_id].cost=lsp_received[chosen].cost[i]+forwarding[chosen].cost;
                 tentative[neighbor_id].nexthop=forwarding[chosen].nexthop;
                 tentative[neighbor_id].before_dest=chosen;
                 //printf("the is node%d\n ",chosen);
                 //printf("added vector is (%d,%d,%d)\n",neighbor_id,tentative[neighbor_id].cost,tentative[neighbor_id].nexthop);
              }//not in tentative list
             else if(tentative[neighbor_id].validation==1)
              {
                if(tentative[neighbor_id].cost>(lsp_received[chosen].cost[i]+forwarding[chosen].cost))             
                {
                  tentative[neighbor_id].cost=lsp_received[chosen].cost[i]+forwarding[chosen].cost;
                 tentative[neighbor_id].nexthop=forwarding[chosen].nexthop;
               // printf("the is node%d\n ",chosen);
                 //printf("updated vector is (%d,%d,%d)\n",neighbor_id,tentative[neighbor_id].cost,tentative[neighbor_id].nexthop);
                }//shorter path
                else if(tentative[neighbor_id].cost==(lsp_received[chosen].cost[i]+forwarding[chosen].cost))  
                {
                      if(tentative[neighbor_id].before_dest>chosen)
                        {
                           tentative[neighbor_id].nexthop=forwarding[chosen].nexthop;
                            tentative[neighbor_id].before_dest=chosen;
                        }
                   



                }

               
              }//in tentative list
           }//if this is a neighbor
      
      } //updating the tentative table
   // printf("the chosen is %d\n",chosen);
    //printf("the tenative information is\n");
   /*for (i=0;i<256;i++)
     {
      if(tentative[i].validation==1)
         {
          printf(" the vector is (%d,%d,%d)\n",i,tentative[i].cost,tentative[i].nexthop);

         }


     }*/
      ten_to_confirm(); 


}




void shortest_path()
{
 int i=0;
 char str[10];
 unsigned int temp_cost=10000;
 
 int neighbor_id;
 int keep_updating=0;
 for(i=0;i<256;i++)
 {
   tentative[i].validation=0;
   tentative[i].destination=0;
   tentative[i].nexthop=0;
    tentative[i].before_dest=0;
   tentative[i].cost=0;
   forwarding[i].cost=0;
   forwarding[i].nexthop=0;
   forwarding[i].destination=0;
   forwarding[i].before_dest=0;
 }
 
 for(i=0;i<256;i++)
  {
    if(neighbors[i].connection==1)
        {
         tentative[i].validation=1;
         tentative[i].cost=neighbors[i].cost;
         tentative[i].nexthop=i;
         tentative[i].destination=i;
         //printf("the neighbor is %d\n",i);
         if(temp_cost>neighbors[i].cost)
          {
            temp_cost=neighbors[i].cost;
            chosen=i;
          }
         
        }
  }//the neighbors path
/*for (i=0;i<256;i++)
     {
      if(tentative[i].validation==1)
         {
        //  printf(" the initial vector is (%d,%d,%d)\n",i,tentative[i].cost,tentative[i].nexthop);

         }


     }*/
  forwarding[chosen].destination=chosen;
  forwarding[chosen].nexthop=chosen;
  forwarding[chosen].cost=neighbors[chosen].cost;
  
  while(1)
   {
   // printf("chosen is %d\n",chosen);
    for(i=0;i<256;i++)
    {
     if(tentative[i].validation==1)
      {
       keep_updating=1;
       updating_tentative();
       break;
      }
    }
    if(keep_updating==0)
    break;
    else
    keep_updating=0;

   }
   updating_flag=1;
  // printf("the computation is over");
   /*for (i=0;i<7;i++)
  {
  updating_tentative();
   }*/
}//whole



void* print_test(void* unusedParam)
{
 int i;
int j;

struct timespec sleepFor;
	sleepFor.tv_sec = 5;
	sleepFor.tv_nsec = 0;
     nanosleep(&sleepFor, 0);
while(1){
if(updating_flag==1)
{
        if(globalMyID==0)
		{
			printf("the neighbor information is \n");
			for (i=0;i<256;i++)
				{
				if(neighbors[i].connection==1)
				{
 							printf("the node is%d,the neighbor is %d, the cost is %d\n",globalMyID,i,neighbors[i].cost);
				}
				}//print neighbor information




				printf("the lsp information is\n");
				for(i=0;i<256;i++)
					{
					if(lsp_received[i].cost[0]!=0)
					{

 						for(j=0;j<100;j++)
 						 {
						if(lsp_received[i].cost[j]!=0)
   						printf("The node is %d, its neighbor is %d, cost is %d\n",i,lsp_received[i].neighbor[j],lsp_received[i].cost[j]);
  						}

					}


					}//print lsp information	
/*for(i=0;i<256;i++)
{
if(lsp_received[4].cost[i]!=0)
printf("node4 neighbor is %d,the cost is %d\n",lsp_received[4].neighbor[i],lsp_received[4].cost[i]);

}
*/


					printf("the forwading information is\n");
					for(i=0;i<256;i++)
				   	{
					if(forwarding[i].cost!=0)
  				 	{
					printf("The destination is %d, the nexthop is %d, cost is %d\n",i,forwarding[i].nexthop,forwarding[i].cost);
  					}


					}//print the forwaring	

		}

	}
updating_flag=0;
}
}




void send_or_forward(unsigned char*buf, int length,int dest)
{
	int i;
        //printf("lsp_sending is %s", buf);
	
          sendto(globalSocketUDP, buf, length, 0,
				  (struct sockaddr*)&globalNodeAddrs[dest], sizeof(globalNodeAddrs[dest]));
}

void *cut_neighbor(void* unusedParam)
{   
int i=0;
double for_cmp;
struct timespec sleepFor;
	sleepFor.tv_sec = 0;
	sleepFor.tv_nsec = 900 * 1000 * 1000; //900 ms
struct timespec sleepForlsp;
        sleepForlsp.tv_sec = 0;
	sleepForlsp.tv_nsec = 900 * 1000 * 1000; //900 ms
	while(1)
	{
		
		
         for(i=0;i<256;i++) 
             {      
                      
                      if(neighbors[i].connection==1)
                       {
                 gettimeofday(&cut_neighborlfc[i], 0);
                 for_cmp=cut_neighborlfc[i].tv_sec-globalLastHeartbeat[i].tv_sec+((cut_neighborlfc[i].tv_usec-globalLastHeartbeat[i].tv_usec)/1000000.0);
                 // printf("the for_cmp is %lf\n",for_cmp);
                         if(for_cmp>1.5&&neighbors[i].connection==1)
                           {
                                 neighbors[i].connection=0;
                                 neighbors[i].cost=0;   
                                 lsp_send();
                               
                                 
                          //  printf("this is node %d\n, the neighbor cut is %d\n",globalMyID,i);           
                           }
                         
                       }  
             } 
	}

}

void* listen_heartbeat(void* unusedParam)
{
        char fromAddr[100];
	struct sockaddr_in theirAddr;
	socklen_t theirAddrLen;
	unsigned char recvBuf[1000];
        int bytesRecvd;
while(1)
	{
              
              
		
             
         }

close(globalSocketUDP);

}



void listenForNeighbors()
{
       struct timeval curr_time[256];
       double for_cmp;
        int temp_neighbor=255;
        int temp_connection=1;
	char fromAddr[100];
	struct sockaddr_in theirAddr;
	socklen_t theirAddrLen;
	unsigned char recvBuf[1000];
        int i=0;
        int j;
       int count =0;
       int found =0;
       char*lsp_forward;
       
       char lsp_desicion[20];
       char str[10];
       sprintf(str,"%d",globalMyID);
       strcat(lsp_desicion,str);
       char originator[10];
       int send_node;
       char cost_message[500];
       int seq1;
       char neighbor_kv[100][10];
       char* for_cost;
	int bytesRecvd;
       char cost_trans[10];
       char for_seq[100];
       char test_cost[50];
       char for_test[500];
       unsigned char recvBuf_forward[1000];
       int dest_pos=sizeof(short int);
      // printf("dest_pos is %d\n",dest_pos);
       char str_dest[4];
       int send_dest;
       char send_message[150];
       char for_nexthop[10];
       char log_message[200];
       int nexthop;
       int for_cost_neighbor;
       int new_cost;
       char for_cost_neighbor1[10];
       char new_cost1[10];
       int dest1,cost1;
       
	while(1)
	{
              
              send_message[0]='\0';
              strcpy(lsp_desicion,"lsp");
              sprintf(str,"%d",globalMyID);
              strcat(lsp_desicion,str);
		found=0;
                  
     theirAddrLen = sizeof(theirAddr);
		if ((bytesRecvd = recvfrom(globalSocketUDP, recvBuf, 1000 , 0, 
					(struct sockaddr*)&theirAddr, &theirAddrLen)) == -1)
		{
			perror("connectivity listener: recvfrom failed");
			exit(1);
		}
                
		
		inet_ntop(AF_INET, &theirAddr.sin_addr, fromAddr, 100);
		
		short int heardFrom = -1;
		if(strstr(fromAddr, "10.1.1."))
		{
			heardFrom = atoi(
					strchr(strchr(strchr(fromAddr,'.')+1,'.')+1,'.')+1);
			
			//TODO: this node can consider heardFrom to be directly connected to it; do any such logic now.   
                     if(strcmp(recvBuf,"HEREIAM")==0&&neighbors[heardFrom].connection==0)
                      {
                                        neighbors[heardFrom].connection=1;
                                        call_count++;
                                    //  if(heardFrom==255)
                              
                                      // printf("i am %d,call time is %d, the neighbor is  %d the cost is %d \n",globalMyID,call_count,heardFrom,neighbors[heardFrom].cost);
                                        lsp_send();
                                        
                      }
                        
//record that we heard from heardFrom just now.
			gettimeofday(&globalLastHeartbeat[heardFrom], 0);
                         
                      
                        //if(heardFrom==2&&globalMyID==1)
                         //{
                       // printf("sec is %d,usec is %d\n",globalLastHeartbeat[heardFrom].tv_sec,globalLastHeartbeat[heardFrom].tv_usec/100000);
                         //}
                          
		}
		  for(i=0;i<256;i++)
                      {  
                       if(neighbors[i].connection==1)
                       { 
                         // printf("the connected neighbor is %d\n",i);
                     	  gettimeofday(&curr_time[i],0);
                     	  for_cmp=curr_time[i].tv_sec-globalLastHeartbeat[i].tv_sec+((curr_time[i].tv_usec-globalLastHeartbeat[i].tv_usec)/1000000.0);
                         
                          if(for_cmp>1.5)
                             	{
                                 //printf("this is node %d\n",globalMyID);
                                 //printf(" the cut is %d\n",i);
                                  //printf("the for_cmp is %lf\n",for_cmp);
                                 neighbors[i].connection=0;
                              //   printf("i is %d, the connection is %d\n",i,neighbors[i].connection);
                                 neighbors[i].cost=0;
                                 temp_neighbor=i;
                                 temp_connection=neighbors[i].connection;
                                 lsp_received[i].seq_num=0;
                                 lsp_send();
                                 
				}


                       }
                      }
                     
                
		
		
		
		//Is it a packet from the manager? (see mp2 specification for more details)
		//send format: 'send'<4 ASCII bytes>, destID<net order 2 byte signed>, <some ASCII message>
		if(!strncmp(recvBuf, "send", 4))
		{
                      
                       send_dest=0;
                         //printf("get here\n");
                        // printf("this node %d, I get a message %s\n",globalMyID,recvBuf);
                        // str_dest[0]=recvBuf[4];
                         //str_dest[1]=recvBuf[5];
                         //str_dest[2]='\0';
                         //printf("get here\n");
			// memcpy(send_dest,recvBuf+4,2);
                        // printf("str_dest is %s\n",str_dest);
                         
                         memcpy(&dest1,recvBuf+4,2);
                         send_dest=ntohs(dest1);
                        // printf("send_dest is %d\n",send_dest);
                        // printf("bytes_recv is %d\n",bytesRecvd);
                         for(i=6;i<bytesRecvd;i++)
                         {
                          
                         // printf("the recvBuf is %c\n",recvBuf[i]);
                          send_message[i-6]=recvBuf[i];
                         }
                          send_message[i-6]='\0';
                          sprintf(str_dest,"%d",send_dest);
                       //  printf("send_message is %s\n",send_message);
                        if(send_dest!=globalMyID)
                        {
                             shortest_path();
                            
                            if(forwarding[send_dest].cost!=0)
                               {
                                 recvBuf[0]='f';
                                 recvBuf[1]='o';
                                 recvBuf[2]='r';
                                 recvBuf[3]='d';
                                // printf("the new message is %s\n",recvBuf);
                                 nexthop=forwarding[send_dest].nexthop;
                                 sprintf(for_nexthop,"%d",nexthop);
                                 strcpy(log_message,"sending packet dest ");
                                 strcat(log_message,str_dest);
                                 strcat(log_message," ");
                                 strcat(log_message,"nexthop ");
                                 strcat(log_message,for_nexthop);
                                 strcat(log_message," ");
                                 strcat(log_message,"message");
                                 strcat(log_message," ");
                                 
                                 strcat(log_message,send_message);
                                 strcat(log_message,"\n");
                               //  printf("log_message is %s\n",log_message);
                                 fputs(log_message,log_events);
                                 fflush(log_events);
                                 //printf("nexthop is %d\n",nexthop);
                                 send_or_forward(recvBuf,bytesRecvd,nexthop);
                               }
                              else
                               {
                               strcpy(log_message,"unreachable dest "); 
                               strcat(log_message,str_dest);
                               strcat(log_message,"\n");
                              // printf("log_message is %s\n",log_message);
                               fputs(log_message,log_events);
                                fflush(log_events);
                               }//not reachable
                        }
                        else
                        {//receive packet message messaged[]
                         strcpy(log_message,"receive packet message ");
                         strcat(log_message,send_message);
                         strcat(log_message,"\n");
                       //  printf("log_message is %s\n",log_message);
                         fputs(log_message,log_events);
                          fflush(log_events);
                        }
                         
                         


                      //TODO send the requested message to the requested destination node
			// ...
		}
		//'cost'<4 ASCII bytes>, destID<net order 2 byte signed> newCost<net order 4 byte signed>
		else if(!strncmp(recvBuf, "cost", 4))
		{
                      
                      for_cost_neighbor1[0]='\0';
                      new_cost1[0]='\0';
                      
                      memcpy(&dest1,recvBuf+4,2);
                      memcpy(&cost1,recvBuf+6,4);
                      for_cost_neighbor=ntohs(dest1);
                      new_cost=ntohl(cost1);
                    //  printf("this is node%d,the neighbor is %d\n",globalMyID,for_cost_neighbor);
                    /* if(recvBuf[8]==0)
                      {
                      new_cost=recvBuf[9];
                      }
                     else
                     {
                      new_cost=recvBuf[8]*256+recvBuf[9];
                     }*/
                       // printf("the new_cost is %d\n",new_cost);
                        cost[for_cost_neighbor]=new_cost;
                       if(neighbors[for_cost_neighbor].connection==1)
                       {
                      
                       lsp_send();
                       }
                        
                   
              
			//TODO record the cost change (remember, the link might currently be down! in that case,
			//this is the new cost you should treat it as having once it comes back up.)
			// ...
		}
                else if(!strncmp(recvBuf, "ford", 4))
		{
                   
                   // printf("this node%d, I get a message %s\n",globalMyID,recvBuf);
                    log_message[0]='\0';
		         send_dest=0;
                         
                         memcpy(&dest1,recvBuf+4,2);
                         send_dest=ntohs(dest1);
                        for(i=6;i<bytesRecvd;i++)
                         {
                          
                         // printf("the recvBuf is %c\n",recvBuf[i]);
                          send_message[i-6]=recvBuf[i];
                         }
                          send_message[i-6]='\0';
                          sprintf(str_dest,"%d",send_dest);
                         // printf("send_message is %s\n",send_message);
                          


                         
                        if(send_dest!=globalMyID)
                        {

                            shortest_path();
                            if(forwarding[send_dest].cost!=0)
                               {
                                 nexthop=forwarding[send_dest].nexthop;
                                 sprintf(for_nexthop,"%d",forwarding[send_dest].nexthop);
                                 strcpy(log_message,"forward packet dest ");
                                 strcat(log_message,str_dest);
                                 strcat(log_message," ");
                                 strcat(log_message,"nexthop");
                                 strcat(log_message," ");
                                 strcat(log_message,for_nexthop);
                                 strcat(log_message," ");
                                 strcat(log_message,"message");
                                 strcat(log_message," ");
                                 strcat(log_message,send_message);
                                 strcat(log_message,"\n");
                               //  printf("log_message is %s\n",log_message);
                                 fputs(log_message,log_events);
                                 fflush(log_events);
                                 send_or_forward(recvBuf,bytesRecvd,nexthop);
                               }
                            else
                               {
                                 strcpy(log_message,"unreachable dest "); 
                               strcat(log_message,str_dest);
                               strcat(log_message,"\n");
                              // printf("log_message is %s\n",log_message);
                               fputs(log_message,log_events);
                               fflush(log_events);

                               }//not reachable
                        }
                        else
                        {//receive packet message messaged[]
                         strcpy(log_message,"receive packet message ");
                         strcat(log_message,send_message);
                         strcat(log_message,"\n");
                       //  printf("log_message is %s\n",log_message);
                         fputs(log_message,log_events);
                         fflush(log_events);
                        }
		}
                else if(!strncmp(recvBuf,"lsp",3))
                {
                  strcpy(recvBuf_forward,recvBuf);
                   // printf("this is node%d, the message is %s\n",globalMyID,recvBuf);
                   i=0;
                  
                   
                   lsp_forward=strtok(recvBuf,";");
                   strcpy(originator,lsp_forward);
                   //printf("the originator is %s\n",originator);
                   while(lsp_forward=strtok(NULL,";"))
                   {
                      if(i==0)
                       strcpy(cost_message,lsp_forward);
                      else if(i==1)
                       strcpy(for_seq,lsp_forward);
                       i++;
                      
                       seq1=atoi(for_seq);
                      //printf("seq is %d\n",seq);
                      //printf("the message is %s\n",lsp_forward);
                   }
                   //printf("the lsp_desicion is %s\n",lsp_desicion);
                  //printf(" the message is %s,the seq is %d, the cost message is%s \n",recvBuf_forward,seq1,cost_message);
                   if(strcmp(lsp_desicion,originator)!=0)
                   
                  
                   
                    {//1st if
                     
                     i=0;
                    
                     lsp_forward=strtok(recvBuf,";");
                     strcpy(originator,lsp_forward);
                     for(i=3;originator[i]!='\0';i++)
                     {
                      originator[i-3]=originator[i];
                    
                     }//for
                      originator[i-3]='\0';
                      send_node=atoi(originator);   
                     // printf("this is node%d, the message is %s,send_node is %d\n",globalMyID,recvBuf,send_node);
                      
                      //printf("the cost message is %s",cost_message);
                      strcpy(for_test,cost_message);
                      //printf("seq1 is %d\n",seq1);
                      //printf("the second seq is %d\n",lsp_received[send_node].seq_num);
                      //i=strlen(recvBuf_forward);
                      //printf("recvbuf_forward is %s, the length is %d\n",recvBuf_forward,i);
                     if(seq1>lsp_received[send_node].seq_num||seq1==0)//whether to update
                      {//2nd if
                         lsp_sending(recvBuf_forward, strlen(recvBuf_forward),send_node);
                        for(i=0;i<100;i++)
                         {
                          lsp_received[send_node].neighbor[i]=0;
                          lsp_received[send_node].cost[i]=0;
                          neighbor_kv[i][0]='\0';

                         }//for
                        i=0;
                        lsp_received[send_node].seq_num=seq1;
                        for_cost=strtok(cost_message,":");
                        strcpy(neighbor_kv[i],for_cost);
                       // printf("neighbor_kv is %s\n",neighbor_kv[i]);
                        
                         strcpy(test_cost,neighbor_kv[i]);
                         strcat(test_cost,":");
                         //printf("get here\n");
                         //printf("the test is %s\n",test_cost);
                         //printf("cost message is %s\n",for_test);
                         i++;  
                       if(strcmp(test_cost,for_test)!=0)
                       {//3rd if
                          
                           // printf("enter the if\n");



                        
                        while(for_cost=strtok(NULL,":"))//neighbor[i]:ID-cost
                          {
                           strcpy(neighbor_kv[i],for_cost);
                         //   printf("neighbor_kv %s\n",neighbor_kv[i]);
                           i++;
                          }


                      }//3rd if
                         //printf("get here2\n");
                        for(i=0;neighbor_kv[i][0]!='\0';i++)
                           {
                           // printf("the message is %s\n",neighbor_kv[i]);
                            for_cost=strtok(neighbor_kv[i],"-");
                            strcpy(cost_trans,for_cost);
                            //printf("get here3\n");
                            lsp_received[send_node].neighbor[i]=atoi(cost_trans);
                            //printf(\n",);
                             strcpy(cost_trans,strtok(NULL,"-"));
                            lsp_received[send_node].cost[i]=atoi(cost_trans);
                            //printf("the node is %d,send node is %d,the neighbor is %d,the cost is %d\n",globalMyID,send_node,lsp_received[send_node].neighbor[i],lsp_received[send_node].cost[i]);
                            } 
                       
                           recvBuf[0]='\0';
                      }//2nd if*/
                   }//1stif
                   
                  }//else if
               
		
		//TODO now check for the various types of packets you use in your own protocol
		//else if(!strncmp(recvBuf, "your other message types", ))
		// ... 
	}
	//(should never reach here)
	close(globalSocketUDP);
}

