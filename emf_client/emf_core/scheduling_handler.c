//-----------------------------------------------------------------------------
#include "emf.h"
#include <sys/ioctl.h>
#include <linux/sockios.h>
#include "PacketFormats.h"

//# include <ctype.h>

# include "openssl/aes.h"

# define BLOCK_LEN	16
# define MAXBUF		65536
# define MAXHEX		(MAXBUF * 2) + 1


#define SO_REUSEPORT 15

#define show_msg 0

//-----------------------------------------------------------------------------

int is_stable(int cid);
void get_and_bind_to_ip(int sock_fd);
void update_emf_send_list(struct assoc_list *assoc_node, unsigned int packet_size);
void update_con_send_buffer(struct assoc_list *assoc_node, unsigned int instant_throughput, int interfaceIndex);
//void save_and_update_emf_buffer(int hdr_size, struct assoc_list *assoc_node, int packet_size, int interfaceIndex);
void save_and_update_emf_buffer(struct assoc_list *assoc_node, char *data, unsigned int seq_no, unsigned int hdr_size, unsigned int packet_size, int interfaceIndex);

//-----------------------------------------------------------------------------

void schedule_data()
{
	//if (show_msg)
		//showmsg("\nscheduling_handler: schedule_data() function called, now inside it...");

	unsigned int i, packet_size, instant_throughput, header_size, seq_no, instant_throughput2; // for new connection (for handover)
	int buffer_empty_space, len, current_data, buffer_empty_space2, current_data2, unread_data;
	struct assoc_list *assoc_node;
	struct sockaddr_in *addr, con_addr, my_addr;
	int diff, packet_type, alldatasent;
	int CompleteTrafficMsg=0;
	int CompleteLinkMsg =0;
	int CompleteConnectionMsg =0;
	int CompleteHA=0;
	char InternalFromIP[16];
	char InternalToIP[16];
	int Portno;
	
	//------loop through the complete association list
	for(assoc_node = assoc_head; assoc_node != NULL; assoc_node = assoc_node->next)
		{
		assoc_node->HandoverCalled=0;

		//---------- if the scenario is normal and there is data to be send then proceed
		if(assoc_node->scenario == NORMAL)
			//if(assoc_node->emf_send_head->length != 0)
				if(assoc_node->emf_send_head->length > 1)
				{
				i = 0;
				
				//---- when scheduling counter is 0, this means this is the first time and send at most 32kB data
				if(assoc_node->sched_ctr[i] == 0)
					{				   
					packet_size = assoc_node->emf_send_head->length;
					
					//--- decide the packet size
					if(packet_size > INITIAL_PACKET_SIZE)
							packet_size = INITIAL_PACKET_SIZE;
					
					if(packet_size > 0)
						{
						header_size = make_emf_packet(assoc_node, assoc_node->emf_send_head->seq_no, packet_size, emf_packet, DATAPACK);
						memcpy(&emf_packet[header_size], assoc_node->emf_send_head->data, packet_size); //copy data in packet seperately
						
						//--- send the data packet created above
						if(assoc_node->dont_send[i] == 0)
							if(send(assoc_node->cid[i], emf_packet, header_size+packet_size, 0))
								{
								
								//printf("\n\npacket_size : %d\n",packet_size);
								//printf("assoc_node->emf_send_head->data %d\n",assoc_node->emf_send_head->length);
								
								//--------remove the data that is send by the emf from the emf list
								save_and_update_emf_buffer(assoc_node, assoc_node->emf_send_head->data, assoc_node->emf_send_head->seq_no, header_size, packet_size, i);
								
								// -----place the data removed from the emf lsit in the emf send buffer and keep it there till an acked for that data is received by the TCP
								update_emf_send_list(assoc_node, packet_size);						
								
								assoc_node->last_data_in_buffer[i] = header_size + packet_size;
								assoc_node->prev_throughput[i] = 0;
								}
							else
								{
								//-----------get extended error information and handle error
								}
						}
					assoc_node->sched_ctr[i]++;			
					}
				
				//---------- don't send any data for next 4 iterations
				else if(assoc_node->sched_ctr[i] < 4) 
					{
					assoc_node->sched_ctr[i]++;
					}
				
				else
					{   
					
					//-- now if the scheduling counter is not 0, and data has been previously send on this connection
					len = sizeof(buffer_empty_space);
					if(getsockopt(assoc_node->cid[i],SOL_SOCKET,SO_SNDBUF,&buffer_empty_space,&len) == -1)//--retrieve the size of data in the tcp buffer of this connection
						{
						buffer_empty_space = 256*1024;
						}
					
					if(ioctl(assoc_node->cid[i],SIOCOUTQ, &current_data) == -1)// retrive the size of unsent data
						current_data = 0;
						
					buffer_empty_space -= current_data;	

					//--- calculate instant_through put
					instant_throughput = assoc_node->last_data_in_buffer[i] - current_data;					
					
					//--- calculate the instant throughput and on its base update the connection send buffer
					update_con_send_buffer(assoc_node, instant_throughput, i);					
					
					//--- calculate average through out
					assoc_node->avg_throughput[i] = (assoc_node->prev_throughput[i] + instant_throughput) / 2;
					
					
					//--- throughput is zero and we have data to send
					if( (assoc_node->avg_throughput[i] == 0) && (current_data > 0) )
						{
						//IMPORTANT: this is complete implemented and tested with HostAgent:: //Zeeshan Dec-4 
						//But not yet used
						
						/*
						printf("\nScheduler: Inside INTERFACE_IP_REQ\n");
						
						//--- sending IP INTERFACE_IP_REQ to Host Agent as throughpput is zero
						
						HA_TLV.TLVType = INTERFACE_IP_REQ;
						
						HA_TLV.TLVLength = 19;
						
						TLVUnit.intValue = assoc_node->aid;
						
						for(i=0; i<4; i++)
							HA_TLV.TLVValue[i] = TLVUnit.charValue[i];
						
						len = sizeof(struct sockaddr);
						if(getsockname(assoc_node->cid[0], (struct sockaddr*)&my_addr, &len) == 0)
						{	
							memcpy(&HA_TLV.TLVValue[i], inet_ntoa(my_addr.sin_addr), strlen(inet_ntoa(my_addr.sin_addr)) );
							HA_TLV.TLVValue[i+strlen(inet_ntoa(my_addr.sin_addr))] = '\0';
							//printf("Got IP: %s\n%s\n", inet_ntoa(my_addr.sin_addr), &HA_TLV.TLVValue[12]);
							//memcpy(&InternalFromIP, inet_ntoa(my_addr.sin_addr), strlen(inet_ntoa(my_addr.sin_addr)) +1 );
							//InternalFromIP[strlen(inet_ntoa(my_addr.sin_addr))+1]='\0';   //AMBREEN
						}
						else 
						{
							memcpy(&HA_TLV.TLVValue[i], "0.0.0.0", 7);
							HA_TLV.TLVValue[i+7] = '\0';
							//printf("Not Got IP: %s\n", &HA_TLV.TLVValue[12]);
						}
						
						if (send(sock_ha, &HA_TLV, sizeof(HA_TLV), 0)==-1)	//1. send association information to HA
						{
							showerr("Can't send local compliance to host agent...:");
						}
						
						printf("\nCORE : INTERFACE_IP_REQ Message sent to Host Agent with AID(%d) and IP(%s) ", assoc_node->aid ,inet_ntoa(my_addr.sin_addr));
					
					*/
						
						}
						
					
					if( (instant_throughput < LOWER_THROUGHPUT_LIMIT) && (current_data > LOWER_THROUGHPUT_LIMIT) )
						{
						if(assoc_node->sched_ctr[i] < 8)
							{
							assoc_node->sched_ctr[i]++;
							continue;
							}
						else
							if(assoc_node->sched_ctr[i] == 8)
								{
								assoc_node->sched_ctr[i]++;
								}
						else
							if(assoc_node->sched_ctr[i] < 13)
								{
								assoc_node->sched_ctr[i]++;
								continue;
								}
						else
							{
							//reschedule in case of BA
							//packet_type = DATAPACK;
							//reschedule_data(assoc_node, emf_packet, i, packet_type);
							assoc_node->sched_ctr[i] = 4;
							//continue;
							}
						}
					else
						assoc_node->sched_ctr[i] = 4;
							
							
					//--- calculate the packet size , depeding upon the data to be send present in the emf buffer 
					//--- assoc_node->emf_send_head->length , maximum packet size defined by the EMF and the instant throughout
					packet_size = instant_throughput*2;
					
					if(packet_size == 0 && assoc_node->last_buf_empty == 1)
						{
						packet_size = assoc_node->emf_send_head->length;
						}
					
					if(packet_size > buffer_empty_space)
						packet_size = buffer_empty_space;
					
					if(packet_size > assoc_node->emf_send_head->length)
						packet_size = assoc_node->emf_send_head->length;
						
					if(packet_size > MAX_PACKET_SIZE)
						packet_size = MAX_PACKET_SIZE;
               
					if(assoc_node->emf_send_head->length == 0)
						{
						assoc_node->last_buf_empty = 1;
						}
					else
						assoc_node->last_buf_empty = 0;
					
					if(packet_size > 0)
						{
						
						//---make the emf header and place the data in it
						header_size = make_emf_packet(assoc_node, assoc_node->emf_send_head->seq_no, packet_size, emf_packet, DATAPACK);
						memcpy(&emf_packet[header_size], assoc_node->emf_send_head->data, packet_size); //copy data in packet seperately
						
						
						if(assoc_node->dont_send[i] == 0)
							if(send(assoc_node->cid[i], emf_packet, header_size+packet_size, 0))
								{
								save_and_update_emf_buffer(assoc_node, assoc_node->emf_send_head->data, assoc_node->emf_send_head->seq_no, header_size, packet_size,i);
								update_emf_send_list(assoc_node, packet_size);					
								assoc_node->last_data_in_buffer[i] = current_data +  header_size + packet_size;
								assoc_node->prev_throughput[i] = instant_throughput;	
								}
							else
								{
								//get extended error information and handle error
								}	
						}
						else
							{
							assoc_node->last_data_in_buffer[i] = current_data;
							assoc_node->prev_throughput[i] = instant_throughput;	
							}
					}			
				}
		
		
		//--- this scenario is executed when there is a trafic shift command or a link shift command from the HA, or when a handover packet is received by the remote side.
		if(assoc_node->scenario == HO_LINK_SHIFT_COMMIT  || assoc_node->scenario == HO_TRAFFIC_SHIFT_COMMIT || assoc_node->scenario == HAND_OVER || assoc_node->scenario == HO_CONNECTION_SHIFT_COMMIT)
			{
			if(assoc_node->scenario == HO_LINK_SHIFT_COMMIT)
				CompleteLinkMsg=1;
			
			if(assoc_node->scenario == HO_TRAFFIC_SHIFT_COMMIT)
				{
				CompleteTrafficMsg=1;
				Portno = assoc_node->ProtocolNo;//my_addr.sin_port;
				}
			
				assoc_node->HandoverCalled=1;
				
				// this procedure of calculating the instant throughput, average throughput and updating the connection send buffer is exactly the same as defined above
				len = sizeof(buffer_empty_space);
				
				if(getsockopt(assoc_node->cid[0],SOL_SOCKET,SO_SNDBUF,&buffer_empty_space,&len) == -1)//total data
					buffer_empty_space = 256*1024;
				
				if(ioctl(assoc_node->cid[0],SIOCOUTQ,&current_data) == -1)// unsent data
					current_data = 0;
				buffer_empty_space -= current_data;					
				instant_throughput = assoc_node->last_data_in_buffer[0] - current_data;				
				//printf("updating con_send_buffer of first connection\n");
				update_con_send_buffer(assoc_node, instant_throughput, 0);
				assoc_node->avg_throughput[0] = (assoc_node->prev_throughput[0] + instant_throughput) / 2;
				
				//------for second connection in the case of hand over
				len = sizeof(buffer_empty_space2);
				
				if(getsockopt(assoc_node->cid[1],SOL_SOCKET,SO_SNDBUF,&buffer_empty_space2,&len) == -1)//total data
					buffer_empty_space2 = 256*1024;
				
				if(ioctl(assoc_node->cid[1],SIOCOUTQ,&current_data2) == -1)// unsent data
					current_data2 = 0;
				
				buffer_empty_space2 -= current_data2;								
				instant_throughput2 = assoc_node->last_data_in_buffer[1] - current_data2;
				update_con_send_buffer(assoc_node, instant_throughput2, 1);
				assoc_node->avg_throughput[1] = (assoc_node->prev_throughput[1] + instant_throughput2) / 2;
				

				//---checking stability of first connection
				if(assoc_node->scenario == HO_LINK_SHIFT_COMMIT  || assoc_node->scenario == HO_TRAFFIC_SHIFT_COMMIT || assoc_node->scenario == HO_CONNECTION_SHIFT_COMMIT)
					{
					
					if(assoc_node->cid[1] != 0)
					
						if(is_stable(assoc_node->cid[1])) //-- the si_stable function check the stability of a connection
							{
							int old_des=0;
							printf("\nCORE : The new Connection got stable CID(%d) , Closing old connection CID(%d) ",assoc_node->cid[1],assoc_node->cid[0]);
							/*************************************************************/
							len = sizeof(struct sockaddr);
							
							if(getsockname(assoc_node->cid[0], (struct sockaddr*)&my_addr, &len) == 0)
								{	
								memcpy(&InternalToIP, inet_ntoa(my_addr.sin_addr), strlen(inet_ntoa(my_addr.sin_addr)) +1 );
								InternalToIP[strlen(inet_ntoa(my_addr.sin_addr))+1]='\0';
								//printf("INternaltoIP%s:\n",InternalToIP);
								}
							/*************************************************************/
							if(assoc_node->con_send_tail[0]!=NULL)
								{
								if(assoc_node->con_send_head[0]->data_size > 0)
									{
									reschedule_data(assoc_node,emf_packet, 0, 1,DATAPACK);
									//-- reschedule the un send data in the tcp buffer of the first connection to the emf buffer of the new connection, remember all the unacked data of the tcp is saved in the con send buffer
									}
								}
							//	else
							//printf("no data to place back\n");		

							//--- receving the remainging data on the new conenction
							while(1)
								{
								unread_data = 0;		
								if(ioctl(assoc_node->cid[0],SIOCINQ,&unread_data) == -1)// unsent data
									break;//unread_data = 0;
								if(unread_data > 0)
									{
									//printf("Getting remaining data after handover: %d\n", unread_data);
									Handle_Reassembly(0, assoc_node);
									}
								else break;
								}
							
							//-- now closing the old connection and setting up the variables for the new connection
							close(assoc_node->cid[0]);
							FD_CLR(assoc_node->cid[0], &read_master);
							//assoc_node->scenario = NORMAL;
												
							
							//printf("interrupted seq_no %d\n",	assoc_node->expected_seq_no);
							old_des=assoc_node->cid[0];
							assoc_node->cid[0] = assoc_node->cid[1];
							assoc_node->con_recv_node[0].length=0;
							assoc_node->con_recv_node[0].byte_received=0;
							
							//printf("new : %d\n",assoc_node->cid[0]);
							assoc_node->c_ctr = 1;
        			        assoc_node->cid[1]=0;
				
        			        //--- send handover commit message to confirm the host agent that a successfull hand over has taken place
        			        if (assoc_node->scenario == HO_CONNECTION_SHIFT_COMMIT)
        			        	HA_TLV.TLVType = INTERFACE_IP_COM;
        			        else
        			        	HA_TLV.TLVType = HANDOVER_COMMIT;
        			        
        			        //printf("\nSchedular: Scenario: %d", assoc_node->scenario);
        			        //printf("\nSchedular: TLV Type: %d", HA_TLV.TLVType);
        			        
        			        
        			        //IMPORTANT: setting the scenario NORMAL must be after TLV Type assignment
        			        assoc_node->scenario = NORMAL;
        			        
        			        HA_TLV.TLVLength = 4;
        			        TLVUnit.intValue = assoc_node->aid; 
        			        for(i=0; i<4; i++)
        			        	HA_TLV.TLVValue[i] = TLVUnit.charValue[i];
				
							TLVUnit.intValue = old_des;//assoc_node->cid[0]; 
				
							for(; i<8; i++)
								HA_TLV.TLVValue[i] = TLVUnit.charValue[i-4];
					
							TLVUnit.intValue = assoc_node->cid[0]; 
							
							for(; i<12; i++)
								HA_TLV.TLVValue[i] = TLVUnit.charValue[i-8];
						
							len = sizeof(struct sockaddr);
							if(getsockname(assoc_node->cid[0], (struct sockaddr*)&my_addr, &len) == 0)
   								{	
   								memcpy(&HA_TLV.TLVValue[i], inet_ntoa(my_addr.sin_addr), strlen(inet_ntoa(my_addr.sin_addr)) );
   								HA_TLV.TLVValue[i+strlen(inet_ntoa(my_addr.sin_addr))] = '\0';
   								//printf("Got IP: %s\n%s\n", inet_ntoa(my_addr.sin_addr), &HA_TLV.TLVValue[12]);
			   					memcpy(&InternalFromIP, inet_ntoa(my_addr.sin_addr), strlen(inet_ntoa(my_addr.sin_addr)) +1 );
			   					InternalFromIP[strlen(inet_ntoa(my_addr.sin_addr))+1]='\0';   //AMBREEN
   								}
			   				else 
			   					{
			   					memcpy(&HA_TLV.TLVValue[i], "0.0.0.0", 7);
   								HA_TLV.TLVValue[i+7] = '\0';
   								//printf("Not Got IP: %s\n", &HA_TLV.TLVValue[12]);
			   					}
							
							if (send(sock_ha, &HA_TLV, sizeof(HA_TLV), 0)==-1)	//1. send association information to HA
								{
								showerr("Can't send local compliance to host agent...:");
								}
							
							printf("\nCORE : HANDOVER_COMMIT Message sent to Host Agent with AID(%d), old CID(%d) newCID(%d) and IP(%s) ", assoc_node->aid, old_des, assoc_node->cid[0],inet_ntoa(my_addr.sin_addr));
							}
					//}
					else
						{
						// ---------now continue sending the data on both connections if the 2nd connection doesnot get stable

						if(instant_throughput >= instant_throughput2)
							packet_size = instant_throughput*2;
						else
							packet_size = instant_throughput2*2;
						
						if(packet_size == 0 && assoc_node->last_buf_empty == 1)
							{
							packet_size = assoc_node->emf_send_head->length;
							}
						
						if(packet_size > buffer_empty_space)
							packet_size = buffer_empty_space;
						
						if(packet_size > assoc_node->emf_send_head->length)
							packet_size = assoc_node->emf_send_head->length;
					
						if(packet_size > MAX_PACKET_SIZE)
							packet_size = MAX_PACKET_SIZE;
                   	                
						if(assoc_node->emf_send_head->length == 0)
							{
							assoc_node->last_buf_empty = 1;
							}
						else
							assoc_node->last_buf_empty = 0;
					
						if(packet_size > 0)
							{
							//------sending data on both connections
							header_size = make_emf_packet(assoc_node, assoc_node->emf_send_head->seq_no, packet_size, emf_packet, DATAPACK);
							memcpy(&emf_packet[header_size], assoc_node->emf_send_head->data, packet_size); //copy data in packet seperately
							
							if(assoc_node->dont_send[0] == 0)				   
								if(send(assoc_node->cid[0], emf_packet, header_size+packet_size, 0))
									{
									save_and_update_emf_buffer(assoc_node, assoc_node->emf_send_head->data, assoc_node->emf_send_head->seq_no, header_size,  packet_size, 0);
									//update_emf_send_list(assoc_node, packet_size);
									assoc_node->last_data_in_buffer[0] = current_data +  header_size + packet_size;
									assoc_node->prev_throughput[0] = instant_throughput;				
									
									}
								else
									{
									//get extended error information and handle error
									}
					
							//------sending data on second connection
							if(assoc_node->cid[1] != 0)
								if(assoc_node->dont_send[1] == 0)
									{
									if(send(assoc_node->cid[1], emf_packet, header_size+packet_size, 0))
										{
										save_and_update_emf_buffer(assoc_node, assoc_node->emf_send_head->data, assoc_node->emf_send_head->seq_no, header_size,  packet_size, 1);
										update_emf_send_list(assoc_node, packet_size);
										
										assoc_node->last_data_in_buffer[1] = current_data2 +  header_size + packet_size;
										assoc_node->prev_throughput[1] = instant_throughput2;
										}
									
									else
										{
										//get extended error information and handle error
										}
									}
								else
									update_emf_send_list(assoc_node, packet_size);
							}
						
						else
							{
							assoc_node->last_data_in_buffer[0] = current_data;
							assoc_node->prev_throughput[0] = instant_throughput;
							assoc_node->last_data_in_buffer[1] = current_data2;
							assoc_node->prev_throughput[1] = instant_throughput2;
							}
						//printf("after HAND_OVER2\n");
						}
					}
			}
		
		// ---- this scenario is executed in teh case of BA, THE PROCEDURE is almost the same as the normal scenario. no big changes except that in this case different data will be send on 2 different connections rather than 1
		if(assoc_node->scenario == BANDWIDTH_AGGREGATION)
			if(assoc_node->emf_send_head->length != 0)
				{
				for(i = 0; i < assoc_node->c_ctr; i++) // this is the loop that ensures the sending of data on 2 connections
					{	
					if(assoc_node->sched_ctr[i] == 0)// first time send at most 32kB data
						{
						packet_size = assoc_node->emf_send_head->length;
						
						if(packet_size > INITIAL_PACKET_SIZE)
						packet_size = INITIAL_PACKET_SIZE;
						
						if(packet_size > 0)
							{
							header_size = make_emf_packet(assoc_node, assoc_node->emf_send_head->seq_no, packet_size, emf_packet, DATAPACK);
							memcpy(&emf_packet[header_size], assoc_node->emf_send_head->data, packet_size); //copy data in packet sepera
							
							if(assoc_node->dont_send[i]==0)
								if(send(assoc_node->cid[i], emf_packet, header_size+packet_size, 0))
									{
									save_and_update_emf_buffer(assoc_node, assoc_node->emf_send_head->data, assoc_node->emf_send_head->seq_no, header_size, packet_size, i);
									update_emf_send_list(assoc_node, packet_size);
									
									assoc_node->last_data_in_buffer[i] = header_size + packet_size;
									assoc_node->prev_throughput[i] = 0;
									}
								else
									{
									//get extended error information and handle error
									}
							}
						
							assoc_node->sched_ctr[i]++;
							}
					
					else if(assoc_node->sched_ctr[i] < 4) // don't send any data for next 4 iterations
						assoc_node->sched_ctr[i]++;
						else
							{
							//current_data = buffer size – buffer empty space;
							//int len=sizeof(sndbufsiz);
							//err=getsockopt(sockfd,SOL_SOCKET,SO_SNDBUF,&sndbufsiz,&len);//total data
							//var=ioctl(sockfd,SIOCOUTQ,&used);// unsent data
							//printf("check1\n");
							
							len=sizeof(buffer_empty_space);
							if(getsockopt(assoc_node->cid[i],SOL_SOCKET,SO_SNDBUF,&buffer_empty_space,&len) == -1)//total data
								buffer_empty_space = 256*1024;
							
							if(ioctl(assoc_node->cid[i],SIOCOUTQ,&current_data) == -1)// unsent data
								{
								//printf("% d : qued data call failed:\n",i);
								current_data = 0;
								}
														
							buffer_empty_space -= current_data;	
							instant_throughput = assoc_node->last_data_in_buffer[i] - current_data;						
							update_con_send_buffer(assoc_node, instant_throughput, i);					
							assoc_node->avg_throughput[i] = (assoc_node->prev_throughput[i] + instant_throughput) / 2;							
																	
							if( (instant_throughput < LOWER_THROUGHPUT_LIMIT) && (current_data > LOWER_THROUGHPUT_LIMIT) )
								{
								if(assoc_node->sched_ctr[i] < 8)
									{	
									assoc_node->sched_ctr[i]++;
									continue;
									}
								else
									if(assoc_node->sched_ctr[i] == 8)
										{
										assoc_node->sched_ctr[i]++;
										}
									else
										if(assoc_node->sched_ctr[i] < 13)
											{
											assoc_node->sched_ctr[i]++;
											continue;
											}
										else
											{
											//reschedule in case of BA
											/*packet_type = DATAPACK;
											int j, max_throughput, toIndex;
											for (j = 0; j < assoc_node->c_ctr; j++)
											if(assoc_node->avg_throughput[j] > max_throughput)
											toIndex = j;*/
											
											//reschedule_data(assoc_node, emf_packet, i, toIndex, packet_type);
											assoc_node->sched_ctr[i] = 4;
											//continue;
											}						
								}
							
							else
								assoc_node->sched_ctr[i] = 4;
							
							packet_size = instant_throughput*2;
							
							if(packet_size == 0 && assoc_node->last_buf_empty == 1)
								{
								packet_size = assoc_node->emf_send_head->length;
								}
							
							if(packet_size > buffer_empty_space)
								packet_size = buffer_empty_space;					
							
							if(packet_size > assoc_node->emf_send_head->length)
								packet_size = assoc_node->emf_send_head->length;						
							
							if(packet_size > MAX_PACKET_SIZE)
								packet_size = MAX_PACKET_SIZE;
							
							if(assoc_node->emf_send_head->length == 0)
								{
								assoc_node->last_buf_empty = 1;
								}
							else
								assoc_node->last_buf_empty = 0;
							
							if(packet_size > 0)
								{
								header_size = make_emf_packet(assoc_node, assoc_node->emf_send_head->seq_no, packet_size, emf_packet, DATAPACK);
								memcpy(&emf_packet[header_size], assoc_node->emf_send_head->data, packet_size); //copy data in packet seperately
								
								if(assoc_node->dont_send[i]==0)
									if(send(assoc_node->cid[i], emf_packet, header_size+packet_size, 0))
										{
										save_and_update_emf_buffer(assoc_node, assoc_node->emf_send_head->data, assoc_node->emf_send_head->seq_no, header_size,  packet_size, i);
										update_emf_send_list(assoc_node, packet_size);
										
										assoc_node->last_data_in_buffer[i] = current_data +  header_size + packet_size;
										assoc_node->prev_throughput[i] = instant_throughput;
										}
									else
										{
										//get extended error information and handle error
										}
								}
							else
								{
								assoc_node->last_data_in_buffer[i] = current_data;
								assoc_node->prev_throughput[i] = instant_throughput;
								}
							}
					}
				
				
				/*if( (assoc_node->terminating) && (alldatasent == 1) && (assoc_node->emf_send_head->length == 0) )
					{
				   	header_size = make_emf_packet(assoc_node, assoc_node->emf_send_head->seq_no, 0, emf_packet, ASSOCIATION_TERMINATION_REQUEST);
					send(assoc_node->cid[0], emf_packet, header_size, 0);
					dont_send[0] = 1;
					continue;
					}*/
				}
		
		
		if( (assoc_node->dont_send[0] == 0) && (assoc_node->terminating == 1) )
			{
			alldatasent = 1;
			for(i = 0; i < assoc_node->c_ctr; i++)
				{
				if(ioctl(assoc_node->cid[i],SIOCOUTQ,&current_data) == -1)// unsent data
					current_data = 0;
				   	if(current_data != 0)
				   		alldatasent = 0;
				}
			
			
			//sending association temination message when there is no data lleft to send
			if( (assoc_node->terminating == 1  ) && (alldatasent == 1) && (assoc_node->emf_send_head->length == 0) )
			   	{
			   	printf("sending association termination request\n");
			   	header_size = make_emf_packet(assoc_node, assoc_node->emf_send_head->seq_no, 0, emf_packet, ASSOCIATION_TERMINATION_REQUEST);
				send(assoc_node->cid[0], emf_packet, header_size, 0);
				
				// setting the time stamp while sending association termination request
				assoc_node->association_termination_request_timestamp = time(NULL); 
				//printf ("\nUsing ctime: %s\n", ctime(&assoc_node->association_termination_request_timestamp));
				
				assoc_node->dont_send[0] = 1;
				
				//continue;
			   	}

			}
		
		}// end of for loop for complete association list search


	
	// link shift complete and traffic shift complete message is send when link shift and traffic shifts on all the connections in an association are succesfful.
	if(CompleteTrafficMsg==1 || CompleteLinkMsg ==1 || CompleteConnectionMsg ==1 )
		{
		int TrafficShift=0;
		int LinkShift=0;
		int ConnectionShift=0;
		
		for(assoc_node = assoc_head; assoc_node != NULL; assoc_node = assoc_node->next)
			{		
			if (assoc_node->HandoverCalled==1)
				{												
				if (assoc_node->scenario == HO_TRAFFIC_SHIFT_COMMIT)
					// this means handover was called but still the other connection is not stable
					{
					TrafficShift=1;
					}
				if (assoc_node->scenario == HO_LINK_SHIFT_COMMIT)
					LinkShift=1;
				
				if (assoc_node->scenario == HO_CONNECTION_SHIFT_COMMIT)
					ConnectionShift=1;
				
				}
			}
		
		if(CompleteLinkMsg==1)
			{
			if(LinkShift==0)
				{		
		
				int remainData1;				

				HA_TLV.TLVType = HO_LINK_SHIFT_COMPLETE;
				HA_TLV.TLVLength = 25;
				char j=0;
						   					
				for (i = 0; i < strlen(InternalToIP); i++)
					{
					HA_TLV.TLVValue[j] = InternalToIP[i];	                    
					j++;
					}
				
				HA_TLV.TLVValue[j]=':';
				j++;
					
				for (i = 0; i < strlen(InternalFromIP); i++, j++)
					{
					HA_TLV.TLVValue[j] = InternalFromIP[i];	                        					
					}
							
				HA_TLV.TLVValue[j] = '\0';
				
				int sendbytes=0;
				if (sendbytes=(send(sock_ha, &HA_TLV, sizeof(HA_TLV), 0))==-1)	//1. send association information to HA
					{
					showerr("Can't send local compliance to host agent...:");
					}
				else
					{
                    printf("\nCORE : HO_LINK_SHIFT_COMPLETE Message sent to Host Agent fromIP(%s) : ToIP(%s) ",InternalToIP, InternalFromIP);
					}
				
				}
			}
		
		
		
		if(CompleteTrafficMsg==1)
			{
			if(TrafficShift == 0)
				{
				HA_TLV.TLVType = HO_TRAFFIC_SHIFT_COMPLETE;
				HA_TLV.TLVLength = 34;
				char j=0;

				TLVUnit.intValue = Portno;	
				for(j=0; j<4; j++)										// Port Number
					{
					HA_TLV.TLVValue[j] = TLVUnit.charValue[j];
					//printf("%d", HA_TLV.TLVValue[j]);
					}
				
				for (i=0; i < strlen(InternalToIP); i++, j++)			// ToIP
					{
					HA_TLV.TLVValue[j] = InternalToIP[i];
					//printf("%c", HA_TLV.TLVValue[j]);
					}
				
				HA_TLV.TLVValue[j++]=':';
				
				for (i=0; i < strlen(InternalFromIP); i++, j++)			// FromIP
					{
					HA_TLV.TLVValue[j] = InternalFromIP[i];
					//printf("%c", HA_TLV.TLVValue[j]);
					}
				
				HA_TLV.TLVValue[j] = '\0';

				if (send(sock_ha, &HA_TLV, sizeof(HA_TLV), 0)==-1)	//1. send association information to HA
					{
					showerr("Can't send local compliance to host agent...:");
					}
				else
					{
				    printf("\nCORE : HO_TRAFFIC_SHIFT_COMPLETE Message sent to Host Agent with fromIP(%s) : ToIP(%s) for PORT(%d) ", InternalToIP , InternalFromIP, Portno);
					//printf("send successfully\n");o
					}
				}
			
			} // end of complete traffir shift 
		
		} // end of main is condition
	
} //  end of schedule data function 




