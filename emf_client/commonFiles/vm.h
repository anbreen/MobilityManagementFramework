///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//					VIRTUAL HEADER FILE FOR EMF PROJECT ON WINDOWS PLATFORM
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/*	All necessary Header files for socket programming in LINUX platform are included here.		*/

#ifdef _WIN32

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys\types.h>
#include<winsock2.h>
#include <signal.h>
#include<windows.h>
#include<ws2tcpip.h>
#include<winbase.h>
#include<malloc.h>
#include<time.h>
typedef unsigned __int8 u_int8_t;

#else

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <netinet/tcp.h>
#include<pthread.h>
#endif

#ifdef _WIN32
int START_FLAG = 0;
#endif


/*	vm_socket() Function for creating socket. Same arguements as in regular socket() function in socket programming.
	This function also has the setting up procedure for the Winsock application		*/


int vm_socket(int af,int type,int protocol)
{
#ifdef _WIN32
	int sock_fd;
	if(START_FLAG == 0)
	{
		WSADATA wsadata;
		if (WSAStartup(MAKEWORD(1,1), &wsadata) == SOCKET_ERROR) 
		{
			printf("Error initializing socket application in WINDOWS");
			exit(0);
		}
	START_FLAG = 1;
	}
	return sock_fd = socket(af,type,protocol);
#else
	return socket(af,type,protocol);
#endif
}


/*	vm_bind() Fuction associates a local address with the socket. Same arguements as in regular bind() function in socket programming	*/


int vm_bind(int sockd,struct sockaddr *add,int len)
{
	return bind(sockd,add,len);
}


/*	vm_listen() Fuction listen for the connections on the mentioned socket. Same arguements as in regular listen() function in socket programming	*/


int vm_listen(int sock,int back_log)
{
	return listen(sock,back_log);
}


/*	vm_accept() Fuction permits an incoming connection attempt on a socket.
	One change as compared to the regular accept() function in socket programming i.e. Third arguement is a void pointer.
	So you dont need now to type cast the arguement, just pass the variable address from the calling place.
*/


int vm_accept(int sock,struct sockaddr *add,void *addr_len)
{
#ifdef _WIN32
	return accept(sock,add,(int *)addr_len);
#else
	return accept(sock,add,(socklen_t *)addr_len);
#endif
}


/*	vm_recv() Fuction receives data from a connected socket or a bound connectionless socket. 
	One change as compared to the regular recv() function in socket programming i.e. Second arguement is a void pointer.
	So you dont need now to type cast the arguement, just pass the variable address from the calling place.
*/


int vm_recv(int sock,void *buff,int len,int flags)
{
#ifdef _WIN32
	return recv(sock,(char *)buff,len,flags);
#else
	return recv(sock,buff,len,flags);
#endif
}

/*	vm_recv() Fuction receives data from a connected socket or a bound connectionless socket. 
	One change as compared to the regular send() function in socket programming i.e. Second arguement is a void pointer.
	So you dont need now to type cast the arguement, just pass the variable address from the calling place.
*/

int vm_send(int sock,void *buff,int len,int flags)
{
#ifdef _WIN32
	return send(sock,(char *)buff,len,flags);
#else
	return send(sock,buff,len,flags);
#endif
}


/*	vm_close() Function closes an existing socket. 
	Same arguements as in regular close() function in socket programming. This function also include the cleanup procedure
	for the Winsock application.	*/


int vm_close(int socket)
{
#ifdef _WIN32

	return closesocket(socket);

#else

	return close(socket);

#endif
}


/*	vm_connect() Function establishes a connection to a specified socket. 
	Same arguements as in regular connect() function in socket programming	*/


int vm_connect(int socket,struct sockaddr *name,int name_len)
{
#ifdef _WIN32
	return connect(socket,name,name_len);
#else
	return connect(socket,name,name_len);
#endif
}

/*	vm_setsockopt() Function sets a socket option. 
	One change as compared to the regular setsockopt() function in socket programming i.e. Fourth arguement is a void pointer.
	So you dont need now to type cast the arguement, just pass the variable address from the calling place.
*/


int vm_setsockopt(int socket,int level,int optname,void *optval,int opt_len)
{
#ifdef _WIN32
	return setsockopt(socket,level,optname,(char *)optval,opt_len);
#else
	return setsockopt(socket,level,optname,optval,opt_len);
#endif
}


/*	vm_getsockopt() Function retrieves a socket option. 
	Two changes as compared to the regular getsockopt() function in socket programming i.e. Last two arguements are void pointers.
	So you dont need now to type cast the arguement, just pass the address of the variable from the calling place.
*/


int vm_getsockopt(int socket,int level,int optname,void *optval,void *opt_len)
{
#ifdef _WIN32
	return getsockopt(socket,level,optname,(char *)optval,(int *)opt_len);
#else
	return getsockopt(socket,level,optname,optval,(socklen_t *)opt_len);
#endif
}


int vm_sendto(int socket,void *buff,size_t len,int flag,void *to,socklen_t tolen)
{
#ifdef _WIN32
	return sendto(socket,(char *)buff,len,flag,(struct sockaddr *)to,tolen);
#else
	return sendto(socket,buff,len,flag,(struct sockaddr *)to,tolen);
#endif

}

int vm_recvfrom(int socket,void *buff,size_t len,int flag,void *from,void *fromlen)
{
#ifdef _WIN32
	return recvfrom(socket,(char *)buff,len,flag,(struct sockaddr *)from,(int *)fromlen);
#else
	return recvfrom(socket,buff,len,flag,(struct sockaddr *)from,(socklen_t *)fromlen);
#endif
}

void vm_sleep(int seconds)
{
#ifdef _WIN32
	Sleep(seconds*1000);
#else
	sleep(seconds);
#endif
}

void vm_exit()
{
#ifdef _WIN32
	WSACleanup();
#else
	return;
#endif
}


//This function creates and runs a new thread. First arguement is it's id, 2nd arguement is the security attributes for that thread, can be NULL, 3rd
//arguement is the function pointer that is to be run as thread, 4th arguement is arguement of the function, 5th and 6th arguement is stack size and
//flag. That can be passed as 0.

int vm_create_thread(void *thread_id,void *attributes,void *(*function_name)(void *),void *arg,size_t stack_size,int flag)
{
#ifdef _WIN32
	if(CreateThread((LPSECURITY_ATTRIBUTES)attributes,stack_size,(LPTHREAD_START_ROUTINE)function_name,(LPVOID)arg,flag,(DWORD *)thread_id) == NULL)
	{
		return -1;
	}
	else
	{
		return 0;
	}
#else
	pthread_t th;
	return pthread_create((pthread_t *)&th,(pthread_attr_t *)attributes,function_name,arg);
#endif
}
