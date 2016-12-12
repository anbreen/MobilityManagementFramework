//-----------------------------------------------------------------------------
#include <sys/ioctl.h>
#include <linux/sockios.h>

#include "emf.h"
//-----------------------------------------------------------------------------
#define TLV_ValueSize    	35
#define SO_REUSEPORT 		15

#define show_msg 0

//-----------------------------------------------------------------------------
/*
enum EMFC_HA_TLV_Types
{LOCAL_COMPLIANCE_REQ=101, LOCAL_COMPLIANCE_RES=102, INTERFACE_IP_REQ=103, INTERFACE_IP_RES=104, ADD_ASSOCIATION=105,
ADD_CONNECTION=106, REMOVE_ASSOCIATION=107, REMOVE_CONNECTION=108, HO_LINK_SHIFT_COMMIT=109, HO_LINK_SHIFT_COMPLETE=110, 
HO_TRAFFIC_SHIFT_COMMIT=111, HO_TRAFFIC_SHIFT_COMPLETE=112, BA_COMMIT=113, BA_COMPLETE=114, HANDOVER_COMMIT=115, 
AIDwise_REMOVE_FROM_BA_COMMIT=116, AIDwise_REMOVE_FROM_BA_COMPLETE=117, 
PORTwise_REMOVE_FROM_BA_COMMIT=118, PORTwise_REMOVE_FROM_BA_COMPLETE=119, PORTwise_BA_COMMIT, PORTwise_BA_COMPLETE};
*/

//-----------------------------------------------------------------------------
struct SingleJoin                 //Single Join
	{
	//unsigned long int EncryptedAIDNonce:64;
	unsigned long int EncryptedAIDNonce;
	} Single_Join;
//-----------------------------------------------------------------------------
struct BandwidthAggregation
	{
	//unsigned long int Encrypted:64;
	unsigned long int Encrypted;
	} Bandwidth_Aggregation;
//-----------------------------------------------------------------------------

char  TLV_Type_Length[2];
char *TLV_Value;       //allocate memory as that of length
char  TLV_Value_Length = 0;

//-----------------------------------------------------------------------------
void handle_ha_cmds(int sock_ha);
//-----------------------------------------------------------------------------



//------this function is called whenevr Host Agent send a command to the emf core
void handle_ha_cmds(int sock_ha)
{
	int receivedBytes;
	//---------------This structure has already been used in emf_sock_lib_handler---
	
	//----- receive the command      
	if( (receivedBytes = recv(sock_ha, &TLV_Type_Length, 2, 0)) == -1 )
		{
		//get extended error and handle error
		showerr("handle_ha_cmds, recv");
		}	
	else 
		if(receivedBytes == 0)
			{
			close(sock_ha);
			FD_CLR(sock_ha, &read_master);
			showerr("Connection with HOST_AGENT closed...");
			exit(0);
			}
		else
			{
			
			if (show_msg)
				showmsg("\nha_command_handler: handle_ha_messages() function called...");

			//-----the function is called to parse and execute the command
			handle_ha_messages(sock_ha, TLV_Type_Length);
			}
}