//-----------------------------------------------------------------------------

int is_stable(int cid)
{
	if (show_msg)
		showmsg("\nscheduling_handler: is_stable() function called, now inside it...");
	
	unsigned int cong_wnd, ssthreshold;
	struct tcp_info tcp_info;
	int tcp_info_length;

	tcp_info_length = sizeof(tcp_info);
	if ( getsockopt( cid, SOL_TCP, TCP_INFO, (void *)&tcp_info, (socklen_t *)&tcp_info_length ) == 0 ) 
		{
		// printf("snd_cwnd: %u\nsnd_ssthresh: %u\nrcv_ssthresh: %u\nrtt: %u\nrtt_var: %u\n", tcp_info.tcpi_snd_cwnd, tcp_info.tcpi_snd_ssthresh, tcp_info.tcpi_rcv_ssthresh, tcp_info.tcpi_rtt, tcp_info.tcpi_rttvar);
		//get congestion window of TCP connection: cwnd = congestion window and slow start threshold: ssthreshold = ssthreshold
		//checking whether TCP is in slow start phase or in linear increase
		if(tcp_info.tcpi_snd_cwnd >= tcp_info.tcpi_snd_ssthresh)
			return 1;
		}
	//return 0;
	return 1;
}

//-----------------------------------------------------------------------------




