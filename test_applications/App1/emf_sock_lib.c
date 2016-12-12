
#include "emf_sock_lib.h"

#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <netinet/tcp.h>

#define id_socket 	1
#define id_connect 	2
#define id_bind 	3
#define id_send 	4
#define id_recv 	5
#define id_sendto 	6
#define id_recvfrom 	7
#define id_close 	8
#define id_getpeername  9
#define id_sendmsg      10
#define id_recvmsg      11
#define id_getsockopt   12
#define id_setsockopt   13
#define id_listen       14
#define id_accept       15
#define id_shutdown     16
#define id_sockatmark   17
#define id_socketpair   18
#define id_getsockname  20
#define id_isfdtype     22

#define id_emfserverRes 	1
#define id_serverRes 		0
#define id_errorRes 		-1

#define LIB_PORT     		8883

//-----------------------------------------------------------------------------------
int globalsockFD = -1;           //Descriptor is b/w sock_lib and emf_client
//-----------------------------------------------------------------------------------

struct sockLib_emf_client
{
	int         id;         //One of the above identifiers
	int         aapSockFD;  //Sock Descriptor used to send and received data from and to server
	struct      sockaddr_in serverAddr; //Server Address
	socklen_t   sin_size;   //Size of the Server Address
	size_t      packetSize; //Size of the Whole packet
	int         flags;      //Flag bit used as the last argument in snd/recv
	pid_t 		app_pid;
}
sockLib_client_TLV;

struct direct_conn_list
{
	struct direct_conn_list *prev;
	int sockfd;
	int flag;							// 1 means connected through EMF_CORE, 0 means direct connection
	struct direct_conn_list *next;
}
*head, *tail;


//================================ Append-Node =========================================
void append_node(struct direct_conn_list *node)
{	
	if(head == NULL)
		{
		head = node;
		node->prev = NULL;
		}
	else
		{
		tail->next = node;
		node->prev = tail;
		}

	node->next = NULL;
	tail = node;
}


//================================ Remove-Node =========================================
void remove_node(struct direct_conn_list *node)
{	

if(node->prev == NULL && node->next==NULL)
	{
	head = NULL;
	tail =NULL;
	}
else
	{
	if(node->prev == NULL)
		{
		head = node->next;
		node->next->prev = NULL;
		}
	else
		{
		node->prev->next = node->next;
		}

	if(node->next == NULL)
		{
		node->prev->next = NULL;
		tail = node->prev;
		}
	else
		{
		node->next->prev = node->prev;
		}
	}	
	free(node);
}


//================================EMF-Socket=========================================
int emf_socket (int sock_family, int sock_type, int protocol)
{	
   return(socket(sock_family, sock_type, protocol) );
}



