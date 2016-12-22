//-----------------------------------------------------------------------------
#include "emf.h"
#include "PacketFormats.h"
#include <linux/sockios.h>
# include "openssl/aes.h"

# define BLOCK_LEN	16
# define MAXBUF		65536
# define MAXHEX		(MAXBUF * 2) + 1

#define id_emfserverRes 	1
//-----------------------------------------------------------------------------


static const int KDF1_SHA1_len = 20;

//function that generates the session key
static void *KDF1_SHA1(const void *in, size_t inlen,   void *out,size_t *outlen)
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
/*
char hex_to_ascii(char first, char second)
{
	char hex[5], *stop;
  	hex[0] = '0';
  	hex[1] = 'x';
  	hex[2] = first;
  	hex[3] = second;
  	hex[4] = 0;
  	return strtol(hex, &stop, 16);
}*/

void decryptAidNonce(int *ret_nonce, int *ret_aid,char *bufat,struct assoc_list *assoc_node)
{	
	unsigned char key[40];
        memcpy (key,  assoc_node->sessionKey, strlen (assoc_node->sessionKey));              			  
	unsigned char iv[40];//  = IV;
	memcpy (iv,  assoc_node->sessionKey, strlen (assoc_node->sessionKey));			                
	unsigned char ybuf[MAXBUF];	// hex decrypt output
	unsigned char dbuf[MAXBUF];
	bufat[32]='\0';
	AES_KEY aeskeyDec;
	AES_set_decrypt_key (key, 256, &aeskeyDec);				
	// convert hex string to binary
	int length = hex2bin (bufat, ybuf, (int) strlen(bufat));
	AES_cbc_encrypt (ybuf, dbuf, length, &aeskeyDec, iv, AES_DECRYPT);	
	dbuf [NoPadLen (dbuf, length)] = 0x00;
	printf ("decode: %s (len = %d)\n", dbuf, strlen(dbuf));
	printf ("\n");
	char *token;
	token=strtok(dbuf,":");
	*ret_aid= atoi(token);
	token=strtok(NULL,":");
	*ret_nonce=atoi(token);
	printf("%d,%d\n",&ret_aid,&ret_nonce);
}

