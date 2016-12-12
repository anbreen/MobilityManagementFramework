//BaseHeader

#include<stdio.h>


typedef struct Base
{
	unsigned int Version:4;
	unsigned int PacketType:4;
	unsigned char Reserved:7;
	unsigned int P:1;
	unsigned int DataLength:16;
} Base_Header;

//-----------------------------------------------------------------------------

struct AssociationRequest
{  
	//Base_Header	   baseheader;
	unsigned int     aID:16;
	unsigned int     encryptionAlgo:8;
	unsigned int     encryptionAlgo_Mode:6;
	unsigned int     kCert:2;
	unsigned int     ulID:16;
	char PubKey[117]; //117bytes, the total structure will be 165 bytes
};

struct AssociationResponse
{  
	//Base_Header      baseheader;
	unsigned int     aID:16;
	unsigned int     encryptionAlgo:8;
	unsigned int     encryptionAlgo_Mode:6;
	unsigned int     kCert:2;
		char PubKey[117]; //117bytes, the total structure will be 165 bytes
};
  
union assoc_p
{
struct	AssociationRequest assoc_req;
char 	str[sizeof(struct AssociationRequest)];
} assoc_req;

union assoc_r
{
struct 	AssociationResponse assoc_res;
char 	str[sizeof(struct AssociationResponse)];
} assoc_res;

//-----------------------------------------------------------------------------



//Extension Headers 
typedef struct Data{
unsigned int EMFSequenceNo:32;
unsigned int ULID:32;
} Data_Packet;

unsigned char *ActualData;

typedef struct BandwidthAggregation
{
unsigned long int EncryptedAIDNonceClear;
char EncryptedAIDNonce[32];
} Bandwidth_Aggregation;


typedef struct Handover
{
unsigned long int EncryptedAIDNonceClear;
char EncryptedAIDNonce[32];
} Hand_over;


typedef struct SingleJoin
{
unsigned long int EncryptedAIDNonceClear;
char EncryptedAIDNonce[32];
} Single_Join;


typedef struct AssocTerminate
{
unsigned long int EncryptedAIDNonceClear;
char EncryptedAIDNonce[32];
} Assoc_Terminate;


union bp
{
Base_Header	baseheader_packet;
char 		str[sizeof(Base_Header)];
} baseheader_packet;

union dp
{
Data_Packet 	data_packet;
char 			str[sizeof(Data_Packet)];
} data_packet;

union hp
{
Hand_over 	handover_packet;
char 		str[sizeof(Hand_over)];
} handover_packet;

union bap
{
Bandwidth_Aggregation ba_packet;
char str[sizeof(Bandwidth_Aggregation)];
} ba_packet;

union sjp
{
Single_Join singlejoin_packet;
char str[sizeof(Single_Join)];
} singlejoin_packet;

union atr
{
Assoc_Terminate assocterminate_packet;
char str[sizeof(Assoc_Terminate)];
} assocterminate_packet;


///possiblE Combinations of Extension Header with Base Header 
typedef struct HandoverPLusData
{
Hand_over HandOverPart;
Data_Packet DataPart;
} Handover_Data;


typedef struct BandwidthAggregationPLusData
{
Bandwidth_Aggregation BandwidthAggregationPart;
Data_Packet DataPart;
} BandwidthAggregation_Data;


typedef struct SingleJoinPLusData
{
Single_Join SingleJoinPart;
Data_Packet DataPart;
} SingleJoin_Data;