//================================EMF-Connect========================================
int emf_connect( int sockfd, struct sockaddr *serv_addr, socklen_t addrlen  )
{
	int ReceivedData, buffer = -1;
	struct sockaddr_in server_addr;
	struct direct_conn_list *dcl_node;
	dcl_node = malloc(sizeof(struct direct_conn_list));

	struct sockaddr_in 	their_clientAddr;
	struct sockaddr		my_addr;
	socklen_t len;
	
	printf("globalsockfd: %d\n", globalsockFD);
	
	if(globalsockFD == -1)
		{
		if ((globalsockFD = socket(PF_INET, SOCK_STREAM, 0)) == -1)
			{
			perror("global socket");		   
			}
		else    
		if( getpeername(globalsockFD, (struct sockaddr*)&my_addr, &len) == -1 ) 		//if Not connected YET, connect to EMF_CORE
			{   
			their_clientAddr.sin_family = AF_INET;    					// host byte order
			their_clientAddr.sin_port = htons(LIB_PORT); 					// short, network byte order
			their_clientAddr.sin_addr.s_addr = inet_addr("127.0.0.1");			// local host address
			memset(their_clientAddr.sin_zero, '\0', sizeof their_clientAddr.sin_zero);	// memset

			if (connect(globalsockFD, (struct sockaddr *)&their_clientAddr, sizeof their_clientAddr) == -1)
				{
				perror("connect to EMF_CORE");
				close(globalsockFD);
				globalsockFD = -1;
				return(connect(sockfd, (struct sockaddr *)serv_addr, addrlen) );
				}
			else
				{
				printf("Connected to EMF_CORE\n");      
				}
			}
		else
			printf("already connected\n");
		}
		
		printf("globalsockfd: %d\n", globalsockFD);
		sockLib_client_TLV.id = id_connect;
		sockLib_client_TLV.aapSockFD = sockfd;
		server_addr = * ((struct sockaddr_in *) serv_addr);
  	
		sockLib_client_TLV.serverAddr.sin_family = server_addr.sin_family;
		sockLib_client_TLV.serverAddr.sin_port = server_addr.sin_port;
		sockLib_client_TLV.serverAddr.sin_addr = server_addr.sin_addr;
		sockLib_client_TLV.app_pid = getpid();
		sockLib_client_TLV.sin_size = addrlen;
	
		dcl_node->sockfd = sockfd;
		dcl_node->flag  = 0;
		
		append_node(dcl_node);
	
		printf("sending connection request to EMF_CORE\n");
		if ((send(globalsockFD, &sockLib_client_TLV, sizeof (struct sockLib_emf_client), 0)) <= 0)
			{
			perror("send to EMF_CORE\n");
			close(globalsockFD);
			globalsockFD = -1;
			return(connect(sockfd, (struct sockaddr *)serv_addr, addrlen) );
			}
		else
			{  
			printf("receiving reply from EMF_CORE\n");
			if(ReceivedData = recv(globalsockFD, &buffer, sizeof(buffer), 0) <= 0)
				{		
				perror("recv from EMF_CORE");
				close(globalsockFD);
				globalsockFD = -1;
				return(connect( sockfd, (struct sockaddr *)serv_addr, addrlen) );
				}
			else
				{
				printf("reply from EMF_CORE: %d\n", buffer);	
				if(buffer < 0)
					{	
					if(connect( sockfd, (struct sockaddr *)serv_addr, addrlen) == -1)
						{
						perror("connect");
						close(globalsockFD);
						globalsockFD = -1;
						return -1;
						}
					else
						{
						printf("connected directly...\n");
						close(globalsockFD);
						globalsockFD = -1;
						return(0);
						}
					}
				else
					{
					printf("connected through EMF_CORE...\n");
					dcl_node->flag  = 1;
				return 0;
					}
				}	
			}
}




//================================EMF-Send========================================
ssize_t emf_send (int sockfd,void *buf, size_t n, int flags)
{
	int ret1 = 0;

	//printf("** receive from the application : %d\n",n );

	struct direct_conn_list *dcl_node;
	
	for(dcl_node = head; dcl_node != NULL; dcl_node = dcl_node->next)
		{
		if(dcl_node->sockfd == sockfd)
			{
			if(dcl_node->flag == 1)
				{
				//printf("found the socket\n");
				break;
				}
			
			printf("returning earlier...\n");
			return( send(sockfd, buf, n, flags) );
			}
		}
		if(dcl_node == NULL)
			{
			printf("returning earlier...1\n");
			return( send(sockfd, buf, n, flags) );
			}
		
		


	   	//if(n==NULL)
	   	//n=0;
	   	
//	   	if(n>0 && (strlen(buf))>0)
	//   	{
	   	printf("n: %d, buf: %d\n",n,strlen(buf));
	   	if ((ret1= send(globalsockFD, buf, n, 0)) <= 0)
			{  

			perror("2send to EMF_CORE");
			printf("ret value while error %d\n", ret1);
			return -1;
			}
	
		printf("returning success %d\n",ret1);
		return ret1 ;
		//}
//		else
	//	return (strlen(buf));
}