void reschedule_data(struct assoc_list *assoc_node, char *emf_packet, int fromIndex, int toIndex, unsigned int packet_type)
{
	if (show_msg)
		showmsg("\nscheduling_handler: reschedule_data() function called, now inside it...");
	
	int diff, ret=0, cdata=0;
	unsigned int packet_size, seq_no, header_size;
	
	if(assoc_node->con_send_head[fromIndex] != NULL)
		{
		diff = assoc_node->con_send_head[fromIndex]->bytes_sent - assoc_node->con_send_head[fromIndex]->hdr_size;
		if(diff <= 0)
			{
			//seq_no = assoc_node->con_send_head[i]->seq_no;
			diff = 0;
			}
		//get unsent data in old connection
		struct con_slist *con_send_node;
		//packet_type = HANDOVER_DATA;
		//assoc_node->last_data_in_buffer[1] = 0;
		
		for (con_send_node = assoc_node->con_send_head[fromIndex]; con_send_node != NULL; con_send_node = con_send_node->next)
			{
			seq_no = con_send_node->seq_no + diff;
			packet_size = (con_send_node->data_size - diff);
			//		reserved = 64;
			header_size = make_emf_packet(assoc_node, seq_no, packet_size, emf_packet, packet_type);
			reserved = 0;
			memcpy(&emf_packet[header_size], &con_send_node->data[diff], packet_size );
			
			//memcpy(&emf_packet[header_size], assoc_node->emf_send_head->data, packet_size); //copy data in packet seperately
			if(ioctl(assoc_node->cid[toIndex],SIOCOUTQ,&cdata) == -1)// unsent data
				cdata = 0;
						
			if((ret=send(assoc_node->cid[toIndex], emf_packet, header_size+packet_size, 0))>0)
				{
				if(ioctl(assoc_node->cid[toIndex],SIOCOUTQ,&cdata) == -1)// unsent data
					cdata = 0;
				save_and_update_emf_buffer(assoc_node, &con_send_node->data[diff], seq_no, header_size, packet_size, toIndex);
				assoc_node->last_data_in_buffer[toIndex] += (header_size + packet_size);
				}
			else
				{
				//get extended error information and handle error
				}
				
			diff = 0;
			packet_type = DATAPACK;
			
			} // end of for loop
		}
	else
		return;
}

