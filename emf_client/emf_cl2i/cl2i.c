#include "iwlib.h"

#include "../commonFiles/emf_ports.h"

#define SO_REUSEPORT 15

///////structures used/////////////////////////////////////////
struct wireinfo{   	//structure used to monitor the information of the event LGD
int link;		//general quality of the reception. 
int level;		//signal strength at the receiver. 
int noise;              //silence level (no packet) at the receiver. 
char name[100];		//name of the interface against which we have gatherred all the stats
};

typedef struct Triggerinfo{   	//the information that is send to the host agent is stored in this structure
int type;			//it defines the type of the trigger 
char interfaceName[100];	//name of the interface on which the trigger took place
char IP[40];			//IP address
}Trigger_info;


struct node {			// single node of the link list
char data[100];
struct node * next;
};

typedef struct Statusinfo{	// structure used to maintain and fetch the status of every interface
int up;				//it can only take two values either 1 or 0, 1 indicates interface is up and 0 indicates it is down
int running;			//it can only take two values either 1 or 0, 1 indicates interface is running and 0 indicates it is not running
}Status_info;

typedef struct node node;  //global variable of our linklist that is maintaing the staus of all the interfaces
node * head,*save,*current;
node * getnode();


////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int OpenUDC()			// function to open a connection with HA
{

    int on=1;
    	// Declarations
	int socketFD, bytesReceived;
	struct sockaddr_in serverAddress, clientAddress;
	
	// Create Socket
	if ((socketFD=socket(AF_INET, SOCK_STREAM, 0))==-1)
	{
		perror("Socket");
		exit(0);
	}

    if (setsockopt(socketFD,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(int)) == -1) 
    {
        perror("setsockopt");
        exit(1);
    }

	clientAddress.sin_family = AF_INET;
	clientAddress.sin_addr.s_addr = inet_addr("127.0.0.1");;
	clientAddress.sin_port = htons(CL2IAGENT_PORT);
	memset(clientAddress.sin_zero, '\0', sizeof clientAddress.sin_zero);
	
	int opt=1;
	setsockopt (socketFD, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof (opt));
	setsockopt (socketFD, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt));
	
	
	if (bind(socketFD, (struct sockaddr*) &clientAddress, sizeof(clientAddress))==-1)
	{
		perror("\nBind");
		return -1;
	}
	
	//Client Connects with the Server at the Server's Listening Port
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");//
	serverAddress.sin_port = htons(HOSTAGENT_PORT);
	memset(serverAddress.sin_zero, '\0', sizeof serverAddress.sin_zero);
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress))==-1)
	{
		perror("Connect");
		return -1;
	}
return socketFD;		//returns the discriptor on which the conneciton with HA has been establishes
}



node * getnode() // function to allocate memory to a node of link list
{
node * temp;
temp = (node *) malloc(sizeof(node));

	if (temp == NULL)
	{
		printf("\nMemory allocation Failure!\n");
		return temp;
	}
	else
		return(temp);
}	



void getIP(Trigger_info *Tinfo, int iofd) // when link up trigger is generated we all need to send the IP address in the trigger infor to the HA
{

int i; 
struct ifreq ifreq;
struct sockaddr_in *sin;
	    strcpy(ifreq.ifr_name,Tinfo->interfaceName);
	    if((ioctl(iofd, SIOCGIFADDR, &ifreq))<0){ 
            //fprintf(stderr, "SIOCGIFADDR: %s\n", strerror(errno)); 
	    return; } 
	    sin = (struct sockaddr_in *)&ifreq.ifr_broadaddr; 
	    sprintf(Tinfo->IP, "%s", inet_ntoa(sin->sin_addr));
}


//gathers information relates to the link,level and noise of the wireless interfaces.
void MakeWireInfo(struct wireinfo *winfo, const iwqual *qual,const iwrange *	range,int has_range)
{
  int		len;
//the following methods are sued inorder to display the stats in understandable format or in the format that is displayed by ifconfig command
	if(has_range && ((qual->level != 0) || (qual->updated & (IW_QUAL_DBM | IW_QUAL_RCPI))))
	{
		if(!(qual->updated & IW_QUAL_QUAL_INVALID))
		{
	        	winfo->link=qual->qual;
		}

		if(qual->updated & IW_QUAL_RCPI)
		{
			if(!(qual->updated & IW_QUAL_LEVEL_INVALID))
			{
				 double	rcpilevel = (qual->level / 2.0) - 110.0;
				winfo->level=rcpilevel;
			}

			if(!(qual->updated & IW_QUAL_NOISE_INVALID))
			{
				double	rcpinoise = (qual->noise / 2.0) - 110.0;
				winfo->noise=rcpinoise;
			}
		}
	else
	{
		if((qual->updated & IW_QUAL_DBM)|| (qual->level > range->max_qual.level))
		{
			if(!(qual->updated & IW_QUAL_LEVEL_INVALID))
			{
				int	dblevel = qual->level;
				if(qual->level >= 64)
				dblevel -= 0x100;
				winfo->level=dblevel;
			}
			if(!(qual->updated & IW_QUAL_NOISE_INVALID))
			{
				int	dbnoise = qual->noise;
				if(qual->noise >= 64)
				dbnoise -= 0x100;
				winfo->noise=dbnoise;
			}
		}
		else
		{
			if(!(qual->updated & IW_QUAL_LEVEL_INVALID))
			{
				winfo->level=qual->level;
			}

			if(!(qual->updated & IW_QUAL_NOISE_INVALID))
			{
			winfo->noise=qual->noise;
			}
		}
	}
    }

	else
	{
	        winfo->level =qual->level;
		winfo->link =qual->qual;
		winfo->noise=qual->noise;
	}
}