//================================EMF-Recv========================================
ssize_t emf_recv (int sockfd, void *buf, size_t n, int flags)
{
	int ReceivedData;
	struct direct_conn_list *dcl_node;
	
	for(dcl_node = head; dcl_node != NULL; dcl_node = dcl_node->next)
		{
		if(dcl_node->sockfd == sockfd)
			{
			if(dcl_node->flag == 1)
				break;
				
			printf("returning earlier...\n");
			return( recv(sockfd, buf, n, flags) );
			}
		}
	if(dcl_node == NULL)
		{
		printf("returning earlier...2\n");
		return( recv(sockfd, buf, n, flags) );
		}
		
	ReceivedData = recv(globalsockFD, buf, n, 0);
	return ReceivedData;
}



//================================EMF-Close========================================
/*int emf_close(int sockfd1)
{
	int ReceivedData;
	struct direct_conn_list *dcl_node;
	dcl_node = malloc(sizeof(struct direct_conn_list));
	struct sockaddr_in 	their_clientAddr;
	struct sockaddr		my_addr;
	socklen_t len;
	//int globalsockFD1 =0;
	printf("closing globalsockfd: %d\n", globalsockFD);
	      		
	for(dcl_node = head; dcl_node != NULL; dcl_node = dcl_node->next)
		{
		if(dcl_node->sockfd == sockfd1)
			{
			if(dcl_node->flag == 1)
				break;
			
			printf("Exiting...\n");
			return( close(sockfd1) );
			}
		}
	if(dcl_node == NULL)
		{
		printf("Exiting1...\n");
		return( close(sockfd1) );
		}
	      		
	/*if(globalsockFD == -1)
		{
		if ((globalsockFD = socket(PF_INET, SOCK_STREAM, 0)) == -1)
			{
			perror("global socket");		   
			}
		else    
		if( getpeername(globalsockFD, (struct sockaddr*)&my_addr, &len) == -1 ) //if Not connected YET, connect to EMF_CORE
			{   
			printf("I am getting peer name\n");
			their_clientAddr.sin_family = AF_INET;    							// host byte order
			their_clientAddr.sin_port = htons(LIB_PORT); 					// short, network byte order
			their_clientAddr.sin_addr.s_addr = inet_addr("127.0.0.1");			// local host address
			memset(their_clientAddr.sin_zero, '\0', sizeof their_clientAddr.sin_zero);	// memset

			if (connect(globalsockFD, (struct sockaddr *)&their_clientAddr, sizeof their_clientAddr) == -1)
				{
				perror("NOT connect to EMF_CORE");
				close(globalsockFD);
				globalsockFD = -1;
				//return(connect(sockfd1, (struct sockaddr *)serv_addr, addrlen) );
				}
			else
				{
				printf("Connected to EMF_CORE\n");      
				}
			}
		else
			printf("already connected\n");
		}*/
	
	//printf("globalsockfd: %d\n", globalsockFD);
	/*sockLib_client_TLV.id = id_close;
	sockLib_client_TLV.aapSockFD = sockfd1;
  	sockLib_client_TLV.app_pid = getpid();
	
	
	
	if ((send(globalsockFD, &sockLib_client_TLV, sizeof (struct sockLib_emf_client), 0)) <= 0)
		{
		perror("send to EMF_CORE\n");
		close(globalsockFD);
		globalsockFD = -1;
		
		}
	else
		{  
		int response=0;
		printf("receiving reply from EMF_CORE\n");
		
		if(ReceivedData = recv(globalsockFD, &response, sizeof(int), 0) <= 0)
			{		
			perror("recv from EMF_CORE");
			close(globalsockFD);
			globalsockFD = -1;
			}
		else
			{

			if(response ==-1 )
				{
				printf("assoc node not found");
				}
			else
				{
				printf("assoc node found");
				ssize_t ret;

				remove_node(dcl_node);
				int ret1 = close(globalsockFD);
				globalsockFD = -1;
				printf("Exiting2...\n");
				return ret1;
				}
			}
		}	
}*/