//-----------------------------------------------------------------------------

void get_and_bind_to_ip(int sock_fd)
{
	if (show_msg)
		showmsg("\nscheduling_handler: get_and_bind_to_ip() function called, now inside it...");

	//need handle any message other than INTERFACE_IP_RES as well and loop until we don't receive our required message
	
	int TLVLength;
	char TLVValue[20];
	struct sockaddr_in con_addr;

	TLVValue[0] = INTERFACE_IP_REQ;
	if (send(sock_ha, TLVValue, 1, 0)==-1)		                              //1. check local compliance by sending ID and port number
		showerr("Can't send INTERFACE_IP_REQ to host agent...:");
	else
		{
		if (recv(sock_ha, TLVValue, 1, 0) ==-1)                               //2. receive Length value from H.A
			{
			showerr("Couldn't received INTERFACE_IP_REQ from host agent...:");
			}
		else
			{
			if(TLVValue[0] == INTERFACE_IP_RES)
				{
				if (recv(sock_ha, &TLVLength, 1, 0) ==-1)                            
					{
					showerr("Couldn't received INTERFACE_IP_REQ from host agent...:");
					}
				else
					{
					if (recv(sock_ha, TLVValue, TLVLength, 0) ==-1)                               
						{
						showerr("Couldn't received INTERFACE_IP_REQ from host agent...:");
						}
					else
						{
						con_addr.sin_family = AF_INET;
						con_addr.sin_addr.s_addr = inet_addr(TLVValue);
						memset(con_addr.sin_zero, '\0', sizeof con_addr.sin_zero);

						int opt=1;
                        setsockopt (sock_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof (opt));
                        setsockopt (sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt));
                        
						if(bind(sock_fd, (struct sockaddr*) &con_addr, sizeof(con_addr)) == -1)
							{
							showerr("bind error:");
							}
						else
							{
		
							}
						}
					}
				}
			}
		}
}
//-----------------------------------------------------------------------------