// fetch and maintain the status of interfaces, status include, up interface and running //interfaces
void StatusFuntion(Status_info *Sinfo, char *interfacename)  
{
int fd;
struct ifreq ifreq;
	    
fd = socket(AF_INET,SOCK_STREAM,0); 
strcpy(ifreq.ifr_name,interfacename);
	    
                if ((ioctl(fd, SIOCGIFFLAGS, &ifreq)<0)) 
		{
		Sinfo->up=0;
		Sinfo->running=0;
	        //fprintf(stderr,  " SIOCGIFFLAGS: %s\n", strerror(errno)); 
	        return;
		}

                if (ifreq.ifr_flags & IFF_UP) 
                Sinfo->up=1;
		else
		Sinfo->up=0;      

                if (ifreq.ifr_flags & IFF_RUNNING)
	    	Sinfo->running=1;
		else
		Sinfo->running=0;
		close(fd);

}


//search the linklist for the specified data, in our case we are searching for a specific interface in the linklist
int search(char *kv)
{
current = head;
	if (head == NULL)
	{
	return 0;
	}

	else
	{
		while ( (current->next != NULL) && (strcmp(current->data, kv)!=0) )
		current = current->next;

		if (strcmp(current->data, kv)==0)
		{
		return 1;
		}
		else
		return 0;
	}
}



//if an interface goes down we delte it from the mainlist and send a notification to the HA
int DeleteFromInterfaceList(int HASocket)
{
char undel[100];
current = head;

	if (head == NULL)
	{}
	else
	{
		while ( current != NULL )
		{
			Status_info Sinfo;
			StatusFuntion(&Sinfo,current->data); //returns the staus of the interface
	
			if(Sinfo.running == 0)//if interface is down
			{
				if (current == head)		//delte is from the list if the node is head
				{
					if((current->next) == NULL)
					{
						Trigger_info Tinfo;
						strcpy(Tinfo.interfaceName,current->data);
						Tinfo.type=3;
						strcpy(Tinfo.IP," "); //when a link is down no IP is associated with it so it is empty
							if (send(HASocket,&Tinfo, sizeof(Trigger_info), 0) == -1)	//send link down trigger to HA
						         perror("send");
					        else
					            //printf("CL2I: LINK DOWN send to H.A\n");
					            printf("%s DOWN\n", Tinfo.interfaceName);
						head=NULL; //deleted from the link list
					}
				else
				{		 
					strcpy(undel,current->data);
					Trigger_info Tinfo;
					strcpy(Tinfo.interfaceName,current->data);
					Tinfo.type=3;
					strcpy(Tinfo.IP," "); //when a link is down no IP is associated with it so it is empty
						if (send(HASocket,&Tinfo, sizeof(Trigger_info), 0) == -1)	//send link down trigger to HA
					         perror("send");
				        else
				            //printf("CL2I: LINK DOWN send to H.A.\n");					         
				            printf("%s DOWN\n", Tinfo.interfaceName);
		                        current = current->next; //deleted from the link list
                                        free(head);
				        head = current;

				}

			}
			else
			{ 		//delte from the lsit if node is not head;
					Trigger_info Tinfo;
					strcpy(Tinfo.interfaceName,current->data);
					Tinfo.type=3;
					strcpy(Tinfo.IP," ");
						if (send(HASocket,&Tinfo, sizeof(Trigger_info), 0) == -1)
					         perror("send");
				        else
				            //printf("CL2I: LINK DOWN send to H.A..\n");
				            printf("%s DOWN\n", Tinfo.interfaceName);
					strcpy(undel,current->data);//deleted from the link list
					save->next = current->next;
					free(current);				
			}
		}
		save = current;
		current = current->next;
	}
}
}