handle_ha_messages(int sock_ha, char TLV_Type_Length[])
{
	
	int ii;
	char *ip_list;
	int CID, PortNumber, AID , sock_bw, j, index, type, header_size;
	char FromIP[16], ToIP[16], BAToIP[16];
	struct sockaddr_in my_addr, *my_addr2,my_addr_con; 				// my address information
	struct assoc_list *assoc_node;
	socklen_t len;

	TLV_Value = malloc((int)(TLV_Type_Length[1]+4));
	if (recv(sock_ha, TLV_Value, (int) TLV_Type_Length[1], 0) ==-1)
		{	
		//get extended error and handle error
		}
	else
	   	{
		printf("\nCMD_HANDLER: Data Length Recev: %d \n", TLV_Type_Length[1]);
		type = TLV_Type_Length[0];
		char i=0;
		i=0;
	   
		if (show_msg)
			showmsg("\nha_command_messages: evaluating the command sent from HA...");
		
		
		printf("\nha_command_messages: evaluating the command sent from HA...");
		
		
		
		switch(type)   //---------according to the type of coomand received execute the specific case
	   		{
	   		strncpy(FromIP, "0", 1);
	   		strncpy(ToIP, "0", 1);

         	case HO_LINK_SHIFT_COMMIT:		//--------- Perform Link Shift Handover
         		
        		if (show_msg)
        			showmsg("\nha_command_messages: performing Link Shift Handover...");
        		
         		if((unsigned int) TLV_Type_Length[1] > 31)	//this command length can't exceed 31
         			TLV_Type_Length[1] = (char)31;
         		
         		//---------------------------Retrieving From-IP and To-IP---------------------------
         		j = 0;
         		index = 0;
         		while( (TLV_Value[index] != ':') && (TLV_Value[index] != '\0') && (index < TLV_Type_Length[1]) )
         			{
         			FromIP[j] = TLV_Value[index];
         			j++;
         			index++;
         			if(j == 14) //max length of IP is 15 (0-14)
         				break;
         			}
         		FromIP[j] =  '\0';
         		j = 0;
         		index++;	//move index ahead from ':'
         		while( (TLV_Value[index] != '\0') && (index < TLV_Type_Length[1]) )
         			{
         			ToIP[j] = TLV_Value[index];
         			j++;
         			index++;
         			if(j == 14) //max length of IP is 15 (0-14)
         				break;
         			}             
         		ToIP[j] =  '\0';
                           
         		len = sizeof(my_addr);
         		
         		//-----check all the association and call the initiate_hadover function for every conection in an association that is made on the From IP interface
         		for(assoc_node = assoc_head; assoc_node != NULL; assoc_node = assoc_node->next)
         			{
         			getsockname(assoc_node->cid[0], (struct sockaddr*)&my_addr, &len);
         			
         			if (strcmp(FromIP, inet_ntoa(my_addr.sin_addr)) == 0)
         				{
         				initiate_handover(assoc_node, ToIP, HO_LINK_SHIFT_COMMIT);
         				}
         			}
         		break;
         	
         	case HO_TRAFFIC_SHIFT_COMMIT:		// --------- Perform Traffic Shift\
         		
        		if (show_msg)
        			showmsg("\nha_command_messages: performing Traffic Shift Handover...");
         		
         		if((unsigned int) TLV_Type_Length[1] > 35)	//this command length can't exceed 35
         			TLV_Type_Length[1] = (char)35;
         		
         		//----------------------------Retrieving Port: From-IP : To-IP---------------------------
         		for(j=0, index=0; index<4; j++, index++)		   // Extract PortNumber
         			TLVUnit.charValue[j] = TLV_Value[index];
         		
         		PortNumber = TLVUnit.intValue;
         		j = 0;
         		while( (TLV_Value[index] != ':') && (TLV_Value[index] != '\0') && (index < TLV_Type_Length[1]) )
         			{	
         			FromIP[j] = TLV_Value[index];
         			j++;
         			index++;
         			//if(j == 14) //max length of IP is 15 (0-14)
         			//break;
         			}
         		FromIP[j] =  '\0';
         		j = 0;
         		index++;	//move index ahead from ':'
         		while( (TLV_Value[index] != '\0') && (index < TLV_Type_Length[1]) )
         			{
         			ToIP[j] = TLV_Value[index];
         			j++;
         			index++;
         			//if(j == 14) //max length of IP is 15 (0-14)
         			//	break;
         			}
         		
         		ToIP[j] =  '\0';
         		
         		/*if(assoc_node->scenario != NORMAL)
            	{
            	//send error to HA and quit
            	break;
            	}*/
         		
                           	
         		//-----check all the association and call the initiate_hadover function for every conection in an association that is made on the FromIP interface and has the port number specified by the HA
         		for(assoc_node = assoc_head; assoc_node != NULL; assoc_node = assoc_node->next)
         			{
         			struct sockaddr_in my_addrtraffic,*my_addr2, rough_addr;
         			len = sizeof(my_addrtraffic);
         			int len1;          
         			my_addrtraffic.sin_port=0;
                
         			getsockname(assoc_node->cid[0], (struct sockaddr*)&my_addrtraffic, &len);

         			//-------- my_addr.sin_port equals to assoc_node->dest_addr
         			if ((strcmp(FromIP, inet_ntoa(my_addrtraffic.sin_addr)) == 0) && (PortNumber ==assoc_node->ProtocolNo ))
         				{
         				initiate_handover(assoc_node, ToIP, HO_TRAFFIC_SHIFT_COMMIT);
         				}
         			}
         		break;
         		
         		
             	// this case added for INTERFACE_IP_RES	
             	case INTERFACE_IP_RES:
             		
					//IMPORTANT: this is complete implemented and tested with HostAgent:: //Zeeshan Dec-4 
					//But not yet used
             		
             		printf("\nINTERFACE_IP_RES: Command Reveived = INTERFACE_IP_RES\n");
             		
             		// extract first 4 bytes of AID
             		for(j=0, index=0; index<4; j++, index++)		
             			TLVUnit.charValue[j] = TLV_Value[index];
             		
             		AID = TLVUnit.intValue;
             		printf("\nAID: %d\n", AID);
             		             		
             		// situation in which HA did not sent any IP
             		// means we do not have any other interface on the machine
             		// yet to define and implement this situation
             		if (TLV_Type_Length[1] == 4)
             			{
             			printf("\nINTERFACE_IP_RES: HA Did not sent any IP\n");
             			break;
             			}
             		else if ((TLV_Type_Length[1] > 4))
             		{
             		
             		// extract IP of length (0-14), maximum 15 bytes
             		j=0;
             		while( (TLV_Value[index] != ':') && (TLV_Value[index] != '\0') && (index < TLV_Type_Length[1]) )
             		{
             			ToIP[j] = TLV_Value[index];
             			j++;
             			index++;
             			if(j == 14) //max length of IP is 15 (0-14)
             				break;
             		}
             		ToIP[j] =  '\0';
             		printf("\nToIP: %s\n", ToIP);
             		
             		
             		//--- check all the association and call the initiate_hadover function for every AID match
             		for(assoc_node = assoc_head; assoc_node != NULL; assoc_node = assoc_node->next)
             			{
             			
             			struct sockaddr_in my_addrtraffic,*my_addr2, rough_addr;
             			len = sizeof(my_addrtraffic);
             			int len1;          
             			
             			//--- call innitiate_handover if AID returned by HA is equal to assoc_node->aid
             			if (assoc_node->aid == AID)
             				{
             				initiate_handover(assoc_node, ToIP, HO_CONNECTION_SHIFT_COMMIT);
             				}
             			
             			}
             		
             		}    		
             		break;
        
             		
             		
          		
         	/***************starting cases for BA*********************************/
         		
         	case BA_COMMIT:		//----message is received at the the start of BA
         		
        		if (show_msg)
        			showmsg("\nha_command_messages: command received is BA_COMMIT...");

         		
         		my_addr.sin_port=0;
         		my_addr_con.sin_port=0;

         		for(j=0, index=0; index<4; j++, index++)		// Extract the AID
         			TLVUnit.charValue[j] = TLV_Value[index];
         		AID = TLVUnit.intValue;
         		len = sizeof(my_addr);               

         		for(assoc_node = assoc_head; assoc_node != NULL; assoc_node = assoc_node->next)
         			{
         			int i; //----------match the AID to the list of AID'S
         			if(AID == assoc_node->aid)
         				{
         				while(index < TLV_Type_Length[1])	//---------loop through all IPs and make connections on them
         					{
         					strncpy(ToIP, "0", 1);
         					j=0;
         					int sflag=0;
         					while (TLV_Value[index] != '\0')
         						{
         						if (TLV_Value[index] != ':')		//-------- Extract 1st IP Address
         							{	
         							ToIP[j++] = TLV_Value[index++]; 
         							}
         						else
         							{	
         							ToIP[j] = '\0'; 								
         							index++;
         							sflag=1;
         							break;
         							}
         						
         						if (index == TLV_Type_Length[1])
         							{
         							sflag=1;
         							ToIP[j] = '\0'; 
         							index++;
         							break;
         							}
         						}
         					
							if(sflag==0)
								{
								index++;
								ToIP[j] = '\0'; 
								}
														
							if(TLV_Value[index] == ':')
								index++;
														
							if ((sock_bw = socket(PF_INET, SOCK_STREAM, 0)) == -1)//create socket descriptor and bind it with the ip matched
								showerr("sock_bw socket");		                  
							else
								{		                  
								int yes;
								yes=1;
								if(setsockopt(sock_bw,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int))==-1)
									perror("socket");
								my_addr.sin_family = AF_INET;          		// host byte order
								my_addr.sin_addr.s_addr = inet_addr(ToIP);
								memset(my_addr.sin_zero, '\0', sizeof my_addr.sin_zero);

								int opt=1;
								setsockopt (sock_bw, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt));
								
								if( bind(sock_bw, (struct sockaddr *)&my_addr, sizeof my_addr) == -1)
									showerr("sock_bw bind");
								else
									{  
									my_addr_con.sin_family = AF_INET;          		// host byte order
									my_addr_con.sin_addr.s_addr = inet_addr(assoc_node->ServerIP);//my_addr2->sin_addr;
									my_addr_con.sin_port = htons(EMF_SERVER_PORT);
									memset(my_addr_con.sin_zero, '\0', sizeof my_addr_con.sin_zero);

									//---- Make the new connection required in BA		                        
									if (connect(sock_bw, (struct sockaddr *)&my_addr_con, sizeof my_addr_con) == -1)
										showerr("sock_bw connect");
									else
		                        		{
										//------ SEND BANDWIDTH AGGREGATION packet on the newly established connection                                
										header_size = make_emf_packet(assoc_node, assoc_node->emf_send_head->seq_no, 0, emf_packet, BANDWIDTHAGGREGATION);
										if(send(sock_bw, emf_packet, header_size, 0) == -1)
											{
											showerr("sock_bw send");
											close(sock_bw);
											}
										else
		                            		{                              	
											FD_SET(sock_bw, &read_master);
											if(sock_bw > fdmax)
												fdmax = sock_bw;
											
											//---- add the connection descriptor in the array of conneion id, increment the counter, set the scenario as BA 
											assoc_node->cid[assoc_node->c_ctr] = sock_bw;
											save_and_update_emf_buffer(assoc_node, assoc_node->emf_send_head->data, assoc_node->emf_send_head->seq_no,header_size, 0, assoc_node->c_ctr);					
											assoc_node->last_data_in_buffer[1] = header_size;
											assoc_node->prev_throughput[1] = 0;											
											assoc_node->c_ctr++;
											assoc_node->scenario = BANDWIDTH_AGGREGATION;
		                            		}
										}
									}
								}
							
							//----- send BA_COMPLETE message to the HA in response to the BA_COMMIT
							HA_TLV.TLVType = BA_COMPLETE;
							TLVUnit.intValue = assoc_node->aid; 
							for(j=0; j<4; j++)
								HA_TLV.TLVValue[j] = TLVUnit.charValue[j];
							
							TLVUnit.intValue = sock_bw; 
							for(; j<8; j++)
								HA_TLV.TLVValue[j] = TLVUnit.charValue[j-4];
					         		   	
							for (i = 0; i < strlen(ToIP); i++, j++)
								{
								HA_TLV.TLVValue[j] = ToIP[i];	                        										
								}		
							
							HA_TLV.TLVValue[j]='\0';				            		

							HA_TLV.TLVValue[j] = 0;
							
							if (send(sock_ha, &HA_TLV, sizeof(HA_TLV), 0)==-1)	//----------1. send association information to HA
								showerr("Can't send new connection information to host agent...:");
					
							printf("\nCORE : BA_COMPLETE Message Sent to Host Agent with AID(%d), CID(%d), IP(%s) ", assoc_node->aid, sock_bw, ToIP); //Omer
							//printf("index1 : %d\n",index);
							//index++;
							}
         				}
         			}
         		break;
         		
         		
         	// -------- WHEN BA is performed at the start of initiating a connection this msg is received from HA
         	//--------- this case is exactly as same as the BA_COMMIT case
         	
         	case BA_COMMIT_IN_LOCAL_COMPLIANCE:
         		
        		if (show_msg)
        			showmsg("\nha_command_messages: command received is BA_COMMIT_IN_LOCAL_COMPLIANCE...");
         		
         		my_addr.sin_port=0;
         		my_addr_con.sin_port=0;
         		
         		for(j=0, index=0; index<4; j++, index++)		//--------- Extract AID
         			TLVUnit.charValue[j] = TLV_Value[index];
         		
         		AID = TLVUnit.intValue;
         		len = sizeof(my_addr);               
         		
         		for(assoc_node = assoc_head; assoc_node != NULL; assoc_node = assoc_node->next)
         			{
         			int i;
         			if(AID == assoc_node->aid)
         				{
         				while(index < TLV_Type_Length[1])	//---------------loop through all IPs and make connections on them
         					{
         					strncpy(ToIP, "0", 1);
         					j=0;
         					int sflag=0;
         					while (TLV_Value[index] != '\0')
         						{
         						if (TLV_Value[index] != ':')					//--------------- Extract 1st IP Address
         							ToIP[j++] = TLV_Value[index++];
         						else
         							{	
         							//printf("1I am placing /0\n");
         							ToIP[j] = '\0'; 
         							index++;
         							sflag=1;
         							break;
         							}
         						
         						if (index == TLV_Type_Length[1])
         							{
         							sflag=1;
         							ToIP[j] = '\0'; 
         							index++;
         							break;
         							}
         						}
         					
							if(sflag==0)
								{
								index++;
								ToIP[j] = '\0'; 
								}
							
							if(TLV_Value[index] == ':')
								index++;
							
							
							if ((sock_bw = socket(PF_INET, SOCK_STREAM, 0)) == -1)//create socket descriptor and bind it with the ip matched
								showerr("sock_bw socket");		                  
							else
								{	                  
								int yes;
								yes=1;
								if(setsockopt(sock_bw,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int))==-1)
									perror("socket");
							  
								my_addr.sin_family = AF_INET;          		// host byte order
								my_addr.sin_addr.s_addr = inet_addr(ToIP);
								memset(my_addr.sin_zero, '\0', sizeof my_addr.sin_zero);
								
								int opt=1;			                
								setsockopt (sock_bw, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt));
			                		                       	
								if( bind(sock_bw, (struct sockaddr *)&my_addr, sizeof my_addr) == -1)
									showerr("sock_bw bind");
								else
									{  
									my_addr_con.sin_family = AF_INET;          		// host byte order
									my_addr_con.sin_addr.s_addr = inet_addr(assoc_node->ServerIP);//ProtocolNo;
									my_addr_con.sin_port = htons(EMF_SERVER_PORT);
									memset(my_addr_con.sin_zero, '\0', sizeof my_addr_con.sin_zero);
		                        
									if (connect(sock_bw, (struct sockaddr *)&my_addr_con, sizeof my_addr_con) == -1)
										showerr("sock_bw connect");		                        
									else
									{
										header_size = make_emf_packet(assoc_node, assoc_node->emf_send_head->seq_no, 0, emf_packet, BANDWIDTHAGGREGATION);
										if(send(sock_bw, emf_packet, header_size, 0) == -1)
											{
											showerr("sock_bw send");
											close(sock_bw);
											}
										else
											{                              	
											FD_SET(sock_bw, &read_master);
											if(sock_bw > fdmax)
												fdmax = sock_bw;
											
											assoc_node->cid[assoc_node->c_ctr] = sock_bw;
											save_and_update_emf_buffer(assoc_node, assoc_node->emf_send_head->data, assoc_node->emf_send_head->seq_no,header_size, 0, assoc_node->c_ctr);					
											assoc_node->last_data_in_buffer[1] = header_size;
											assoc_node->prev_throughput[1] = 0;
											
											assoc_node->c_ctr++;
											assoc_node->scenario = BANDWIDTH_AGGREGATION;
											
											//HA_TLV.TLVType = BA_COMPLETE_IN_LOCAL_COMPLIANCE;
											//---whenevr a new connection is created an add connection msg is send to the HA
											HA_TLV.TLVType = ADD_CONNECTION;
											TLVUnit.intValue = assoc_node->aid; 
											for(j=0; j<4; j++)
												HA_TLV.TLVValue[j] = TLVUnit.charValue[j];
											
											TLVUnit.intValue = sock_bw; 
											for(; j<8; j++)
												HA_TLV.TLVValue[j] = TLVUnit.charValue[j-4];
													
											for (i = 0; i < strlen(ToIP); i++, j++)
												{
												HA_TLV.TLVValue[j] = ToIP[i];	                        					
												}
											
											HA_TLV.TLVValue[j]='\0';
											
											if (send(sock_ha, &HA_TLV, sizeof(HA_TLV), 0)==-1)	//1. send association information to HA
												showerr("Can't send new connection information to host agent...:");
									
											printf("\nCORE : ADD_CONNECTION Message Sent to Host Agent with AID(%d), CID(%d), IP(%s) ", assoc_node->aid, sock_bw, ToIP); //Omer
											}
										
										}
									
									}
								
								}
								
							}
         				
         				}
         			
         			}
         		break;
         
         		
            // Perform removal of BandWidth Aggregation, the msg is specificaly received when the interface goes down
         	case LD_AIDwise_REMOVE_FROM_BA_COMMIT:
         		
        		if (show_msg)
        			showmsg("\nha_command_messages: command received is LD_AIDwise_REMOVE_FROM_BA_COMMIT...");
         		
         		printf("\nCORE : Wireless INterface removed ");
         		
         		for(j=0, index=0; index<4; j++, index++)		// Extract AID
         			TLVUnit.charValue[j] = TLV_Value[index];
         		
         		AID = TLVUnit.intValue;
         		for(assoc_node = assoc_head; assoc_node != NULL; assoc_node = assoc_node->next)
         		{
         			int i;
         			if(assoc_node->aid == AID)
         				{               
         				while(index < TLV_Type_Length[1])	//loop through all IPs and remove connections on them
         					{
         					strncpy(FromIP, "0", 1);
         					j = 0;                  	
         					while( (TLV_Value[index] != '\0') && (TLV_Value[index] != ':') && (index < TLV_Type_Length[1]) && (TLV_Value[index] != ' ' ))
         						{	
         						FromIP[j++] = TLV_Value[index++];
         						if(j == 14) //max length of IP is 15 (0-14)
         							break;
         						}						   
         					FromIP[j] = '\0';						   
         					
         					if(TLV_Value[index] == ':')
         						index++;
						
         					int counter;
         					counter = assoc_node->c_ctr;
         					for(i = 0; i < counter;)
         						{
         						len = sizeof(my_addr);
         						getsockname(assoc_node->cid[i], (struct sockaddr*)&my_addr, &len);
         						
         						if (strcmp(FromIP, inet_ntoa(my_addr.sin_addr)) == 0)
         							{						    
         							//send remove connection notification to HA					
         							HA_TLV.TLVType = REMOVE_CONNECTION;
         							HA_TLV.TLVLength = 8;								
         							TLVUnit.intValue = assoc_node->aid; 
         							
         							for(j=0; j<4; j++)
         								HA_TLV.TLVValue[j] = TLVUnit.charValue[j];
         							
         							TLVUnit.intValue = assoc_node->cid[i];
         							
         							for(; j<8; j++)
         								HA_TLV.TLVValue[j] = TLVUnit.charValue[j-4];
         							
         							HA_TLV.TLVValue[j] = 0;	
         							
         							if (send(sock_ha, &HA_TLV, sizeof(HA_TLV), 0) <= 0)	//1. send association information to HA
         								showerr("Can't send remove connection message to host agent...:");
         								         							
         							printf("\nCORE : REMOVE_CONNECTION Message sent to Host Agent with AID(%d), CID(%d) ", assoc_node->aid, assoc_node->cid[i]);				
         							
         							//----- RESECHEDULE any data that was in the emf buffer and was not send due to the removal of connection
         							if(i==0)								
         								reschedule_data(assoc_node,emf_packet, i, 1,DATAPACK);
         							else
         								reschedule_data(assoc_node,emf_packet, i, 0,DATAPACK);
         							
         							close(assoc_node->cid[i]);
         							
         							//		printf("closed : %d",i);
         							FD_CLR(assoc_node->cid[i], &read_master);
         							assoc_node->cid[i]=0;
         							assoc_node->c_ctr = assoc_node->c_ctr - 1;;
         							printf("\nconnection counter : %d ",assoc_node->c_ctr);
         							
         							if(assoc_node->c_ctr > 0)
         								{
         								for(j = i; j < assoc_node->c_ctr ;j++)
         									{
         									assoc_node->cid[j] = assoc_node->cid[j+1];											
         									assoc_node->con_recv_node[j].length = assoc_node->con_recv_node[j+1].length;
         									assoc_node->con_recv_node[j].byte_received = assoc_node->con_recv_node[j+1].byte_received;   					
         									assoc_node->con_send_head[j] = assoc_node->con_send_head[j+1];
         									assoc_node->con_send_tail[j] = assoc_node->con_send_tail[j+1];
         									assoc_node->last_data_in_buffer[j] = assoc_node->last_data_in_buffer[j+1];
         									assoc_node->sched_ctr[j] = assoc_node->sched_ctr[j+1];
         									assoc_node->current_data_in_buffer[j] = assoc_node->current_data_in_buffer[j+1];
         									assoc_node->RTT[j] = assoc_node->RTT[j+1];
         									assoc_node->avg_throughput[j] = assoc_node->avg_throughput[j+1];
         									assoc_node->prev_throughput[j] = assoc_node->prev_throughput[j+1];
         									assoc_node->min_tp_cnt[j] = assoc_node->min_tp_cnt[j+1];
         									
         									}			
         								}
         							
         							assoc_node->cid[assoc_node->c_ctr]=0;	    																
         							printf("\n");       	
         							break;
         							}
         						else
         							i++;
         						}
         					index++;
         					}
         				}
         			}
         		break;
         		
         		
         		
         	// Perform removal of BandWidth Aggregation, the msg is specificaly received when the remove BA button is pressed at interface
         	case AIDwise_REMOVE_FROM_BA_COMMIT:
         		
        		if (show_msg)
        			showmsg("\nha_command_messages: command received is AIDwise_REMOVE_FROM_BA_COMMIT...");
         		
         		for(j=0, index=0; index<4; j++, index++)		// Extract AID
         			TLVUnit.charValue[j] = TLV_Value[index];
         			AID = TLVUnit.intValue;	
         			for(assoc_node = assoc_head; assoc_node != NULL; assoc_node = assoc_node->next)
         				{
         				int i;
         				if(assoc_node->aid == AID)
         					{
         					
         					while(index < TLV_Type_Length[1])	//loop through all IPs and make connections on them
         						{
         						strncpy(ToIP, "0", 1);//?
         						j=0;
         						int sflag=0;
         						while (TLV_Value[index] != '\0')
         							{
         							if (TLV_Value[index] != ':')					// Extract 1st IP Address
         								ToIP[j++] = TLV_Value[index++];
         							else	
         								{	
         								ToIP[j] = '\0'; 
         								//tempIndex = index;
         								index++;
         								sflag=1;
         								break;
         								}
         							
         							if (index == TLV_Type_Length[1])
         								{
         								sflag=1;
         								ToIP[j] = '\0'; 
         								index++;
         								break;
         								}
         							}
						
         						if(sflag==0)
         							{
         							index++;						
         							ToIP[j] = '\0'; 
         							}
							
         						if(TLV_Value[index] == ':')
         							index++;
         						
         						for(i = 0; i < assoc_node->c_ctr; i++)
         							{
         							
         							len = sizeof(my_addr);
         							getsockname(assoc_node->cid[i], (struct sockaddr*)&my_addr, &len);
         							
         							if (strcmp(ToIP, inet_ntoa(my_addr.sin_addr)) == 0)
         								{
         								
         								//----sends finish packet to indicate BA removal on the other end
         								header_size = make_emf_packet(assoc_node, 0, 0, emf_packet, FINISH);
         								if( send(assoc_node->cid[i], emf_packet, sizeof(header_size), 0) )
         									{
         									assoc_node->dont_send[i] = 1;						      	
         									}
         								//shutdown this connection for receiving
         								//receive data from this connection
         								//reschedule its unsent data to some other connection
         								//close it and remove from association list
         								//break;		            					            	   			
         								}
         							}
         						//index++;
         						}
         					}
         				}
         			break;
         		
         		default:
         			{
         			printf("\ndidn't fint any command TLV %d ",type);
         			exit(0);
         			}
         	}
		}
		//free(TLV_Value);
}
//-----------------------------------------------------------------------------------------




