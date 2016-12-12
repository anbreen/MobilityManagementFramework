//-----------------------------------------------------------------------------

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
#include <netinet/tcp.h>

#include <math.h>

#include <sys/ioctl.h>
#include <linux/sockios.h>
#include <string.h>
#include <openssl/opensslconf.h>	/* for OPENSSL_NO_ECDH */
#include <openssl/crypto.h>
#include <openssl/bio.h>
#include <openssl/bn.h>
#include <openssl/objects.h>
#include <openssl/sha.h>
//#include <openssl/rand.h>
//#include <openssl/sha.h>
//#include <openssl/err.h>
#include <openssl/des.h>
#include <openssl/ec.h>
#include <openssl/ecdh.h>



//-----------------------------------------------------------------------------
#define nINTERFACES 				5
#define INITIAL_PACKET_SIZE			32*1024 - 1 //max packet size sent first time
#define MAX_PACKET_SIZE				32*1024 - 1 //max packet size to sent

#define SEND_NODE_SIZE				128*1024 - 1 //size of send buffer
#define LOWER_THROUGHPUT_LIMIT 		512

#define TOTL_AIDs          			65535

#define EMF_VERSION 				1
//-----------------------------------------------------------------------------

//------scenario, used in scheduling_handler and set in HA_command_handler-----

#define NORMAL 						0
#define HAND_OVER 					1
#define BANDWIDTH_AGGREGATION 		2
//-----------------------------------------------------------------------------

//------------------header sizes for different packet types--------------------

#define DATA_PACKET_HEADER_SIZE		20
//-----------------------------------------------------------------------------

//------------------------------EMF packet types-------------------------------

#define ASSOCIATION_ESTABLISHMENT_REQUEST 		1
#define ASSOCIATION_ESTABLISHMENT_RESPONSE 		2
#define ASSOCIATION_TERMINATION_REQUEST 		3
#define ASSOCIATION_TERMINATION_RESPONSE 		4
#define FINISH 									5
#define FINISH_ACK 								6

#define HANDOVER 								8
#define BANDWIDTHAGGREGATION 					9
#define DATAPACK 								10
#define HANDOVER_DATA 							11
#define BANDWIDTHAGGREGATION_DATA 				12
#define SINGLEJOIN 								13

#define SINGLEJOIN_DATA 						15


#define ASSOCIATION_ESTABLISHMENT_PENDING 		20

//-----------------------------------------------------------------------------


//-------------ports used throughout EMF_CLIENT--------------------------------

#define HA_BIND_PORT       	8753
#define HA_PORT      		8751
#define LIB_PORT     		8883
#define EMF_CLIENT_PORT    	8885
#define EMF_SERVER_PORT    	8885

//#define HA_BIND_PORT      6763
//#define HA_PORT      		6761

#define TLVValueSize     	35

//-----------------------------------------------------------------------------

/*enum CoreHA_HA_TLV_Types
{LOCAL_COMPLIANCE_REQ=101, LOCAL_COMPLIANCE_RES=102, INTERFACE_IP_REQ=103, INTERFACE_IP_RES=104, ADD_ASSOCIATION=105,
ADD_CONNECTION=106, REMOVE_ASSOCIATION=107, REMOVE_CONNECTION=108, HO_LINK_SHIFT_COMPLETE=109, HO_TRAFFIC_SHIFT_COMPLETE=110, 
BANDWIDTH_AGGREGATION_COMPLETE=111, HO_LINK_SHIFT_COMMIT=112, HO_TRAFFIC_SHIFT_COMMIT=113, BA_COMMIT=114, HANDOVER_COMMIT=115};
*/
/*enum EMFC_HA_TLV_Types
{LOCAL_COMPLIANCE_REQ=101, LOCAL_COMPLIANCE_RES=102, INTERFACE_IP_REQ=103, INTERFACE_IP_RES=104, ADD_ASSOCIATION=105,
ADD_CONNECTION=106, REMOVE_ASSOCIATION=107, REMOVE_CONNECTION=108, HO_LINK_SHIFT_COMMIT=109, HO_LINK_SHIFT_COMPLETE=110, 
HO_TRAFFIC_SHIFT_COMMIT=111, HO_TRAFFIC_SHIFT_COMPLETE=112, BA_COMMIT=113, BA_COMPLETE=114, HANDOVER_COMMIT=115, 
AIDwise_REMOVE_FROM_BA_COMMIT=116, AIDwise_REMOVE_FROM_BA_COMPLETE=117, 
PORTwise_REMOVE_FROM_BA_COMMIT=118, PORTwise_REMOVE_FROM_BA_COMPLETE=119, PORTwise_BA_COMMIT, PORTwise_BA_COMPLETE};
*/


