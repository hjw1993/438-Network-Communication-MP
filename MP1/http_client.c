

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

 

#define MAXDATASIZE 3000 // max number of bytes we can get at once 

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
	int sockfd, numbytes=1,numbytes1=1;;  
	char buf[MAXDATASIZE];
	char relocate_buf[MAXDATASIZE];
  char relocate_buf1[MAXDATASIZE];
	char request[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];
  char parse[100];
  int i;
  char send_req[100];
  char port[50];
  int f;
  FILE* file_write;
  FILE* file_clear;
  FILE* file_relocate;
     //freopen("output.txt","w",stdout);
	if (argc !=2) {
	    fprintf(stderr,"path to file\n");
	    exit(1);
	}
     file_clear=fopen("output","w+");
     fputs("",file_clear);
     fclose(file_clear);
     file_write=fopen("output","a");
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;


printf("argv%s\n",argv[1] );
	char *for_parse = strstr(argv[1],"//");

if(for_parse!=NULL)
	{
     // parse the argument from command line
    strcpy(parse,for_parse);

	for(i=2;parse[i]!='\0';i++)
       {
       	parse[i-2]=parse[i];
       }
        parse[i-2]='\0';
        //the same as else
        // parse="www.someweb.com/pathtofile"
      //to get the request message
        //send_req=/pathtofile
        char* port_num=strstr(parse,":");
        printf("port_num is %s\n",port_num );
        //have a port
      if(port_num!=NULL)
     { 

	     strcpy(port,port_num);
        for(i=1;port[i]!='\0';i++)
       {
       	port[i-1]=port[i];
       }
        port[i-1]='\0';

        
      char* port_num2 =strstr(port,"/");
      printf("port:%s\n",port );
      printf("port_num2:%s\n",port_num2 );
      if(port_num2==NULL)

      {
      	strcpy(send_req,"/");
         
      }
      else
      {
      	char* for_send_req= strstr(parse,"/");
      	strcpy(send_req,for_send_req);
          
        printf("the send_req:%s\n",send_req );
        //parse="www.someweb.com"
        for_parse =strtok(parse,"/");
        
         printf("after parse:%s\n",parse);
         strtok(port,"/");
      }

      char* for_parse1=strtok(parse,":");
      printf("%s\n",for_parse1 );
      printf("%s\n",parse );
     }
     //http://www.someweb.com/ no port
      else
      {
        char* for_send_req= strstr(parse,"/");
        //if the argument entered like this www.someweb.com no"/
         if(for_send_req==NULL)
         {
          strcpy(send_req,"/");
         }
         else{
        strcpy(send_req,for_send_req);
          }
        printf("the send_req:%s\n",send_req );
        //parse="www.someweb.com"
        
        
         

        char* for_parse1=strtok(parse,"/");

      printf("%s\n",for_parse1 );
      printf("%s\n",parse );
      	strcpy(port,"80");
      }
}
     
        //if the argument entered like this www.someweb.com no"/
         
       // strcpy(parse,for_parse);
         
        
    
  
    //argument entered without http://
    else
    {
      strcpy(parse,argv[1]);
    	char* port_num=strstr(parse,":");
  if(port_num!=NULL)
     { 

	 strcpy(port,port_num);
        for(i=1;port[i]!='\0';i++)
       {
        port[i-1]=port[i];
       }
        port[i-1]='\0';

        
      char* port_num2 =strstr(port,"/");
      printf("port:%s\n",port );
      printf("port_num2:%s\n",port_num2 );
      if(port_num2==NULL)

      {
        strcpy(send_req,"/");
         
      }
      else
      {
        char* for_send_req= strstr(parse,"/");
        strcpy(send_req,for_send_req);
          
        printf("the send_req:%s\n",send_req );
        //parse="www.someweb.com"
        for_parse =strtok(parse,"/");
        
         printf("after parse:%s\n",parse);
         strtok(port,"/");
      }

      char* for_parse1=strtok(parse,":");
      printf("%s\n",for_parse1 );
      printf("%s\n",parse );
     }
      else
      {
      	char* for_send_req= strstr(parse,"/");
        //if the argument entered like this www.someweb.com no"/
         if(for_send_req==NULL)
         {
          strcpy(send_req,"/");
         }
         else{
        strcpy(send_req,for_send_req);
          }
        printf("the send_req:%s\n",send_req );
        //parse="www.someweb.com"
        
        
         

        char* for_parse1=strtok(parse,"/");

      printf("%s\n",for_parse1 );
      printf("%s\n",parse );
        strcpy(port,"80");      }


    }
    printf("parse%s\n",parse );
    printf("port%s\n",port );
	if ((rv = getaddrinfo( parse, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure
  request[0]='\0';
  strcat(request, "GET ");
  strcat(request,send_req);
	strcat(request, " HTTP/1.0\r\n");
	strcat(request,"HOST:");
	strcat(request,parse);
	strcat(request,"\r\n\r\n");

	printf("send:%s\n",request);
		if (send(sockfd, request ,strlen(request), 0) == -1)
				perror("send request");
      numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0);
	  // using sta to decide wether the return message is a redirection message
    char status[MAXDATASIZE] = "301 Moved Permanently";
    char *sta = strstr(buf,status);

// if it is not 301 status
if(sta==NULL){
     f=1;
    
while(numbytes!=0){

	//printf("entered while" );
	  buf[numbytes]='\0';
  if (f==1)//handling the header
  {
    char*for_buf=strstr(buf,"\r\n\r\n");
    strcpy(relocate_buf1,for_buf);
    //deleting the CRLF
    for(i=4;relocate_buf1[i]!='\0';i++)
     {
      relocate_buf1[i-4]=relocate_buf1[i];
     }
     relocate_buf1[i-4]='\0';
     printf("s:%s\n",relocate_buf1);
     printf("numbytes is %d\n, f is %d\n",numbytes,f);
     fputs(relocate_buf1,file_write);
    f=2;
  }
  else
  {
  numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0);
  buf[numbytes]='\0';
  printf("buf is %s\n",buf );
  fputs(buf,file_write);
  }
  
  //printf("numbytes is %d\n, f is %d\n",numbytes,f);
  
 }
     //printf("s:'%s'\n",buf);
     printf("output of loop" );
} 
     
     
   
     // to find if the location of certain file has changed
    
    else   //redirection
    {
      fclose(file_write);
    	FILE*clear=fopen("output","w+");
      fputs("",clear);
      fclose(clear);
      file_relocate =fopen("output","a");
    	sta = strstr(buf,"Location");
    	sta = strtok(sta, "\n");
    	sta = strstr(buf,"//");
    	char relocate[100];
    	strcpy(relocate,sta);
    
    for(i=2;relocate[i]!='\0';i++)
       {
       	relocate[i-2]=relocate[i];
       }
        relocate[i-2]='\0';
        printf("redirection:%s\n",relocate );
       close(sockfd);
       printf("start relocate\n");
       char *for_relocate =strtok(relocate,"/");
       printf("the relocate:%s\n",relocate );
        

    if ((rv = getaddrinfo(relocate, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}
    for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure
    request[0]='\0';
	strcat(request, "GET ");
    strcat(request, send_req);
	strcat(request, " HTTP/1.0\r\n");
	strcat(request, "HOST:");
	strcat(request, relocate);
	strcat(request, "\r\n\r\n");
	printf("send: %s",request);
		if (send(sockfd, request ,strlen(request), 0) == -1)
				perror("send username");

 numbytes1 = recv(sockfd, relocate_buf, MAXDATASIZE-1, 0);
 char*for_relocate_buf=strstr(relocate_buf,"\r\n\r\n");
     
      //printf("%s\n",buf );
       strcpy(relocate_buf1,for_relocate_buf);
  for(i=4;relocate_buf1[i]!='\0';i++){
      relocate_buf1[i-4]=relocate_buf1[i];
     }
     relocate_buf1[i-4]='\0';
     printf("relocate_buf1 is %s\n", relocate_buf1);
     fputs(relocate_buf1,file_relocate);
     relocate_buf[0]='\0';
while(numbytes1!=0){
  
numbytes1 = recv(sockfd, relocate_buf, sizeof(relocate_buf), 0);
relocate_buf[numbytes1]='\0';
fputs(relocate_buf,file_relocate);
printf("numbytes1 is %d\n",numbytes1);



}

}
printf("sockfd closed" );
close(sockfd);

	return 0;
}