//insert interace name in the list if an interface goes up
int AddInterfaceList(char *x)
{
node *temp;
temp = getnode();
	if (temp == NULL)
	{
		return -1;
	}
	else
	{
		strcpy(temp->data,x);
		temp->next = NULL;

		if (head == NULL)
		head = temp;
		else
		{
			current = head;
				while (current != NULL)
				{
					save = current;
					current = current->next;
				}	
			save->next = temp;
		}
	return 0;
	}

}//on success return 0 and on failure retursn-1


void display() //the function is used to display the linklist on the screen, it is not really used anywhere, was jsut sued for debugging
{
current = head;
	if (current == NULL)
	printf("\aThe List Is Empty!\n");

	while (current != NULL)
	{	
		printf("\n: %s",current->data);
		current = current->next;
		
	}
printf("\n");
}



int main(int argc, char *argv[])
{
    char buf[2];
head = NULL;


int iofd = socket(AF_INET, SOCK_DGRAM, 0); 	//open a socket for iocl function
	if(iofd==-1)
	{perror("send");
	exit(0);}

	while(1) 				//a loop that runs after every 5 seconds and update the staus of all the links
	{
		///////Link Up and Link Down////////////		
		struct ifconf ifc; 
		struct ifreq *ifr;
		int fd;
		int interf;
		char Names[100];
		char buff[1024]; 
		int searchresult=0;
		
		//returns the total number of up and running interfaces
		ifc.ifc_len = sizeof(buff); 
		ifc.ifc_buf = buff; 
		if(ioctl(iofd, SIOCGIFCONF, &ifc) < 0) 
        	{ 
			return; 
        	} 
		ifr = ifc.ifc_req; 	 
		Names[0]= '\0'; 
        	interf=ifc.ifc_len / sizeof(struct ifreq);	


		//check individually each and every interface
		for(;interf > 0;ifr++)
		{   
		    Status_info einfo; 
		    sprintf(Names, ifr->ifr_name);
		    if(strcmp(Names,"lo")==0)			//if it is a local interface ignore it
        	    {  }
        	    else
        	    {
			StatusFuntion(&einfo,Names);		//retirves the staus of the interface
			if(einfo.up==1 && einfo.running==1)
			{				
								//check for the event LINK UP	
				searchresult=search(Names);	//checks if the interface is already a part of the linklist
				if(searchresult==1)
				{				// if it is already a part of linklist then do nothing
				}
				else
				{
					int AddCheck = AddInterfaceList(Names); //insert it in the link list
					if (AddCheck==-1)
					{
					}
					else
					{
						//AddInterfaceList(Names);
						Trigger_info sinfo;
					   	sinfo.type=2;
						strcpy(sinfo.interfaceName,Names);
						getIP(&sinfo,iofd);
							if (send(HASocket,&sinfo, sizeof(Trigger_info), 0) == -1) //send the trigger to the HA
						         perror("send");
					        //else
					        //    printf("%s\n", sinfo.interfaceName,Names);
					}
				}

				////////////////////link going dow/////////////////
				//check for the evernt link going down
				/*
				iwrange	range;
				int has_range = 0;
				int skfd;
				Trigger_info sinfoLGR;
		                iwstats	stats;
					if(iw_get_range_info(iofd, Names, &(range)) >= 0)
					has_range = 1;

				struct wireinfo winfo;
				strcpy(winfo.name,Names);
	      				if(iw_get_stats(iofd, winfo.name, &stats, &range, has_range) >= 0) //retrieve the stats from /wireless/net/proc
					{
						MakeWireInfo(&winfo, &stats.qual, &range, has_range);	//insert them in a proper structure
//							if(winfo.link < 30 && winfo.link >10)		//check if the stats are indicating LGD
							if(winfo.link < 10)		//check if the stats are indicating LGD
							{   	
								sinfoLGR.type=3;
								strcpy(sinfoLGR.interfaceName,winfo.name);
								getIP(&sinfoLGR,iofd);
						
									if (send(HASocket,&sinfoLGR, sizeof(Trigger_info), 0) == -1)//send the trigger to HA
								         perror("send");
				                    else
				                        printf("CL2I: LINK GOING DOWN send to H.A\n");
							}
					}
				*/
				//close(skfd);	
			}
		}
		    interf--;			//decrement the interface count and move to the next interface
	}					//close the for loop here
	DeleteFromInterfaceList(HASocket); 	//if some interface goes down, this function handles it.
	//display();
	
	if (recv(HASocket, &buf, sizeof(buf), MSG_DONTWAIT) == 0)
	{
        close(iofd);
        close(HASocket);
	}
	
	
	fflush(stdin);
	sleep(5);
}						//clsoe the while loop here

close(iofd);
close(HASocket);
return 0;
}						//close the function here

