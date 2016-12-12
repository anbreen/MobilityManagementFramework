//-----------------------------------------------------------------------------
#include "emf.h"
#include "PacketFormats.h"
//----------------------------------------------------------------------------





static const int KDF1_SHA1_len = 20;

//function that generates the session key
static void *KDF1_SHA11(const void *in, size_t inlen,   void *out,size_t *outlen)
{

#ifndef OPENSSL_NO_SHA
    if (*outlen < SHA_DIGEST_LENGTH)
        return NULL;
    else
    {
    *outlen = SHA_DIGEST_LENGTH;
    return SHA1(in, inlen, out);
    }
#else
    return NULL;
#endif
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char hex_to_ascii1(char first, char second)
{
    char hex[5], *stop;
      hex[0] = '0';
      hex[1] = 'x';
      hex[2] = first;
      hex[3] = second;
      hex[4] = 0;
      return strtol(hex, &stop, 16);
}


char TLVLength[2];
char *TLVValue;//allocate memory as that of length
#define SO_REUSEPORT 15
#define LOCAL_APP_SERVER_PORT	9999

#define show_msg 0

//----- this is the function that sends the data from teh emf core to the application, it places the data in the buffer and whenever the application calls recv, the data from that buffer is retrieved. this function is the reason why the id_recv in the emf_sock_lib_handler is commented.

void send_to_local_app(struct assoc_list *assoc_node)
{
	if (show_msg)
		showmsg("\nHandleSJBA: send_to_local_app() function called, now inside it...");
	
	char *buf = NULL;
	int index = 0;
	struct emf_list *emf_node;
	//emf_node = malloc(sizeof(struct emf_list));			
	/*if(emf_node == NULL)
	{
	printf("out of memory\n");
	}*/

	for(emf_node = assoc_node->emf_recv_head; emf_node != NULL; )
		{ 
		//---- compares the seq number, only if the current sequence number = the seq number to deliver , the data will be send to the application
		if(emf_node->seq_no == assoc_node->seq_no_to_deliver)
			{
			buf = malloc(emf_node->length+1);
		
			if(buf == NULL)
				{
				printf("realloc failed\n");
				return;
				}
		
			memcpy(buf, emf_node->data, emf_node->length);
			buf[emf_node->length]='\0';		
			index = emf_node->length;
			assoc_node->seq_no_to_deliver += emf_node->length;
			//remove this node
			if(index != 0)
				{
				if (send(assoc_node->lib_sockfd, buf, index,  0) <= 0)           //. send data to local server
					{
					perror("socket");
					if(errno == ECONNRESET)
					   break;
					}
				else
					{
					// nothing in it yet
					}
				}
						
			if(emf_node->next == NULL)
				{
				emf_head = assoc_node->emf_recv_head;
				emf_tail = assoc_node->emf_recv_tail;			
				remove_emf_node(emf_node);
				assoc_node->emf_recv_head = emf_head;
				assoc_node->emf_recv_tail = emf_tail;
				//pintf( "local server 3.2.3\n");
				//free(buf);
				break;
				}
			else
				{
				//	printf( "local server 3.3\n");
            	emf_node = emf_node->next;
            	emf_head = assoc_node->emf_recv_head;
            	emf_tail = assoc_node->emf_recv_tail;
            	//  printf( "local server 3.3.1\n");
            	remove_emf_node(emf_node->prev);
            	// printf( "local server 3.3.2\n");
            	assoc_node->emf_recv_head = emf_head;
            	assoc_node->emf_recv_tail = emf_tail;
            	//printf( "local server 3.3.3\n");
				}
			//free(buf);
			}
		else
			break;
		
		} // end of for loop
	//	free(buf);
	//free(emf_node);   //6th Aug
}





// --- this function place the data in the emf list, right now it is only used in rare scenarios.

void PlaceinEMFList(struct assoc_list *assocSearch_node,int EMFseqno, int flag)
{
	if (show_msg)
		showmsg("\nHandleSJBA: PlaceinEMFList() function called, now inside it...");
	
	int i;
	int AddSeqNo;
	
	if(assocSearch_node->con_recv_node[0].byte_received == 0 && assocSearch_node->con_recv_node[0].length == 0)
		{
		assocSearch_node->con_recv_node[0].byte_received = 0;
		assocSearch_node->con_recv_node[0].length = 0;
		}
	else
		{
		//assocSearch_node->con_recv_node[0].length=assocSearch_node->con_recv_node[0].byte_received ;
	 	struct emf_list *new_node;
		new_node = (struct emf_list *) malloc(sizeof(struct emf_list));

		if (new_node == NULL)
			{
			////printf("\nMemory allocation Failure!\n");
			exit(0);
			}
		else
			{
			new_node->prev=NULL;
			new_node->seq_no=assocSearch_node->con_recv_node[0].seq_no; //how to fix it
						
			//this is the case of handover+data
			if(flag==1)
				{
				new_node->length=((assocSearch_node->con_recv_node[0].seq_no+assocSearch_node->con_recv_node[0].byte_received)-EMFseqno);
				memcpy(new_node->data,assocSearch_node->con_recv_node[0].data,new_node->length);
				}
			if(flag==0)
				{
				new_node->length=assocSearch_node->con_recv_node[0].byte_received;
				memcpy(new_node->data,assocSearch_node->con_recv_node[0].data,assocSearch_node->con_recv_node[0].byte_received);
				}

			/*for(i = new_node->length; i < assocSearch_node->con_recv_node[0].length; i++)
				{
				assocSearch_node->con_recv_node[0].data[i-new_node->length] = assocSearch_node->con_recv_node[0].data[i];
				}
	
				assocSearch_node->con_recv_node[0].length -= new_node->length;
			assocSearch_node->con_recv_node[0].seq_no += new_node->length;
			*/	
			new_node->next=NULL;
			//insert in the list

			if(assocSearch_node->expected_seq_no == new_node->seq_no)
				{	
				emf_head = assocSearch_node->emf_recv_head;
				emf_tail = assocSearch_node->emf_recv_tail;
				AddSeqNo = insert_emf_node(new_node);
				assocSearch_node->emf_recv_head = emf_head;
				assocSearch_node->emf_recv_tail = emf_tail;
				assocSearch_node->expected_seq_no += AddSeqNo;			    
	
				for(i = new_node->length; i < assocSearch_node->con_recv_node[0].length; i++)
					{
					assocSearch_node->con_recv_node[0].data[i-new_node->length] = assocSearch_node->con_recv_node[0].data[i];
					}
	
				assocSearch_node->con_recv_node[0].length -= new_node->length;
				assocSearch_node->con_recv_node[0].seq_no += new_node->length;
	
				}
			else
				if(assocSearch_node->expected_seq_no < new_node->seq_no)
					{
					emf_head = assocSearch_node->emf_recv_head;
					emf_tail = assocSearch_node->emf_recv_tail;
					insert_emf_node(new_node);
					assocSearch_node->emf_recv_head = emf_head;
					assocSearch_node->emf_recv_tail = emf_tail;
				
					for(i = new_node->length; i < assocSearch_node->con_recv_node[0].length; i++)
						{
						assocSearch_node->con_recv_node[0].data[i-new_node->length] = assocSearch_node->con_recv_node[0].data[i];
						}
					assocSearch_node->con_recv_node[0].length -= new_node->length;
					assocSearch_node->con_recv_node[0].seq_no += new_node->length;	
					}
			send_to_local_app(assocSearch_node);
			} //end of if-else, is new node exists
		}
}







int searchAssocList(int RemoteSock,unsigned int Encrypted, int iFlag, int LengthofDataPacket)
{
	if (show_msg)
		showmsg("\nHandleSJBA: searchAssocList() function called, now inside it...");

	int AddSeqNo;

	struct assoc_list *assocSearch_node = assoc_head;

	if (assoc_head == NULL)
		return 0;
	else
		{
		while(assocSearch_node!=NULL)
			{
			if (assocSearch_node->aid==Encrypted)
				{
				if(iFlag==0)
					{
					//--------- 0 flag is the case of BA
					//assocSearch_node->c_ctr++;															
					//------ADD the new descriptor in the connection array, set the scenario as BA
					assocSearch_node->cid[assocSearch_node->c_ctr]=	RemoteSock;
					assocSearch_node-> scenario=BANDWIDTH_AGGREGATION ;
					printf("CORE : Received Bandwidth Aggregation Packet from the other side\n");				
					
					//----- SEND ADD CONNECTION message to the HA				
					int j;
					HA_TLV.TLVType = ADD_CONNECTION;
					TLVUnit.intValue = assocSearch_node->aid; 
					
					for(j=0; j<4; j++)
						HA_TLV.TLVValue[j] = TLVUnit.charValue[j];				
					
					TLVUnit.intValue = RemoteSock; 											
					
					for(; j<8; j++)
						HA_TLV.TLVValue[j] = TLVUnit.charValue[j-4];
					
					int len;
					char ToIP[16];	
					struct sockaddr_in  my_addr;
					len = sizeof(struct sockaddr);									 
					
					if(getsockname(RemoteSock, (struct sockaddr*)&my_addr, &len) == 0)
						{
						//memcpy(&HA_TLV.TLVValue[j], inet_ntoa(my_addr.sin_addr), strlen(inet_ntoa(my_addr.sin_addr)) );
						HA_TLV.TLVLength = 8 + strlen(inet_ntoa(my_addr.sin_addr) );	
						strncpy(ToIP,inet_ntoa(my_addr.sin_addr),strlen(inet_ntoa(my_addr.sin_addr)));
						ToIP[strlen(inet_ntoa(my_addr.sin_addr))]='\0';
						memcpy(&HA_TLV.TLVValue[j], ToIP, strlen(ToIP) );				            															
						
						HA_TLV.TLVValue[j] = 0;
						
						if (send(sock_ha, &HA_TLV, sizeof(HA_TLV), 0)==-1)	//1. send association information to HA
							{
							showerr("Can't send new connection information to host agent...:");
							}
						printf("CORE : ADD_CONNECTION Message Sent to Host Agent with AID(%d), CID(%d), IP(%s)\n", assocSearch_node->aid, RemoteSock, ToIP); //Omer			
						}                  
					}
				
				// 1 flag is the case of Hand over
				if(iFlag==1)
					{
					//------ADD the new descriptor in the connection array, set the scenario as hand over
									
					PlaceinEMFList(assocSearch_node,0,0);
					//initiate_handover2(assocSearch_node, "192.168.1.35", NORMAL);			
					//assocSearch_node->c_ctr++;////////////////////////////////////////////////////////////////
					assocSearch_node->cid[assocSearch_node->c_ctr]=	RemoteSock;
					assocSearch_node-> scenario=HAND_OVER;
					printf("CORE : %d : %d Received Handover Packet from the other side\n",assocSearch_node->c_ctr,assocSearch_node->cid[assocSearch_node->c_ctr]);		
					//printf("CORE : Received Handover Packet from the other side\n");
					//assocSearch_node-> scenario=HO_LINK_SHIFT_COMMIT;
					//assocSearch_node->cid[assocSearch_node->c_ctr]=	RemoteSock;
					}
				
				// --- not implemented
				if(iFlag==2)
					{			
					Data_Packet ReceivedDataPacket;			
					char *ReceivedData;
					int receivedbytes=0;
					
					ReceivedData = malloc(LengthofDataPacket);
					assocSearch_node->con_recv_node[assocSearch_node->c_ctr].data = malloc(LengthofDataPacket);
					if ((recv(RemoteSock, &ReceivedDataPacket,8, MSG_WAITALL)) <= 0) 
						{
						perror("recv");
						return -1;
						}					

					if ((receivedbytes=(RemoteSock, ReceivedData,LengthofDataPacket, 0)	) <= 0) 
						{
						perror("recv");
						return -1;
						}
					
					assocSearch_node-> scenario=HAND_OVER;
					assocSearch_node->cid[assocSearch_node->c_ctr]=	RemoteSock;
					
					//assocSearch_node->c_ctr++;												
					//assocSearch_node->cid[assocSearch_node->c_ctr]=	RemoteSock;

					if(LengthofDataPacket == receivedbytes)
						{
						struct emf_list *new_node;
						new_node = (struct emf_list *) malloc(sizeof(struct emf_list));
						
						if (new_node == NULL)
							{
							////printf("\nMemory allocation Failure!\n");
							exit(0);
							}
						else
							{
							new_node->prev=NULL;
							new_node->seq_no=ReceivedDataPacket.EMFSequenceNo;
							new_node->length=receivedbytes;
							memcpy(new_node->data,ReceivedData,receivedbytes);
							new_node->next=NULL;

							//assoc_node->expected_seq_no += receivedbytes;			    
							//insert_emf_node(assoc_node->emf_recv_head, assoc_node->emf_recv_tail, new_node);

							if(assocSearch_node->expected_seq_no == new_node->seq_no)
								{	
								emf_head = assocSearch_node->emf_recv_head;
								emf_tail = assocSearch_node->emf_recv_tail;
								AddSeqNo = insert_emf_node(new_node);
								assocSearch_node->emf_recv_head = emf_head;
								assocSearch_node->emf_recv_tail = emf_tail;
								assocSearch_node->expected_seq_no += AddSeqNo;			    
								}
							else
								if(assocSearch_node->expected_seq_no < new_node->seq_no)
									{
									emf_head = assocSearch_node->emf_recv_head;
									emf_tail = assocSearch_node->emf_recv_tail;
									insert_emf_node(new_node);
									assocSearch_node->emf_recv_head = emf_head;
									assocSearch_node->emf_recv_tail = emf_tail;
									}
								//insert in the list
								send_to_local_app(assocSearch_node);
							}
						assocSearch_node->con_recv_node[assocSearch_node->c_ctr].byte_received = 0;
						assocSearch_node->con_recv_node[assocSearch_node->c_ctr].length = 0;
						free(assocSearch_node->con_recv_node[assocSearch_node->c_ctr].data);
						}
					else
						{
						assocSearch_node->con_recv_node[assocSearch_node->c_ctr].seq_no=ReceivedDataPacket.EMFSequenceNo;
						assocSearch_node->con_recv_node[assocSearch_node->c_ctr].length=LengthofDataPacket;
						assocSearch_node->con_recv_node[assocSearch_node->c_ctr].byte_received=receivedbytes;
						memcpy(assocSearch_node->con_recv_node[assocSearch_node->c_ctr].data,ReceivedData,receivedbytes);
						}
					
					PlaceinEMFList(assocSearch_node,ReceivedDataPacket.EMFSequenceNo,1);
					
					} // end of if, when flag=2
				
				assocSearch_node->c_ctr++;
				printf("I am incrementing %d\n",assocSearch_node->c_ctr);
				//it will always be 1 :
				
				return 1;
				break;
				}// end if if, aid == encrupted
			
			assocSearch_node=assocSearch_node->next;
			}
		}
}







//--- this function is called whenerv there is a new connection from the other side
int HandleNewConnection(int RemoteSock)
{
	if (show_msg)
		showmsg("\nHandleSJBA: HandleNewConnection() function called, now inside it...");
		
	int index, j, len;
	struct sockaddr_in my_addr, *my_addr2;
	Base_Header ReceivedBaseHeader;
		char IP[16];
		
	//--- receive the 4 bytes of the base header and then on the base on the message type perform the activity
	if ((recv(RemoteSock, &ReceivedBaseHeader,4,MSG_WAITALL)) == -1)
		{
		perror("recv");
		return 0;
	    }
	
	switch(ReceivedBaseHeader.PacketType)
		{
		//-- if the established new connection is the case of BA
			case ASSOCIATION_ESTABLISHMENT_REQUEST:
			{
			//printf("received assoc_est_packet\n");
			struct AssociationRequest assoc_req;
			//if (( len = recv(RemoteSock, &assoc_req,sizeof(struct AssociationRequest), 0)) == -1) 
			if (( len = recv(RemoteSock, &assoc_req,123, MSG_WAITALL)) == -1) 
				{
			   	perror("recv");
			   	// send back error code...
				close(RemoteSock);					
				return 0;
				}
			
			printf("association request packet %d,%d,%d,%d,%d\n",assoc_req.aID, assoc_req.encryptionAlgo, assoc_req.encryptionAlgo_Mode, assoc_req.kCert,assoc_req.ulID);
			assoc_req.PubKey[117]='\0';
			printf("\nstrlen () %d : public key of other side %s\n",strlen(assoc_req.PubKey),assoc_req.PubKey);

			//assoc_req.assoc_req.ulID = ntohs(sockLib_client_TLV.serverAddr.sin_port);//Upper Layer
			//printf("received assoc_est_request\n");
            //connect to local server
            /************************************************************************************************************************/
			index = 0;
			HA_TLV.TLVType = LOCAL_COMPLIANCE_REQ;
			HA_TLV.TLVLength = 4;
			
			//Port Number (Application)
			//TLVUnit.intValue = ntohs(assoc_req.ulID);
			TLVUnit.intValue = assoc_req.ulID;  
			
			//TLVUnit.intValue = ntohs(sockLib_client_TLV.serverAddr.sin_port); 
			for(j=0; j<4; j++)
				HA_TLV.TLVValue[j] = TLVUnit.charValue[j];
			int sent;
			if ((sent=send(sock_ha, &HA_TLV, sizeof(HA_TLV), 0)) <= 0)		         //1. check local compliance by sending ID and port number
				{
				showerr("Can't send local compliance to host agent...:");
				}
			else
				{
				int localcomp=0;
				printf("CORE : LOCAL_COMPLIANCE_REQ Message Sent to Host Agent with PORT(%d)\n", assoc_req.ulID); //Omer
				while(1)
					{
					if (recv(sock_ha, TLVLength, 2, 0) <= 0)                               //2. receive Length value from H.A
						{
						showerr("Couldn't received local complicance from host agent...:");
						break;
						}
					else
						{
						if(TLVLength[0] == LOCAL_COMPLIANCE_RES)
							{
							if((unsigned int) TLVLength[1] > 15)
								TLVLength[1] = (char)15;
							
							TLVValue = malloc((int)TLVLength[1]);
							if( ((int)TLVLength[1]) == 0)
								{
								showmsg("Request not EMF complient...\n");
								free(TLVValue);
								localcomp=1;
								break;
								}
							else
								{
								if (recv(sock_ha, TLVValue, TLVLength[1], 0) <= 0)               //3. receive IP Addresses / (0)
									{
									showerr("Couldn't received local complicance from host agent...2:");
									free(TLVValue);
									break;
									}
								else
									{
									index = 0;
									j = 0;   
									while ( index < (int) TLVLength[1] )
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
									
									//printf("IP : %s\n",IP);
		    		                    	
									if( (sock_lib = socket(PF_INET, SOCK_STREAM, 0)) == -1)
										{			
										showerr("Can't create socket..., No connection with local server");
										}
									
									/*else
									showmsg("sock_local: Success...");*/
									
									
									// added code for binding to a certain port 13-nov zeeshan
									my_addr.sin_family = AF_INET;          		// host byte order
									my_addr.sin_port = htons(LOCAL_APP_SERVER_PORT);      // short, network byte order
									my_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
									memset(my_addr.sin_zero, '\0', sizeof my_addr.sin_zero);
									
									int opt=1;
									setsockopt (sock_ha, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof (opt));
									setsockopt (sock_ha, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt));   
									
									if( bind(sock_lib, (struct sockaddr *)&my_addr, sizeof my_addr) == -1)
										{
										showerr("sock_local bind");
										}
									else
										showmsg("sock_local bind: Success...");
									// added code end here
									
									
									my_addr.sin_family = AF_INET;                                     //inet_addr("127.0.0.1");
									my_addr.sin_addr.s_addr = inet_addr("127.0.0.1");//----------Change
									my_addr.sin_port = htons(assoc_req.ulID);
									memset(my_addr.sin_zero, '\0', sizeof my_addr.sin_zero);
									
									if (connect(sock_lib, (struct sockaddr*)&my_addr, sizeof(my_addr))==-1)
										{
										perror("Cconnect");
										close(RemoteSock);
										FD_CLR(RemoteSock, &read_master);
										//	FD_CLR(RemoteSock);
										return 0;
										}
									
									//printf("Local socket %d\n",sock_local);
									//showmsg("connected to local server...\n");
									FD_SET(sock_lib, &read_master); /* add new descriptor to set */
									
									if (sock_lib > fdmax)
										fdmax = sock_lib; /* for select */
									

									////////////////////////////////SECURE ASSOCIATION MODULE//////////////

								    ////this is only used to print the publick ket on the screem/////////

										struct assoc_list *assoc_node;
										assoc_node = malloc(sizeof(struct assoc_list));									
								
										


				       /*                                 BIO *out;
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


				         /*                               EC_KEY *MyKey=NULL;
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
				                                        if (!EC_POINT_get_affine_coordinates_GFp(group,EC_KEY_get0_public_key(MyKey), MyX, MyY, ctx))                                                              printf("error1\n");
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

		Decresult[117] ='\0';*/

		///////////////////////////////////////////////////////////////////

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


                                                        //EC_KEY *MyKey=NULL;
                                                        BIGNUM *MyX=NULL, *MyY=NULL;//,*HisX=NULL, *HisY=NULL,*bin=NULL;
                                                        BIGNUM *bin=NULL;
   
                                                        if ((MyX=BN_new()) == NULL)  printf("error1\n");  
                                                        if ((MyY=BN_new()) == NULL)  printf("error1\n");
                                                        if ((bin=BN_new()) == NULL)  printf("error1\n");
   
                                                        const EC_POINT *MyPublic;
                                                        //const EC_GROUP *group;
   
                                                        assoc_node->MyKey = EC_KEY_new_by_curve_name(nid);  
                                                        if (assoc_node->MyKey == NULL) printf("error1\n");
                                                        assoc_node->group = EC_KEY_get0_group(assoc_node->MyKey);
   
                                                        if (!EC_KEY_generate_key(assoc_node->MyKey))  printf("error1\n");
                                if (!EC_POINT_get_affine_coordinates_GFp(assoc_node->group,EC_KEY_get0_public_key(assoc_node->MyKey), MyX, MyY, ctx))                                                              printf("error1\n");
                                                        MyPublic= EC_KEY_get0_public_key(assoc_node->MyKey);

                                                        //////////only used for printing///////////////////
                                                        BIO_puts(out,"\n\tCurveName,  key generation with :");
                                                        BIO_puts(out,text);
                                                        BIO_puts(out,"\n");
                                                        BIO_puts(out,"\n\t  pri 1=");
                                                        BN_print(out,EC_KEY_get0_private_key(assoc_node->MyKey));
                                                        BIO_puts(out,"\n\t  pub 1=");
                                                        BN_print(out,MyX);
                                                        BIO_puts(out,",");
                                                        BN_print(out,MyY);
                                                        BIO_puts(out,"\n");
                                                        ///////////////////////////////////////////////////
                                                       
                                                        //Converting PublicKey(EC_POINT)
                                                        MyPublic=EC_KEY_get0_public_key(assoc_node->MyKey);
                                            bin=EC_POINT_point2bn(assoc_node->group, MyPublic,EC_KEY_get_conv_form(assoc_node->MyKey), NULL, ctx);

                                                        //Converting Bignum INTO Decimal/////////////
                                                        char *Decresult;
                                                        Decresult = BN_bn2dec(bin);   
                                                        printf("size of public key : %d\n",strlen(Decresult));   
                                                        printf("decimal form %s",Decresult);

Decresult[117] ='\0';







//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
									baseheader_packet.baseheader_packet.Version = EMF_VERSION;
									baseheader_packet.baseheader_packet.PacketType = ASSOCIATION_ESTABLISHMENT_RESPONSE;
									baseheader_packet.baseheader_packet.Reserved = reserved;
									baseheader_packet.baseheader_packet.DataLength = 0;
									
									memcpy(emf_packet, baseheader_packet.str, sizeof(baseheader_packet));
									index = sizeof(baseheader_packet);
									assoc_res.assoc_res.aID = getAssociationID();                  // Association ID, Generate Association ID
									assoc_res.assoc_res.encryptionAlgo=0;
									assoc_res.assoc_res.encryptionAlgo_Mode=0;
									assoc_res.assoc_res.kCert=0;
									memcpy(assoc_res.assoc_res.PubKey,Decresult, 117);
					


									printf("sizeof(struct AssociationResponse) : %d\n",sizeof(struct AssociationResponse));										
									//printf("assoc_res.assoc_res.aID : %d\n",assoc_res.assoc_res.aID);
									memcpy(&emf_packet[index],assoc_res.str,sizeof(struct AssociationResponse));
									index += sizeof(struct AssociationResponse);

printf("index %d \n\n",index);									
									if (send(RemoteSock, emf_packet, index, 0) <= 0)
										{
										perror("send");
										close(RemoteSock);
										return 0;
										}
									else
										{


										FD_SET(RemoteSock, &read_master); /* add new descriptor to set */			
										if (RemoteSock > fdmax)
											fdmax = RemoteSock; /* for select */
										//assoc_node = malloc(sizeof(struct assoc_list));
										
										initialize_assoc_node(assoc_node);
										
////////////////////////////////////////////////////////////////////////////////////////////////////

BIO *out;
                                        out=BIO_new(BIO_s_file());
                                        if (out == NULL) exit(0);
                                        BIO_set_fp(out,stdout,BIO_NOCLOSE);
                               
                               
                                    BIGNUM *bin=NULL;
                                    const EC_POINT *HisPublic;
                                        BN_CTX *ctx=NULL;

                                    CRYPTO_dbg_set_options(V_CRYPTO_MDEBUG_ALL);
                                    CRYPTO_mem_ctrl(CRYPTO_MEM_CHECK_ON);
                                    if ((ctx=BN_CTX_new()) == NULL) printf("error2\n");

                                    if ((bin=BN_new()) == NULL) printf("error\n");
   
       
                                BN_dec2bn(&bin, assoc_req.PubKey);
                               
                                HisPublic=EC_POINT_bn2point(assoc_node->group,bin,NULL,ctx);
                                if(HisPublic==NULL)
                                {
                                    printf("error in server");
                                }

                                //Computing Session Key
                                unsigned char *abuf=NULL;
                                int ii,alen,aout,ret=0;

                                printf("\n\tSession Key: ");
                                char mySessionKey[20];
                                abuf=(unsigned char *)OPENSSL_malloc(100);
                                aout=ECDH_compute_key(abuf,100,HisPublic,assoc_node->MyKey,KDF1_SHA11);
                                for (ii=0; ii<aout; ii++)
                                {
                                    sprintf(mySessionKey,"%02X",abuf[ii]);
                                    BIO_puts(out,mySessionKey);
                                }
                                BIO_puts(out,"\n");
   
   
////////////////////////////////////////////////////////////////////////////////////////////////






										assoc_node->emf_send_head = NULL;
										assoc_node->emf_send_tail = NULL;
										assoc_node->emf_recv_head = NULL;
										assoc_node->emf_recv_tail = NULL;
										
										for(j = 0; j < nINTERFACES; j++)
											{
											assoc_node->con_send_head[j] = NULL;
											assoc_node->con_send_tail[j] = NULL;
											}		

										assoc_node->aid = ( (assoc_req.aID << 16) | assoc_res.assoc_res.aID );
										//printf("req: %d, res: %d, aid created: %d\n", assoc_req.aID, assoc_res.assoc_res.aID, assoc_node->aid);
										assoc_node->lib_sockfd = sock_lib;
										
										
										assoc_node->ProtocolNo=assoc_req.ulID;
										assoc_node->cid[0] = RemoteSock;
										assoc_node->c_ctr = 1;
										assoc_node->Cflag=0;
										assoc_node->scenario = NORMAL;
										assoc_node->expected_seq_no   = 1;
										assoc_node->seq_no_to_deliver = 1;
										assoc_node->terminating = 0;
										len = sizeof(struct sockaddr);
										
										if(getpeername(RemoteSock, (struct sockaddr*)&my_addr, &len) == 0)
											{
											my_addr.sin_port = assoc_req.ulID;
											//assoc_node->dest_addr = (struct sockaddr*) &my_addr;
											//printf("successful\n");
											//assoc_node->dest_addr = (struct sockaddr*) &sockLib_client_TLV.serverAddr;
											//my_addr2 = (struct sockaddr_in *) assoc_node->dest_addr;
											//printf("PEER my_addr.sin_addr %s\n",inet_ntoa(my_addr.sin_addr));
											
											strncpy(assoc_node->ServerIP,inet_ntoa(my_addr.sin_addr),strlen(inet_ntoa(my_addr.sin_addr)));
											assoc_node->ServerIP[strlen(inet_ntoa(my_addr.sin_addr))]='\0';
											
											//printf("assoc_node->ClientIP : %s\n",assoc_node->ClientIP);
											//my_addr = (struct sockaddr_in *) assoc_node->dest_addr;
											//printf("PEER my_addr.sin_addr %s\n",inet_ntoa(my_addr->sin_addr));     			                  
											//printf("Get Peer Name : %s\n",inet_ntoa(assoc_node->dest_addr.sin_addr));
											}
										else
											{
											my_addr.sin_port = assoc_req.ulID;
											//printf("error in getpeername\n");
											}
										
										struct emf_list *emf_node;
										emf_node = malloc(sizeof(struct emf_list));
										emf_node->length = 0;
										emf_node->seq_no = 1;
										emf_node->data = malloc(SEND_NODE_SIZE);			      
										emf_head = assoc_node->emf_send_head;
										emf_tail = assoc_node->emf_send_tail;
										append_emf_node(emf_node);
										assoc_node->emf_send_head = emf_head;
										assoc_node->emf_send_tail = emf_tail;				   
										append_assoc_node(assoc_node);			      
										
										
										HA_TLV.TLVType = ADD_ASSOCIATION;
										HA_TLV.TLVLength = 4;
										//printf("assoc_node->aid %d",assoc_node->aid);
										TLVUnit.intValue = assoc_node->aid; 													
										for(j=0; j<4; j++)
											HA_TLV.TLVValue[j] = TLVUnit.charValue[j];
										
										//my_addr2 = (struct sockaddr_in *) assoc_node->dest_addr;
										//printf("my_addr2->sin_port %d\n",my_addr2->sin_port);
										
										TLVUnit.intValue = assoc_node->ProtocolNo;
										
										for(j=4; j<8; j++)
											HA_TLV.TLVValue[j] = TLVUnit.charValue[j-4];
										
										//printf("RemoteSock %d\n",my_addr2->sin_port);
										TLVUnit.intValue = RemoteSock; 
										len = sizeof(struct sockaddr);
										
										for(; j<12; j++)
											HA_TLV.TLVValue[j] = TLVUnit.charValue[j-8];
										
										if(getsockname(RemoteSock, (struct sockaddr*)&my_addr, &len) == 0)
											{
											//printf("inet_ntoa(my_addr.sin_addr) : %s\n",inet_ntoa(my_addr.sin_addr));
											memcpy(&HA_TLV.TLVValue[j], inet_ntoa(my_addr.sin_addr), strlen(inet_ntoa(my_addr.sin_addr)) );
											HA_TLV.TLVValue[j+strlen(inet_ntoa(my_addr.sin_addr))] = '\0';
											}
										
										//HA_TLV.TLVValue[j] = 0;
										//HA_TLV.TLVValue[++j] = '\0';
											
										if (send(sock_ha, &HA_TLV, sizeof(HA_TLV), 0) <= 0)	//1. send association information to HA
											{
											showerr("Can't send local compliance to host agent...:");
											}
										printf("CORE : ADD_ASSOCIATION Message Sent to Host Agent with AID(%d) CID(%d) PORT(%d)\n", assoc_node->aid, RemoteSock, my_addr2->sin_port); //Omer
										}
									}
								}
							}
						}
					//printf("returning from Association_ESTABLISHMENT_REQUEST\n");
					break;
					} // end of while loop
				
				if(localcomp==1)
					{
					baseheader_packet.baseheader_packet.Version = EMF_VERSION;
					baseheader_packet.baseheader_packet.PacketType = ASSOCIATION_ESTABLISHMENT_RESPONSE;
					baseheader_packet.baseheader_packet.Reserved = reserved;
					baseheader_packet.baseheader_packet.DataLength = 0;
					
					memcpy(emf_packet, baseheader_packet.str, sizeof(baseheader_packet));
					index = sizeof(baseheader_packet);
					assoc_res.assoc_res.aID = 0;//getAssociationID();                  // Association ID, Generate Association ID
					//printf("assoc_res.assoc_res.aID : %d\n",assoc_res.assoc_res.aID);
					memcpy(&emf_packet[index],assoc_res.str,sizeof(struct AssociationResponse));
					index += sizeof(struct AssociationResponse);
										    		                    		    		                    	
					if (send(RemoteSock, emf_packet, index, 0) <= 0)
						{
						perror("send");
						close(RemoteSock);
						return 0;
						}
					}
				} //else
			} //case
			
			
			
			
		case BANDWIDTHAGGREGATION :
			{
			Bandwidth_Aggregation BandwidthAggregationPacket;
			if ((recv(RemoteSock, &BandwidthAggregationPacket,sizeof(Bandwidth_Aggregation), 0)) == -1) 
				{
				perror("recv");
				return 0;
				}
			return (searchAssocList(RemoteSock,BandwidthAggregationPacket.EncryptedAIDNonce,0,0));
			break;
			}
			
		//---- this case is not implemented yet
		case SINGLEJOIN :
			{	
			Single_Join SingleJoinPacket;
			
			if ((recv(RemoteSock, &SingleJoinPacket,sizeof(Single_Join), 0)) == -1)
				{
				perror("recv");
				return 0;
				}
			
			return (searchAssocList(RemoteSock,SingleJoinPacket.EncryptedAIDNonce,0,0));
			break;
			}
			
		
		//-- if the established new connection is the case of hand over
		case HANDOVER :
			{
			Hand_over HandOverPacket;
			if ((recv(RemoteSock, &HandOverPacket,4, MSG_WAITALL)) == -1)
				{
			   	perror("recv");
				exit(1);
				}
			//break;	
			//place the remaing data in the final list
			//--- it calls the search assoc list with the flags 1,0
			return(searchAssocList(RemoteSock,HandOverPacket.EncryptedAIDNonce,1,0));
			break;
			}
			
		//--- not implemented
		case HANDOVER_DATA :
			{
			Hand_over HandoverPacket;
			
			if ((recv(RemoteSock, &HandoverPacket,4, MSG_WAITALL)) == -1)
				{
				perror("recv");
				exit(1);
				}
			
			return(searchAssocList(RemoteSock,HandoverPacket.EncryptedAIDNonce,2,ReceivedBaseHeader.DataLength));
			break;
			}
			
			
		//--- not implemented
		case BANDWIDTHAGGREGATION_DATA :
			{
				Bandwidth_Aggregation BandwidthAggregationPacket;
				if ((recv(RemoteSock, &BandwidthAggregationPacket,sizeof(Bandwidth_Aggregation), 0)) == -1) 
			   		{
			   		perror("recv");
					exit(1);
			   		}
				//ReceiveDataPacketNow(i,ReceivedBaseHeader.DataLength,assoc_node);
				//return(searchAssocList(RemoteSock,HandOverPacket.EncryptedAIDNonce,0,0));
				break;
			}
			
			
		//--- not implemented
		case SINGLEJOIN_DATA :
			{
			Single_Join SingleJoinPacket;
			if ((recv(RemoteSock, &SingleJoinPacket,sizeof(Single_Join), 0)) == -1) 
				{
				perror("recv");
				exit(1);
				}
			//ReceiveDataPacketNow(i,ReceivedBaseHeader.DataLength,assoc_node);
			//return(searchAssocList(RemoteSock,HandOverPacket.EncryptedAIDNonce,0,0));
			break;
			}
		}
}