void update_emf_send_list(struct assoc_list *assoc_node, unsigned int packet_size)
{
	//if (show_msg)
		//showmsg("\nscheduling_handler: update_emf_send_list() function called, now inside it...");

	int i;

	// need to rethink to delete data below
	//assoc_node->emf_send_head->data   += packet_size;
	for(i = packet_size; i < assoc_node->emf_send_head->length; i++)
		{
		assoc_node->emf_send_head->data[i-packet_size] = assoc_node->emf_send_head->data[i];
		}
	
	assoc_node->emf_send_head->length -= packet_size;
	assoc_node->emf_send_head->seq_no += packet_size;
}




//-----------------------------------------------------------------------------





int make_emf_packet(struct assoc_list *assoc_node, unsigned int seq_no, unsigned int packet_size, char *emf_packet, int packet_type)
{
	//if (show_msg)
		//showmsg("\nscheduling_handler: make_emf_packet() function called, now inside it...");
	
	int index;
	
	//struct sockaddr_in *my_addr = (struct sockaddr_in *) assoc_node->dest_addr;
					
					
	baseheader_packet.baseheader_packet.Version = EMF_VERSION;
	baseheader_packet.baseheader_packet.PacketType = packet_type;
	baseheader_packet.baseheader_packet.Reserved = reserved;
	baseheader_packet.baseheader_packet.DataLength = packet_size;

	memcpy(emf_packet, baseheader_packet.str, sizeof(baseheader_packet));
	index = sizeof(baseheader_packet);
	
	if( (packet_type == HANDOVER) || (packet_type == BANDWIDTHAGGREGATION) || (packet_type == SINGLEJOIN) || (packet_type == HANDOVER_DATA) || (packet_type == BANDWIDTHAGGREGATION_DATA) || (packet_type == SINGLEJOIN_DATA) || (packet_type == ASSOCIATION_TERMINATION_REQUEST) || (packet_type == ASSOCIATION_TERMINATION_RESPONSE) || (packet_type == FINISH) || (packet_type == FINISH_ACK)) 
		{
		
		BIO *out;
	out=BIO_new(BIO_s_file());
	if (out == NULL) exit(0);
	BIO_set_fp(out,stdout,BIO_NOCLOSE);

	BIO_puts(out,assoc_node->sessionKey);

printf("length %d :\n", strlen(assoc_node->sessionKey)); 		
		//////////////////////////////////////////////////////////////////////////////////////////////////////////

			  unsigned char key[40];
  			  unsigned char iv[40];//  = IV;
              memcpy (key,  assoc_node->sessionKey, strlen (assoc_node->sessionKey));
			  memcpy (iv,  assoc_node->sessionKey, strlen (assoc_node->sessionKey));
              
              unsigned long ilen;			  
			  unsigned char ibuf[MAXBUF];	// padded input to encrypt
			  unsigned char obuf[MAXHEX];	// encrypt output
			  unsigned char xbuf[MAXHEX];	// hex encrypt output
			  unsigned char ybuf[MAXBUF];	// hex decrypt output
			  unsigned char dbuf[MAXBUF];	// decrypt output
			  
			                
				assoc_node->SNonce++;
				AES_KEY aeskeyEnc;
				int nonce =assoc_node->SNonce;
				char buf[100];

				
				sprintf(buf,"%d%s%d%s",assoc_node->aid,":",nonce,":");
		
				printf("\n%d\n",strlen(buf));
				// prepare the input data with padding
				memset (ibuf, 0x00, sizeof (ibuf));
				memcpy (ibuf, buf, strlen (buf));

				// pad and calc length of block to encode
				ilen = PadData (ibuf, (unsigned int) strlen (buf), BLOCK_LEN);
			 
				// init cipher keys 
				AES_set_encrypt_key (key, 256, &aeskeyEnc);

				// encrypt string
	//			memcpy (iv, IV, sizeof (IV));
				AES_cbc_encrypt (ibuf, obuf, ilen, &aeskeyEnc, iv, AES_ENCRYPT);

				// convert encoded string to hex and display
				bin2hex (obuf,xbuf,ilen);
				printf ("encode: %s (len = %d)\n", xbuf, (int) strlen (xbuf));

				//////////////decryption/////////////////////////////////////
  /*              AES_KEY aeskeyDec;
				AES_set_decrypt_key (key, 256, &aeskeyDec);
				// convert hex string to binary
				hex2bin (xbuf, ybuf, (int) strlen(xbuf));

				// compare binary values
				if (memcmp (obuf, ybuf, ilen) != 0) {
				  printf ("HEX DECODE FAILED!\n");
				  return (0);
				}

				// decrypt string
//				memcpy (iv, IV, sizeof (IV));
				AES_cbc_encrypt (ybuf, dbuf, ilen, &aeskeyDec, iv, AES_DECRYPT);
				dbuf [NoPadLen (dbuf, ilen)] = 0x00;
				printf ("decode: %s (len = %d)\n", dbuf, strlen(dbuf));

				// compare binary values
				//if (memcmp (buf, dbuf, (unsigned int) strlen (buf)) != 0) {
				  //printf ("DECRYPT FAILED!\n");
			//	  return (0);
				//}

				// insert blank line before next input prompt
				printf ("\n");



			aidnon *aidn2;
			aidn2 = (aidnon *)buf;

			printf("%d",aidn2->aid);
			printf("%d",aidn2->nonce);

*/


							  
			///////////////////////////////////////////////////////////////////////////////////////////////////////
			
		handover_packet.handover_packet.EncryptedAIDNonceClear = assoc_node->aid;
		strncpy(handover_packet.handover_packet.EncryptedAIDNonce,xbuf,strlen(xbuf));
		memcpy(&emf_packet[index],handover_packet.str,sizeof(handover_packet));
		index += sizeof(handover_packet);
		}
		
	if( (packet_type == DATAPACK) || (packet_type == HANDOVER_DATA) || (packet_type == BANDWIDTHAGGREGATION_DATA) || (packet_type == SINGLEJOIN_DATA) ) 
		{
		data_packet.data_packet.EMFSequenceNo = seq_no;
		data_packet.data_packet.ULID = assoc_node->ProtocolNo;
		memcpy(&emf_packet[index],data_packet.str,sizeof(data_packet));
		index += sizeof(data_packet);		
		}
	
//		printf("seq_no : %d \n",seq_no);
	//	data_packet.data_packet.EMFSequenceNo = assoc_node->emf_send_head->seq_no;
	//	data_packet.data_packet.ULID = my_addr->sin_port;
	
	//	emf_packet = malloc(sizeof(baseheader_packet) + sizeof(data_packet) + packet_size);
	
	//	memcpy(emf_packet, baseheader_packet.str, sizeof(baseheader_packet));
	//	memcpy(&emf_packet[sizeof(baseheader_packet)],data_packet.str,sizeof(data_packet));
	//	memcpy(&emf_packet[sizeof(baseheader_packet) + sizeof(data_packet)], assoc_node->emf_send_head->data, packet_size);
	
	/*if (packet_type == 1)
            	printf("\nCORE:CORE:[-_-_-_-_(EMF PACKET of TYPE ASSOCIATION_ESTABLISHMENT_REQUEST)-_-_-_-_]\n");
        else if (packet_type == 2)
            printf("\nCORE:[-_-_-_-_(EMF PACKET of TYPE ASSOCIATION_ESTABLISHMENT_RESPONSE)-_-_-_-_]\n");
        else if (packet_type == 3)
            printf("\nCORE:[-_-_-_-_(EMF PACKET of TYPE ASSOCIATION_TERMINATION_REQUEST)-_-_-_-_]\n");
        else if (packet_type == 4)
            printf("\nCORE:[-_-_-_-_(EMF PACKET of TYPE ASSOCIATION_TERMINATION_RESPONSE)-_-_-_-_]\n");
        else if (packet_type == 5)
            printf("\nCORE:[-_-_-_-_(EMF PACKET of TYPE FINISH)-_-_-_-_]\n");
        else if (packet_type == 6)
            printf("\nCORE:[-_-_-_-_(EMF PACKET of TYPE FINISH_ACK)-_-_-_-_]\n");
        else if (packet_type == 7)
            printf("\nCORE:[-_-_-_-_(EMF PACKET of TYPE "")-_-_-_-_]\n");
        else if (packet_type == 8)
            printf("\nCORE:[-_-_-_-_(EMF PACKET of HANDOVER)-_-_-_-_]\n");
        else if (packet_type == 9)
            printf("\nCORE:[-_-_-_-_(EMF PACKET of TYPE BANDWIDTHAGGREGATION)-_-_-_-_]\n");
		//else if (packet_type == 10)
  		//  printf("\nCORE:[-_-_-_-_(EMF PACKET of TYPE DATAPACK)-_-_-_-_]\n");
        else if (packet_type == 11)
            printf("\nCORE:[-_-_-_-_(EMF PACKET of TYPE HANDOVER_DATA)-_-_-_-_]\n");
        else if (packet_type == 12)
            printf("\nCORE:[-_-_-_-_(EMF PACKET of TYPE BANDWIDTHAGGREGATION_DATA)-_-_-_-_]\n");
        else if (packet_type == 13)
            printf("\nCORE:[-_-_-_-_(EMF PACKET of TYPE SINGLEJOIN)-_-_-_-_]\n");
        else if (packet_type == 14)
            printf("\nCORE:[-_-_-_-_(EMF PACKET of TYPE "")-_-_-_-_]\n");
        else if (packet_type == 15)
            printf("\nCORE:[-_-_-_-_(EMF PACKET of TYPE SINGLEJOIN_DATA)-_-_-_-_]\n");*/

	
	return index;
}


