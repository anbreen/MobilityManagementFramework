//-----------------------------------------------------------------------------
#include "emf.h"
//---------------------------------------------------------------------------

#define	BACKLOG			20
#define SO_REUSEPORT 	15

//---- Main Function -----------------------------
int main()
{
	int show_msg = 1; 	// setting logging as true for client
	
	counter =0;
	struct sockaddr_in my_addr;	// my address information
	struct sockaddr_in their_addr;
	struct timeval tv;
	socklen_t sin_size;
	int flag;
	int nready;
   
	initializeAIDs();
   
	FD_ZERO(&read_fds);
	FD_ZERO(&write_fds);
	FD_ZERO(&read_master);
	FD_ZERO(&write_master);

	//-----------Creating socket for HA	
	if( (sock_ha = socket(PF_INET, SOCK_STREAM, 0)) == -1)
		{
		showerr("Can't create socket..., No connection with HOST AGENT");
		}
	else
		//printf("sock_ha: Success...");
		if (show_msg)
			showmsg("\nsock_ha: Success...");
	
	//-----------Creating socket that will listen to the connections from the emf library
	if( (sock_lib_listen = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		{
		showerr("Can't create socket..., No connection with EMF_Socket_Library");
		}
	else
		//printf("sock_lib_listen: Success...\n");
		if (show_msg)
				showmsg("\nsock_lib_listen: Success...");
	
	//-----------Creating socket that will listen to the connections from the remote side 
	if( (sock_rem_listen = socket(PF_INET, SOCK_STREAM, 0)) == -1)
		{
		showerr("Can't create socket..., No connection will be accepted from remote end");
		}
	else
		//printf("sock_rem_listen: Success...\n");
		if (show_msg)
			showmsg("\nsock_rem_listen: Success...");
   
	my_addr.sin_family = AF_INET;          		// host byte order
	my_addr.sin_port = htons(HA_BIND_PORT);      // short, network byte order
	my_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	memset(my_addr.sin_zero, '\0', sizeof my_addr.sin_zero);
   
	int opt=1;
    setsockopt (sock_ha, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof (opt));
    setsockopt (sock_ha, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt));
                        
    if( bind(sock_ha, (struct sockaddr *)&my_addr, sizeof my_addr) == -1)
    	{
    	showerr("sock_ha bind");
    	}
    else
    	//printf("sock_ha bind: Success...\n");
    	if (show_msg)
    		showmsg("\nsock_ha bind: Success...");
	
    //----------- Connect to Host Agent here...
    their_addr.sin_family = AF_INET;                     // host byte order
    their_addr.sin_port = htons(HA_PORT);                // short, network byte order
    their_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // automatically fill with my IP
    memset(their_addr.sin_zero, '\0', sizeof my_addr.sin_zero);
    
    //-----------connecting to HA
    if (connect(sock_ha, (struct sockaddr *) &their_addr, sizeof(their_addr)) == -1)
    	{
    	showerr("sock_ha connect");
    	}
    else
    	{
    	//printf("sock_ha connect: Success...\n");
    	if (show_msg)
    		showmsg("\nsock_ha connect: Success...");
   	
    	FD_SET(sock_ha, &read_master);
    	fdmax = sock_ha;	
    	}
   
    my_addr.sin_family = AF_INET;            // host byte order
    my_addr.sin_port = htons(LIB_PORT);      // short, network byte order     //I have Changed it from LIB_PORT, EMF_CLIENT_PORT
    my_addr.sin_addr.s_addr = INADDR_ANY;    // automatically fill with my IP
    memset(my_addr.sin_zero, '\0', sizeof my_addr.sin_zero);
   
    setsockopt (sock_lib_listen, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof (opt));
    setsockopt (sock_lib_listen, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt));
    
    if (bind(sock_lib_listen, (struct sockaddr *)&my_addr, sizeof my_addr) == -1)
    	{
    	showerr("sock_lib_listen bind");
    	}
    else
    	//printf("sock_lib_listen bind: Success...");
    	if (show_msg)
    		showmsg("\nsock_lib_listen bind: Success...");

    //-----------listening for connections from library
    if (listen(sock_lib_listen, BACKLOG) == -1)
    	{	
    	showerr("sock_lib_listen listen");
    	}
    else
    	{
    	//printf("sock_lib_listen listen: Success...");
    	if (show_msg)
    		showmsg("\nsock_lib_listen listen: Success...");
    	
    	FD_SET(sock_lib_listen, &read_master);	
    	if(sock_lib_listen > fdmax)
    		fdmax = sock_lib_listen;	
    	}

    my_addr.sin_family = AF_INET;                    // host byte order
    my_addr.sin_port = htons(EMF_CLIENT_PORT);       // short, network byte order             // I have changed it from CLIENT, EMF_SERVER_PORT
    my_addr.sin_addr.s_addr = INADDR_ANY;            // automatically fill with my IP
    memset(my_addr.sin_zero, '\0', sizeof my_addr.sin_zero);

    setsockopt (sock_rem_listen, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof (opt));
    setsockopt (sock_rem_listen, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt));
       
    if (bind(sock_rem_listen, (struct sockaddr *)&my_addr, sizeof my_addr) == -1)
    	{
    	showerr("sock_rem_listen bind");
    	}
    else
    	//printf("sock_rem_listen bind: Success...");
    	if (show_msg)
    		showmsg("\nsock_rem_listen bind: Success...");

    //-----------listening for connections from remote end
    if (listen(sock_rem_listen, BACKLOG) == -1)
    	{
    	showerr("sock_rem_listen listen");
    	}
    else
    	{
    	//printf("sock_rem_listen listen: Success...");
    	if (show_msg)
    		showmsg("\nsock_rem_listen listen: Success...");

    	FD_SET(sock_rem_listen, &read_master);
    	if(sock_rem_listen > fdmax)
    		fdmax = sock_rem_listen;
    	}
    
	while(1)
		{
		read_fds = read_master;
		write_fds = write_master;

		tv.tv_sec = 0;
		tv.tv_usec = 100;
		//schedule_data();
		//////printf("before select...\n");		
		
    	//if (show_msg)
    		//showmsg("\nclient: main_loop in select, awaiting data at any descriptor...");
				
		//-----------select function that will return whever the set descriptors will return
		if( (nready = select(fdmax+1, &read_fds, NULL, NULL, &tv)) < 0 )
			{
			showerr("select");
			if(errno == EINTR)
				continue;
			}
		
		//-----------------------------------------------------------------------------
		//-----------the main schedule function that will schedule the data coming from the application using the EMF
		//if (show_msg)
    		//showmsg("\nclient: before schedule_data...");

		//CASE 5
		schedule_data();
				
		//CASE 1
    	//-------------------Connection request from EMF_Socket_Library----------------
		if (FD_ISSET(sock_lib_listen, &read_fds))
			{ 
			sin_size = sizeof their_addr;
			
		 	if ((sock_lib = accept(sock_lib_listen, (struct sockaddr *)&their_addr, &sin_size)) == -1) 
		 		{
		 		perror("sock_lib accept");
		 		continue;
		 		}
		 	else
         		{
		    	if (show_msg)
		    		showmsg("\nclient: handler for socket library called...");
		    				
		 		handle_emf_sock_lib_connect(sock_lib);	
         		}
		 	}
		
		//CASE 3
		//-----------------------------------------------------------------------------
		//-----------------------Connection request from Remote End--------------------
		if (FD_ISSET(sock_rem_listen, &read_fds))
			{ 
			sin_size = sizeof their_addr;
		 	if ((sock_rem = accept(sock_rem_listen, (struct sockaddr *)&their_addr, &sin_size)) == -1) 
		 		{
		 		showerr("sock_rem accept");
		 		continue;
		 		}
		 	else
		    	if (show_msg)
		    		showmsg("\nsock_rem accept: Success...");
		 	
		 	//printf("CORE : Got connection from Remote end %s\n", inet_ntoa(their_addr.sin_addr));
		 	
		 	if (show_msg)
		 		{
	    		showmsg("\nClient: CORE Got connection from Remote end...");
		 		showmsg(inet_ntoa(their_addr.sin_addr));
		 		}
		 	
		 	// -----------add new descriptor to set
		 	FD_SET(sock_rem, &read_master);  
			
			if (sock_rem > fdmax)
				fdmax = sock_rem; //----------- for select
			
			if(HandleNewConnection(sock_rem)==0)
				{
				//some internal error
				}
			else
				{
				sock_rem=0;
		    	if (show_msg)
		    		showmsg("\nClient: new connection handled...");
				}
			}
		
		//CASE 2
		//-----------------------------------------------------------------------------
		//-----------------------Some Command from Host_Agent--------------------------
		if (FD_ISSET(sock_ha, &read_fds))
			{
	    	if (show_msg)
	    		showmsg("\nClient: command from HA is received...");

			handle_ha_cmds(sock_ha);
			}
		
		//CASE 4
		//-----------------------------------------------------------------------------
		//-------------------Received data/command from remote end---------------------
 		flag = 0;
		struct assoc_list *assoc_node;
//		assoc_node = (struct assoc_list *) malloc(sizeof(struct assoc_list));
		assoc_node=assoc_head;

		while(assoc_node!=NULL)
			{
	    	//if (show_msg)
	    		//showmsg("\nClient: command/data received from remote end...");
	    	
			int i;
			for (i = 0; i < assoc_node->c_ctr; i++)
				{
				if (FD_ISSET(assoc_node->lib_sockfd, &read_fds))
					{
					//---------any data received from the library				
					//handle_emf_sock_lib_cmd(assoc_node->lib_sockfd, assoc_node);
					//printf("I am sending data\n");
					int retemf = handle_emf_sock_lib_send(assoc_node->lib_sockfd, assoc_node);
					if (retemf == 0)
					{assoc_node->terminating = 1;
					FD_CLR(assoc_node->lib_sockfd, &read_master);}
					
					if (--nready <= 0)
						{
						flag = 1;
						break;
						}	
					}	
				
				//---------any data received from the remote side
				if (FD_ISSET(assoc_node->cid[i], &read_fds))
					{
					//printf("before handle_reassembly\n");
					
					//if (show_msg)
					//showmsg("\nClient: before handle reassembly called...");
			    	
					Handle_Reassembly(i, assoc_node);
					
					if (--nready <= 0)
						{
						flag = 1;
						break;
						}
					}
				}
			
			if(assoc_head == NULL)
				assoc_node = NULL;
			else
				{
	      		if (flagNodeDeleted==1)					// If Node is deleted, flagNodeDeleted has been set
	      			{
	      			assoc_node = nextOfDeletedNode; //---------flags used for removing node from the assoc_list
      				flagNodeDeleted = 0;
	      			}
      			else
      				assoc_node = assoc_node->next;
				}
			
			}
//-----------------------------------------------------------------------------
		}
    return 0;
}

//-----------------------------------------------------------------------------
void showerr(char *str)
{
	perror(str);
	////printf("\n");
	//exit(1);
}

//-----------------------------------------------------------------------------
void showmsg(char *str)
{
	//printf("\n");
	printf(str);
	//printf("\n");
}

//-----------------------------------------------------------------------------
int getAssociationID()   //---------generates an association ID
{
	for(assoc_Index=0; assoc_Index < TOTL_AIDs; assoc_Index++)
		{
		//printf("flag index val: %d\n", associationID[assoc_Index].aid_flag);
		if(associationID[assoc_Index].aid_flag == 0)
			{
			associationID[assoc_Index].aid_flag = 1;
			return (assoc_Index+1);
			}
		}
	return -1;
}
//-----------------------------------------------------------------------------

//---------initialize the association ID
void initializeAIDs()
{  
	unsigned int index;
	for (index = 0; index < TOTL_AIDs; index++)
		associationID[index].aid_flag = 0;
}
//-----------------------------------------------------------------------------