void initiate_handover(struct assoc_list *assoc_node, char *ToIP, int scenario)
{
	
	if (show_msg)
		showmsg("\nha_command_messages: initiate_handover() called now inside it...");

	int sock_ho_link, j;
	socklen_t len;
	struct sockaddr_in my_addr,*my_addr2,my_addr_con; 				// my address information
	my_addr.sin_port=0;
	my_addr_con.sin_port=0;
	
	
	unsigned int header_size, packet_type;
	
	//--- make a new socket for the new conenction
	//--- create socket descriptor and bind it with the ip you have matched
	if ((sock_ho_link = socket(PF_INET, SOCK_STREAM, 0)) == -1)
		{
		showerr("sock_ho_link socket");
		}
	else	//--------Writing Global Socket Descriptor to file
		{
		int yes;
		yes=1;
		if(setsockopt(sock_ho_link,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int))==-1)
			{
			perror("socket");
			}		
		
		if(ToIP == NULL)
			{
			get_and_bind_to_ip(sock_ho_link);						
			}
		else
			{
			
			my_addr.sin_family = AF_INET;          		// host byte order
		   	my_addr.sin_addr.s_addr = inet_addr(ToIP);
		   	memset(my_addr.sin_zero, '\0', sizeof my_addr.sin_zero);

		   	int opt=1;
		   	//setsockopt (sock_ho_link, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof (opt));
		   	setsockopt (sock_ho_link, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt));
		   	//if((setsockopt (sock_ho_link, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt)))==-1)
									
		   	
		   	int len;	                        
		   	len = sizeof(my_addr);
						    						     	
		   	while( bind(sock_ho_link, (struct sockaddr *)&my_addr, sizeof my_addr) == -1)
		   		{
		   		showerr("sock_ho_link bind");
		   		}
		   	
		   	getsockname(sock_ho_link, (struct sockaddr *)&my_addr, &len);
			}
		int opt;
			
			
		if((setsockopt (sock_ho_link, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt)))==-1)
			perror(" ");
		
		
		//************************************************************************
		my_addr_con.sin_family = AF_INET;          		// host byte order
		my_addr_con.sin_addr.s_addr =inet_addr(assoc_node->ServerIP);                        
		my_addr_con.sin_port = htons(EMF_SERVER_PORT);
		memset(my_addr_con.sin_zero, '\0', sizeof my_addr_con.sin_zero);
		
		//-- create the new connection
		if (connect(sock_ho_link, (struct sockaddr *)&my_addr_con, sizeof my_addr_con) == -1)
			{
			showerr("sock_ho_link connect");
			}
		else
			{      

			//--------set the new descriptor in the select, add the new descriptor in the cid array, increment the counter
			FD_SET(sock_ho_link, &read_master);
			if(sock_ho_link > fdmax)
				fdmax = sock_ho_link;
			assoc_node->cid[1] = sock_ho_link;
			assoc_node->c_ctr = 2;
			assoc_node->scenario = scenario;
			printf("\nCMD_HANDLER: scenario = %d\n", assoc_node->scenario);
			
			assoc_node->prev_throughput[1] = 0;
			
			//if(assoc_node->con_send_head[0] == NULL)
			{
			
			//--------send the HANDOVER packet on the newly established connection
			header_size = make_emf_packet(assoc_node, assoc_node->emf_send_head->seq_no, 0, emf_packet, HANDOVER);
			
			if(send(assoc_node->cid[1], emf_packet, header_size, 0))
				{						
				save_and_update_emf_buffer(assoc_node, assoc_node->emf_send_head->data, assoc_node->emf_send_head->seq_no, header_size, 0, 1);
				assoc_node->last_data_in_buffer[1] = header_size;
				assoc_node->prev_throughput[1] = 0;
				}
			else
				{
				
				//get extended error information and handle error
				FD_CLR(sock_ho_link, &read_master);
				close(sock_ho_link);
				return;
				}
			}
			}	
		}
}
//-----------------------------------------------------------------------------------------