enum EMFC_HA_TLV_Types
	{
	INTERFACE_IP_REQ					=81, 
	INTERFACE_IP_RES					=82, 
	INTERFACE_IP_COM					=83, 
		
	LOCAL_COMPLIANCE_REQ				=101, 
	LOCAL_COMPLIANCE_RES				=102, 
	
	//INTERFACE_IP_REQ					=103, 
	//INTERFACE_IP_RES					=104,
		
	ADD_ASSOCIATION						=105,
	ADD_CONNECTION						=106, 
	REMOVE_ASSOCIATION					=107, 
	REMOVE_CONNECTION					=108, 
	HO_LINK_SHIFT_COMMIT				=109, 
	HO_LINK_SHIFT_COMPLETE				=110, 
	HO_TRAFFIC_SHIFT_COMMIT				=111, 
	HO_TRAFFIC_SHIFT_COMPLETE			=112, 
	BA_COMMIT							=113, 
	BA_COMPLETE							=114, 
	HANDOVER_COMMIT						=115, 
	AIDwise_REMOVE_FROM_BA_COMMIT		=116, 
	AIDwise_REMOVE_FROM_BA_COMPLETE		=117,
	BA_COMMIT_IN_LOCAL_COMPLIANCE		=118, 
	BA_COMPLETE_IN_LOCAL_COMPLIANCE		=119,
	LD_AIDwise_REMOVE_FROM_BA_COMMIT	=120,
	
	// added new for single connection shift
	HO_CONNECTION_SHIFT_COMMIT			=121	
	};

//PORTwise_REMOVE_FROM_BA_COMMIT=118, PORTwise_REMOVE_FROM_BA_COMPLETE=119, PORTwise_BA_COMMIT, PORTwise_BA_COMPLETE};
//, AIDwise_REMOVE_FROM_BA_COMPLETE=121 };



//-----------------------------------------------------------------------------

int 	sock_ha, 
		sock_lib, 
		sock_lib_listen, 
		sock_rem, 
		sock_rem_listen; 	// sockets for communication with Host Agent, EMF_Sock_Lib and a listening socket

fd_set 	read_fds, 
		write_fds, 
		read_master, 
		write_master;

int 	assoc_Index;        //used to generate the Association Identifier

char 	emf_packet[MAX_PACKET_SIZE + 200]; // emf_packet will of size (max data size + header size)

int 	reserved, 
		fdmax;


//int 	show_msg; // global flage for logging

//-----------------------------Structures used for communication with Emf_Sock_Lib and Host_Agent------------------------------

struct sockLib_emf_client
	{
	int         id;         //One of the above identifiers
	int         appSockFD;  //Sock Descriptor used to send and received data from and to server
	struct      sockaddr_in serverAddr; //Server Address
	socklen_t   sin_size;   //Size of the Server Address
	size_t      packetSize; //Size of the Whole packet
	int         flags;      //Flag bit used as the last argument in snd/recv
	pid_t 		app_pid;
	} sockLib_client_TLV;


struct TLVInfo
	{
	char TLVType;
	char TLVLength;
	char TLVValue[TLVValueSize];
	} HA_TLV;


union TLVUnitInfo
	{
	int intValue;
	char charValue[4];
	} TLVUnit;

//-----------------------------------------------------------------------------
struct aIDi
	{
   unsigned int aid_flag : 1;
	} 

associationID[TOTL_AIDs];//Define the Size above and use it else where
void initializeAIDs();

//-----------------structures relating association list------------------------

struct emf_list
	{
	struct emf_list *prev;
	char *data;
	unsigned int seq_no, length;
	struct emf_list *next;
	};

struct con_slist
	{
	struct con_slist *prev;
	unsigned int hdr_size, seq_no, data_size, bytes_sent;
	char *data;
	struct con_slist *next;
	};

struct con_rlist
	{
	struct con_rlist *prev;
	unsigned int seq_no, length, byte_received;
	char *data;
	struct con_rlist *next;
	};