//-----------------------------------------------------------------------------

void update_con_send_buffer(struct assoc_list *assoc_node, unsigned int instant_throughput, int interfaceIndex)
{
	//if (show_msg)
		//showmsg("\nscheduling_handler: update_con_send_buffer() function called, now inside it...");

	while(instant_throughput > 0 && assoc_node->con_send_head[interfaceIndex] != NULL)
		{
		// if(assoc_node->con_send_head[interfaceIndex] == NULL)
		if(instant_throughput < (assoc_node->con_send_head[interfaceIndex]->hdr_size + assoc_node->con_send_head[interfaceIndex]->data_size - assoc_node->con_send_head[interfaceIndex]->bytes_sent) )
			{
			//instant_throughput = 0;
			assoc_node->con_send_head[interfaceIndex]->bytes_sent += instant_throughput;
			}
		else
			if(instant_throughput > (assoc_node->con_send_head[interfaceIndex]->hdr_size + assoc_node->con_send_head[interfaceIndex]->data_size - assoc_node->con_send_head[interfaceIndex]->bytes_sent) )
				{
				instant_throughput = instant_throughput - (assoc_node->con_send_head[interfaceIndex]->hdr_size + assoc_node->con_send_head[interfaceIndex]->data_size - assoc_node->con_send_head[interfaceIndex]->bytes_sent);
			
				con_head = assoc_node->con_send_head[interfaceIndex];
				con_tail = assoc_node->con_send_tail[interfaceIndex];
				remove_con_send_node(assoc_node->con_send_head[interfaceIndex]);
				assoc_node->con_send_head[interfaceIndex] = con_head;
				assoc_node->con_send_tail[interfaceIndex] = con_tail;
				}
			else
				{
				instant_throughput = 0;
				con_head = assoc_node->con_send_head[interfaceIndex];
				con_tail = assoc_node->con_send_tail[interfaceIndex];
				remove_con_send_node(assoc_node->con_send_head[interfaceIndex]);
				assoc_node->con_send_head[interfaceIndex] = con_head;
				assoc_node->con_send_tail[interfaceIndex] = con_tail;
				}	
		}
}