void ReceiveDataPacketNow(int i, int LengthofDataPacket,struct assoc_list *assoc_node)
{
	Data_Packet ReceivedDataPacket;			
	int AddSeqNo;
	int receivedbytes=0;
	//printf("in ReceiveDataPacketNow..., receiving data packet, i: %d\n", i);
	if ((recv(assoc_node->cid[i], &ReceivedDataPacket,8,MSG_WAITALL)) == -1) 
	{
		perror("recv");
		return;
	}
	char *ReceivedData;//[10000];
	ReceivedData = malloc(LengthofDataPacket+1);
	if(ReceivedData ==NULL)
	{
		perror("malloc: ");
		exit(0);
	}
	assoc_node->con_recv_node[i].data = malloc(LengthofDataPacket);
	//printf("receiving data\n");
	if ((receivedbytes=(recv(assoc_node->cid[i], ReceivedData,LengthofDataPacket, 0))) == -1) 
	{
		perror("recv");
		//exit(1);
		return;
	}
	ReceivedData[receivedbytes]='\0';
	
	if((LengthofDataPacket-receivedbytes)==0)
	{
		if(assoc_node->expected_seq_no > ReceivedDataPacket.EMFSequenceNo)
		{ 
			assoc_node->con_recv_node[i].byte_received = 0;
			assoc_node->con_recv_node[i].length = 0;
			free(assoc_node->con_recv_node[i].data);
		}
		else
		{
			//place it in the final list
			struct emf_list *new_node;	
			new_node = (struct emf_list *) malloc(sizeof(struct emf_list));
			if (new_node == NULL)
			{
				printf("\nMemory allocation Failure!\n");
				exit(0);
			}
			else
			{
				//
				//printf("Before INsertion %d : ",ReceivedDataPacket.EMFSequenceNo);
				//printf("Before INsertion %d : \n",LengthofDataPacket);
				//printf("Before INsertion %s : ",ReceivedData);	
				new_node->prev=NULL;
				new_node->seq_no=ReceivedDataPacket.EMFSequenceNo;
				new_node->length=LengthofDataPacket;
				new_node->data=malloc(new_node->length);
		
				if(new_node->data == NULL)
					perror("malloc");
				
				memcpy(new_node->data,&ReceivedData[0],receivedbytes);
				new_node->data[receivedbytes] = '\0';
				new_node->next=NULL;
				if(assoc_node->expected_seq_no == new_node->seq_no)
				{	
					emf_head = assoc_node->emf_recv_head;
					emf_tail = assoc_node->emf_recv_tail;
					AddSeqNo = insert_emf_node(new_node);
					assoc_node->emf_recv_head = emf_head;
					assoc_node->emf_recv_tail = emf_tail;
					assoc_node->expected_seq_no += AddSeqNo;			    
				}
				else
				{
					emf_head = assoc_node->emf_recv_head;
					emf_tail = assoc_node->emf_recv_tail;
					insert_emf_node(new_node);
					assoc_node->emf_recv_head = emf_head;
					assoc_node->emf_recv_tail = emf_tail;
				}
				
				//send to local server here...
				send_to_local_app(assoc_node);
			}
			assoc_node->con_recv_node[i].byte_received = 0;
			assoc_node->con_recv_node[i].length = 0;
			//free(assoc_node->con_recv_node[i].data);
		}	
	}
	else
	{
		assoc_node->con_recv_node[i].seq_no=ReceivedDataPacket.EMFSequenceNo;
		assoc_node->con_recv_node[i].length=LengthofDataPacket;
		assoc_node->con_recv_node[i].byte_received=receivedbytes;
		memcpy(assoc_node->con_recv_node[i].data,ReceivedData,receivedbytes);

	}
	free(ReceivedData);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////

void ReceiveDataReschedule(int i, int LengthofDataPacket,struct assoc_list *assoc_node)
{
	int j, ret=0;
	int AddSeqNo;
	Data_Packet ReceivedDataPacket;			
	char *ReceivedData;
	int receivedbytes=0;
	if ((ret=(recv(assoc_node->cid[i], &ReceivedDataPacket,sizeof(Data_Packet), 0))) == -1) 
	{
		perror("recv");
		exit(1);
	}					
	ReceivedData = malloc(LengthofDataPacket+1);
	if(ReceivedData == NULL)
	printf("Memory Allocation Failed\n");
	assoc_node->con_recv_node[i].data = malloc(LengthofDataPacket);
	if ((receivedbytes=(recv(assoc_node->cid[i], ReceivedData,LengthofDataPacket, 0))) == -1) 
	{
		perror("recv");
		exit(1);
	}
	ReceivedData[receivedbytes]='\0';
	//printf("packetlength: %d, receivedbytes: %d\n%s\n", LengthofDataPacket, receivedbytes, ReceivedData);
	int conIndex;
	for(conIndex=0;conIndex<assoc_node->c_ctr;conIndex++)
	{
		if(ReceivedDataPacket.EMFSequenceNo > assoc_node->con_recv_node[conIndex].seq_no && ReceivedDataPacket.EMFSequenceNo <= (ReceivedDataPacket.EMFSequenceNo + assoc_node->con_recv_node[conIndex].byte_received))
		{
			struct emf_list *new_node;
			new_node = (struct emf_list *) malloc(sizeof(struct emf_list));
			if (new_node == NULL)
			{
				printf("\nMemory allocation Failure!\n");
				exit(0);
			}
			else
			{
				new_node->prev=NULL;
				new_node->seq_no=assoc_node->con_recv_node[conIndex].seq_no;   
				new_node->length=((assoc_node->con_recv_node[conIndex].seq_no+assoc_node->con_recv_node[conIndex].byte_received)-(ReceivedDataPacket.EMFSequenceNo));
				new_node->data=malloc(new_node->length);
				memcpy(new_node->data,assoc_node->con_recv_node[conIndex].data,new_node->length);
				new_node->next=NULL;
		
				if(assoc_node->expected_seq_no == new_node->seq_no)
				{	
					emf_head = assoc_node->emf_recv_head;
					emf_tail = assoc_node->emf_recv_tail;
					AddSeqNo = insert_emf_node(new_node);
					assoc_node->emf_recv_head = emf_head;
					assoc_node->emf_recv_tail = emf_tail;				
					assoc_node->expected_seq_no += AddSeqNo;			    
					for(j = new_node->length; j < assoc_node->con_recv_node[conIndex].length; j++)
					{
						assoc_node->con_recv_node[conIndex].data[j-new_node->length] = assoc_node->con_recv_node[conIndex].data[j];
					}
	
					assoc_node->con_recv_node[conIndex].length -= new_node->length;
					assoc_node->con_recv_node[conIndex].seq_no += new_node->length;
	
				}
				else
					if(assoc_node->expected_seq_no < new_node->seq_no)
					{
						emf_head = assoc_node->emf_recv_head;
						emf_tail = assoc_node->emf_recv_tail;
						insert_emf_node(new_node);
						assoc_node->emf_recv_head = emf_head;
						assoc_node->emf_recv_tail = emf_tail;
						
						for(j = new_node->length; j < assoc_node->con_recv_node[conIndex].length; j++)
						{
							assoc_node->con_recv_node[conIndex].data[j-new_node->length] = assoc_node->con_recv_node[conIndex].data[j];
						}
						assoc_node->con_recv_node[conIndex].length -= new_node->length;
						assoc_node->con_recv_node[conIndex].seq_no += new_node->length;
					}
				break;
				}
			}
		}
	
	if((LengthofDataPacket-receivedbytes)==0)
	{
		//place it in the final list
		struct emf_list *new_node;
		new_node = (struct emf_list *) malloc(sizeof(struct emf_list));
		if (new_node == NULL)
			{
			printf("\nMemory allocation Failure!\n");
			exit(0);
			}
		else
			{
			new_node->prev=NULL;
			new_node->seq_no=ReceivedDataPacket.EMFSequenceNo;
			new_node->length=receivedbytes;
			new_node->data=malloc(new_node->length);
			memcpy(new_node->data,ReceivedData,receivedbytes);
			new_node->next=NULL;

			//assoc_node->expected_seq_no += receivedbytes;			    
			//insert_emf_node(assoc_node->emf_recv_head, assoc_node->emf_recv_tail, new_node);

			if(assoc_node->expected_seq_no == new_node->seq_no)
				{	
				emf_head = assoc_node->emf_recv_head;
				emf_tail = assoc_node->emf_recv_tail;
				AddSeqNo = insert_emf_node(new_node);
				assoc_node->emf_recv_head = emf_head;
				assoc_node->emf_recv_tail = emf_tail;
				assoc_node->expected_seq_no += AddSeqNo;			    
				}
			else if(assoc_node->expected_seq_no < new_node->seq_no)
				{
				emf_head = assoc_node->emf_recv_head;
				emf_tail = assoc_node->emf_recv_tail;
				AddSeqNo = insert_emf_node(new_node);
				assoc_node->emf_recv_head = emf_head;
				assoc_node->emf_recv_tail = emf_tail;
				}
				
			//send to local server here...
			send_to_local_app(assoc_node);
			}
		
		assoc_node->con_recv_node[i].byte_received = 0;
		assoc_node->con_recv_node[i].length = 0;
		free(assoc_node->con_recv_node[i].data);
		}
	else
		{
		//place it in the temporary list
		assoc_node->con_recv_node[i].seq_no=ReceivedDataPacket.EMFSequenceNo;
		assoc_node->con_recv_node[i].length=LengthofDataPacket;
		assoc_node->con_recv_node[i].byte_received=receivedbytes;
		memcpy(assoc_node->con_recv_node[i].data,ReceivedData,receivedbytes);
		}
	}



//////////////////////////////////////////
void Handle_Reassembly(int i , struct assoc_list *assoc_node)
	{
	char IP[16];
	int sent=0, ret=0;
	
	int AddSeqNo;
	int j, index, header_size, received_bytes;
	Base_Header ReceivedBaseHeader;

//	printf("i  =  %d\n",i);
	
	//printf("%d\n", assoc_node->con_recv_node[i].byte_received);
	if(assoc_node->con_recv_node[i].byte_received == -1)
		{
		assoc_node->con_recv_node[i].byte_received = 0;
		}
	//printf("b:%d, length: %d\n", assoc_node->con_recv_node[i].byte_received, assoc_node->con_recv_node[i].length);
	
	int Difference = assoc_node->con_recv_node[i].length - assoc_node->con_recv_node[i].byte_received;
	//printf("received at the reassembler ...%d, %d\n", Difference, assoc_node->con_recv_node[i].byte_received);
	
	
	
	if(Difference == 0)
		{		
		//	printf("receiving packet...\n");
		if( (received_bytes = (recv(assoc_node->cid[i], &ReceivedBaseHeader,4,MSG_WAITALL))) == -1) 
			{
		    perror("recv");
		    //exit(1);
			}
		//printf("version %d :packet type %d : data length %d \n", ReceivedBaseHeader.Version,ReceivedBaseHeader.PacketType ,ReceivedBaseHeader.DataLength);
				
		if(received_bytes == 0)
			{
			if(assoc_node->scenario == HAND_OVER)
				{	
				if(assoc_node->con_send_tail[0]!=NULL)					
					{
					if(assoc_node->con_send_head[0]->data_size > 0)
						{
						//printf("i am calling re schedule \n");	
						reschedule_data(assoc_node,emf_packet,0,1,DATAPACK);
						}
					}
					//else
					//printf("no data to place back\n");
					
					/*int unread_data;
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
						}*/
				//printf("I am making the scenario normal assoc_nodee->aid = %d\n",assoc_node->aid);
				
				int old_des;
				close(assoc_node->cid[0]);
				free(assoc_node->con_recv_node[0].data);
				FD_CLR(assoc_node->cid[0], &read_master);
				assoc_node->scenario = NORMAL;					
				old_des=assoc_node->cid[0];
				assoc_node->cid[0] = assoc_node->cid[1];
				assoc_node->c_ctr = 1;
        		assoc_node->cid[1]=0;
				assoc_node->Cflag =0;
				
				HA_TLV.TLVType = HANDOVER_COMMIT;
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
						
				int len = sizeof(struct sockaddr);
				struct sockaddr_in  my_addr;
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
				printf("CORE : HANDOVER_COMMIT here Message Sent to Host Agent with AID(%d) oldConnection(%d) newConnection(%d)\n", assoc_node->aid, old_des, assoc_node->cid[0]); //Omer	
				
				}
			else 
				{
				//if(assoc_node->scenario==NORMAL)
				//{
				//printf("connection closed received bytes %d: %d assoc_nodee->aid = %d\n\n\n\n\n",received_bytes,assoc_node->cid[i],assoc_node->aid);
			
				assoc_node->Cflag++;
				
				//printf("but I am going to wait for an iteration\n");
				//assoc_node->dont_send[0]=1;
				if(assoc_node->Cflag >= 2)
					{
					printf("now I am closing the connection i= %d cid[i]= %d \n",i, assoc_node->cid[i]);
					close(assoc_node->cid[i]);
					FD_CLR(assoc_node->cid[i], &read_master);
					//printf("assoc_nodee->aid = %d\n",assoc_node->aid);
					if(assoc_node->c_ctr > 0)
						{
						for(j = i; j < assoc_node->c_ctr-1; j++)
							{
							assoc_node->cid[j] = assoc_node->cid[j+1];						
							}			
							assoc_node->c_ctr--;
						}
						assoc_node->Cflag =0;
					}
				//}
				}
			return;
			}
		
		//return;
		//}
		//printf("packet type: %d...\n", ReceivedBaseHeader.PacketType);
		//check if the base bit is set
		switch(ReceivedBaseHeader.PacketType)
			{
			case FINISH :
				{

				char bufat[64];
				if (recv(assoc_node->cid[i], &bufat, 36,MSG_WAITALL) <= 0)
				{
				}

				bufat[36]='\0';
				printf("I have received termination request buffer\n%s \n: %d\n",bufat,strlen(bufat));

				/////////////////////////////decrypt///////////////////////////////////////////////////////////
				int ret_nonce,ret_aid;
				decryptAidNonce(&ret_nonce,&ret_aid,bufat,assoc_node);
				printf("   %d,   %d\n",ret_aid,ret_nonce);
				if(ret_aid != assoc_node->aid|| ret_nonce < assoc_node->RNonce)
				exit(0);


				printf("CORE : Remove from BA received\n");
				//printf("received finish %d \n\n\n\n",i);
				make_emf_packet(assoc_node, 0, 0, emf_packet, FINISH_ACK);
				if( send(assoc_node->cid[i], emf_packet, sizeof(emf_packet), 0) )
					{
					//printf("sending ack on %d : \n",i);
					close(assoc_node->cid[i]);
					FD_CLR(assoc_node->cid[i], &read_master);
					
					HA_TLV.TLVType = REMOVE_CONNECTION;
					HA_TLV.TLVLength = 8;
					//Port Number (Application)
					TLVUnit.intValue = assoc_node->aid; 
					for(j=0; j<4; j++)
						HA_TLV.TLVValue[j] = TLVUnit.charValue[j];
			
					TLVUnit.intValue = assoc_node->cid[i]; 
					for(; j<8; j++)
						HA_TLV.TLVValue[j] = TLVUnit.charValue[j-4];
				
					//////////////////////////////////////////////
			
					HA_TLV.TLVValue[j] = '\0';
	
					//printf("sending remove connection message to host agent...\n");
					if (send(sock_ha, &HA_TLV, sizeof(HA_TLV), 0) <= 0)	//1. send association information to HA
						{
						showerr("Can't send remove connection message to host agent...:");
						}
				    printf("CORE : REMOVE_CONNECTION Message Sent to Host Agent with AID(%d) CID(%d)\n", assoc_node->aid, assoc_node->cid[i]); //Omer
			
				    for(j = i; j < assoc_node->c_ctr-1; j++)
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
					
					assoc_node->c_ctr--;
					assoc_node->dont_send[i] = 0;
					}
				
				break;
				}
				
			
			case FINISH_ACK :
				{

				char bufat[64];
				if (recv(assoc_node->cid[i], &bufat, 36,MSG_WAITALL) <= 0)
				{
				}

				bufat[36]='\0';
				printf("I have received termination request buffer\n%s \n: %d\n",bufat,strlen(bufat));

				/////////////////////////////decrypt///////////////////////////////////////////////////////////
				int ret_nonce,ret_aid;
				decryptAidNonce(&ret_nonce,&ret_aid,bufat,assoc_node);
				printf("   %d,   %d\n",ret_aid,ret_nonce);
				if(ret_aid != assoc_node->aid|| ret_nonce < assoc_node->RNonce)
				exit(0);




				//printf("received finish ack\n\n\n\n");
				HA_TLV.TLVType =AIDwise_REMOVE_FROM_BA_COMPLETE;
				HA_TLV.TLVLength = 8;
				//Port Number (Application)
				//printf("aid remove check\n");
				TLVUnit.intValue = assoc_node->aid; 
				for(j=0; j<4; j++)
					HA_TLV.TLVValue[j] = TLVUnit.charValue[j];
				//printf("aid remove check1\n");
				TLVUnit.intValue = assoc_node->cid[i]; 
				//printf("aid remove check2\n");
				for(; j<8; j++)
					HA_TLV.TLVValue[j] = TLVUnit.charValue[j-4];
				//printf("aid remove  check3\n");   			
				HA_TLV.TLVValue[j] = 0;
				
				//printf("sending remove connection message to host agent...\n");
				if (send(sock_ha, &HA_TLV, sizeof(HA_TLV), 0) <= 0)	//1. send association information to HA
					{
					showerr("Can't send remove connection message to host agent...:");
					}
				printf("CORE : REMOVE_CONNECTION Message Sent to Host Agent with AID(%d) CID(%d)\n", assoc_node->aid, assoc_node->cid[i]); //Omer

				close(assoc_node->cid[i]);
				FD_CLR(assoc_node->cid[i], &read_master);
				
				for(j = i; j < assoc_node->c_ctr-1; j++)
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
				
				//printf("\n\nbefore the connection counter is  %d : ",				assoc_node->c_ctr);
				
				assoc_node->c_ctr--;
				assoc_node->dont_send[i] = 0;
				
				//printf("the connection counter is  %d : ",				assoc_node->c_ctr);
				
				/*close(assoc_node->cid[i]);
				FD_CLR(assoc_node->cid[i], &read_master);
				
				for(j = i; j < assoc_node->c_ctr-1; j++)
					{
					assoc_node->cid[j] = assoc_node->cid[j+1];
            		assoc_node->con_recv_node[j].length = assoc_node->con_recv_node[j+1].length;
					assoc_node->con_recv_node[j].byte_received = assoc_node->con_recv_node[j+1].byte_received;
					
					assoc_node->con_send_head[j] = assoc_node->con_send_head[j+1];
            		assoc_node->con_send_tail[j] = assoc_node->con_send_tail[j+1];
            		assoc_node->last_data_in_buffer[j] = assoc_node->last_data_in_buffer[j+1];
            		assoc_node->sched_ctr[j] = assoc_node->sched_ctr[j+1];
            		assoc_node->current_data_in_buffer[j] = assoc_node->current_data_in_buffer[j+1];
               		assoc_node->RTT[j] = assoc_node->RTT[j];
               		assoc_node->avg_throughput[j] = assoc_node->avg_throughput[j+1];
               		assoc_node->prev_throughput[j] = assoc_node->prev_throughput[j+1];
              		assoc_node->min_tp_cnt[j] = assoc_node->min_tp_cnt[j+1];
					}
					
				assoc_node->c_ctr--;
				assoc_node->dont_send[i] = 0;
				
				HA_TLV.TLVType = REMOVE_CONNECTION;
				HA_TLV.TLVLength = 8;
				//Port Number (Application)
				TLVUnit.intValue = assoc_node->aid; 
				
				for(j=0; j<4; j++)
					HA_TLV.TLVValue[j] = TLVUnit.charValue[j];
			
				TLVUnit.intValue = assoc_node->cid[index]; 
				for(; j<8; j++)
				 	HA_TLV.TLVValue[j] = TLVUnit.charValue[j-4];
				
   				//////////////////////////////////////////////
				HA_TLV.TLVValue[j] = 0;*/
	
				break;
				}
				
				
			case ASSOCIATION_TERMINATION_RESPONSE:
				{

				char bufat[64];
				Assoc_Terminate AssocTerminate;
				if (recv(assoc_node->cid[i], &AssocTerminate, sizeof(Assoc_Terminate),MSG_WAITALL) <= 0)
				{
				}

				
				printf("I have received termination request buffer\n \n: %d\n",AssocTerminate.EncryptedAIDNonceClear);

				/////////////////////////////decrypt///////////////////////////////////////////////////////////
				int ret_nonce,ret_aid;
				decryptAidNonce(&ret_nonce,&ret_aid,AssocTerminate.EncryptedAIDNonce,assoc_node);
				printf("   %d,   %d\n",ret_aid,ret_nonce);
				if(ret_aid != assoc_node->aid|| ret_nonce < assoc_node->RNonce)
				exit(0);


				//printf("association termination response\n");
				//			printf("No. of Connections = %d", assoc_node->c_ctr);
					
				// setting the time stamp while receiving the association termination response
				assoc_node->association_termination_response_timestamp = time(NULL); 
				//printf ("\nUsing ctime: %s\n", ctime(&assoc_node->association_termination_response_timestamp));
										
				
				for(index = 0; index < assoc_node->c_ctr; index++)
					{			         		
					HA_TLV.TLVType = REMOVE_CONNECTION;
					HA_TLV.TLVLength = 8;
					//Port Number (Application)
					TLVUnit.intValue = assoc_node->aid; 
					for(j=0; j<4; j++)
						HA_TLV.TLVValue[j] = TLVUnit.charValue[j];
					
					TLVUnit.intValue = assoc_node->cid[index]; 
					for(; j<8; j++)
						HA_TLV.TLVValue[j] = TLVUnit.charValue[j-4];
				
					//////////////////////////////////////////////
					
					HA_TLV.TLVValue[j] = 0;
					
					//printf("sending remove connection message to host agent...\n");
					if (send(sock_ha, &HA_TLV, sizeof(HA_TLV), 0) <= 0)	//1. send association information to HA
						{
						showerr("Can't send remove connection message to host agent...:");
						}
				    printf("CORE : REMOVE_CONNECTION Message Sent to Host Agent with AID(%d) CID(%d)\n", assoc_node->aid, assoc_node->cid[index]); //Omer
			
				    assoc_node->con_recv_node[index].length = 0;
					assoc_node->con_recv_node[index].byte_received = 0;
					//printf(" closing %d\n",assoc_node->cid[index]);
					
					close(assoc_node->cid[index]);
					FD_CLR(assoc_node->cid[index], &read_master);
					assoc_node->cid[index]=0;
					}
				
				HA_TLV.TLVType = REMOVE_ASSOCIATION;
				HA_TLV.TLVLength = 4;
				//Port Number (Application)
				TLVUnit.intValue = assoc_node->aid; 
				for(j=0; j<4; j++)
					HA_TLV.TLVValue[j] = TLVUnit.charValue[j];
				
				//////////////////////////////////////////////
				HA_TLV.TLVValue[j] = 0;
				
				//printf("sending remove association message to host agent...\n");
				if (send(sock_ha, &HA_TLV, sizeof(HA_TLV), 0) <= 0)	//1. send association information to HA
					{
					showerr("Can't send remove association message to host agent...:");
					}
				
			    printf("CORE : REMOVE_ASSOCIATION Message fghjkSent to Host Agent with AID(%d)\n", assoc_node->aid); //Omer
		    printf("association node removed...\n");
				//printf("aid: %d, %d\n",(assoc_node->aid >> 16) ,associationID[assoc_node->aid >> 16].aid_flag);
				//printf("aid: %d, %d\n",(assoc_node->aid >> 16) ,associationID[assoc_node->aid >> 16].aid_flag);

			    associationID[(assoc_node->aid >> 16)-1].aid_flag = 0;
			    close(assoc_node->lib_sockfd);
			    FD_CLR(assoc_node->lib_sockfd, &read_master);
			    assoc_node->lib_sockfd=0;	




/*				if (assoc_node->MyKey) {EC_KEY_free(assoc_node->MyKey); printf("I have freed the key\n");}
else
{
printf("I have not freed the key\n");
}*/

			    remove_assoc_node(assoc_node);	
//				free(assoc_node);
			    

			    break;
				}
				
				
			///////////////////////////
				//case ASSOCIATION_ESHTABLISHMENT_RESPONSE:
			//////////////////////////
			
			case ASSOCIATION_ESTABLISHMENT_RESPONSE:
				
				{
					//printf("Reassembly: received association establishment response\n");	
					//receive assoc response
					
					// setting the time stamp while receiving the association establishment response
					assoc_node->association_eshtablishment_response_timestamp = time(NULL); 
					//printf ("\nUsing ctime: %s\n", ctime(&assoc_node->association_eshtablishment_response_timestamp));
																		
																			
					
					struct AssociationResponse assoc_res;
					
					assoc_res.aID = 0;
					
						//struct assoc_list *assoc_node1;
						//if (recv(assoc_node->cid[i], &assoc_res, sizeof(struct AssociationResponse), 0) <= 0)//4. remote compl response
						if (recv(assoc_node->cid[i], &assoc_res, 124,MSG_WAITALL) <= 0)//4. remote
							
						{
							showerr("CORE : Remote node is not EMF Compliant, making Direct connection ");
							close(assoc_node->cid[i]);
							connect_with_org_port();
							//free(TLVValue);
							break;
						}
						else
						{
							printf("Received remote compliance\n");
						
							if(assoc_res.aID == 0)
							{
								showerr("CORE : Remote node is not EMF Compliant, making Direct connection ");
								close(assoc_node->cid[i]);
								connect_with_org_port();
								//free(TLVValue);
								break;
							}	   
							
							else
							
							{									         
								assoc_res.PubKey[117]='\0';
								
								printf ("\nstrlen ()  %d : public key of ther other side %s\n",strlen(assoc_res.PubKey),assoc_res.PubKey);
								
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
	
		
								BN_dec2bn(&bin, assoc_res.PubKey);
								
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
								assoc_node->sessionKey[0]='\0';
								mySessionKey[0]='\0';
								abuf=(unsigned char *)OPENSSL_malloc(100);
								aout=ECDH_compute_key(abuf,100,HisPublic,assoc_node->MyKey,KDF1_SHA1);
								for (ii=0; ii<aout; ii++)
								{
									sprintf(mySessionKey,"%02X",abuf[ii]);
									strcat(assoc_node->sessionKey,mySessionKey);

								}
								BIO_puts(out,assoc_node->sessionKey);

								BIO_puts(out,"\n");
	
	
	
	
	
								
								///////////////////////////////generate Session key////////////////////////////
								
								assoc_node->aid = ( (assoc_node->aid << 16) | assoc_res.aID );
								
								//printf("res: %d, aid created: %d\n", assoc_res.aID, assoc_node->aid);
								
								int len = sizeof(struct sockaddr);
								struct sockaddr_in  my_addr;
								if(getsockname(assoc_node->cid[i], (struct sockaddr*)&my_addr, &len) == 0)
								{	
									memcpy(&HA_TLV.TLVValue[i], inet_ntoa(my_addr.sin_addr), strlen(inet_ntoa(my_addr.sin_addr)) );
									HA_TLV.TLVValue[i+strlen(inet_ntoa(my_addr.sin_addr))] = '\0';
									//printf("\nIP in TLV Format: %s\n",&HA_TLV.TLVValue[i]);
									//printf("Got IP: %s\n%s\n", inet_ntoa(my_addr.sin_addr), &HA_TLV.TLVValue[12]);
									memcpy(&IP, inet_ntoa(my_addr.sin_addr), strlen(inet_ntoa(my_addr.sin_addr)) +1 );
									IP[strlen(inet_ntoa(my_addr.sin_addr))+1]='\0';
								}
											
								// change the scenario to NORMAL
								assoc_node->scenario = NORMAL;				
								
								//---------inform HA about this new association											         							         											      
								
								HA_TLV.TLVType = ADD_ASSOCIATION;
								HA_TLV.TLVLength = 4;
								//Port Number (Application)
								TLVUnit.intValue = assoc_node->aid; 													
								for(j=0; j<4; j++)
									HA_TLV.TLVValue[j] = TLVUnit.charValue[j];
								
								//my_addr2 = (struct sockaddr_in *) assoc_node->dest_addr;
								TLVUnit.intValue = assoc_node->ProtocolNo;//htons(my_addr2->sin_port);
								//printf("Peer port IN SOCKET LIBRARY HANDLER : %d\n", htons(my_addr2->sin_port));
								
								
								for(j=4; j<8; j++)
									HA_TLV.TLVValue[j] = TLVUnit.charValue[j-4];
								
								TLVUnit.intValue = assoc_node->cid[i]; 
								for(j=8; j<12; j++)
									HA_TLV.TLVValue[j] = TLVUnit.charValue[j-8];
								
								
								int i;		
								for (i = 0; i < strlen(IP); i++, j++)
								{
									HA_TLV.TLVValue[j] = IP[i]; 	                        					
									//		printf("%c", HA_TLV.TLVValue[j]);
								}
								
								HA_TLV.TLVValue[j]='\0';				            													//}
															
								
								
								if ((sent=send(sock_ha, &HA_TLV, sizeof(HA_TLV), 0)) <= 0)	//1. send association information to HA
								{
									showerr("Can't send local compliance to host agent...:");
								}
								
																printf("assoc_node1->scenario %d",assoc_node->scenario);
																
																
								printf("\nhello  CORE : ADD_ASSOCIATION Message Sent to Host Agent AID(%d), CID(%d), IP(%s) port : %d \n", assoc_node->aid, assoc_node->cid[i], &HA_TLV.TLVValue[i],assoc_node->ProtocolNo);											         

								
								ret = id_emfserverRes;
								if ((sent=send(sock_lib, &ret, sizeof(ret), 0)) <= 0)           //5. send response to emf_socket_library
									showerr("Can't send response emf library...:");
							}
						}
						
								
				break;

				}
				
				
				
				
			case ASSOCIATION_TERMINATION_REQUEST :
				{
				char bufat[64];
				Assoc_Terminate AssocTerminate;
				if (recv(assoc_node->cid[i], &AssocTerminate, sizeof(Assoc_Terminate),MSG_WAITALL) <= 0)
				{
				}

				
				printf("I have received termination request buffer\n \n: %d\n",AssocTerminate.EncryptedAIDNonceClear);

				/////////////////////////////decrypt///////////////////////////////////////////////////////////
				int ret_nonce,ret_aid;
				decryptAidNonce(&ret_nonce,&ret_aid,AssocTerminate.EncryptedAIDNonce,assoc_node);
				printf("   %d,   %d\n",ret_aid,ret_nonce);
				if(ret_aid != assoc_node->aid|| ret_nonce < assoc_node->RNonce)
				exit(0);



				////////////////////////////////////////////////////////////////////////////////////////
				//printf("association termination request received\n");
				send_to_local_app(assoc_node);
					
				header_size = make_emf_packet(assoc_node, assoc_node->emf_send_head->seq_no, 0, emf_packet, ASSOCIATION_TERMINATION_RESPONSE);
				
				if( (send(assoc_node->cid[i], emf_packet, header_size, 0)))
				{
					// no code
				}
				
				//printf("End No. of Connections = %d", assoc_node->c_ctr);
				
				for(index = 0; index < assoc_node->c_ctr; index++)
	         		{			         		
    	     		HA_TLV.TLVType = REMOVE_CONNECTION;
					HA_TLV.TLVLength = 8;
					//Port Number (Application)
					TLVUnit.intValue = assoc_node->aid; 
					for(j=0; j<4; j++)
						HA_TLV.TLVValue[j] = TLVUnit.charValue[j];
				
					TLVUnit.intValue = assoc_node->cid[index]; 
					for(; j<8; j++)
						HA_TLV.TLVValue[j] = TLVUnit.charValue[j-4];
					
					//////////////////////////////////////////////
					HA_TLV.TLVValue[j] = 0;
					
					//printf("sending remove connection message to host agent...\n");
					if (send(sock_ha, &HA_TLV, sizeof(HA_TLV), 0)==-1)	//1. send association information to HA
						{
						showerr("Can't send remove connection message to host agent...:");
						}
					
					printf("CORE : REMOVE_CONNECTION Message Sent to Host Agent with AID(%d) CID(%d)\n", assoc_node->aid, assoc_node->cid[index]); //Omer
					assoc_node->con_recv_node[index].length = 0;
					assoc_node->con_recv_node[index].byte_received = 0;
					close(assoc_node->cid[index]);
					//printf(" closing %d\n",assoc_node->cid[index]);					
					
					FD_CLR(assoc_node->cid[index], &read_master);
					assoc_node->cid[index]=0;
					//close(assoc_node->lib_sockfd);					// By Ehsan
					//FD_CLR(assoc_node->lib_sockfd, &read_master);		// By Ehsan
	         		}
				
				HA_TLV.TLVType = REMOVE_ASSOCIATION;
				HA_TLV.TLVLength = 4;
				//Port Number (Application)
				TLVUnit.intValue = assoc_node->aid; 
				for(j=0; j<4; j++)
					HA_TLV.TLVValue[j] = TLVUnit.charValue[j];
			
				//////////////////////////////////////////////
				HA_TLV.TLVValue[j] = 0;
				//printf("sending remove association message to host agent...\n");
				
				if (send(sock_ha, &HA_TLV, sizeof(HA_TLV), 0)==-1)	//1. send association information to HA
					{
					showerr("Can't send remove association message to host agent...:");
					}
				
				//printf("aid: %d, %d\n",(assoc_node->aid >> 16) ,associationID[assoc_node->aid >> 16].aid_flag);
				//printf("aid: %d, %d\n",(assoc_node->aid >> 16) ,associationID[assoc_node->aid >> 16].aid_flag);
			    printf("CORE : REMOVE_ASSOCIATION Message Sent to Host Agent with AID(%d)\n", assoc_node->aid); //Omer
			    associationID[(assoc_node->aid >> 16)-1].aid_flag = 0;
			    close(assoc_node->lib_sockfd);
			    FD_CLR(assoc_node->lib_sockfd, &read_master);
			    assoc_node->lib_sockfd=0;	

			
		//free(assoc_node->emf_send_head->data);
//		free(assoc_node->emf_send_tail->data);
//		free(assoc_node->emf_recv_tail->data);
		//free(assoc_node->emf_recv_head->data);

//remove_emf_node(assoc_node->emf_send_head);
//remove_emf_node(assoc_node->emf_send_tail);
//remove_emf_node(assoc_node->emf_recv_tail);
//remove_emf_node(assoc_node->emf_recv_head);
//EC_KEY_free(assoc_node->MyKey);

/*struct con_slint *con;


for(con=assoc_node->con_send_tail[0];con !=NULL; con= assoc_node->con_send_tail[0]->prev)
{
free(con->data);
remove_con_send_node(con);
}*/

		



/*		if (assoc_node->MyKey) {EC_KEY_free(assoc_node->MyKey); printf("I have freed the key\n");}
else
{
printf("I have not freed the key\n");
}*/

			    remove_assoc_node(assoc_node);	
//				free(assoc_node);
			    //assoc_node->lib_sockfd=0;
			    //printf("association node removed...\n");

			    printf("association node removed...\n");
	      	            //exit(0);
	      	
			    break;
				}
			
			case DATAPACK :
				{
				if(ReceivedBaseHeader.Reserved)		
					{
					//printf("Data Packet received1\n");
					ReceiveDataReschedule(i,ReceivedBaseHeader.DataLength,assoc_node);
					}
				else
					{
					//printf("Data Packet received2\n");
					ReceiveDataPacketNow(i,ReceivedBaseHeader.DataLength,assoc_node);
					}
				break;
				}
			}
		//Depending upon the type receive the next header;
		} // end of main if command for difference==0

	else
		{
		if(Difference > 0)
			{
			int receivedbytes1=0, oldreceivedbytes = assoc_node->con_recv_node[i].byte_received;
			char *ReceivedData1;
			ReceivedData1 = malloc(Difference);
			//printf("receiving %d bytes\n", Difference);
			if ((receivedbytes1=(recv(assoc_node->cid[i], ReceivedData1,Difference, 0))) == -1) 
				{
				perror("recv");
				//exit(1);
				}
			if(receivedbytes1 == 0)
				{
				close(assoc_node->cid[i]);
				FD_CLR(assoc_node->cid[i], &read_master);
			
				for(j = i; j < assoc_node->c_ctr-1; j++)
					{
					assoc_node->cid[j] = assoc_node->cid[j+1];						
					}
			
				assoc_node->c_ctr--;
				return;
				}

			//printf("copying data, oldbytes: %d, length: %d\n", oldreceivedbytes, assoc_node->con_recv_node[i].length);
			assoc_node->con_recv_node[i].byte_received=oldreceivedbytes+receivedbytes1;
			memcpy(&assoc_node->con_recv_node[i].data[oldreceivedbytes],ReceivedData1,receivedbytes1);
			//free(ReceivedData1);
			if(assoc_node->con_recv_node[i].byte_received == assoc_node->con_recv_node[i].length)
				{
				//  printf("packet received completely\n");
				if(assoc_node->expected_seq_no > assoc_node->con_recv_node[i].seq_no)
					{
					assoc_node->con_recv_node[i].byte_received = 0;
					assoc_node->con_recv_node[i].length = 0;
					//free(assoc_node->con_recv_node[i].data);
					}
				else
					{
				 	struct emf_list *new_node;
					new_node = (struct emf_list *) malloc(sizeof(struct emf_list));

					if (new_node == NULL)
						{
						printf("\nMemory allocation Failure!\n");
						exit(0);
						}
					else
						{
						new_node->prev=NULL;
						new_node->seq_no=assoc_node->con_recv_node[i].seq_no;
						new_node->length=assoc_node->con_recv_node[i].length;
						new_node->data=malloc(new_node->length);
						
						//printf("copying data: %s\n", assoc_node->con_recv_node[i].data);
						//memcpy(&assoc_node->con_recv_node[i].data[assoc_node->con_recv_node[i].byte_received], ReceivedData1,receivedbytes1);///////
						memcpy(new_node->data,assoc_node->con_recv_node[i].data,new_node->length);

						new_node->next=NULL;
						
						//insert in the list
						//assoc_node->expected_seq_no += assoc_node->con_recv_node[i].byte_received;
						//place it in the final list
						//insert_emf_node(assoc_node->emf_recv_head, assoc_node->emf_recv_tail, new_node);

						if(assoc_node->expected_seq_no == new_node->seq_no)
							{
							emf_head = assoc_node->emf_recv_head;
							emf_tail = assoc_node->emf_recv_tail;
							AddSeqNo = insert_emf_node(new_node);
							assoc_node->emf_recv_head = emf_head;
							assoc_node->emf_recv_tail = emf_tail;
							assoc_node->expected_seq_no += AddSeqNo;
//							free(new_node);			    
							}
						else
							{
							emf_head = assoc_node->emf_recv_head;
							emf_tail = assoc_node->emf_recv_tail;
							insert_emf_node(new_node);
							assoc_node->emf_recv_head = emf_head;
							assoc_node->emf_recv_tail = emf_tail;
//							free(new_node);
							}
						// send data to local server here...
						send_to_local_app(assoc_node);
						}
					}
				
				
				assoc_node->con_recv_node[i].byte_received = 0;
				assoc_node->con_recv_node[i].length = 0;
				//free(assoc_node->con_recv_node[i].data);
										free(ReceivedData1);
				}
			}

		}
	}