struct assoc_list
	{
	struct assoc_list *prev;
	unsigned int aid;
	int app_sockfd, lib_sockfd;
	int terminating;
	int cid[nINTERFACES];//a message from HA will arrive, that message will inform to make connections on different interfaces for BA, the descriptors of 	those connection will be stored here
	pid_t app_pid;
	int dont_send[nINTERFACES]; // flag to indicate, don't send data on this connection... connection is going to close...
	int c_ctr; //COUNTER FOR THE CONNECTIONS
	int Cflag;
	struct emf_list *emf_send_head, *emf_recv_head;//send and receive buffer
	struct emf_list *emf_send_tail, *emf_recv_tail;

	struct con_slist *con_send_head[nINTERFACES];//list per connection
	struct con_slist *con_send_tail[nINTERFACES];
	
	struct con_rlist con_recv_node[nINTERFACES];

	int sched_ctr[nINTERFACES];
	unsigned int current_data_in_buffer[nINTERFACES], last_data_in_buffer[nINTERFACES];
	unsigned int RTT[nINTERFACES];
	unsigned int avg_throughput[nINTERFACES], prev_throughput[nINTERFACES];
		char sessionKey[40];
		// struct sockaddr *dest_addr;

	int last_buf_empty;
	int min_tp_cnt[nINTERFACES];
	int scenario;
	unsigned int expected_seq_no, seq_no_to_deliver;
	int HandoverCalled;
	EC_KEY *MyKey;	
	const EC_GROUP *group;
	unsigned int RNonce;
	unsigned int SNonce;
	char ServerIP[16];
	int ProtocolNo;
	struct assoc_list *next;

	
	time_t 	association_eshtablishment_request_timestamp,
			association_eshtablishment_response_timestamp,
			association_termination_response_timestamp,
			association_termination_request_timestamp;
	
	};

//-----------------------------------------------------------------------------

struct assoc_list *assoc_head, *assoc_tail, *nextOfDeletedNode; 
struct emf_list   *emf_head,   *emf_tail;
struct con_slist  *con_head, *con_tail;
char flagNodeDeleted;
int counter;

//-----------------------------------------------------------------------------

//-----------------public function(ha_cmd_handler.c)---------------------------
void initiate_handover(struct assoc_list *assoc_node, char *ToIP, int scenorio);
//-----------------public functions(scheduling_handler.c)----------------------

void showerr(char *str);
void showmsg(char *str);

void initialize_assoc_node(struct assoc_list *assoc_node);
void reschedule_data(struct assoc_list *assoc_node, char *emf_packet, int fromIndex, int toIndex, unsigned int packet_type);
int make_emf_packet(struct assoc_list *assoc_node, unsigned int seq_no, unsigned int packet_size, char *emf_packet, int packet_type);
unsigned int bin2hex (unsigned char *ibuf, unsigned char *obuf, unsigned int ilen);
unsigned int NoPadLen (unsigned char *ibuf, unsigned int ilen);
unsigned int PadData (unsigned char *ibuf, unsigned int ilen, int blksize);

unsigned int hex2bin (unsigned char *ibuf, unsigned char *obuf, unsigned int ilen);

//----------------Link list handling functions(list_handler.c)-----------------
void append_assoc_node(struct assoc_list *assoc_node);
void insert_assoc_node(struct assoc_list *assoc_node, struct assoc_list *after);
void remove_assoc_node(struct assoc_list *assoc_node);
//-----------------------------------------------------------------------------
void append_emf_node(struct emf_list *emf_node);
unsigned int  insert_emf_node(struct emf_list *emf_node);//insert at position where list remains sorted by sequence numbers.
void remove_emf_node(struct emf_list *emf_node);
//-----------------------------------------------------------------------------
void append_con_send_node(struct con_slist *con_node);
void insert_con_send_node(struct con_slist *con_node, struct con_slist *after);
void remove_con_send_node(struct con_slist *con_node);
//-----------------------------------------------------------------------------
/*void append_con_recv_node(struct con_rlist *con_recv_head, struct con_rlist *con_recv_tail, struct con_rlist *con_recv_node);
void insert_con_recv_node(struct con_rlist *con_recv_head, struct con_rlist *con_recv_tail, struct con_rlist *con_recv_node, struct con_rlist *after);
void remove_con_recv_node(struct con_rlist *con_recv_head, struct con_rlist *con_recv_tail, struct con_rlist *con_recv_node);
//-----------------------------------------------------------------------------
*/