//-----------------------------------------------------------------------------


void save_and_update_emf_buffer(struct assoc_list *assoc_node, char *data, unsigned int seq_no, unsigned int hdr_size, unsigned int packet_size, int interfaceIndex)
{
	//if (show_msg)
		//showmsg("\nscheduling_handler: save_and_update_emf_buffer() function called, now inside it...");
	
	struct con_slist *con_send_node=0;

	con_send_node = malloc(sizeof(struct con_slist));
	
	if(con_send_node !=NULL)
		{
		con_send_node->hdr_size = hdr_size;
		con_send_node->seq_no = seq_no;//assoc_node->emf_send_head->seq_no;
		con_send_node->data_size = packet_size;
		con_send_node->bytes_sent = 0;
		con_send_node->data = malloc(packet_size);
		if(con_send_node->data != NULL)
			{	 
			memcpy(con_send_node->data, data/*assoc_node->emf_send_head->data*/, packet_size);
			//printf("con_send_node created...\n");
			con_head = assoc_node->con_send_head[interfaceIndex];
			con_tail = assoc_node->con_send_tail[interfaceIndex];
			//printf("inserting node in list...\n");
			append_con_send_node(con_send_node);
			//printf("list updated...\n");
			assoc_node->con_send_head[interfaceIndex] = con_head;
			assoc_node->con_send_tail[interfaceIndex] = con_tail;
			}
		else
			printf("Memory error1\n");
		}
	else
		printf("Memory error2\n");
}



