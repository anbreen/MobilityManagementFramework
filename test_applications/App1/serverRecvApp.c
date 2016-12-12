    #include  <stdio.h>
    #include  <stdlib.h>
    #include  <unistd.h>
    #include  <errno.h>
    #include  <string.h>
    #include  <sys/types.h>
    #include  <sys/socket.h>
    #include  <netinet/in.h>
    #include  <arpa/inet.h>
    #include  <sys/wait.h>
    #include  <signal.h>
    #include <netinet/tcp.h>

    #define MYPORT 3490 // the port users will be connecting to
    #define MAXDATASIZE 100 // max number of bytes we can get at once
    #define MAXARRAYLENGTH 100
    #define BACKLOG 10    // how many pending connections queue will hold

	struct file_info
	{
		char name[100];
	}obj;

    void sigchld_handler(int s)
    {
	while(waitpid(-1, NULL, WNOHANG) > 0);
    }
    int main(void)
    {
	int sockfd, new_fd, optlen, numbytes, i=0, j=0; // listen on sock_fd, new connection on new_fd
	int array[MAXARRAYLENGTH];
	struct sockaddr_in my_addr; // my address information
	struct sockaddr_in their_addr; // connector's address information
	struct tcp_info info;
	socklen_t sin_size;
	char buf[MAXDATASIZE];
	struct sigaction sa;
	int yes=1;
	char *line=NULL;
	char *name;
	    size_t len = 0;
	ssize_t read;
      
	
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	    perror("socket");
	    exit(1);
	}
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
	    perror("setsockopt");
	    exit(1);
	}
	my_addr.sin_family = AF_INET;         // host byte order
	my_addr.sin_port = htons(MYPORT);     // short, network byte order
	my_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
	memset(my_addr.sin_zero, '\0', sizeof my_addr.sin_zero);
	if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof my_addr) == -1) {
	    perror("bind");
	    exit(1);
	}
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
	   
	    
	while(1) { // main accept() loop
	    sin_size = sizeof their_addr;
	    if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, \
		    &sin_size)) == -1) {
		perror("accept");
		continue;
	    }
	    printf("server: got connection from %s\n", \
		inet_ntoa(their_addr.sin_addr));
		printf("%d\n", getpid());
	    if (!fork()) { // this is the child process
		close(sockfd); // child doesn't need the listener

		u_int8_t length;		
		if(recv(new_fd,&length,sizeof(u_int8_t),0) == -1)
		{
			perror("recv");
			exit(0);
		}
		name = malloc(length+1);
		memset(name,'\0',length+1);
		if(recv(new_fd,name,length,0) == -1)
		{
			perror("recv");
			exit(0);
		}
		strcat(name,"\0");
		FILE *fs = fopen(name, "wb");
		int count = 0;
		do
		{
			memset(buf,'\0',MAXDATASIZE);
		    if ((numbytes=recv(new_fd, buf, MAXDATASIZE-1, 0)) == -1) 
		    {
				    perror("recv");
				    exit(1);
			}
			if (numbytes==0)
			{
			printf("Exiting because no data");	
			break;
			}
			buf[numbytes] = '\0';
			count += (numbytes);
			if( fwrite(buf,sizeof(char),numbytes,fs) <= 0)
			{
				printf("Errot Writing File");
			}
//			fprintf(fs, "%s", buf);
			//printf( "%s", buf);
	     }while(1);
		printf("\n%d\n",count);
		printf("exited: \n");
		fclose(fs);
		close(new_fd);
		exit(0);
	    }
	    close(new_fd); // parent doesn't need this
	    
	}
	return 0;
    }
