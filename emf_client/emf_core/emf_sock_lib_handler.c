//-----------------------------------------------------------------------------
#include "emf.h"
#include "PacketFormats.h"



//-----------------------------------------------------------------------------
//---------possible commands that can be received from the emf library

#define id_socket			1
#define id_connect 	   		2
#define id_bind 			3
#define id_send 			4
#define id_recv 			5
#define id_sendto 	    	6
#define id_recvfrom 		7
#define id_close 			8


#define id_emfserverRes 	1
#define id_serverRes 		0
#define id_errorRes 		-1
#define SO_REUSEPORT 		15

#define show_msg 			0

char TLVLength[2];
char *TLVValue;//allocate memory as that of length

//-----------------------------------------------------------------------------
struct 	sockaddr_in 
		server_addr, 
		emf_server_addr,
		emf_server_addr_con;

socklen_t sin_size;
//-----------------------------------------------------------------------------

void connect_with_org_port();
//void handle_emf_sock_lib_cmd(int sock_lib, struct assoc_list *assoc_node);
struct assoc_list* find_sockfd(int sockfd, int app_pid);

void handle_emf_sock_lib_connect(int sock_lib);

void handle_emf_sock_lib_connect(int sock_lib)
{
	char id;
	unsigned int j;
	ssize_t index;
	int new_sock, receivedBytes, ret, header_size, sent=0;
	char IP[16];
	int current_data, buffer_empty_space, len;
	struct sockaddr_in my_addr, *my_addr2;
	struct timeval tv;
	fd_set read_fdd, read_masterr;
	int fdmaxx, nready;//, len;

	Base_Header ReceivedBaseHeader;
	struct assoc_list *assoc_node1=NULL;
	struct emf_list   *emf_node;
	char *buf;
/*		int s = sizeof(struct assoc_list);															
		assoc_node1 = malloc(s);
		if(assoc_node1==NULL)
	printf("Malloc Failed\n");*/
															
															
															
															
	//---------receive the command in the sockLib_client_TLV format (defiend in the emf.h) from the emf library
	receivedBytes = recv(sock_lib, &sockLib_client_TLV, sizeof(sockLib_client_TLV), 0);
	
	
	if (receivedBytes ==-1)
		printf(" i am going to connect or close \n ");
	else 
		if(receivedBytes == 0)
			{
			close(sock_lib);
			FD_CLR(sock_lib, &read_master);
			sock_lib=0;
			}
		else  // Data is available
			{
			
			switch(sockLib_client_TLV.id)
							{
						case	id_connect:
								
						if (show_msg)
							showmsg("\nemf_sock_lib: handling connect() function called...");

					   	index = 0;
					   	HA_TLV.TLVType = LOCAL_COMPLIANCE_REQ;
					   	HA_TLV.TLVLength = 4;
						TLVUnit.intValue = ntohs(sockLib_client_TLV.serverAddr.sin_port);
						
						for(j=0; j<4; j++)
							HA_TLV.TLVValue[j] = TLVUnit.charValue[j];
						
						if ((sent=send(sock_ha, &HA_TLV, sizeof(HA_TLV), 0)) <= 0)   //---------check local compliance by sending ID and port number to HA
							{
							showerr("Can't send local compliance to host agent...:");
							connect_with_org_port();
							}
						else
							{
						    printf("\nCORE : LOCAL_COMPLIANCE_REQ sent to H.A with port Number(%d)\n", TLVUnit.intValue);
						    
							while(1)
								{
								
								if (recv(sock_ha, TLVLength, 2, 0) <= 0)                               //2. receive Length value from H.A
									{
									showerr("Couldn't received local complicance from host agent...:");
									connect_with_org_port();
									break;
									}
								else
									if(TLVLength[0] == LOCAL_COMPLIANCE_RES)//--------- check the compliance response
										{
										if (show_msg)
											showmsg("\nemf_sock_lib: local compliance received ...");
										
										if((unsigned int) TLVLength[1] > 15)
											TLVLength[1] = (char)15;
										
										TLVValue = malloc((int)TLVLength[1]);
										if( ((int)TLVLength[1]) == 0)
											{
											printf("CORE : Request not EMF complient Locally , making Direct connection \n");
											connect_with_org_port();
											free(TLVValue);
											break;
											}
										else
											if (recv(sock_ha, TLVValue, TLVLength[1], 0) <= 0)  //3. receive IP Addresses / (0)
												{
												printf("CORE : Couldn't received local complicance from host agent,making Direct connection :");
												connect_with_org_port();
												free(TLVValue);
												break;
												}
											else
												{
													index = 0;
													j = 0;   
		                     	
													while ( index < (int) TLVLength[1] )  //---------parse the reponse received from the HA
														{
														IP[j] =  TLVValue[index];
														index++;
														j++;
														if( (TLVValue[index] == '\0') || (TLVValue[index] == ':') )
															break;
														}
													IP[j] =  '\0';
													index++;
													j = 0;
													if((new_sock = socket(AF_INET, SOCK_STREAM, 0))==-1)
														{
														perror("socket");
														}
													emf_server_addr.sin_family = AF_INET;                                     //inet_addr("127.0.0.1");
													emf_server_addr.sin_addr.s_addr = inet_addr(IP);//----------Change   		                        
													memset(emf_server_addr.sin_zero, '\0', sizeof emf_server_addr.sin_zero);

													int opt=1;
													if((setsockopt (new_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt)))==-1)
														perror(" ");
								
													if (bind(new_sock,(struct sockaddr *)&emf_server_addr, sizeof emf_server_addr) == -1)
														{
														perror("Can't bind with the IP address sent by host agent...:\n");
														connect_with_org_port();		                     
														break;
														}
													
													int len;	                        
													len = sizeof(my_addr);
													getsockname(new_sock, (struct sockaddr *)&emf_server_addr_con, &len);
													
													if((setsockopt (new_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt)))==-1)
														perror(" ");
													
													//---------make a connection with the emf server		                        
													emf_server_addr_con.sin_addr = sockLib_client_TLV.serverAddr.sin_addr;//EMF Server Address
													emf_server_addr_con.sin_port = htons(EMF_SERVER_PORT);
													memset(emf_server_addr_con.sin_zero, '\0', sizeof emf_server_addr_con.sin_zero);
		                        
													if (connect(new_sock, (struct sockaddr*)&emf_server_addr_con, sizeof(emf_server_addr_con))==-1)
														{
														printf("EMF_SERVER not found...Making Direct Connection\n");
														perror("connect");
														connect_with_org_port();
														break;
														}
													else    //Connected with EMF Server
													{
														printf("CORE: Connection Established with emfserver\n");
														getpeername(new_sock, (struct sockaddr *)&emf_server_addr_con, &len);
														
														//***********************************************//
														//******  Secure Association Establishment  *****//
														//***********************************************//

															int s = sizeof(struct assoc_list);
															//printf("%d\n", s);
															
															assoc_node1 = malloc(s);
															if(assoc_node1==NULL)
																printf("Malloc Failed\n");
															
															
															
														////this is only used to print the publick ket on the screem/////////
														BIO *out;
														out=BIO_new(BIO_s_file());
														if (out == NULL) exit(0);
														BIO_set_fp(out,stdout,BIO_NOCLOSE);
														////////////////////////////////////////////////////////////////////
														
														BN_CTX *ctx=NULL;
														int nid;
														CRYPTO_dbg_set_options(V_CRYPTO_MDEBUG_ALL);
														CRYPTO_mem_ctrl(CRYPTO_MEM_CHECK_ON);
														if ((ctx=BN_CTX_new()) == NULL) printf("error1\n");

														const char *text="NIST Prime-Curve P-192";//this can be changed, we have taken it as as example
														nid = NID_X9_62_prime192v1;	
													/*nid can have the following values depending on the curve we want to use, the curves used are prime
														NIST PRIME CURVES TESTS 
														NID_X9_62_prime192v1, "NIST Prime-Curve P-192"
														NID_secp224r1, "NIST Prime-Curve P-224"
														NID_X9_62_prime256v1, "NIST Prime-Curve P-256"
														NID_secp384r1, "NIST Prime-Curve P-384"
														NID_secp521r1, "NIST Prime-Curve P-521"
														NIST BINARY CURVES TESTS 
														NID_sect163k1, "NIST Binary-Curve K-163"
														NID_sect163r2, "NIST Binary-Curve B-163"
														NID_sect233k1, "NIST Binary-Curve K-233"
														NID_sect233r1, "NIST Binary-Curve B-233"
														NID_sect283k1, "NIST Binary-Curve K-283"
														NID_sect283r1, "NIST Binary-Curve B-283"
														NID_sect409k1, "NIST Binary-Curve K-409"
														NID_sect409r1, "NIST Binary-Curve B-409"
														NID_sect571k1, "NIST Binary-Curve K-571"
														NID_sect571r1, "NIST Binary-Curve B-571"*/


														EC_KEY *MyKey=NULL; 
														BIGNUM *MyX=NULL, *MyY=NULL;//,*HisX=NULL, *HisY=NULL,*bin=NULL;
														BIGNUM *bin=NULL;
	
														if ((MyX=BN_new()) == NULL)  printf("error1\n");   
														if ((MyY=BN_new()) == NULL)  printf("error1\n");
														if ((bin=BN_new()) == NULL)  printf("error1\n");
	
														const EC_POINT *MyPublic;
														const EC_GROUP *group;
	
														MyKey = EC_KEY_new_by_curve_name(nid);   
														if (MyKey == NULL) printf("error1\n");
														group = EC_KEY_get0_group(MyKey); 
	
														if (!EC_KEY_generate_key(MyKey))  printf("error1\n");
								if (!EC_POINT_get_affine_coordinates_GFp(group,EC_KEY_get0_public_key(MyKey), MyX, MyY, ctx))  															printf("error1\n");
														MyPublic= EC_KEY_get0_public_key(MyKey);

														//////////only used for printing///////////////////
														BIO_puts(out,"\n\tCurveName,  key generation with :");
														BIO_puts(out,text);
														BIO_puts(out,"\n");
														BIO_puts(out,"\n\t  pri 1=");
														BN_print(out,EC_KEY_get0_private_key(MyKey));
														BIO_puts(out,"\n\t  pub 1=");
														BN_print(out,MyX);
														BIO_puts(out,",");
														BN_print(out,MyY);
														BIO_puts(out,"\n");
														///////////////////////////////////////////////////
														
														//Converting PublicKey(EC_POINT)
														MyPublic=EC_KEY_get0_public_key(MyKey);
											bin=EC_POINT_point2bn(group, MyPublic,EC_KEY_get_conv_form(MyKey), NULL, ctx);

														//Converting Bignum INTO Decimal/////////////
														char *Decresult;
														Decresult = BN_bn2dec(bin);	
														printf("size of public key : %d\n",strlen(Decresult));	
														printf("decimal form %s",Decresult);

Decresult[117] ='\0';

														///////////////////////////////////////////////////
														int index;
														struct AssociationResponse assoc_res;
										   
														//---------send request for association establishment request to the emf server
														baseheader_packet.baseheader_packet.Version = EMF_VERSION;
														baseheader_packet.baseheader_packet.PacketType = ASSOCIATION_ESTABLISHMENT_REQUEST;
														baseheader_packet.baseheader_packet.Reserved = reserved;
														baseheader_packet.baseheader_packet.DataLength = 0;

														memcpy(emf_packet, baseheader_packet.str, sizeof(baseheader_packet));
														index = sizeof(baseheader_packet);
		                        
														assoc_req.assoc_req.aID = getAssociationID();   // Association ID, Generate Association ID
														assoc_req.assoc_req.encryptionAlgo=0;
														assoc_req.assoc_req.encryptionAlgo_Mode=0;
														assoc_req.assoc_req.kCert=0;
														assoc_req.assoc_req.ulID = ntohs(sockLib_client_TLV.serverAddr.sin_port);//Upper Layer Identifier (Application Requested PORT)
														memcpy(assoc_req.assoc_req.PubKey,Decresult, 117);
															                        
														memcpy(&emf_packet[index],assoc_req.str,sizeof(struct AssociationRequest));
														index += sizeof(struct AssociationRequest);
printf ("sizeof(struct AssociationRequest) %d\n",sizeof(struct AssociationRequest));
		                        printf("index %d\n",index);
														if ((send(new_sock, emf_packet, index, 0)) <= 0)
															{
															printf("Can't send remote compliance to emf server, making Direct connection \n");
															close(new_sock);
															connect_with_org_port();
															//free(TLVValue);
															break;
															}
														else
															{
															//printf("send association establishment request\n");
															
															FD_SET(new_sock, &read_master); /* add new descriptor to set */
															if (new_sock > fdmax)
																fdmax = new_sock; /* for select */
															
															//------------initialize the newly created association
															/*int s = sizeof(struct assoc_list);
															//printf("%d\n", s);
															
															assoc_node1 = malloc(s);
															if(assoc_node1==NULL)
																printf("Malloc Failed\n");*/
															//printf("I am iniitalizing the structure\n");
															initialize_assoc_node(assoc_node1);
															assoc_node1->emf_send_head = NULL;
															assoc_node1->emf_send_tail = NULL;
															assoc_node1->emf_recv_head = NULL;
															assoc_node1->emf_recv_tail = NULL;
															for(j = 0; j < nINTERFACES; j++)
															{
																assoc_node1->con_send_head[j] = NULL;
																assoc_node1->con_send_tail[j] = NULL;
															}
															
															// setting the time stamp while sending the association establishment request
															assoc_node1->association_eshtablishment_request_timestamp = time(NULL); 
															//printf ("\nUsing ctime: %s\n", ctime(&assoc_node->association_eshtablishment_request_timestamp));
															
															//assoc_node1->aid = ( (assoc_req.assoc_req.aID << 16) | assoc_res.aID );
															
															assoc_node1->aid = (assoc_req.assoc_req.aID);
															
															//printf("aid generated and sent to remote end: %d\n", assoc_node1->aid);
															
															assoc_node1->app_sockfd = sockLib_client_TLV.appSockFD;
															assoc_node1->lib_sockfd = sock_lib;
															assoc_node1->app_pid = sockLib_client_TLV.app_pid;
															FD_SET(sock_lib, &read_master); /* add new descriptor to set */
															
															if (sock_lib > fdmax)
																fdmax = sock_lib; /* for select */
															
															assoc_node1->cid[0] = new_sock;
															assoc_node1->c_ctr = 1;
															assoc_node1->scenario = ASSOCIATION_ESTABLISHMENT_PENDING;  // change this to assoc pending
															assoc_node1->terminating = 0;
															assoc_node1->Cflag=0;
															assoc_node1->expected_seq_no   = 1;
															assoc_node1->seq_no_to_deliver = 1;
															
															//		assoc_node1->dest_addr = (struct sockaddr*) &sockLib_client_TLV.serverAddr;//////////////////////////////////////
															assoc_node1->ProtocolNo = ntohs(sockLib_client_TLV.serverAddr.sin_port);
															strncpy(assoc_node1->ServerIP,inet_ntoa(sockLib_client_TLV.serverAddr.sin_addr),strlen(inet_ntoa(sockLib_client_TLV.serverAddr.sin_addr)));
															assoc_node1->ServerIP[strlen(inet_ntoa(sockLib_client_TLV.serverAddr.sin_addr))]='\0';
															
															struct emf_list *emf_node;
															emf_node = malloc(sizeof(struct emf_list));
															emf_node->length = 0;
															emf_node->seq_no = 1;
															emf_node->data = malloc(SEND_NODE_SIZE);
															
															emf_head = assoc_node1->emf_send_head;
															emf_tail = assoc_node1->emf_send_tail;
															append_emf_node(emf_node);
															assoc_node1->emf_send_head = emf_head;
															assoc_node1->emf_send_tail = emf_tail;
															assoc_node1->MyKey=MyKey;
															assoc_node1->group=group;

															append_assoc_node(assoc_node1);
															
															} // end of else for send() remote compliance
														
														
														
														} // else of connect for EMF_SERVER
													} // else of local compliance receive
										//free(TLVValue);
										break;
										
										} // end of if() for check response compliance
								else
									{
									handle_ha_messages(sock_ha, TLVLength);
									}
								
								}   // end of while inside local compliance
							
							} // end of else for local compliance 
								
			break;
			
							case id_close: 
								printf("EMF: I am in id close\n");
								
								if (show_msg)
								showmsg("\nemf_sock_lib: handling close() function called...");
																		
								int response=0;
					
								struct assoc_list *assoc_node;
					
								if(assoc_node = find_sockfd(sockLib_client_TLV.appSockFD, sockLib_client_TLV.app_pid))
								{		
									
									assoc_node->terminating = 1;
									 printf("I am in emf_close2\n");
									response = 1;
								
									if ((sent=send(sock_lib, &response, sizeof(int), 0)) <= 0)           //. send response to emf_socket_library
										perror("send to sock_library:");
									}
									else
								{
									printf("assoc not found...\n");
									response = -1;
									if ((sent=send(sock_lib, &response, sizeof(response), 0)) <= 0)           //. send response to emf_socket_library
										perror("send to sock_library:");
								}
								
								break;
												
							}
													
													
													
			} // end of else when data is available
			

} // end of function
//-----------------------------------------------------------------------------









