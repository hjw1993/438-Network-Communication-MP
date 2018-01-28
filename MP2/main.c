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

struct 
{
   int connection;
   int cost;
}neighbors[256];
struct
{
  int n_name;
  int cost;
}cost_test;
 
struct lsp_message
{
  int origin_node;
  int neighbor[100];
  int cost_to_neighbor[100];
 
  int seq_num;
}; 
struct lsp_received
{
  
  int neighbor[100];
  int cost[100];
  int seq_num;
}lsp_received[256];
 
struct 
{  
  int destination;
  int nexthop;
  int cost;
  int before_dest;
  
}forwarding[256];

struct 
{  
  int validation;
  int destination;
  int nexthop;
  int cost;
  int before_dest;
  
}tentative[256];

int chosen;
int call_count=0;
int cost[256];  
int seq;
FILE* log_events;

void listenForNeighbors();
void* announceToNeighbors(void* unusedParam);
void* print_test(void* unusedParam);
void* cut_neighbor(void* unusedParam);
void* listen_heartbeat(void* unusedParam);

void lsp_send();
void shortest_path();
void updating_tentative();
void ten_to_confirm();
int globalMyID = 0;
int chosen;
int updating_flag=1;
//last time you heard from each node. TODO: you will want to monitor this
//in order to realize when a neighbor has gotten cut off from you.
struct timeval globalLastHeartbeat[256];
struct timeval cut_neighborlfc[256];
struct timeval listen_neighbor[256];


//our all-purpose UDP socket, to be bound to 10.1.1.globalMyID, port 7777
int globalSocketUDP;
//pre-filled for sending to 10.1.1.0 - 255, port 7777
struct sockaddr_in globalNodeAddrs[256];

int main(int argc, char** argv)
{
        char cost_file[20];
	if(argc != 4)
	{
		fprintf(stderr, "Usage: %s mynodeid initialcostsfile logfile\n\n", argv[0]);
		exit(1);
	}
 
	FILE* f_cost;
        f_cost=fopen(argv[2],"r");
      //  printf("the iniatial cost is \n");
       while( fscanf(f_cost,"%d%d",&cost_test.n_name,&cost_test.cost)!=EOF)
     {
        
            cost[cost_test.n_name]=cost_test.cost;
          //  printf("this is node%d, the neigbor is %d, the cost is %d\n",globalMyID,cost_test.n_name,cost_test.cost);
            
     } 
        //printf("the argv is %s\n",argv[3]);
        log_events=fopen(argv[3],"w");
        
        seq=1;
	//initialization: get this process's node ID, record what time it is, 
	//and set up our sockaddr_in's for sending to the other nodes.
	globalMyID = atoi(argv[1]);
	int i;
	for(i=0;i<256;i++)
	{
		gettimeofday(&globalLastHeartbeat[i], 0);
		
		char tempaddr[100];
		sprintf(tempaddr, "10.1.1.%d", i);
		memset(&globalNodeAddrs[i], 0, sizeof(globalNodeAddrs[i]));
		globalNodeAddrs[i].sin_family = AF_INET;
		globalNodeAddrs[i].sin_port = htons(7777);
		inet_pton(AF_INET, tempaddr, &globalNodeAddrs[i].sin_addr);
	}
	
	//TODO: read and parse initial costs file. default to cost 1 if no entry for a node. file may be empty.
	
	//socket() and bind() our socket. We will do all sendto()ing and recvfrom()ing on this one.
	if((globalSocketUDP=socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("socket");
		exit(1);
	}
       
	char myAddr[100];
	struct sockaddr_in bindAddr;
	sprintf(myAddr, "10.1.1.%d", globalMyID);	
	memset(&bindAddr, 0, sizeof(bindAddr));
	bindAddr.sin_family = AF_INET;
	bindAddr.sin_port = htons(7777);
	inet_pton(AF_INET, myAddr, &bindAddr.sin_addr);
	if(bind(globalSocketUDP, (struct sockaddr*)&bindAddr, sizeof(struct sockaddr_in)) < 0)
	{
		perror("bind");
		close(globalSocketUDP);
		exit(1);
	}
        
	
	//start threads... feel free to add your own, and to remove the provided ones.
	pthread_t announcerThread;
	pthread_create(&announcerThread, 0, announceToNeighbors, (void*)0);
	//pthread_t listenforheartbeat;
      //  pthread_create(&listenforheartbeat, 0, listen_heartbeat, (void*)0);
        pthread_t printtest;
        pthread_t supervise_neighbor;
        pthread_t first_compute1;
        
	
      // pthread_create(&supervise_neighbor,0, cut_neighbor,(void*)0);
      // pthread_create(&printtest,0, print_test,(void*)0);
       // pthread_create(&first_compute1,0,first_compute,(void*)0);
        listenForNeighbors();
        
}