int emf_close(int sockfd)
{
   char payload[10];    

	ssize_t ret;
	int ReceivedData;
	struct direct_conn_list *dcl_node;
	
	for(dcl_node = head; dcl_node != NULL; dcl_node = dcl_node->next)
	{
		if(dcl_node->sockfd == sockfd)
		{
			if(dcl_node->flag == 1)
				break;
				
			printf("Exiting...\n");
			return( close(sockfd) );
		}
	}
	if(dcl_node == NULL)
	{
		printf("Exiting1...\n");
		return( close(sockfd) );
	}
		
	/*sockLib_client_TLV.id = id_close;
	sockLib_client_TLV.aapSockFD = sockfd;
   sockLib_client_TLV.app_pid = getpid();
	if ((send(globalsockFD, &sockLib_client_TLV, sizeof (struct sockLib_emf_client), 0)) <= 0)
	{
		perror("send to EMF_CORE");
	}
	else
	{
		if((ReceivedData = recv(globalsockFD, &ret, sizeof (ssize_t), 0)) != -1)      		
		{
			if(ret != -1)
			{*/
				remove_node(dcl_node);
				int ret_close = close(globalsockFD);
				globalsockFD = -1;
				printf("Exiting3...\n");
								
				/*printf("Exiting2...\n");*/
				return ret_close;
			//}
   		//}
	   	//else
		//return(-1);
	//}
	
//}*/


}
//================================ EMF-Accept ========================================
int emf_accept (int sockfd, struct sockaddr *their_clientAddr, socklen_t *addrlen)
{
   int new_fd;
   struct direct_conn_list *dcl_node;
   dcl_node = malloc(sizeof(struct direct_conn_list));
   struct sockaddr_in server_addr;
   
   // calling accept with the same parameters
   if((new_fd=accept(sockfd, (struct sockaddr *)&their_clientAddr, addrlen)) == -1)
   	{
	perror("accept");
	exit(1);
   	}
   
   //extracting the sock address structure
   server_addr = *((struct sockaddr_in *)&their_clientAddr);
   printf("emf_accpet: got connection from PORT %i \n", ntohs(server_addr.sin_port));
   
   //checking if request was from EMF server or is direct
   if (ntohs(server_addr.sin_port) == 9999)
	   printf("\nemf_accept: connection request from emf server");
   else
   	{
	   printf("\nemf_accept: direct connection request from remote end");
	   dcl_node->sockfd = new_fd;
	   dcl_node->flag  = 0;
	   append_node(dcl_node);   	
   	}
   
   //printf("\nemf_accpet: Got Connection from Remote end %s : %d \n", inet_ntoa(server_addr.sin_addr), new_fd);
        
   //printf("\nemf_accpet: called by server application\n");
   
   return new_fd; 
}





/*

int EMF_getpeername (int sockfd,struct sockaddr *their_clientAddr ,
			socklen_t addrlen){
			
}*/

/*
			
ssize_t EMF_send (int sockfd,void *buf, size_t n, int flags){

  }

ssize_t EMF_recv (int sockfd, void *buf, size_t n, int flags){


        if(recv(sockfd, buff,sizeof(buff),0)== -1)
      {
	printf("receive file error");
	exit(1);
      }

}

ssize_t EMF_sendto (int sockfd,void *buf, size_t  n,
		       int flags,struct sockaddr *their_clientAddr,
		       socklen_t addrlen){
}
		       
		       
ssize_t EMF_recvfrom (int sockfd, void *buf, size_t n,
			 int flags,struct sockaddr *their_clientAddr,
			 socklen_t *addrlen){
}			 	
			 
ssize_t EMF_sendmsg (int sockfd,const struct msghdr *msg,
			int flags){
}
			
ssize_t EMF_recvmsg (int sockfd, struct msghdr *msg, int flags){

}	

int EMF_getsockopt (int sockfd, int level, int optname,
		       void *optval,
		       int *optlen){
}
		       
		       
int EMF_setsockopt (int sockfd, int level, int optname,
		        void *__optval, int optlen){
}
		       
		 	  
int EMF_listen (int sockfd, int backlog){
       if(listen(sockfd,BACKLOG) == -1)
    {
      perror("listen");
      exit(1);
    }

}

		   
int EMF_shutdown (int sockfd, int how){

}

int EMF_sockatmark (int sockfd){

}		   */