int handle_emf_sock_lib_send(int sock_lib, struct assoc_list *assoc_node)
{
	
	//printf("sending function\n");
	int index = 0, receivedBytes, packetSize;
	char *buf;
		
		
	packetSize = SEND_NODE_SIZE - assoc_node->emf_send_head->length;
	//printf("packetSize %d\n",packetSize);

if(packetSize > 0)
{		
	buf = malloc(packetSize);
	
	if(buf==NULL)
	{
	//perror("malloc");
	//printf("memory NULL\n");
	}
		
	receivedBytes = recv(assoc_node->lib_sockfd, buf, packetSize, 0);
		//printf("lib sock fd : %d , return bytes %d\n",assoc_node->lib_sockfd, receivedBytes);
		

		
	if (receivedBytes < 0)
	{
		free(buf);
		showerr("Can't receive data...from local server:");
		return 0;	//Ambreen Sheikh 4th June
	}
	else
    {		
		if(receivedBytes == 0 && assoc_node->emf_send_head->length == 0)
		{
		//printf("Packet sixe %d : assoc_node->emf_send_head->length %d\n",packetSize,assoc_node->emf_send_head->length);
		//printf("I have received 0 bytes\n");
		//assoc_node->lib_sockfd=0;
		free(buf);				
		return 0;
		}
		else
		{
		packetSize = receivedBytes;
		//printf("now packet size is equaql to received bytes\n");
		}
	}
//		printf("data received from local server: %s, size: %d\n", buf, packetSize);
		
		memcpy(&assoc_node->emf_send_head->data[assoc_node->emf_send_head->length], buf, packetSize);
		//printf("previous assoc_node->emf_send_head->length %d\n",assoc_node->emf_send_head->length);
		assoc_node->emf_send_head->length += packetSize;
		//printf("new assoc_node->emf_send_head->length %d\n",assoc_node->emf_send_head->length);
		free(buf);
		return 1;
		}
		
		return 1;
}
//-----------------------------------------------------------------------------



//------ this function is called whens ome kind of error occurs in the emf and a direct connection is created
void connect_with_org_port()
{
	int buf;
	buf = id_errorRes;
	
	if (show_msg)
		showmsg("\nemf_sock_lib: connect() with original port called...");
	
	if ((send(sock_lib, &buf, sizeof(buf), 0))==-1)           //5. send response to emf_socket_library
		{ 
		perror("send to sock_library:");
		exit(0);
		}
   
	close(sock_lib);
	//exit(1);
}

//-----------------------------------------------------------------------------

//find the socket descriptor from the association list
struct assoc_list* find_sockfd(int sockfd, int app_pid)
{
	struct assoc_list *assoc_node2;

	for(assoc_node2 = assoc_head; assoc_node2 != NULL; assoc_node2 = assoc_node2->next)
		{
		//printf("searching for connection in EMF list...\n");
		if(sockfd == assoc_node2->app_sockfd && app_pid == assoc_node2->app_pid)
			{
			//printf("connection under EMF confirmed...\n");
			return assoc_node2;
			}
		}
	return NULL;
}
//-----------------------------------------------------------------------------

