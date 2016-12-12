  #include  <stdio.h>
  #include  <stdlib.h>
  #include  <unistd.h>
  #include  <errno.h>
  #include  <string.h>
  #include  <netdb.h>
  #include  <sys/types.h>
  #include  <netinet/in.h>
  #include  <sys/socket.h>
  #include "emf_sock_lib.h"
  
  #define PORT 3490 // the port client will be connecting to
  #define MAXDATASIZE 2046 // max number of bytes we can get at once
  #define MAXARRAYLENGTH 100
	
	struct file_info
	{
		char name[100];
	}obj;

  int main(int argc, char *argv[])
  {
      int sockfd, numbytes;
      char buf[MAXDATASIZE];
      
      int array[MAXARRAYLENGTH], i, j;
      char *line=NULL;
      size_t len = 0;
      ssize_t read;
      char *hostname = "localhost";
      struct hostent *he;
      struct sockaddr_in their_addr; // connector's address information
      if (argc != 3) {
	  fprintf(stderr,"usage: client hostname filename_to_send\n");
	  exit(1);
      }
      if ((he=gethostbyname(argv[1])) == NULL) {   // get the host info
	  herror("gethostbyname");
    
	  exit(1);
      }
      if ((sockfd = emf_socket(PF_INET, SOCK_STREAM, 0)) == -1) {
	  perror("socket");
	  exit(1);
      }
            //printf("sockfd 1: %d\n", sockfd);
      their_addr.sin_family = AF_INET;    // host byte order
      their_addr.sin_port = htons(PORT); // short, network byte order
      their_addr.sin_addr = *((struct in_addr *)he->h_addr);
      memset(their_addr.sin_zero, '\0', sizeof their_addr.sin_zero);
      
     // printf("sockfd 2: %d\n", sockfd);
      if (emf_connect(sockfd, (struct sockaddr *)&their_addr,
					    sizeof their_addr) == -1) {
	 // printf("out of hell\n");
	  perror("connect");
	  
      }   
      /******************************************************/
     // printf("sockfd 3: %d\n", sockfd);
      FILE *fs = fopen(argv[2], "rb");
	  if(!fs){
		  perror("File");
		  exit(1);
		  }
	u_int8_t length= strlen(argv[2]);
	if(emf_send(sockfd,&length,sizeof(u_int8_t),0) == -1)
	{
		perror("send");
		exit(0);
	}
	
	if(emf_send(sockfd,argv[2],length,0) == -1)
	{
		perror("send");
		exit(0);
	}
	  i=0;
	  int ret;
	  int ret1 =0;
	int count=0;
	  
	while (1) 
	  {
	memset(buf,'\0',MAXDATASIZE);
	  ret = fread(buf,sizeof(char),2046,fs);
	      if(ret <= 0)
		      break;
		      printf("\n\nfread %d\n",ret);
//      ret1=0;
  //    int ret3=ret;
	  int c = 0;
	//  	do
	  //	{  
	  printf("I am going to send now\n");
	  		if ((c=emf_send(sockfd, buf, ret, 0)) != -1)
	  		{	printf("c: %d , ret: %d\n",c,ret);
	  			/*if(ret==c)
	  			{	//printf("breaking\n");
	  				break;
	  			}
	  			else
	  			{*/
	  				/*ret1+=c;
	  				count += ret1;
	  				ret3 -= c;
	  				printf("%d:\n",strlen(&buf[ret1]));
	  				if(strlen(&buf[ret1]) == 0)
	  				break;*/
	  			//}
	  		}
	  		else
	  		{
	  			perror("send");
	  			break;
	  		}
	  		
	  		
//		}while(ret3>0);
      
	  }
      
//	printf("%d",count);
      printf("exited: \n");
      fclose(fs);
      emf_close(sockfd);
      return 0;
  }

