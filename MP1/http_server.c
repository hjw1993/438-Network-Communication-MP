/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


  
#define BACKLOG 100// how many pending connections queue will hold
#define MAXDATASIZE 1024
void *client_thread(void *com_var)
{
        int read_byte;      
	int new_id;
	int numbytes;
	 char buf[MAXDATASIZE];
    char path[100];
    char message_buf[MAXDATASIZE];
    char copy_buf[MAXDATASIZE];
    int fstream;
    char line[256];
    int i;
    //byte for_byte[1000];
    new_id = (int)(*((int*)com_var));
	if ((numbytes = recv(new_id, buf, MAXDATASIZE-1, 0)) == -1) {
	    perror("recv");
	    
	} 
	     buf[numbytes] = '\0';
     printf("s: %s\n",buf);
     char *path_file=strstr(buf,"/" );
     path_file=strtok(path_file," ");
     strcpy(path,path_file);
     if((strcmp(path,"/"))==0)
     strcpy(path,"/index.html");
     for(i=1;path[i]!='\0';i++ )
     {
     	path[i-1]=path[i];
     }
     path[i-1]='\0';
     printf("the path is : %s\n",path);
     fstream=open(path,O_RDONLY);
     //int fd = fstream->_file;
     printf("fstream is defined\n" );
     if(fstream == -1)
     {
        message_buf[0]='\0';
     	strcpy(message_buf,"HTTP/1.0 404 Not Found\r\n\r\n");
      //  perror("fopen");
      //  printf("\n");
        if (send(new_id, message_buf, strlen(message_buf), 0) == -1)
				perror("send");
printf("send response: %s\n",message_buf);
            close(new_id);
     }
     else
     {
     	
            message_buf[0]='\0';
			strcat(message_buf,"HTTP/1.0 200 OK\r\n\r\n");
			write(new_id, message_buf, strlen(message_buf));
			//printf("message_buf is %s\n",message_buf );	
			
			printf("works here\n");
			message_buf[0]='\0';
        while ((read_byte=read(fstream,message_buf,MAXDATASIZE))>0)
        {   
        	//strcpy(message_buf,copy_buf);
        	//printf("enter loop\n" );
                printf("using write now \n");
        	printf("messgae is %s\n",message_buf );
        	printf("messagelength is %lu\n",strlen(message_buf) );
          write(new_id, message_buf, read_byte);
          
        }
       // printf("copy is %s\n",fgets(copy_buf,MAXDATASIZE-1,fstream) );
      // strcpy(message_buf,copy_buf);
       //send(new_id, message_buf, strlen(message_buf), 0);
        close(new_id);
        //printf("thread_create function is working!\n");
        //printf("message_buf:%s\n",message_buf);

        //printf("messagelength is %lu\n",strlen(message_buf) );
        //fclose(fstream);
      	  //printf("fstream close" );
        //if (send(new_id, message_buf, strlen(message_buf), 0) == -1)
				//perror("send");
        
      new_id=0;  
     }


}


void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}



int main(int argc, char *argv[])
{
	int numbytes;
	int sockfd, new_fd[100];  // listen on sock_fd, new connection on new_fd
	int n=0;
        struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;
	pthread_t tid;
   int thread_rv;
   if (argc != 2) {
	    fprintf(stderr,"port number!!!\n");
	    exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connections...\n");

	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		new_fd[n] = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd[n] == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		printf("server: got connection from %s\n", s);
        if((thread_rv=pthread_create(&tid,NULL,client_thread,&new_fd[n]))!=0)
        	{
        		perror("pthread_create");
        	};
		// this is the child process
		// child doesn't need the listener
			printf("out of pthread function\n");
                               
			
		while(new_fd[n]!=0) n=(n+1)%100;	
		
		  // parent doesn't need this
	}

	return 0;
}