//-----------------------------------------------------------------------------
void initialize_assoc_node(struct assoc_list *assoc_node)
{
	if (show_msg)
		showmsg("\nscheduling_handler: initialize_assoc_node() function called, now inside it...");

	int j;

   assoc_node->emf_send_head = NULL;
   assoc_node->emf_send_tail = NULL;
   assoc_node->emf_recv_head = NULL;
   assoc_node->emf_recv_tail = NULL;
   // assoc_node->ServerIP = NULL;
   
   strncpy(assoc_node->ServerIP, "0", 1);//?
   assoc_node->ProtocolNo=0;
   
   for(j = 0; j < nINTERFACES; j++)
	   {
	   assoc_node->con_send_head[j] = NULL;
	   assoc_node->con_send_tail[j] = NULL;
	   assoc_node->last_data_in_buffer[j] = 0;
	   assoc_node->sched_ctr[j] = 0;
	   assoc_node->cid[j] = 0;
	   assoc_node->dont_send[j] = 0;
	   assoc_node->current_data_in_buffer[j] = 0;
	   assoc_node->RTT[j] = 0;
	   assoc_node->avg_throughput[j] = 0;
	   assoc_node->prev_throughput[j] = 0;
	   assoc_node->min_tp_cnt[j] = 0;	
	   assoc_node->con_recv_node[j].length = 0;
	   assoc_node->con_recv_node[j].byte_received = 0;
	   }
   
   assoc_node->c_ctr = 1;
   assoc_node->scenario = NORMAL;
   assoc_node->terminating = 0;
   
   assoc_node->expected_seq_no   = 1;
   assoc_node->seq_no_to_deliver = 1;

/*   struct emf_list *emf_node;
   emf_node = malloc(sizeof(struct emf_list));
   emf_node->length = 0;
   emf_node->seq_no = 1;
   emf_node->data = malloc(SEND_NODE_SIZE);
   
   emf_head = assoc_node->emf_send_head;
   emf_tail = assoc_node->emf_send_tail;
   append_emf_node(emf_node);
   assoc_node->emf_send_head = emf_head;
   assoc_node->emf_send_tail = emf_tail;*/
   															assoc_node->MyKey=NULL;
															assoc_node->group=NULL;


   assoc_node->last_buf_empty = 0;
   
   	assoc_node->RNonce=0;
	assoc_node->SNonce=0;
	
}

char hex_to_ascii(char first, char second)
{
	char hex[5], *stop;
  	hex[0] = '0';
  	hex[1] = 'x';
  	hex[2] = first;
  	hex[3] = second;
  	hex[4] = 0;
  	return strtol(hex, &stop, 16);
}



//-----------------------------------------------------------------------------
//End of file...


/*if(ReceivedDataPacket.EMFSequenceNo > assoc_node->con_recv_node[conIndex].seq_no && 
ReceivedDataPacket.EMFSequenceNo <= (ReceivedDataPacket.EMFSequenceNo + assoc_node->con_recv_node[conIndex].byte_received))*/





unsigned int bin2hex (unsigned char *ibuf, unsigned char *obuf, unsigned int ilen)

{
  unsigned int  i;			// loop iteration conuter
  unsigned int  j = (ilen * 2) + 1;	// output buffer length
  unsigned char *p;

  p = obuf;		// point to start of output buffer

  for (i = 0; i < ilen; i++) {
    sprintf (p, "%2.2x", (unsigned char) ibuf [i]);
    p += 2;
    j -= 2;
  }
  *p = '\0';
  return (strlen (obuf));
}


unsigned int hex2bin (unsigned char *ibuf, unsigned char *obuf, unsigned int ilen)
{
  unsigned int   i;			// loop iteration variable
  unsigned int   j;			// current character
  unsigned int   by = 0;		// byte value for conversion
  unsigned char  ch;			// current character

  // process the list of characaters
  for (i = 0; i < ilen; i++) {
    ch = toupper(*ibuf++);		// get next uppercase character

    // do the conversion
    if(ch >= '0' && ch <= '9')
      by = (by << 4) + ch - '0';
    else if(ch >= 'A' && ch <= 'F')
      by = (by << 4) + ch - 'A' + 10;
    else {				// error if not hexadecimal
      memcpy (obuf,"ERROR",5);
      return 0;
    }

    // store a byte for each pair of hexadecimal digits
    if (i & 1) {
      j = ((i + 1) / 2) - 1;
      obuf [j] = by & 0xff;
    }
  }

  return (j+1);
}

unsigned int PadData (unsigned char *ibuf, unsigned int ilen, int blksize)
{
  unsigned int   i;			// loop counter
  unsigned char  pad;			// pad character (calculated)
  unsigned char *p;			// pointer to end of data

  // calculate pad character
  pad = (unsigned char) (blksize - (ilen % blksize));

  // append pad to end of string
  p = ibuf + ilen;
  for (i = 0; i < (int) pad; i++) {
    *p = pad;
    ++p;
  }

  return (ilen + pad);
}

unsigned int NoPadLen (unsigned char *ibuf, unsigned int ilen)
{
  unsigned int   i;			// adjusted length
  unsigned char *p;			// pointer to last character

  p = ibuf + (ilen - 1);
  i = ilen - (unsigned int) *p;
  return (i);
}



