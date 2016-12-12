///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// TASK: Create a Selective 'Host Agent' Server that communicates  with EMF CL2I, 
// EMF User Agent and EMF Core as per agreed TLV format and performs the requested 
// functionality including 'Handover' & 'Bandwidth Aggregation' Decision Algorithms.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////


// EMF Defined
#include "../commonFiles/vm.h"
#include "ConInfLL.h"
#include "../commonFiles/emf_ports.h"

#define SO_REUSEPORT 		15

#define queueSize 			20
#define TLVValueSize		75	// HA-EMFC requires 34Bytes(portNo->4B, fromIP->15B, toIP->15B), 
								// HA-EMFC requires 27B (AID->4B, TRID->4B, CID->4B, IP->15B)
								// HA_EMFC requires (15*INTERFACES)Bytes (IP:IP:...:IP\0)


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// User Defined Data Types (enums & structures)
//

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	CL2I Related Types and L2 Parameters
//
// Abbreviations:
// Received Signal Strength (RSS), Signal-to-Noise Ration (SNR), Packet Error Rate (PER)
// Missed Beacons (MB)
//
//	Bit MAP:		7		6		5		4		3		2		1		0
//	Parameter:		-		-		-		-		MB		PER		SNR		RSS
//	Values:			128		64		32		16		8		4		2		1
//	
//	Compound Values:	
//					RSS+SNR=3, RSS+PER=5, SNR+PER=6, RSS+SNR+PER=7

enum CL2I_HA_TLV_Types
{LINK_DETECTED=1, LINK_UP=2, LINK_DOWN=3, LINK_GOING_DOWN=4, L2_PARAMETER_REQ=21, L2_PARAMETER_RES=22};

enum L2Parameters
{RSS=1, SNR=2, RSS_SNR=3, PER=4, RSS_PER=5, SNR_PER=6, RSS_SNR_PER=7,
MB=8, RSS_MB=9, SNR_MB=10, PER_MB=11, RSS_SNR_MB=12, RSS_PER_MB=13, SNR_PER_MB=14, 
RSS_SNR_PER_MB=15}; // More parameters can be added

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// User Agent Related Types
//
// Request Types from User Agent are in Odd numbers, while from Host Agent are in Event numbers.
enum UA_HA_TLV_Types
{HO_LINK_SHIFT_COMMIT_REQ=51, HO_LINK_SHIFT_COMMIT_CONF=52, 
HO_TRAFFIC_SHIFT_COMMIT_REQ=53, HO_TRAFFIC_SHIFT_COMMIT_CONF=54,
BA_COMMIT_REQ=55, BA_COMMIT_CONF=56, LINK_UP_Popup=57, LINK_DOWN_Popup=58,
USER_PREFERENCE_CHANGED_IND=59, INTERFACE_CHANGE_IND=60, 
LOCATION_UPDATE_IND=61, READ_USER_PREF_FILE=62};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// EMF Core Related Types
//
enum EMFC_HA_TLV_Types
{INTERFACE_IP_REQ=81, INTERFACE_IP_RES=82, INTERFACE_IP_COM=83, 
LOCAL_COMPLIANCE_REQ=101, LOCAL_COMPLIANCE_RES=102, ADD_ASSOCIATION=105,
ADD_CONNECTION=106, REMOVE_ASSOCIATION=107, REMOVE_CONNECTION=108, 
HO_LINK_SHIFT_COMMIT=109, HO_LINK_SHIFT_COMPLETE=110, 
HO_TRAFFIC_SHIFT_COMMIT=111, HO_TRAFFIC_SHIFT_COMPLETE=112, BA_COMMIT=113, 
BA_COMPLETE=114, HANDOVER=115, AIDwise_REMOVE_FROM_BA_COMMIT=116, 
AIDwise_REMOVE_FROM_BA_COMPLETE=117, BA_COMMIT_IN_LOCAL_COMPLIANCE=118, 
BA_COMPLETE_IN_LOCAL_COMPLIANCE=119, LD_AIDwise_REMOVE_FROM_BA_COMMIT=120, 
LD_AIDwise_REMOVE_FROM_BA_COMPLETE=121};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CL2I Maintained structure
struct CL2ITriggerInfo
{
	int triggerType;
	char interfaceName[100];
	char IP[40];
}
CL2ITrigger;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HA Maintained structure used for exchanging information
// between UserAgent-HostAgent & EMFCore-HostAgent
union TLVUnitInfo
{
	int intValue;
	char charValue[4];
}
TLVUnit;

struct TLVInfo
{
	char TLVType;
	char TLVLength;
	char TLVValue[TLVValueSize];
}
UA_TLV, EMFC_TLV;

char EMFC_TLVTypeLength[2], *EMFC_TLVValue = NULL, EMFC_TLVValue_Length = 0;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Host Agent Managed Structure
//
struct BATransactionInfo
{
	char manualBAFlag;				// Identify BA Request Source i.e. Is manual or auto BA request being furnished???
	char portIndex;					// Holds indexOfPort for which BARequest is done
	char fromLinkIndex;				// Holds indexOfIP for which BA request (Add or Remove) has been issued
	struct Node *AIDPointer;		// Stores the pointer of last sent AID
}
BATransaction[PROTOCOLS];			// BA Request can be issues for all the Protocols (FTP, HTTP, etc.), one for each.


struct BATransactionInLinkHOInfo
{
	char HOIdentifier;				// ManualHO=1, AutoHO=2, LinkDownHO=3, Invalid=0
	char fromLinkIndex;				// Holds indexOfIP from which Link Handover (HO) is being done
	char toLinkIndex;				// Holds indexOfIP to which Link Handover (HO) is being done
	struct Node *AIDPointer;		// Stores the pointer of last sent AID
}
BATransactionInHOLinkShift[INTERFACES], BATransactionInHOLinkDown[INTERFACES];	// This can be an Array equal to INTERFACES

////////////////////// START (Structure used in DNS Dynamic Update Code Taken from Yasir) ///////////////////////
struct DNSLocationUpdateInfo
{
	char DNSIP[40];
	char thisNodeIP[40];
	char EMFDomainName[100];
	char DNSRecordType[10];
}
DNSLocationUpdate;
////////////////////// END (Structure used in DNS Dynamic Update Code Taken from Yasir) ///////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Functions
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
// Purpose:
// To get System Time and Produde
char* getMicroSecFormattedTime()
{
	myTime = time(NULL);
	now = localtime(&myTime);
	gettimeofday(&microTime, NULL);
	
	// TimeStamp Format (hh:mm:ss:ms:us) 
	sfprintf(formattedTime, "%d:%d:%d:%d:%d",
			now->tm_hour, 						// hours
			now->tm_min, 						// minutes
			now->tm_sec, 						// seconds
			microTime.tv_usec/1000,  			// milli-seconds
			microTime.tv_usec);  				// micro-seconds
	return formattedTime;
}
*/

// Purpose:
// This function opens File descriptors for the files that will contain Logged Data.
void openTraceFiles()
{
	if ((HA_FD = fopen("HA_Trace_File.txt", "a")) == NULL)
		perror("ERROR (HA_UA.txt) ");

/*
	if ((HA_CL2I_FD = fopen("HA_CL2I.txt", "a")) == NULL)
		perror("ERROR (HA_CL2I.txt) ");

	if ((HA_AH_FD = fopen("HA_AH.txt", "a")) == NULL)
		perror("ERROR (HA_AH.txt) ");

	if ((HA_DH_FD = fopen("HA_DH.txt", "a")) == NULL)
		perror("ERROR (HA_DH.txt) ");
*/
}

////////////////////// START (DNS Dynamic Update Code Taken from Yasir) //////////////////////////////////////////////////

// Purpose:
// To Add a DNS Record in DNS

/*
	The function addDNSRecord() takes 4 arguements. 
	
	1-) The server ip address.
	2-) DNS Specific name of the node
	3-) Node's IP address
	4-) DNS Record type of the node

	The function returns -1 if the system call fails. However it is important to note that it can still 
	return 1 even if the DNS not updated. So check after adding or deleting a record from DNS. 
*/

int addDNSRecord(struct DNSLocationUpdateInfo *DNSLocationUpdate)
{
	FILE *out_file;
	char *str;

	str = calloc(120,120);
	//memset(str,'\0',120);

	strcat(str, "server ");
	strcat(str, DNSLocationUpdate->DNSIP);
	strcat(str, "\nupdate add ");
	strcat(str, DNSLocationUpdate->EMFDomainName);
	strcat(str, " 300 ");				// TTL (Time to Live)
	strcat(str, DNSLocationUpdate->DNSRecordType);
	strcat(str, " ");
	strcat(str, DNSLocationUpdate->thisNodeIP);
	strcat(str, "\nsend\n");

	if((out_file = fopen("DNSAddSessionFile.txt","w")) == NULL)
	{
		perror("\nERROR (File Open)");
		exit(0);
	}
	
	if(fprintf(out_file,"%s", str) == 0)
	{
		perror("\nERROR (File Write)");
		exit(0);
	}
	
	fclose(out_file);
	free(str);
	
	if(system("nsupdate -v DNSAddSessionFile.txt") == -1)
	{
		perror("\nERROR (nsupdate)");
		return -1;
	}

	return 1;
}

// Purpose:
// To Delete a DNS Record from DNS
/*
	The function deleteDNSRecord() takes 3 arguements. 
	
	1-) The server ip address.
	2-) DNS Specific name of the node
	3-) DNS Record type of the node

	The function returns -1 if the system call fails. However it is important to note that it can still
	return 1 even if the DNS not updated. So check after adding or deleting a record from DNS. 
*/

int deleteDNSRecord(struct DNSLocationUpdateInfo *DNSLocationUpdate)
{
	FILE *out_file;
	char *str;

	str = calloc(120,120);

	strcat(str, "server ");
	strcat(str, DNSLocationUpdate->DNSIP);
	strcat(str,"\nupdate delete ");
	strcat(str, DNSLocationUpdate->EMFDomainName);
	strcat(str," ");
	strcat(str, DNSLocationUpdate->DNSRecordType);
	strcat(str, " ");
	strcat(str, DNSLocationUpdate->thisNodeIP);
	strcat(str, "\nsend\n");

	if((out_file = fopen("DNSDeleteSessionFile.txt","w")) == NULL)
	{
		perror("\nERROR (File Open)");
		exit(0);
	}
	
	if(fprintf(out_file,"%s",str) == 0)
	{
		perror("\nERROR (File Write)");
		exit(0);
	}
	
	fclose(out_file);
	free(str);
	
	if(system("nsupdate -v DNSDeleteSessionFile.txt") == -1)
	{
		perror("\nERROR (nsupdate)");
		return -1;
	}
	
	return 1;	
}
////////////////////// END (DNS Dynamic Update Code Taken from Yasir) ///////////////////////////////////////////////////


// Prupose:
// This function initilizes variables and arrays used in HostAgent.
void initialize()
{
	char index, j;

	// DNS Related Code
	// Initialize with backSlashZero
	/*bzero(DNSLocationUpdate.DNSIP, 40);
	bzero(DNSLocationUpdate.thisNodeIP, 40);
	bzero(DNSLocationUpdate.EMFDomainName, 100);
	bzero(DNSLocationUpdate.DNSRecordType, 10);*/
	memset(DNSLocationUpdate.DNSIP,'\0',40);
	memset(DNSLocationUpdate.thisNodeIP,'\0',40);
	memset(DNSLocationUpdate.EMFDomainName,'\0',100);
	memset(DNSLocationUpdate.DNSRecordType,'\0',10);
	
	strcpy(DNSLocationUpdate.DNSIP, "192.168.1.44\0");								// To be Taken from User Agent			
	strcpy(DNSLocationUpdate.EMFDomainName, "EMFServer.newemf.com\0");				// To be Taken from User Agent
	strcpy(DNSLocationUpdate.DNSRecordType,"A\0");

	// Other Initializations
	for (index=0; index < INTERFACES; index++)
	{
		// NOTE:
		// Below Lines MUST be omitted when User Preferences will be stored Permanently.
		strcpy(userPref.adv_interfaces_name[index], ""); 		// Empty Interfaces
		
		userPref.adv_interfaces[index] = 0;						// Empty Interfvace Status
		IPAddress[index] = NULL;

		for (j=0; j < 7; j++)
		   	userPref.HOPreference[index][j] = 0;

		for (j=0; j < INTERFACES; j++)
			userPref.BAPreference[index][j] = 0;
		   
		for (j= 0; j < PROTOCOLS; j++)
			BATransaction[j].manualBAFlag = 2;					// Invalid code
		   
		addToBAIPs[index] = NULL;
		removeFromBAIPs[index] = NULL;
	}
	memset(IPNotUnderEMF,'\0',16);
	/*bzero(IPNotUnderEMF, 16);*/								// Write all '\0'
	userPref.indexOfMaxPrefHOIP[0] = -1;						// '-1' is invalid code
	userPref.indexOfMaxPrefHOIP[1] = 0;
}

// Purpose:
// To inform User Agent that indexOfMaxPrefHOIP has changed
void informUserAgent()
{
	// --------------------------------------------------------------
	// Inform User Agent
	UA_TLV.TLVType = READ_USER_PREF_FILE;
	strcpy(UA_TLV.TLVValue, "\0");
	UA_TLV.TLVLength = 0;

	if (vm_send(UA_FD, &UA_TLV, sizeof(struct TLVInfo), 0)==-1)
		perror("ERROR (READ_USER_PREF_FILE Send) ");
	// --------------------------------------------------------------
}

// Purpose:
// Read Preferences from User Preferences File maintained by UA
void readUserPreferences()
{
	FILE *emfFileWrite = NULL;
	char emfEnableDisableValue, tmpValue;

	if ((emfFileWrite = fopen("userPref.txt", "r")) == NULL)						// File does not Exist
	{
		emfFileWrite = fopen("userPref.txt", "w");									// Create userPref.txt File
		tmpValue = 1;
		fwrite(&tmpValue, 1, 1, emfFileWrite);										// Write EMF Enable Status
		fwrite (&userPref, sizeof(struct userPreferences), 1, emfFileWrite); 		// Write EMF settings, [of initialize()]
		fclose (emfFileWrite);
	}
    else																			// File Exist
    {
		fread(&emfEnableDisableValue, 1, 1, emfFileWrite);
 		fread(&userPref, sizeof(userPref), 1, emfFileWrite);
		fclose (emfFileWrite);
	}
}

// Purpose:
// This function reads the first character of userPref.txt, which is "isEMFEnabled"
char readEMFEnableStatus()
{
	FILE *readFP = NULL;
	char emfEnableDisableValue;
    if((readFP = fopen("userPref.txt", "r")) == NULL)
    	perror("\nERROR (userPref.txt) "); 
    else
    {
		fread(&emfEnableDisableValue, 1, 1, readFP);
 		fclose(readFP);
 		return emfEnableDisableValue;
	}
}

// Purpose:
// This Function is called when HostAgent makes a change in the userPref.txt file.
// This function updates User Preferences File.
void updateUserPrefFile()
{
	FILE *emfFileWrite = NULL;
	char emfEnableDisableValue, tmpValue;

	emfFileWrite = fopen("userPref.txt", "r");					// Re-open userPref.txt file, because pointer is after BOF
	fread(&emfEnableDisableValue, 1, 1, emfFileWrite);			// Read EMF Enable Status
	tmpValue = emfEnableDisableValue;							// Copy emfEnableDisable Status
	fclose (emfFileWrite);										// Close the file, because pointer is after 1st Character
	emfFileWrite = fopen("userPref.txt", "w");					// Re-open userPref.txt file, because pointer is after BOF

	tmpValue = 1;
	fwrite(&tmpValue, 1, 1, emfFileWrite);										// Write EMF Enable Status
	fwrite (&userPref, sizeof(struct userPreferences), 1, emfFileWrite); 		// Write EMF settings, [of initialize()]
	fclose (emfFileWrite);
	
	readUserPreferences();										// Re-load HA structure.
}

// Purpose:
// To Get the IP address of Highest Preference Interface from Running Interfaces in HOPreference Array.
char getIndexOfMaxPrefIPInHOPref(char indexOf_InterfaceIPRequest_IP)
{
	// NOTE: Preference value '1' is greater than Preference value '2', and so on.
	char index, maxPrefIndex, flag=1, interfaceFound=0;
	float dataRateSelected=0, costSelected=1, otherCost=0, dataRateScaled=0;
	
	for (index=0; index < INTERFACES; index++)						// Traverse BA Preference against the 'portNo'
	{
		if (index != indexOf_InterfaceIPRequest_IP)		// INTERFACE_IP_REQ sends an IP and says that its throughput is zero or near zero,
		{												// so replace it with some other IP. We exclude the sent IP by identifying its index.
		
			// Is Interface Running, is under EMF & its HO Preference > 0? 
			if ((IPAddress[index] != NULL) && (userPref.HOPreference[index][2] > 0))
			{
				if ((userPref.adv_interfaces[index] == 1) && (LDTransactionLock[index] == 0))	// Is Interface UnderEMF & Unlocked?
				{
					interfaceFound = 1;
					if (flag == 1)										// Record Preference of First Running (L3 Connected) Interface
					{
						maxPrefIndex = index;
						flag = 0;
					}
					else	// Start comparing from 2nd to Nth Running (L3 Connected) Interfaces
					{
						if (userPref.HOPreference[maxPrefIndex][2] == userPref.HOPreference[index][2])			// [2] is Preference
						{
							// Scaling TWO Subscriptions based on Cost and Data Rate
							//
							// Formula :	
							// 			dataRateScaled = (dataRateSelected/costSelected) * costScaled
							//
							if (userPref.HOPreference[maxPrefIndex][0] != userPref.HOPreference[index][0])			// Costs are different
							{
								if (userPref.HOPreference[maxPrefIndex][0] < userPref.HOPreference[index][0])		// [0] is Cost
								{
									dataRateSelected = userPref.HOPreference[maxPrefIndex][1];						// [1] is Data Rate
									costSelected = userPref.HOPreference[maxPrefIndex][0];
									otherCost = userPref.HOPreference[index][0];
								}
								else if (userPref.HOPreference[maxPrefIndex][0] > userPref.HOPreference[index][0])
								{
									dataRateSelected = userPref.HOPreference[index][1];								// [1] is Data Rate
									costSelected = userPref.HOPreference[index][0];
									otherCost = userPref.HOPreference[maxPrefIndex][0];
								}
								dataRateScaled = (dataRateSelected / costSelected) * otherCost;						// Scaling Formula
								if (dataRateScaled < userPref.HOPreference[index][1])
									maxPrefIndex = index;
								else if (dataRateScaled == userPref.HOPreference[index][1])
								{
									// Get RSS value from CL2I and then decide.
								}
							}
							else																					// Data Rates are different
							{
								if (userPref.HOPreference[maxPrefIndex][1] < userPref.HOPreference[index][1])
									maxPrefIndex = index;
							}	
						}
						else if (userPref.HOPreference[maxPrefIndex][2] > userPref.HOPreference[index][2])	// 'Pref 1' > 'Pref 2'
							maxPrefIndex = index;
					}
				}
			}
		}
	}
	if (interfaceFound == 0)			// NO IP found in HO_Preferences defined by the user
		return -1;
	else if (interfaceFound == 1)		// Max Preferred IP found in HO_Preferences defined by the user
		return maxPrefIndex;
}

// Purpose:
// To Get the IP address of interface NOT under EMF. Used in Forded Handover.
int getIndexOfInterfaceNotUnderEMF()   
{
	char index;
	for (index=0; index < INTERFACES; index++)						// Traverse BA Preference against the 'portNo'
		if (IPAddress[index] != NULL)								// Is Interface Running? 
			if (userPref.adv_interfaces[index] == 0)				// Is Interface NOT UnderEMF?
				return index;
}

// Purpose:
// Get Index of Free Location. Positions of Interfaces are Dynamic in Fixed Array. At Link Down, UserAgent
// places Empty String("") in the array to mark position as empty, so we return index of first empty location.
int getFreeIndex()
{
	char index;
	for(index=0; index < INTERFACES; index++)
		if (strcmp(userPref.adv_interfaces_name[index], "") == 0)	// Is location empty?
			return index;
	return -1;
}

// Purpose:
// Get Index of Protocol portNo passed as argument.
char getIndexFromPortNo(int portNo)
{
	// Position of Interfaces are Dynamic in Array
	char index;
	for(index=0; index < INTERFACES; index++)
		if (userPref.adv_protocol_PortNo[index] == portNo)
			return index;
	return -1;
}

// Purpose:
// Get Index of Link/Interface Name passed as argument. Position of Interfaces are Dynamic in Fixed Array.
char getIndexFromInterfaceName(char *interfaceName)
{
	char index;
	for(index=0; index < INTERFACES; index++)
		if (strcmp(userPref.adv_interfaces_name[index], interfaceName)== 0)
			return index;
	return -1;
}

// Purpose:
// Get Index of Interface IP passed as argument. Position of Interfaces are Dynamic in Array
char getIndexFromInterfaceIP(char *IP)
{
	char index;
	for(index=0; index < INTERFACES; index++)
		if (IPAddress[index]!=NULL)
			if (strcmp(IPAddress[index], IP)==0)
				return index;
	return -1;
}

// Purpose:
// This Function finds Next AID, based on Protocol portNo, and returns it.
// For this AID, addToBA and/or removeFromBA will be performed by HostAgent.
int getNextAIDInBATransaction(char portIndex)
{
	char index=0, j=0, i=0, flag=1;
	// BATransaction is a global structure
	while (BATransaction[portIndex].AIDPointer != NULL)
	{
		if (BATransaction[portIndex].AIDPointer->TRIDIndex == portIndex)				// Match Ports Indices
		{
			////////////////// New Code Inserted /////////////////////////////////
			if (BATransaction[portIndex].AIDPointer->BACount == 1)						// We check HO-to-BA Case, identify ONE link against this AID
			{
				// We check through which link this ONLY connection has been established.
				for (index=0; index<INTERFACES; index++)
				{
					if (BATransaction[portIndex].AIDPointer->connectionID[index] > 0)		// One connection Found
					{
						j = index;					// Store the index
						index = INTERFACES;			// Break the Loop
					}										
				}
				
				index=j; j=0; i=0;
				HOtoBAIPs = (char*) malloc (strlen(addToBAIPs[portIndex])+1);	// We make 'HOtoBAIPs' equal to 'addToBAIPs'
				memset(extractedIP,'\0',16);
				/*bzero(extractedIP, 16);*/											// Place all Backslash Zeros
			
				while (addToBAIPs[portIndex][j] != '\0')						// Traverse till '\0' in [IP:IP: ... :IP'\0']
				{
					if (addToBAIPs[portIndex][j] != ':')
						extractedIP[i++] = addToBAIPs[portIndex][j++];
					else
					{
						extractedIP[i] = '\0';
						i=0; j++;
						if (strcmp(extractedIP, IPAddress[index])!=0)			// We include every IP in HOtoBAIPs excluding the one placed at 'index'
						{
							if (flag==1)
							{
								strcpy(HOtoBAIPs, extractedIP);					// IP
								HOtoBAIPs[strlen(HOtoBAIPs)] = '\0';
								flag = 0;
							}
							else
							{
								strcat(HOtoBAIPs, ":");							// :
								strcat(HOtoBAIPs, extractedIP);					// IP
								HOtoBAIPs[strlen(HOtoBAIPs)] = '\0';
							}
						}
					}
				}
				extractedIP[i] = '\0';
				if (strcmp(extractedIP, IPAddress[index])!=0)					// We include every IP in HOtoBAIPs excluding the one placed at 'index'
				{
					if (flag==1)
					{
						strcpy(HOtoBAIPs, extractedIP);							// IP
						HOtoBAIPs[strlen(HOtoBAIPs)] = '\0';
						flag = 0;
					}
					else
					{
						strcat(HOtoBAIPs, ":");									// :
						strcat(HOtoBAIPs, extractedIP);							// IP
						HOtoBAIPs[strlen(HOtoBAIPs)] = '\0';
					}
				}
			}
			/////////////////////////////////////////////////////////////////////			
			return BATransaction[portIndex].AIDPointer->associationID;					// Return AID
		}
		BATransaction[portIndex].AIDPointer = BATransaction[portIndex].AIDPointer->nextAID;
	}
	return -1;	// When AIDPointer is NULL
}

// Purpose:
// This Function finds Next AID, based on Protocol portNo, and returns it.
// For this AID, removeFromBA will be performed by HostAgent.
int getNextAIDInHOTransaction(char fromLinkIndex)
{
	while (BATransactionInHOLinkShift[fromLinkIndex].AIDPointer != NULL)
	{
		if (BATransactionInHOLinkShift[fromLinkIndex].AIDPointer->connectionID[fromLinkIndex] > 0)		// Connection is Present for BA
			if (BATransactionInHOLinkShift[fromLinkIndex].AIDPointer->BACount > 1)						// More than ONE connections, BA Case
				return BATransactionInHOLinkShift[fromLinkIndex].AIDPointer->associationID;				// Return AID
			
		BATransactionInHOLinkShift[fromLinkIndex].AIDPointer = BATransactionInHOLinkShift[fromLinkIndex].AIDPointer->nextAID;
	}
	return -1;	// When AIDPointer is NULL
}

// Purpose:
// This Function finds Next AID, based on Protocol portNo, and returns it.
// For this AID, removeFromBA will be performed by HostAgent.
int getNextAIDInHOLDTransaction(char LDIndex)
{
	while (BATransactionInHOLinkDown[LDIndex].AIDPointer != NULL)
	{
		if (BATransactionInHOLinkDown[LDIndex].AIDPointer->connectionID[LDIndex] > 0)					// Connection is Present for BA
			if (BATransactionInHOLinkDown[LDIndex].AIDPointer->BACount > 1)								// More than ONE connections, BA Case
				return BATransactionInHOLinkDown[LDIndex].AIDPointer->associationID;					// Return AID
		
		BATransactionInHOLinkDown[LDIndex].AIDPointer = BATransactionInHOLinkDown[LDIndex].AIDPointer->nextAID;
	}
	return -1;	// When AIDPointer is NULL
}

// Purpose:
// This function copies current BAPreferences into prevBAPref array.
// This function is called when HostAgent receives USER_PREFERENCE_CHANGED_IND message from UserAgent.
void saveBAPreferences()
{
	char index, j;
	for (index=0; index<INTERFACES; index++)
		for (j=0; j<INTERFACES; j++)
			prevBAPref[index][j] = userPref.BAPreference[index][j];
}

// Purpose:
// This function is called when HostAgent receives USER_PREFERENCE_CHANGED_IND message from UserAgent.
// This function checks if BAPreference has changed. If it has changed, it composes IPs (AddToBA, RemoveFromBA) 
// for New BA Messages and returns '1'. It returns '0' if No IP is composed.
char composeBACommitRemoveMessage(char portIndex)
{
	char index, i, j, IPLength, flagAdd=1, flagRemove=1, returnCode=0;

	if (addToBAIPs[portIndex] != NULL)
	{
		free(addToBAIPs[portIndex]);
		addToBAIPs[portIndex] = NULL;
	}

	if (removeFromBAIPs[portIndex] != NULL) 
	{
		free(removeFromBAIPs[portIndex]);
		removeFromBAIPs[portIndex] = NULL;
	}
	
	for (j=0; j<INTERFACES; j++)
	{
		if ((prevBAPref[portIndex][j]==0) && (userPref.BAPreference[portIndex][j]==1))			// If(prevBA=0, newBA=1), case: Add To BAPreferences
		{
			if (flagAdd==1)
			{
				addToBAIPs[portIndex] = (char*) realloc (addToBAIPs[portIndex], strlen(IPAddress[j])+1);	// IP + '\0'
				strcpy(addToBAIPs[portIndex], IPAddress[j]);
				addToBAIPs[portIndex][strlen(IPAddress[j])] = '\0';
				addRemoveIPCountInBATransaction[portIndex][0] += 1;								// For each IP case, whether Add or Remove, increment it
				flagAdd = 0;
				returnCode = 1;
			}
			else if (flagAdd==0)
			{
				// :IP + '\0' + oldLength
				addToBAIPs[portIndex] = (char*) realloc (addToBAIPs[portIndex], 1+strlen(IPAddress[j])+strlen(addToBAIPs[portIndex])+1);
				strcat(addToBAIPs[portIndex], ":");
				strcat(addToBAIPs[portIndex], IPAddress[j]);
				addToBAIPs[portIndex][strlen(addToBAIPs[portIndex])] = '\0';
				addRemoveIPCountInBATransaction[portIndex][0] += 1;								// For each IP case, whether Add or Remove, increment it
			}
		}
		else if ((prevBAPref[portIndex][j]==1) && (userPref.BAPreference[portIndex][j]==0))		// if(prevBA=1, newBA=1), case: Remove From BAPreferences 
		{
			if (flagRemove==1)
			{
				removeFromBAIPs[portIndex] = (char*) realloc (removeFromBAIPs[portIndex], strlen(IPAddress[j])+1);	// '\0'
				strcpy(removeFromBAIPs[portIndex], IPAddress[j]);
				removeFromBAIPs[portIndex][strlen(IPAddress[j])] = '\0';
				addRemoveIPCountInBATransaction[portIndex][0] += 1;								// For each IP case, whether Add or Remove, increment it
				flagRemove = 0;
				returnCode = 1;
			}
			else if (flagRemove==0)
			{
				// :IP + '\0' + oldLength
				removeFromBAIPs[portIndex] = (char*) realloc (removeFromBAIPs[portIndex], 1+strlen(IPAddress[j])+strlen(removeFromBAIPs[portIndex])+1);
				strcat(removeFromBAIPs[portIndex], ":");
				strcat(removeFromBAIPs[portIndex], IPAddress[j]);
				removeFromBAIPs[portIndex][strlen(removeFromBAIPs[portIndex])] = '\0';
				addRemoveIPCountInBATransaction[portIndex][0] += 1;								// For each IP case, whether Add or Remove, increment it
			}
		}
	}
	return returnCode;
}

// Purpose:
// This function is called when LinkUp is triggered by CL2I.
// This Function checks if an interface with the name, triggered by CL2I, already exists?
// If yes index of the interface is returned otherwise -1 is returned.
char sameInterfaceExists()
{
	char index;
	for (index=0; index<INTERFACES; index++)
		if (strcmp(userPref.adv_interfaces_name[index], CL2ITrigger.interfaceName) == 0)
			return index;
	return -1;
}								


// Purpose:
// Check Protocol's Local Compliance(i.e. Is protocol/portNo/TRID under EMF as set by user?)
// If Yes, return the IP Address(es) from BAPreference. If No BAPreference is set by user,
// then get the most preferred IP address from HOPreferences.
int checkLocalCompliance(int portNo)
{
	char index;
	if (readEMFEnableStatus() == 1)
		for (index=0; index<INTERFACES; index++)
			if (userPref.adv_protocol_PortNo[index] == portNo)		// Is Protocol known to EMF?
			{
				if (userPref.adv_protocol[index] == 1)				// Check Compliance => Is Protocol under EMF? 
					return index;									// Port is EMF Compliant
				else
					return -1;										// Port is Not EMF Compliant
			}
	
	return -1;														// Port Not Found
}

// Purpose:
// Protocol Compliance has already been checked. Here, we Compose TLVValue based on EMFC_TLVTypeLength[1].
char composeNextTLVInLocalComplianceReq(char portIndex, char IPIndex, int AID)
{
	// Initially value of 2nd parameter (IPIndex) is passed as -1.
	char index, j, charIndex, lengthOfTLV=0, length=0, flag=0;
	
	if (EMFC_TLVValue != NULL)				// Free the memory, if held
	{
		//free(EMFC_TLVValue);
		//EMFC_TLVValue = NULL;
	}
	
	for (j=(IPIndex+1); j < INTERFACES; j++)							// Locate individual BAPreference
	{
		if (userPref.BAPreference[portIndex][j] == 1)					// Is interface part of BAPreference?
		{
			if (IPAddress[j] != NULL)									// Is Interface Running?
			{
				if (LDTransactionLock[j] == 0)							// Is Interface Unlocked? It means interface is NOT Down.
				{
					if (IPIndex == -1)		// Execute this code as a result of Local Complaince
					{
						lengthOfTLV = strlen(IPAddress[j]);
						EMFC_TLVValue = (char*) malloc (lengthOfTLV+1);
						
						index=-1;
						while (IPAddress[j][++index] != '\0')									// IP
							EMFC_TLVValue[index] = IPAddress[j][index];
						EMFC_TLVValue[index] = '\0';
						EMFC_TLVTypeLength[1] = lengthOfTLV;									// Assign TLVLength

						return 0;			// Send first BA IP and return. Next time send all remaining IPs seperated by ':'
					}
					else					// Execute this code as result of Add Association
					{
						if (flag == 0)
						{
							lengthOfTLV = strlen(IPAddress[j]) + 4;								// '4' for AID
							EMFC_TLVValue = (char*) realloc (EMFC_TLVValue, lengthOfTLV + 1);
							TLVUnit.intValue = AID;
							for(index=0; index<4; index++)										// AID
								EMFC_TLVValue[index] = TLVUnit.charValue[index];
					
							charIndex = 0;
							while (IPAddress[j][charIndex] != '\0')								// IP
								EMFC_TLVValue[index++] = IPAddress[j][charIndex++];
							EMFC_TLVValue[index] = '\0';
							
							flag = 1;
						}
						else		// flag == 1
						{
							length = strlen(IPAddress[j]) + 1;									// '1' for ':'
							lengthOfTLV = lengthOfTLV + length;
							EMFC_TLVValue = (char*) realloc (EMFC_TLVValue, lengthOfTLV + 1);
							
							EMFC_TLVValue[index++] = ':';										// ':'

							charIndex = 0;														// IP
							while (IPAddress[j][charIndex] != '\0')
								EMFC_TLVValue[index++] = IPAddress[j][charIndex++];
							EMFC_TLVValue[index] = '\0';
						}
					}
				}
			}
		}
	}

	if (flag == 1)						// It means, One or More IPs are found and will be sent to EMF Core
	{									// as a respopnse of ADD_ASSSOCIATION message
		EMFC_TLVTypeLength[1] = lengthOfTLV;					// Assign TLVLength
		return 0;
	}
	else if (flag == 0)											// BA Preference Not found
	{
		j = getIndexOfMaxPrefIPInHOPref(-1);					// IndexOf Running & Unlocked MaxPreferredIP is returned
		if (j==-1)
			return -1;
		else
		{
			EMFC_TLVValue = (char*) malloc (strlen(IPAddress[j]));
			memset(EMFC_TLVValue,'\0',strlen(IPAddress[j]));
			/*bzero(EMFC_TLVValue, strlen(IPAddress[j]));*/
			strcpy(EMFC_TLVValue, IPAddress[j]);					// Copy IP Address
			EMFC_TLVValue[strlen(IPAddress[j])] = '\0';
			EMFC_TLVTypeLength[1] = strlen(IPAddress[j]);
			
			userPref.indexOfMaxPrefHOIP[0] = j;
			userPref.indexOfMaxPrefHOIP[1] = 0;						// No Connection Present right now
			
			// Map changes to User Preference File
			updateUserPrefFile();
			informUserAgent();
			
			return 1;
		}
	}
}

// Purpose:
// This function composes HO_TRAFFIC_SHIFT_COMMIT TLV.
void composeHOTrafficShiftTLV(char fromIPIndex, char toIPIndex)
{
	// NOTE:
	// TLVUnit is assigned PortNo(TRID) before the call of this function.
	char index, j;
	
	if (EMFC_TLVValue != NULL)				// Free the memory, if held
	{
		//free(EMFC_TLVValue);
		//EMFC_TLVValue = NULL;
	}

	EMFC_TLVTypeLength[1] = strlen(IPAddress[UA_TLV.TLVValue[1]]) + strlen(IPAddress[UA_TLV.TLVValue[2]])+ 5;	// 4Bytes for TRID, 1Byte for '\0'
	EMFC_TLVValue = (char*) malloc(EMFC_TLVTypeLength[1]);

	for(index=0; index<4; index++)
		EMFC_TLVValue[index] = TLVUnit.charValue[index];				// Compose TRID
	
	j=0;
	while (IPAddress[fromIPIndex][j] != '\0')
		EMFC_TLVValue[index++] = IPAddress[fromIPIndex][j++];			// Compose fromIP
		
	EMFC_TLVValue[index++] = ':';										// ':'
	
	j=0;
	while (IPAddress[toIPIndex][j] != '\0')								
		EMFC_TLVValue[index++] = IPAddress[toIPIndex][j++];				// Compose toIP
	EMFC_TLVValue[index] = '\0';
}

// Purpose:
// This function composes BA_COMMIT TLV
void composeBACommitTLV(char portIndex)
{
	// NOTE:
	// TLVUnit contains AID value, which we have stored in it before calling this function.
	
	char index, j=0, fromIndex;

	if (EMFC_TLVValue != NULL) 
	{
		//free(EMFC_TLVValue);						// Free the memory, if held
		//EMFC_TLVValue = NULL;
	}

	if (BATransaction[portIndex].manualBAFlag == 1)						// case: Manual BA
	{
		fromIndex = BATransaction[portIndex].fromLinkIndex;
		EMFC_TLVTypeLength[1] = strlen(IPAddress[fromIndex]) + 5;			// 5Bytes (4Bytes for AID + 1Byte for '\0')
		EMFC_TLVValue = (char*) malloc (EMFC_TLVTypeLength[1]);

		for(index=0; index<4; index++)										// Compose AID or TRID
			EMFC_TLVValue[index] = TLVUnit.charValue[index];

		while (IPAddress[fromIndex][j] != '\0')								// Compose IP
			EMFC_TLVValue[index++] = IPAddress[fromIndex][j++];

		EMFC_TLVValue[index] = '\0';
	}	
	else if (BATransaction[portIndex].manualBAFlag == 0)				// case: Auto BA
	{
		if (HOtoBAIPs == NULL)
			EMFC_TLVTypeLength[1] = strlen(addToBAIPs[portIndex]) + 5;			// 5Bytes (4Bytes for AID + 1Byte for '\0')
		else
			EMFC_TLVTypeLength[1] = strlen(HOtoBAIPs) + 5;						// 5Bytes (4Bytes for AID + 1Byte for '\0')
		
		EMFC_TLVValue = (char*) malloc (EMFC_TLVTypeLength[1]);

		for(index=0; index<4; index++)										// Compose AID or TRID
			EMFC_TLVValue[index] = TLVUnit.charValue[index];

		j=0;
		if (HOtoBAIPs != NULL)												// It means we are dealing, HO-to-BA case.
		{
			while (HOtoBAIPs[j] != '\0')									// Compose HOtoBAIP
				EMFC_TLVValue[index++] = HOtoBAIPs[j++];
		}
		else
		{
			while (addToBAIPs[portIndex][j] != '\0')						// Compose addToBAIPs
				EMFC_TLVValue[index++] = addToBAIPs[portIndex][j++];
		}
		EMFC_TLVValue[index] = '\0';
	}
}

// Purpose:
// This function composes AIDwise_REMOVE_FROM_BA TLV in AutoBA case.
void composeRemoveFromBATLV(char portIndex)
{
	char index=0, j=0;
	
	if (EMFC_TLVValue != NULL)				// Free the memory, if held
	{
		//free(EMFC_TLVValue);
		//EMFC_TLVValue = NULL;
	}

	EMFC_TLVTypeLength[1] = strlen(removeFromBAIPs[portIndex]) + 5;			// 5Bytes (4Bytes for AID + 1Byte for '\0')
	EMFC_TLVValue = (char*) malloc (EMFC_TLVTypeLength[1]);	

	for(index=0; index<4; index++)											// Compose AID
		EMFC_TLVValue[index] = TLVUnit.charValue[index];

	while (removeFromBAIPs[portIndex][j] != '\0')							// Compose removeIP
		EMFC_TLVValue[index++] = removeFromBAIPs[portIndex][j++];
	EMFC_TLVValue[index] = '\0';
}

// Purpose:
// This function composes AIDwise_REMOVE_FROM_BA TLV in HO_LINK_SHIFT case.
void composeRemoveFromBATLVInHO(char fromLinkIndex)
{
	char index=0, j=0;
	
	if (EMFC_TLVValue != NULL)				// Free the memory, if held
	{
		//free(EMFC_TLVValue);
		//EMFC_TLVValue = NULL;
	}

	EMFC_TLVTypeLength[1] = strlen(IPAddress[fromLinkIndex]) + 5;		// 5Bytes (4Bytes for AID + 1Byte for '\0')
	EMFC_TLVValue = (char*) malloc (EMFC_TLVTypeLength[1]);	
	
	TLVUnit.intValue = BATransactionInHOLinkShift[fromLinkIndex].AIDPointer->associationID;
	for(index=0; index<4; index++)										// Compose AID
		EMFC_TLVValue[index] = TLVUnit.charValue[index];
	
	while (IPAddress[fromLinkIndex][j] != '\0')							// Compose removeIP
		EMFC_TLVValue[index++] = IPAddress[fromLinkIndex][j++];
	EMFC_TLVValue[index] = '\0';
}

// Purpose:
// This function composes AIDwise_REMOVE_FROM_BA TLV in LinkDown case.
void LD_ComposeRemoveFromBATLVInHO(char LDIndex)
{
	char index=0, j=0;
	
	if (EMFC_TLVValue != NULL)				// Free the memory, if held
	{
		//free(EMFC_TLVValue);
		//EMFC_TLVValue = NULL;
	}

	EMFC_TLVTypeLength[1] = strlen(IPAddress[LDIndex]) + 5;				// 5Bytes (4Bytes for AID + 1Byte for '\0')
	EMFC_TLVValue = (char*) malloc (EMFC_TLVTypeLength[1]);
	
	
	TLVUnit.intValue = BATransactionInHOLinkDown[LDIndex].AIDPointer->associationID;
	for(index=0; index<4; index++)										// Compose AID
		EMFC_TLVValue[index] = TLVUnit.charValue[index];

	while(IPAddress[LDIndex][j] != '\0')								// Compose LD_removeIP
		EMFC_TLVValue[index++] = IPAddress[LDIndex][j++];
	EMFC_TLVValue[index] = '\0';
}

// Purpose:
// This function parses BAComplete TLV.
// This Function is called when BA_COMPLETE message is received from EMFCore.
int parseBACompleteTLV(char *TLVValue, int *AID, int *CID, char *IP)
{
	char index, j;
	
	for(j=0, index=0; index<4; j++, index++)		// Extract AID
		 TLVUnit.charValue[j] = TLVValue[index];
	*AID = TLVUnit.intValue;
	if (*AID <= 0) return -1;
	
	for(j=0; index<8; index++, j++)					// Extract CID
		TLVUnit.charValue[j] = TLVValue[index]; 
	*CID = TLVUnit.intValue;
	if (*CID <= 0) return -1;

	j=0;
	while (TLVValue[index] != '\0')					// Extract IP
		IP[j++] = TLVValue[index++];
	IP[j] = '\0';

	return 0;
}

// Purpose:
// This function is called during Auto BA Transaction. All messages sent to EMF Core
// regarding Auto BA Transaction are sent from here. Since the code inside function was being
// used in two cases, i.e. BA_COMPLETE and REMOVE_CONNECTION, therefore, I put the code in this function.
void autoBATransaction(char portIndex)
{
	if (addRemoveIPCountInBATransaction[portIndex][1] == 0)		// Does all messages, whether add or remove, have been processed? 
	{
		BATransaction[portIndex].AIDPointer = BATransaction[portIndex].AIDPointer->nextAID;		// Increment AID
		TLVUnit.intValue = getNextAIDInBATransaction(portIndex);								// Get next AID based on Protocol portNo

		if (TLVUnit.intValue == -1)								// Auto BA Transaction is Complete
		{
			addRemoveIPCountInBATransaction[portIndex][0] = 0;
			BATransaction[portIndex].manualBAFlag = 2;				// Invalid Code

			if (userPref.generalEMFoptions[2] == 1)					// Is "Auto Handover_Complete_Popup" checked?
			{
				UA_TLV.TLVType = BA_COMMIT_CONF;
				
				// New Code
				strcpy(UA_TLV.TLVValue, " Auto BA complete.\0");
				
				// Old Code
				//strcpy(UA_TLV.TLVValue, userPref.adv_interfaces_name[BATransaction[portIndex].fromLinkIndex]);		// IP
				//strcat(UA_TLV.TLVValue, " Auto BA complete is now part of Aggregation for ");
				//strcat(UA_TLV.TLVValue, userPref.adv_protocol_name[portIndex]);	   									// Port
			
				if (vm_send(UA_FD, &UA_TLV, sizeof(struct TLVInfo), 0) == -1)
					perror("ERROR (BA_COMPLETE Send) ");
				else
				{
/*****/				//fprintf(HA_UA_FD, "%s, (AUTO_BA_COMPLETE) TLVValue SENT TO UserAgent is (%s).\n\n", 
																					//getMicroSecFormattedTime(), UA_TLV.TLVValue);
					//getMicroSecFormattedTime();
					fprintf(HA_FD, "HA : (AUTO_BA_COMPLETE) TLVValue SENT TO UserAgent is (%s).\n\n", UA_TLV.TLVValue);
				}
			}

			addToBAMessageFlag[portIndex] = removeFromBAMessageFlag[portIndex] = 0;		// Clean-up: Clear AddToBA & RemoveFromBA Flag
		}
		else
		{
			// Copy index[0] value to index[1], so that we can process index[1] value and keep index[0] value safe
			addRemoveIPCountInBATransaction[portIndex][1] = addRemoveIPCountInBATransaction[portIndex][0]; 		// For next AID, Copy [0] to [1]
			addToBAMessageFlag[portIndex] = removeFromBAMessageFlag[portIndex] = 0;								// For next AID, Clear it
	
			if (addToBAIPs[portIndex] != NULL)
			{
				addToBAMessageFlag[portIndex] = 1;							// User Added Preference for this portNo
				EMFC_TLVTypeLength[0] = BA_COMMIT;
				composeBACommitTLV(portIndex);

				if (vm_send(EMFC_FD, &EMFC_TLVTypeLength, 2, 0)==-1)
					perror("ERROR (autoBATransaction() Send) ");
				else
				{
					if (vm_send(EMFC_FD, EMFC_TLVValue, EMFC_TLVTypeLength[1], 0)==-1)
						perror("ERROR (autoBATransaction() Send) ");
					else
					{
						fprintf(HA_FD, "HA : (Auto_BA_Transaction) TLVValue SENT TO Core is AID(%d), addIP(%s).\n\n", 
																				TLVUnit.intValue, addToBAIPs[portIndex]);
					}
				}
			}
		
			if (removeFromBAIPs[portIndex] != NULL)
			{
				removeFromBAMessageFlag[portIndex] = 1;
				EMFC_TLVTypeLength[0] = AIDwise_REMOVE_FROM_BA_COMMIT;
				composeRemoveFromBATLV(portIndex);
	
				if (vm_send(EMFC_FD, &EMFC_TLVTypeLength, 2, 0)==-1)
					perror("ERROR (autoBATransaction() Send) ");
				else
				{
					if (vm_send(EMFC_FD, EMFC_TLVValue, EMFC_TLVTypeLength[1], 0) == -1)
						perror("ERROR (autoBATransaction() Send) ");
					else
					{
						fprintf(HA_FD, "HA : (Auto_BA_Transaction) TLVValue SENT TO Core is AID(%d), removeIP(%s).\n\n",
																				 TLVUnit.intValue, removeFromBAIPs[portIndex]);
					}
				}
			}
		}
	}
}


// Purpose:
// To Send HO_LINK_SHIFT_COMMIT in two cases: case HO_LINK_SHIFT_COMMIT and case USER_PREF_CHANGED_IND
void HOLinkShiftCommit(char fromHOIndex, char toHOIndex)
{
	if (EMFC_TLVValue != NULL)				// Free the memory, if held
	{
		//free(EMFC_TLVValue);
		//EMFC_TLVValue = NULL;
	}

	// Step 1.
	if (HOLinkShiftCompleteFlag[fromHOIndex] == 0)						// HO Processing can be done.
	{
		HOLinkShiftCompleteFlag[fromHOIndex] = 1;						// Set it to Zero when HO Complete

		//fprintf(HA_FD, "2: IPAddress[fromHOIndex] = %s, HOLinkStatus.isActive[fromHOIndex][0] = %d.\n", IPAddress[fromHOIndex], HOLinkStatus.isActive[fromHOIndex][0]);
		
		if (HOLinkStatus.isActive[fromHOIndex][0] == 1)					// Is fromHOLink Active?
		{
			// Compose & Send Generic HO Command
			EMFC_TLVTypeLength[0] = HO_LINK_SHIFT_COMMIT;
	
			EMFC_TLVTypeLength[1] = strlen(IPAddress[fromHOIndex]) + strlen(IPAddress[toHOIndex]) + 2;
			EMFC_TLVValue = (char*) malloc (EMFC_TLVTypeLength[1]);
			strcpy(EMFC_TLVValue, IPAddress[fromHOIndex]);				// From IP Address
			strcat(EMFC_TLVValue, ":");									// ':' is IP Seperator
			strcat(EMFC_TLVValue, IPAddress[toHOIndex]);				// To IP Address
			EMFC_TLVValue[strlen(EMFC_TLVValue)] = '\0';
	
			if (vm_send(EMFC_FD, &EMFC_TLVTypeLength, 2, 0) == -1)
				perror("ERROR (HO_LINK_SHIFT_COMMIT_REQ Send) ");
			else
			{
				if (vm_send(EMFC_FD, EMFC_TLVValue, EMFC_TLVTypeLength[1], 0) == -1)
					perror("ERROR (HO_LINK_SHIFT_COMMIT_REQ Send) ");
				else
				{
					fprintf(HA_FD, "HA : (HO_LINK_SHIFT_COMMIT) TLVValue SENT TO Core is %s.\n", EMFC_TLVValue);
				}
				// NOTE:
				// HA will receive EMF Core's per Association reply under "case HANDOVER"
				// and Handover Trasaction Complete under "case HO_LINK_SHIFT_COMPLETE" in EMF Core portion of Host Agent.
				// Host Agent will send "Handover Complete" to User Agent from HO_LINK_SHIFT_COMPLETE.
			}
		}

		// Step 2. Process AIDwise_Remove_From_BA
		BATransactionInHOLinkShift[fromHOIndex].AIDPointer = connectionInfoHead;		// One Time Assignment, at start
	
		BATransactionInHOLinkShift[fromHOIndex].fromLinkIndex = fromHOIndex;			// Copy HO fromLinkIndex
		BATransactionInHOLinkShift[fromHOIndex].toLinkIndex = toHOIndex;				// Copy HO toLinkIndex
		TLVUnit.intValue = getNextAIDInHOTransaction(fromHOIndex);
	
		if (TLVUnit.intValue == -1)									// No Connection is available for removeFromBAInHO
		{
			//fprintf(HA_FD, "Noting for removeFromBAInHO.\n");
		}
		else
		{
			BATransactionInHOLinkShift[fromHOIndex].HOIdentifier = 1;		// ManualHO=1, AutoHO=2, LinkDownHO=3, Nothing=0
			EMFC_TLVTypeLength[0] = AIDwise_REMOVE_FROM_BA_COMMIT;
			composeRemoveFromBATLVInHO(fromHOIndex);
		
			if (vm_send(EMFC_FD, &EMFC_TLVTypeLength, 2, 0)==-1)
				perror("ERROR (HO_LINK_SHIFT_COMMIT_REQ Send) ");
			else
			{
				if (vm_send(EMFC_FD, EMFC_TLVValue, EMFC_TLVTypeLength[1], 0) == -1)
					perror("ERROR (HO_LINK_SHIFT_COMMIT_REQ Send) ");
				else
				{
					fprintf(HA_FD, "HA : (HO_BA_REMOVE) TLVValue SENT TO Core is AID(%d), IP(%s).\n\n", TLVUnit.intValue, IPAddress[fromHOIndex]);
				}
			}
		}
	}
}


// Purpose:
// Parse HO_Complete TLV
void parseHOCompleteTLV(char *TLVValue, char portFlag, char *fromIP, char *toIP)
{
	char index=0, j;

	if (portFlag == 1)								// If Port is part of TLVValue, Skip it.
		for (; index<4; index++)
			TLVUnit.charValue[index] = TLVValue[index];
		
	j=0;
	while (TLVValue[index] != ':')					// Extract 1st IP Address
		fromIP[j++] = TLVValue[index++];
	fromIP[j] = '\0'; 
	
	j=0;
	while (TLVValue[++index] != '\0')				// Extract 2nd IP Address
		toIP[j++] = TLVValue[index];
	toIP[j] = '\0';
}

/*
// Purpose:
// Compose HO_LINK_SHIFT TLV
void composeHOTLV(char *TLVValue, char *IP[], int *AID, char LDIndex, char maxPrefIndex)
{
	char index, j;
	TLVUnit.intValue = *AID;
	for (index=0; index<4; index++)
		TLVValue[index] = TLVUnit.charValue[index];		// Compose AID

	j=0;
	while (IP[LDIndex][j] != '\0')
		TLVValue[index++] = IP[LDIndex][j++];			// Compose fromLink => LinkDown IP

	j=0;
	while (IP[maxPrefIndex][j] != '\0')
		TLVValue[index++] = IP[maxPrefIndex][j++];		// Compose toLink => MaxPreferred IP
	TLVValue[index] = '\0';
}
*/

// Purpose:
// Parse Handover TLV
char parseHOTLV(char *TLVValue, int *AID, int *oldCID, int *newCID, char *IP)
{
	char index, j;
	
	for(j=0, index=0; index<4; j++, index++)		// Extract AID
		 TLVUnit.charValue[j] = TLVValue[index];
	*AID = TLVUnit.intValue;
	if (*AID <= 0) return -1;
	
	for(j=0; index<8; index++, j++)					// Extract oldCID
		TLVUnit.charValue[j] = TLVValue[index];
	*oldCID = TLVUnit.intValue;
	if (*oldCID <= 0) return -1;

	for(j=0; index<12; index++, j++)				// Extract newCID
		TLVUnit.charValue[j] = TLVValue[index];
	*newCID = TLVUnit.intValue;
	if (*newCID <= 0) return -1;

	j=0;
	while (TLVValue[index] != '\0')					// Extract IP
		IP[j++] = TLVValue[index++];
	IP[j] = '\0';

	return 0;
}

// Purpose:
// Parse Connection TLVValue
char parseConnectionTLV(char *TLVValue, int *AID, int *TRID, int *CID, char *IP)
{
	char index, j;
	
	// NOTE:
	// In each of the following cases, TLVValue contains AID so we extract 
	// it here.
	for(j=0, index=0; index<4; j++, index++)			// Extract AID
		 TLVUnit.charValue[j] = TLVValue[index];
	*AID = TLVUnit.intValue;
	if (*AID <= 0) return -1;
	
	// Case 1 (Add Association):
	// TLVValue contains AID, TRID, CID, & toIP. AID is already Parsed.
	if (*TRID==1)
	{
		for(j=0; index<8; index++, j++)					// Extract TRID
			TLVUnit.charValue[j] = TLVValue[index];
		*TRID = TLVUnit.intValue;
	
		for(j=0; index<12; index++, j++)				// Extract CID
			TLVUnit.charValue[j] = TLVValue[index];
		*CID = TLVUnit.intValue;

		j=0;
		while (TLVValue[index] != '\0')					// Extract IP
			IP[j++] = TLVValue[index++];
		IP[j] = '\0';

		if (*TRID <= 0) return -1; 
		else return 0;
	}

	// Case 2 (Remove Connection): 
	// TLVValue contains AID, CID & fromIP. AID is already Parsed.
	else if (*TRID==0 && *CID==0)
	{
		for(j=0; index<8; index++, j++)					// Extract CID
			TLVUnit.charValue[j] = TLVValue[index];
		*CID = TLVUnit.intValue;

		if (*CID <= 0)  return -1;
		else return 0;
	}

	// Case 3 (Add Connection): 
	// TLVValue contains AID, CID & toIP AID is already Parsed
	else if (*TRID==0 && *CID==1)
	{
		for(j=0; index<8; index++, j++)					// Extract CID
			TLVUnit.charValue[j] = TLVValue[index];
		*CID = TLVUnit.intValue;

		j=0;
		while (TLVValue[index] != '\0')					// Extract IP
			IP[j++] = TLVValue[index++];
		IP[j] = '\0';

		if (*CID <= 0)  return -1;
		else return 0;
	}

	// Case 4 (Interface IP Request): 
	// TLVValue contains AID & IP. AID is already Parsed
	else if (*TRID==-1)
	{
		j=0;
		while (TLVValue[index] != '\0')					// Extract IP
			IP[j++] = TLVValue[index++];
		IP[j] = '\0';
		
		return 0;
	}
 }

// Purpose:
// This function is called to Identify, removeFromBA Source i.e. in the past,
// either AutoBA, HOLinkShift or HOLinkDown sent it.
char getRemoveFromBASource(int AID, int CID, int *removeFromBACode)
{
	char index, portIndex, returnIndex=-1;
	
	// First we check if the message is response of Auto removeFromBA
	for (portIndex=0; portIndex<PROTOCOLS; portIndex++)
	{
		if (BATransaction[portIndex].manualBAFlag == 0)			// Auto BA
		{
			if (BATransaction[portIndex].AIDPointer->associationID == AID)
			{
				*removeFromBACode = 1;								// Auto BA Remove
				returnIndex = portIndex;
				portIndex = PROTOCOLS;								// Break the Loop
			}
		}
	}

	if (returnIndex == -1)		// Request was NOT from Auto BA case, so it is either from HOLinkDown or HOLinkShift
	{
		for (index=0; index<INTERFACES; index++)
		{
			if (BATransactionInHOLinkDown[index].HOIdentifier == 3)				// Case: HOLinkDown
			{
				if ((BATransactionInHOLinkDown[index].AIDPointer->associationID == AID) &&
					(BATransactionInHOLinkDown[index].AIDPointer->connectionID[index] == CID))
				{
					*removeFromBACode = 2;						// HOLinkDown
					returnIndex = index;
					index = INTERFACES;
				}
			}
			else if (BATransactionInHOLinkShift[index].HOIdentifier == 1)		// Case: HOLinkShift
			{
				if ((BATransactionInHOLinkShift[index].AIDPointer->associationID == AID) &&
					(BATransactionInHOLinkShift[index].AIDPointer->connectionID[index] == CID))
				{
					*removeFromBACode = 3;						// HOLinkShift
					returnIndex = index;
					index = INTERFACES;
				}
			}
		}
	}
	
	return returnIndex;
}

// Purpose:
// This function checks, HO_LINK_SHIFT_COMPLETE source. Since HO_LINK_SHIFT_COMMIT is sent
// in TWO cases, i.e. in HO_LINK_SHIFT_REQ and in LINK_DOWN, so we have to identify whether
// HOLinkShiftComplete message is from HO_LINK_SHIFT_REQ or LINK_DOWN cases.
char getHOLinkShiftCompleteSource(char fromIPIndex, char toIPIndex, int *HOCompleteSource)
{
	char returnIndex = -1, index;
	for (index=0; index<INTERFACES; index++)
	{
		if ((BATransactionInHOLinkDown[index].fromLinkIndex == fromIPIndex) && (BATransactionInHOLinkDown[index].toLinkIndex == toIPIndex))
		{
			*HOCompleteSource = 1;				// LinkDown HOComplete
			returnIndex = index;
			index = INTERFACES;
		}
		else if ((BATransactionInHOLinkShift[index].fromLinkIndex == fromIPIndex) && (BATransactionInHOLinkShift[index].toLinkIndex == toIPIndex))
		{
			*HOCompleteSource = 2;				// Manual HOComplete
			returnIndex = index;
			index = INTERFACES;
		}
	}
	return returnIndex;
}

// Purpose:
// This function displays HostAgent status in terms of Handover and BA.
void displayAll()
{
	char index, j=0;
	fprintf(HA_FD, "\n--------------------------------------------------------------------------------------\n");
	fprintf(HA_FD, "\n Interfaces & IPs\t\t\t\t[");
	for (index=0; index<INTERFACES; index++)
	{
		if (strlen(userPref.adv_interfaces_name[index]) > 0)
		{
			if (j == 0)
			{
				fprintf(HA_FD, "(%s : %s)", userPref.adv_interfaces_name[index], IPAddress[index]);
				j=1;
			}
			else
				fprintf(HA_FD, ", (%s : %s)", userPref.adv_interfaces_name[index], IPAddress[index]);
		}
	}
	fprintf(HA_FD, "]\n");


	j = 0;
	fprintf(HA_FD, "\n EMF_Compliance_of_Interfaces   [");
	for (index=0; index<INTERFACES; index++)
	{
		if (j==0)
		{
			fprintf(HA_FD, "%d", userPref.adv_interfaces[index]);
			j = 1;
		}
		else
			fprintf(HA_FD, " %d", userPref.adv_interfaces[index]);
	}
	fprintf(HA_FD, "]\n");


	j=0;
	fprintf(HA_FD, "\n HO_Preferences\t\t\t\t\t[");
	for (index=0; index<INTERFACES; index++)
	{
		if (strlen(userPref.adv_interfaces_name[index]) > 0)
		{
			if (j==0)
			{
				fprintf(HA_FD, "(%.0f, ", userPref.HOPreference[index][2]);
				fprintf(HA_FD, "%.2f, ", userPref.HOPreference[index][0]);
				fprintf(HA_FD, "%.0f)", userPref.HOPreference[index][1]);
				j=1;
			}
			else
			{
				fprintf(HA_FD, ", (%.0f, ", userPref.HOPreference[index][2]);
				fprintf(HA_FD, "%.2f, ", userPref.HOPreference[index][0]);
				fprintf(HA_FD, "%.0f)", userPref.HOPreference[index][1]);
			}
		}
	}
	fprintf(HA_FD, "]\n");


	j=0;
	fprintf(HA_FD, "\n Protocols\t\t\t\t\t\t[");
	for (index=0; index<INTERFACES; index++)
		if (strlen(userPref.adv_protocol_name[index]) > 0)
		{
			if (j==0)
			{
				fprintf(HA_FD, "%s", userPref.adv_protocol_name[index]);
				j=1;
			}
			else
				fprintf(HA_FD, ", %s", userPref.adv_protocol_name[index]);
		}
		fprintf(HA_FD, "]\n");


	j = 0;
	fprintf(HA_FD, "\n EMF_Compliance_of_Protocols\t[");
	for (index=0; index<INTERFACES; index++)
	{
		if (j==0)
		{
			fprintf(HA_FD, "%d", userPref.adv_protocol[index]);
			j = 1;
		}
		else
			fprintf(HA_FD, " %d", userPref.adv_protocol[index]);
	}
	fprintf(HA_FD, "]\n");


	fprintf(HA_FD, "\n BA_Preferences");
	for (index=0; index<INTERFACES; index++)
	{
		if (index==0)
		{
			fprintf(HA_FD, "\t\t\t\t\t[");
			for (j=0; j<INTERFACES; j++)
			{
				if (j==0)
					fprintf(HA_FD, "%d", userPref.BAPreference[index][j]);
				else
					fprintf(HA_FD, " %d", userPref.BAPreference[index][j]);
			}
			fprintf(HA_FD, "]\n");
		}
		else if (index>0 && index<3)
		{
			fprintf(HA_FD, "\t\t\t\t\t\t\t\t[");
			for (j=0; j<INTERFACES; j++)
			{
				if (j==0)
					fprintf(HA_FD, "%d", userPref.BAPreference[index][j]);
				else
					fprintf(HA_FD, " %d", userPref.BAPreference[index][j]);
			}
			fprintf(HA_FD, "]\n");
		}
	}
	
	fprintf(HA_FD,  "\n--------------------------------------------------------------------------------------\n");
}


////////////////////////////////////////////////////////////////////
//
// Main Function
//
////////////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{
	// Variable Declarations
	fd_set baseFDList, tmpFDList;
	int maxFDValue, on=1;
	struct sockaddr_in serverAddress, clientAddress;
	char index=0, i, j=0, fromIndex=0, toIndex=0, flag=0, portIndex=0, IPIndex=0, IPLength=0;
	char tmp[5], fromIP[16], toIP[16], localComlpianceReq[PROTOCOLS];
	char HOFromIP[16], HOToIP[16], length=0;
	int AID=0, TRID=0, TRIDIndex=0, CID=0, newCID=0, fromHOIndex=0, removeFromBASource=0, HOCompleteSource=0;
//	pthread_t addDNSRecordThread, deleteDNSRecordThread;
	
    openTraceFiles();
	readUserPreferences();
	initialize();
	updateUserPrefFile();
	displayAll();

	// Create Socket 
	if ((socketFD=vm_socket(AF_INET, SOCK_STREAM, 0))== -1)
	{
		perror("\nERROR (Socket) ");
		exit(0);
	}	

	// 2. Bind the Server with Port
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
	serverAddress.sin_port = htons(HOSTAGENT_PORT);
	memset(serverAddress.sin_zero, '\0', sizeof serverAddress.sin_zero);

	int opt=1;
	vm_setsockopt(socketFD, SOL_SOCKET, SO_REUSEPORT, (char *)&opt, sizeof(opt));
	vm_setsockopt(socketFD, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));
	
	if (vm_bind(socketFD, (struct sockaddr*) &serverAddress, sizeof(serverAddress))==-1)
	{
		perror("\nERROR (Bind) ");
		exit(0);
	}

	// 3. Listen on the Sockets for UA, EMF Core (EC), CL2I
	if (vm_listen(socketFD, queueSize)==-1)
	{
		perror("\nERROR (Listen) ");
		exit(0);	
	}
	else
		fprintf(HA_FD, "\n Host Agent is Listening...\n");
	
	FD_ZERO(&baseFDList);
	FD_ZERO(&tmpFDList);
	
	FD_SET(socketFD, &baseFDList);	// Add socketFD to baseFDList
	maxFDValue = socketFD;
	while(1)
	{
		tmpFDList = baseFDList;	// To keep baseFDList away from 'select'
	
		select(maxFDValue+1, &tmpFDList, NULL, NULL, NULL);

		// When Select Returns, perform following code
		if (FD_ISSET(socketFD, &tmpFDList))		// Connection is requested at Listening Socket
		{
			clientSize = sizeof(clientAddress);
			if ((acceptFD = vm_accept(socketFD, (struct sockaddr*)&clientAddress, &clientSize))==-1)
				perror("\nERROR (Accept) ");
			else								// No Error, Add the Client to baseFDList
			{
				FD_SET(acceptFD, &baseFDList);
				if (acceptFD > maxFDValue)
					maxFDValue = acceptFD;

				// Who is the Sender (UA, EMFCore, or CL2I???)
				if (ntohs(clientAddress.sin_port)== USERAGENT_PORT)	// User Agent
				{
					UA_FD = acceptFD;
					fprintf(HA_FD, "******* Connection Established with User Agent *******\n");
				}
				else if (ntohs(clientAddress.sin_port)== COREAGENT_PORT)	// EMF Core
				{
					EMFC_FD = acceptFD;
					fprintf(HA_FD, "******* Connection Established with EMF Core *******\n");
				}

				else if (ntohs(clientAddress.sin_port)== CL2IAGENT_PORT)		// CL2I
				{
					CL2I_FD = acceptFD;
					fprintf(HA_FD, "******* Connection Established with CL2I *******\n");
				}
			}
		}
		else	// Non Listening Socket Sensed something
		{
			if (FD_ISSET(UA_FD, &tmpFDList))
			{
				bytesReceived = vm_recv(UA_FD, &UA_TLV, sizeof(struct TLVInfo), 0);
				if (bytesReceived ==-1)
					perror("\nERROR (UserAgent Receive) ");
				
				else if (bytesReceived == 0)	// TCP Connection Closed
				{
					// Host Agent will exit gracefully, because User Agent has been Closed.
					fprintf(HA_FD, "\n******* Connection Closed by User Agent *******\n");
			
					fprintf(HA_FD, "\nHost Agent will Exit in 1 Second...\n");
					//sleep(1);
					FD_CLR(UA_FD, &baseFDList);
					FD_CLR(EMFC_FD, &baseFDList);
					FD_CLR(CL2I_FD, &baseFDList);
					
					// Close 'Socket Descriptors'
					vm_close(UA_FD);					// UA
					vm_close(EMFC_FD);					// Core
					vm_close(CL2I_FD);					// CL2I
					vm_close(socketFD);					// socket
					vm_close(acceptFD);					// accept
					
					// Clean-up Code
					if (EMFC_TLVValue != NULL)
					{
						//free(EMFC_TLVValue);
						//EMFC_TLVValue = NULL;
					}
					
					if (addToBAIPs[portIndex] != NULL) 
					{
						free(addToBAIPs[portIndex]);
						addToBAIPs[portIndex] = NULL;
					}
					
					if (removeFromBAIPs[portIndex] != NULL)
					{
						free(removeFromBAIPs[portIndex]);
						removeFromBAIPs[portIndex] = NULL;
					}

					for (index=0; index<INTERFACES; index++)
						if (IPAddress[index] != NULL)
						{
							free(IPAddress[index]);
							IPAddress[index] = NULL;
						}
					
					// Close Log File
					fclose(HA_FD);					// HostAgent
					//fclose(HA_AH_FD);				// HostAgent-AssociationHandler
					//fclose(HA_DH_FD);				// HostAgent-DataHandler
					//fclose(HA_CL2I_FD);			// HostAgent-CL2I
					vm_exit();
					return 0;
				}
				else  // Data is available
				{
					switch(UA_TLV.TLVType)					// Process Request based on TLV Type
					{
						case HO_LINK_SHIFT_COMMIT_REQ:		// Perform Link Shift Handover
				
							// TLVValue received is
							// 2 Bytes (indexOfFromLinkName=1st Byte, indexOfToLinkName=2nd Byte)
							
							fprintf(HA_FD, "HA : (HO_LINK_SHIFT_COMMIT_REQ) RECEIVED FROM UserAgent is fromIPIndex(%d), toIPIndex(%d).\n", 
																					 			UA_TLV.TLVValue[0], UA_TLV.TLVValue[1]);
							HOLinkShiftCommit(UA_TLV.TLVValue[0], UA_TLV.TLVValue[1]);
						break;
								
						case HO_TRAFFIC_SHIFT_COMMIT_REQ:	// Perform Traffic Shift Handover

							// TLVValue received is
							// 3 Bytes (indexOfportNo(trafficID)=1st Byte, indexOfFromLinkName=2nd Byte, 
							//			indexOfToLinkName=3rd Byte)

							fprintf(HA_FD, "HA : (HO_TRAFFIC_SHIFT_COMMIT_REQ) RECEIVED FROM UserAgent is portIndex(%d), fromIPIndex(%d), toIPIndex(%d).\n", 
																		 				UA_TLV.TLVValue[0], UA_TLV.TLVValue[1], UA_TLV.TLVValue[2]);
							
							EMFC_TLVTypeLength[0] = HO_TRAFFIC_SHIFT_COMMIT;
							TLVUnit.intValue = userPref.adv_protocol_PortNo[UA_TLV.TLVValue[0]];		// Copy Protocol's Port No
							
							composeHOTrafficShiftTLV(UA_TLV.TLVValue[1], UA_TLV.TLVValue[2]);
							
							// NOTE:
							// We Do Not Lock the index, because it is Traffic Shift case. 
							// Other Traffics May also be passing throug the Interface.

							if (vm_send(EMFC_FD, &EMFC_TLVTypeLength, 2, 0)==-1)
								perror("ERROR (HO_TRAFFIC_SHIFT_COMMIT_REQ Send) ");
							else
							{
								if (vm_send(EMFC_FD, EMFC_TLVValue, EMFC_TLVTypeLength[1], 0)==-1)
									perror("ERROR (HO_TRAFFIC_SHIFT_COMMIT_REQ Send) ");
								else
								{
									fprintf(HA_FD, "HA : (HO_TRAFFIC_SHIFT_COMMIT) TLVValue SENT TO Core is AID(%d), fromIP(%s), toIP(%s).\n", 
														TLVUnit.intValue, IPAddress[UA_TLV.TLVValue[1]], IPAddress[UA_TLV.TLVValue[2]]);
								}
								// NOTE:
								// HA will receive EMF Core's per Association reply under "case HANDOVER"
								// and Handover Trasaction Complete under "case HO_TRAFFIC_SHIFT_COMPLETE".
								// Host Agent will send "Handover Complete" to User Agent from HO_TRAFFIC_SHIFT_COMPLETE.
							}
						break;

						case BA_COMMIT_REQ:										// Perform Bandwidth Aggregation
																				// Right Now System Entertains only ONE BA at a time
							// TLVValue Received is
							// 2 Bytes (portIndex : [0], interfaceIndex : [1])
							
							fprintf(HA_FD, "(BA_COMMIT_REQ) TLV Received from UA is portIndex(%d), IPIndex(%d).\n", UA_TLV.TLVValue[0], UA_TLV.TLVValue[1]);
							portIndex = UA_TLV.TLVValue[0];
							
							EMFC_TLVTypeLength[0] = BA_COMMIT;

							// BA Transaction (Add Link to Protocol) starts from Here.
							BATransaction[portIndex].manualBAFlag = 1;						// Its Manula BA Case
							BATransaction[portIndex].AIDPointer = connectionInfoHead;		// One Time Assignment, only at start
							BATransaction[portIndex].fromLinkIndex = UA_TLV.TLVValue[1];	// Save linkIndex for later use
							TLVUnit.intValue = getNextAIDInBATransaction(portIndex);
							
							if (TLVUnit.intValue == -1)							// Next AID NOT Found based on portIndex, or some other reason.
							{
								//fprintf(HA_FD, "Manual BA cannot be done.\n");
							}
							else												// Next AID Found, send Maual BA
							{
								composeBACommitTLV(portIndex);
							
								if (vm_send(EMFC_FD, &EMFC_TLVTypeLength, 2, 0)==-1)
									perror("ERROR (BA_COMMIT Send) ");
								else
								{
									if (vm_send(EMFC_FD, EMFC_TLVValue, EMFC_TLVTypeLength[1], 0)==-1)
										perror("ERROR (BA_COMMIT Send) ");
									else
									{
										fprintf(HA_FD, "HA : (BA_COMMIT_REQ) TLVValue SENT TO Core is AID(%d), IP(%s).\n\n", 
																				TLVUnit.intValue, IPAddress[UA_TLV.TLVValue[1]]);
									}
								}
							}
							// NOTE:
							// HA will receive EMF Core's reply under "case BA_COMPLETE" in EMF Core portion and
							// will then reply to User Agent from there.
						break;

						case USER_PREFERENCE_CHANGED_IND:	// Update Message from UA
							
							// TLVValue received is 
							// Zero Bytes
							
							// NOTE:
							// During Data exchange, what can change?
							// 1. 	If HO Preference changes, 
							//		1.1.	Auto HO will be reflected at Auto HO Time
							//		1.2.	Manual HO option can be used any time and will take place immediately. 
							//				Case:				
							//				If user performs 'HO_TRAFFIC_SHIFT_COMMIT_REQ' and specifies those fromLink & toLink interfaces against  
							//				a TRID which are also included in BA, then we shall perform 'ManualHO'.
							//
							// 2. 	In case of BA Preference,
							//		2.1.	Manual BA option can be used any time and will take place immediately
							//		2.2.	If user Removes a link from BA Preference, HA will send 'removeFromBA(IP)' to EMFCore
							//		2.3.	If user Adds a Link in BA Preferences, HA will send BA_COMMIT against that traffic.
							
							// "identifyBAPrefChange" Strategy
							// Host Agent will Identify Change in BAPreferences using the following strategy
							// We create, char trackUserPref[INTERFACES]. When a Link goes Up we place a '1' in this array at the index.
							// When User adds the link to BA Preferences, we track it in BAPreferences. If new index is specified for
							// just a single TRID, we execute 'BA_COMMIT(AID, upLinkIP)' by starting from first entry of 'connectionInfoList'.
							
							if (connectionInfoHead == NULL)						// No Connection Exists
								readUserPreferences();
							else
							{
								// NOTE:
								// Following HO Code will become part of "case LINK_UP", when User Preferences along with
								// INTERFACES will be stored Permanantly in the file.
								
								// Step 1: First we compute MaxPreferredIP in HO For the OLD preferences.
								if (strlen(IPNotUnderEMF) == 0)					// IPNotUnderEMF contains data, Last Time Forced HO happened.
								{
									index = getIndexOfMaxPrefIPInHOPref(-1);
									if(index>-1)
									{
										strcpy(fromIP, IPAddress[index]);
										fromIP[strlen(IPAddress[index])] = '\0';
									}
									else
										memset(fromIP,'\0',16);/*bzero(fromIP, 16);*/
								}
								else													// Last Time Forced HO happened
								{
									index = getIndexFromInterfaceIP(IPNotUnderEMF);		// We have ForcedHOIP, get its index
									strcpy(fromIP, IPNotUnderEMF);
									fromIP[strlen(IPNotUnderEMF)] = '\0';
								}
									
								saveBAPreferences();
								readUserPreferences();
								
								// ++++++++++++++++++++++++ (Handover) Link with Higher Preference may be UP  +++++++++++++++++++++++
								
								// Step 2: Next we compute Max Preferred IP in HO For the NEW preferences.
								maxPrefIndex = getIndexOfMaxPrefIPInHOPref(-1);
								
								if (maxPrefIndex > -1)									// IP Found in HO Preferences.
								{
									memset(IPNotUnderEMF,'\0',16);
									/*bzero(IPNotUnderEMF, 16);							*/// Write all '\0' on the ForcedHO fromIP
									strcpy(toIP, IPAddress[maxPrefIndex]);
									toIP[strlen(IPAddress[maxPrefIndex])] = '\0';
								
									if (strcmp(fromIP, toIP) != 0)			// New and Old MaxPrefHOIP are Different
									{
										userPref.indexOfMaxPrefHOIP[0] = maxPrefIndex;
										userPref.indexOfMaxPrefHOIP[1] = 0;					// No Connection Present right now

										// Map changes to User Preference File
										updateUserPrefFile();
										informUserAgent();
									
										// Send HO_LINK_SHIFT_COMMIT to EMF Core
										HOLinkShiftCommit(index, maxPrefIndex);						// HO_LINK_SHIFT_COMMIT
									}
								}
								else
								{
									// Case 1:	Last Time Forced HO happened so traffic is on Non-EMF IP. This time, No IP is present
									// 			in HO Preferences. Hence, No HO will happen. 
									//			NOTE:
									//			When Core closes all connections, connectionInfo LL maintained by HostAgent 
									//			will then destroy. In the mean time, valid HO Commands can be executed.
									// Case 2:	Last Time HA sent maxPreferredIP in LocalCompliance response to EMF Core. This time, 
									//			user removed all the links from HO_Preference. Hence, No HO will happen. Above NOTE is
									//			applicable here as well.

									if (strlen(fromIP) > 0)
										fprintf(HA_FD, "\nHA : (USER_PREFERENCE_CHANGED_IND) toIP is NOT Present. HO cannot take place.\n\n");
								}
								
								// +++++++++++++++++++++++++ (BA) Handling ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
								
								// NOTE:
								// For each Traffic/Protocol, we check if addToBA or removeFromBA is required. If yes, we process it.
								for (portIndex=0; portIndex<PROTOCOLS; portIndex++)
								{
									if (composeBACommitRemoveMessage(portIndex) == 1)	// Detect change in BA Preferences & Compose New BA message.
									{
										// NOTE:
										// Sending Sequence => 'AddToBA Message' is followed by 'RemoveFromBA Message'
										// Receiving Sequence => will be same i.e. 
										// 	'AddToBAComplete' Message followed by 'RemoveFromBAComplet' Message

										fprintf(HA_FD, "\n(USER_PREFERENCE_CHANGED_IND) BA Preferences Composed.\n\n");
										
										// We copy index[0] value to index[1], because, we process index[1] value 
										// and want index[0] value to be safe
										addRemoveIPCountInBATransaction[portIndex][1] = addRemoveIPCountInBATransaction[portIndex][0]; 
										addToBAMessageFlag[portIndex] = removeFromBAMessageFlag[portIndex] = 0;
										
										BATransaction[portIndex].manualBAFlag = 0;						// Auto BA Case
										BATransaction[portIndex].AIDPointer = connectionInfoHead;		// One Time Assignment, only at start
										TLVUnit.intValue = getNextAIDInBATransaction(portIndex);		// getNextAIDInBATransaction() returns AID 
									
										if (addToBAIPs[portIndex] != NULL)
										{
											addToBAMessageFlag[portIndex] = 1;							// User Added Preference for this portNo
											EMFC_TLVTypeLength[0] = BA_COMMIT;
											composeBACommitTLV(portIndex);

											if (vm_send(EMFC_FD, &EMFC_TLVTypeLength, 2, 0)==-1)
												perror("ERROR (USER_PREFERENCE_CHANGED_IND Send) ");
											else
											{
												if (vm_send(EMFC_FD, EMFC_TLVValue, EMFC_TLVTypeLength[1], 0)==-1)
																perror("ERROR (USER_PREFERENCE_CHANGED_IND Send) ");
												else
												{
													if (HOtoBAIPs == NULL)
													fprintf(HA_FD, "HA : (USER_PREFERENCE_CHANGED_IND) TLVValue SENT TO Core is AID(%d), addIP(%s).\n\n", 
																						 					TLVUnit.intValue, addToBAIPs[portIndex]);
													else
													{
													fprintf(HA_FD, "HA : (USER_PREFERENCE_CHANGED_IND) TLVValue SENT TO Core is AID(%d), addIP(%s).\n\n", 
																					 									TLVUnit.intValue, HOtoBAIPs);
				 										free(HOtoBAIPs);
				 									}
												}
											}
										}
										
										if (removeFromBAIPs[portIndex] != NULL)
										{
											removeFromBAMessageFlag[portIndex] = 1;
											EMFC_TLVTypeLength[0] = AIDwise_REMOVE_FROM_BA_COMMIT;
											composeRemoveFromBATLV(portIndex);
									
											if (vm_send(EMFC_FD, &EMFC_TLVTypeLength, 2, 0)==-1)
												perror("ERROR (USER_PREFERENCE_CHANGED_IND Send) ");
											else
											{
												if (vm_send(EMFC_FD, EMFC_TLVValue, EMFC_TLVTypeLength[1], 0) == -1)
													perror("ERROR (USER_PREFERENCE_CHANGED_IND Send) ");
												else
												{
													fprintf(HA_FD, "HA : (USER_PREFERENCE_CHANGED_IND) TLVValue SENT TO Core is AID(%d), removeIP(%s).\n\n",
																						 				TLVUnit.intValue, removeFromBAIPs[portIndex]);
												}
											}
										}
									}
								}
							}
							//displayAll();
						break;
					} 
				}
			}
			else if (FD_ISSET(EMFC_FD, &tmpFDList))
			{
				bytesReceived = vm_recv(EMFC_FD,  &EMFC_TLV, sizeof (struct TLVInfo), 0);
				if (bytesReceived == -1)
					perror("ERROR (EMFCore Receive) ");
				
				else if (bytesReceived == 0)		// TCP Connection Closed
				{
					fprintf(HA_FD, "\n******* Connection Closed by EMF Core *******\n");
					FD_CLR(EMFC_FD, &baseFDList);
					
					// Remove ConnectionInfo Linked List
					if (connectionInfoHead != NULL)							// If ConnectionInfo LL contains nodes
					{
						deleteConnInfoLL(&connectionInfoHead);				// Delete ConnectionInfo LL
						connectionInfoHead = NULL;
					}
					vm_close(EMFC_FD);
				}
				else  // Data is available
				{
					switch(EMFC_TLV.TLVType)		// Process Request based on TLV Type
					{
						case LOCAL_COMPLIANCE_REQ:
						
							// TLV received is
							// 4Bytes (portNo)
							
							EMFC_TLVTypeLength[0] = LOCAL_COMPLIANCE_RES;					// TLVType
							for (index=0; index<4; index++)									// Extract Port No
								TLVUnit.charValue[index] = EMFC_TLV.TLVValue[index];

							fprintf(HA_FD, "\nHA : (LOCAL_COMPLIANCE_REQ) TLVValue RECEIVED FROM Core is port(%d).\n", TLVUnit.intValue);
							
							portIndex = checkLocalCompliance(TLVUnit.intValue);
							if (portIndex >= 0)												// Valid Indices: ['0' -> 'n']
							{
								fprintf(HA_FD, "HA : (LOCAL_COMPLIANCE) Port %d is Locally Compliant.\n", TLVUnit.intValue);
								
								// NOTE:
								// Value of 2nd parameter (IPIndex) in composeNextTLVInLocalComplianceReq() is passed as '-1'
								// EMFC_TLVTypeLength[1] and EMFC_TLVValue has been composed inside composeNextTLVInLocalComplianceReq().
								j = composeNextTLVInLocalComplianceReq(portIndex, -1, 0);
								if (j == 0)			// Does more IPs in BA Preference exist?
								{
									localComlpianceReq[portIndex] = 1;										// Used in ADD_ASSOCIATION to locate TRID
									fprintf(HA_FD, "HA : (LOCAL_COMPLIANCE_REQ) BA Preference Found.\n"); 
								}
								else if (j == 1)	// BA Preference NOT found, maxPreferredHOIP is returned  
								{
									localComlpianceReq[portIndex] = 0;
									fprintf(HA_FD, "HA : (LOCAL_COMPLIANCE_REQ) HO Preference Found.\n");
								}
								else if (j == -1)	// NO IP found either in BA_Preferences or HO_Preferences
								{
									fprintf(HA_FD, "HA : (LOCAL_COMPLIANCE_REQ) NO IP found either in BA_Preferences or HO_Preferences.\n"); 
									EMFC_TLVTypeLength[1] = 0;						// Assign ZERO to TLVLength, No Need to send TLVValue
								}
							}
							else if (portIndex == -1)
							{
								fprintf(HA_FD, "HA : (LOCAL_COMPLIANCE) Port %d is Locally NOT Compliant.\n", TLVUnit.intValue);
								EMFC_TLVTypeLength[1] = 0;							// Assign TLVLength, No Need to send TLVValue
							}
							
							if (vm_send(EMFC_FD, &EMFC_TLVTypeLength, 2, 0) == -1)		// Send TLVType & TLVLength
								perror("ERROR (LOCAL_COMPLIANCE_RES Send) ");
							else
							{
								fprintf(HA_FD, "HA : (LOCAL_COMPLIANCE_RES) TLVType & TLVLength SENT TO Core is %d %d.\n", 
																	 EMFC_TLVTypeLength[0], EMFC_TLVTypeLength[1]);

								if (EMFC_TLVTypeLength[1] > 0)						// When port is NOT Compliant, Dont send TLVValue
								{
									EMFC_TLVValue[EMFC_TLVTypeLength[1]] = '\0';
									if (vm_send(EMFC_FD, EMFC_TLVValue, EMFC_TLVTypeLength[1], 0) == -1)
										perror("ERROR (LOCAL_COMPLIANCE_RES Send) ");
									else
										fprintf(HA_FD, "HA : (LOCAL_COMPLIANCE_RES) TLVValue SENT TO Core is IP(%s).\n\n", EMFC_TLVValue);
									
									// NOTE:
									// We have yet sent only First IP, we have to send rest of the IPs. 
									// We adopt following procedure.
									// - EMF Core responds with ADD_ASSOCIATION TLV. HostAgent creates Node in connInfo LinkedList 
									//   for this AID.
									// - Next HostAgent sends BA_COMMIT_IN_LOCAL_COMPLIANCE TLV that contains rest of the IPs
									//   seperated by ':'. 
									// - EMF_Core responses for each IP, with an ADD_CONNECTION or BA_COMPLETE_IN_LOCAL_COMPLIANCE TLV. 
									//   HostAgent adds them against the AID entry.
								}
							}
							
							displayAll();
						break;
						
						case INTERFACE_IP_REQ:	// NOTE: As per UserAgent, HA stores Single HO Preference, 
												// which apply to all applications/traffics/ports
							// TLVValue received is 
							// 19Bytes (AID=1st 4Byte, IP=Last 15Bytes max)
							
							TRID = -1; 			// Parsing Function will check AID, TRID, CID & IP
							CID = -1;
							parseConnectionTLV(EMFC_TLV.TLVValue, &AID, &TRID, &CID, toIP);

							printf("\nHA : (INTERFACE_IP_REQ) TLVValue RECEIVED FROM Core is AID(%d),IP(", AID);
							fprintf(HA_FD, "\nHA : (INTERFACE_IP_REQ) TLVValue RECEIVED FROM Core is AID(%d),IP(", AID);
							for(i=0; i<strlen(toIP); i++)
							{
								printf("%c", toIP[i]);
								fprintf(HA_FD, "%c", toIP[i]);
							}
							printf(").\n");
							fprintf(HA_FD, ").\n");
							
							EMFC_TLVTypeLength[0] = INTERFACE_IP_RES;
							
							TLVUnit.intValue = AID;										// Assign AID taken from CORE

							index = getIndexOfMaxPrefIPInHOPref(getIndexFromInterfaceIP(toIP)); 
							if (index == -1)
							{
								// NO IP is found in HO_Preferences
								EMFC_TLVTypeLength[1] = 4;				// No IP is available
								EMFC_TLVValue = (char*) malloc(EMFC_TLVTypeLength[1]);		
								i=0;
								for(i=0; i<4; i++)											// Compose AID, If IP found, will be composed below
									 EMFC_TLVValue[i] = TLVUnit.charValue[i];
								EMFC_TLVValue[EMFC_TLVTypeLength[1]] = '\0';
							}
							else
							{
								//if (EMFC_TLVValue != NULL) //free(EMFC_TLVValue);			// Free the memory, if held
								
								EMFC_TLVTypeLength[1] = strlen(IPAddress[index]) + 4;		// +4 Bytes for
								EMFC_TLVValue = (char*) malloc(EMFC_TLVTypeLength[1]);		

								i=0, j=0;
								for(i=0; i<4; i++)											// Compose AID, If IP found, will be composed below
									 EMFC_TLVValue[j++] = TLVUnit.charValue[i];

								for(i=0; i<EMFC_TLVTypeLength[1]-4; i++)					// Compose IP, AID is already composed
									 EMFC_TLVValue[j++] = IPAddress[index][i];

								EMFC_TLVValue[EMFC_TLVTypeLength[1]] = '\0';
							}

							if (vm_send(EMFC_FD, &EMFC_TLVTypeLength, 2, 0)==-1)
								perror("ERROR (INTERFACE_IP_REQ Send) ");
							else
							{
								fprintf(HA_FD, "(INTERFACE_IP_REQ) TLVLength sent to Core is %d.\n", EMFC_TLVTypeLength[1]);
								if (vm_send(EMFC_FD, EMFC_TLVValue, EMFC_TLVTypeLength[1], 0)==-1)
									perror("ERROR (USER_PREFERENCE_CHANGED_IND Send) ");
								else
								{
									printf("HA : (INTERFACE_IP_REQ) TLVValue SENT TO Core is AID(%d)", TLVUnit.intValue);
									fprintf(HA_FD, "HA : (INTERFACE_IP_REQ) TLVValue SENT TO Core is AID(%d)", TLVUnit.intValue);
									
									printf("IP(");
									fprintf(HA_FD, "IP(");
									for(i=4; i<EMFC_TLVTypeLength[1]; i++)
									{
										printf("%c", EMFC_TLVValue[i]);
										fprintf(HA_FD, "%c", EMFC_TLVValue[i]);
									}
									printf(").\n");
									fprintf(HA_FD, ").\n");
								}
							}
						break;
						
						case INTERFACE_IP_COM:	// This message shows that handover from ZERO throughput link to new link has completed.

							// TLVValue received is
							// 27Bytes (AID=4Bytes, oldCID=4Bytes, newCID=4Bytes, newIP=15Bytes)
							
							parseHOTLV(EMFC_TLV.TLVValue, &AID, &CID, &newCID, toIP);

							fprintf(HA_FD, "HA : (HANDOVER-INTERFACE_IP_COM) TLVValue RECEIVED FROM Core is AID(%d), oldCID(%d), newCID(%d), toIP(%s).\n", 
																													AID, CID, newCID, toIP);

							printf("HA : (INTERFACE_IP_COM) TLVValue RECEIVED FROM Core is AID(%d), oldCID(%d), newCID(%d), toIP(%s).\n", 
																													AID, CID, newCID, toIP);
							
							index = getIndexFromInterfaceIP(toIP);
							if (index > -1)
							{
								if (HOUpdate(AID, CID, newCID, index) == -1)
									fprintf(HA_FD, "ERROR (HOUpdate).\n");
								else
									display(&connectionInfoHead);
							}
						break;
						
						case ADD_ASSOCIATION:
							
							// TLVValue
							// 27 Bytes (AID=1st 4Bytes, TRID=2nd 4Bytes, CID=3rd 4Bytes, IP=Last 15Bytes)

							TRID=1; 			// Parsing Function will check AID, TRID, CID & IP
							parseConnectionTLV(EMFC_TLV.TLVValue, &AID, &TRID, &CID, toIP);

							fprintf(HA_FD, "HA : (ADD_ASSOCIATION) TLVValue RECEIVED FROM Core is AID(%d), TRID(%d), CID(%d), toIP(%s).\n", 																											AID, TRID, CID, toIP);

							IPIndex = getIndexFromInterfaceIP(toIP);
							if (IPIndex > -1)															// IP found in IPAddress list
							{
								TRIDIndex = getIndexFromPortNo(TRID);
								if (addAID(AID, TRIDIndex, CID, IPIndex) == -1)
								{
									EMFC_TLVTypeLength[0] = BA_COMMIT_IN_LOCAL_COMPLIANCE;				// Response TLV
									EMFC_TLVTypeLength[1] = 0;											// Insufficient Memory Resources (ERROR)

									if (vm_send(EMFC_FD, &EMFC_TLVTypeLength, 2, 0)==-1)
										perror("ERROR (ADD_ASSOCIATION_LOCAL_COMPLIANCE Send) ");
									else
									{
										fprintf(HA_FD, "HA : Insufficient Memory : (ADD_ASSOCIATION) TLVType & TLVLength SENT TO Core is %d %d.\n", 
																								EMFC_TLVTypeLength[0], EMFC_TLVTypeLength[1]);
									}
								}
								else
								{
									//--------------------------------------------------------------------------------------
									if (informUserAgentFlag == 1)	// Did conInfLL.h set the flag?
									{
										updateUserPrefFile();
										informUserAgent();
										informUserAgentFlag = 0;
										//fprintf(HA_FD, "HA:addAID => userPref.indexOfMaxPrefHOIP[1]=%d.\n", userPref.indexOfMaxPrefHOIP[1]);
									}
									//--------------------------------------------------------------------------------------
									displayAll();
									display(&connectionInfoHead);

									// NOTE:
									// ADD_ASSOCIATION is received as a result of LOCAL_COMPLIANCE_REQ. We check if more IPs
									// exist in BA Preference against this TRID, if yes, we send BA_COMMIT_IN_LOCAL_COMPLIANCE.
									if (localComlpianceReq[TRIDIndex] == 1)				// More IPs may be present, we check here.
									{
										composeNextTLVInLocalComplianceReq(TRIDIndex, IPIndex, AID);
										EMFC_TLVTypeLength[0] = BA_COMMIT_IN_LOCAL_COMPLIANCE;
										// EMFC_TLVTypeLength[1] has already been composed inside composeNextTLVInLocalComplianceReq

										if (vm_send(EMFC_FD, &EMFC_TLVTypeLength, 2, 0)==-1)
											perror("ERROR (ADD_ASSOCIATION Send) ");
										else
										{
											if (vm_send(EMFC_FD, EMFC_TLVValue, EMFC_TLVTypeLength[1], 0)==-1)
												perror("ERROR (ADD_ASSOCIATION_LOCAL_COMPLIANCE Send) ");
											else
											{
												fprintf(HA_FD, "HA : (ADD_ASSOCIATION_LOCAL_COMPLIANCE) TLVValue SENT TO Core is AID(%d), IPs(", TLVUnit.intValue);
												for (index=4; index<EMFC_TLVTypeLength[1]; index++)
													fprintf(HA_FD, "%c", EMFC_TLVValue[index]);
												fprintf(HA_FD, ").\n");
											}
										}
									}
								}
							}
							else if (IPIndex==-1)			// No IP found in IP Address List
							{
							fprintf(HA_FD, "\nHA : (ADD_ASSOCIATION) Host Agent received an IP(%s) which is NOT in IP address list. Command Aborted.\n\n", 
																																				toIP);
							}
						break;
						
						case REMOVE_ASSOCIATION:

							// TLVValue
							// 4 Bytes (AID=4Bytes)

							TRID=0; CID=2;
							parseConnectionTLV(EMFC_TLV.TLVValue, &AID, &TRID, &CID, NULL);

							fprintf(HA_FD, "HA : (REMOVE_ASSOCIATION) TLVValue RECEIVED FROM Core is AID(%d).\n", AID);
							
							j = removeAID(AID);
							if (j==-1)
							{
								fprintf(HA_FD, "\nHA : (REMOVE_ASSOCIATION) ERROR removeAID() : AID(%d) NOT Found. Aborted.\n", AID);
							}
							else if (j==1)
								connectionInfoHead = NULL;
							
							display(&connectionInfoHead);
						break;

						case ADD_CONNECTION:
						case BA_COMPLETE_IN_LOCAL_COMPLIANCE:
							// TLVValue
							// 23 Bytes (AID=1st 4Bytes, CID=2nd 4Bytes & IP=Last 15Bytes)
							
							TRID=0; CID=1;
							parseConnectionTLV(EMFC_TLV.TLVValue, &AID, &TRID, &CID, toIP);

							fprintf(HA_FD, "HA : (ADD_CONNECTION) TLVValue RECEIVED FROM Core is AID(%d), CID(%d), toIP(%s).\n", AID, CID, toIP);
							
							IPIndex = getIndexFromInterfaceIP (toIP);
							if (IPIndex > -1)
							{
								if (addCID(AID, CID, IPIndex) == -1)
									fprintf(HA_FD, "\nERROR addCID.\n");
								else
								{
									display(&connectionInfoHead);

									//--------------------------------------------------------------------------------------
									if (informUserAgentFlag == 1)	// Did conInfLL.h set the flag?
									{
										updateUserPrefFile();
										informUserAgent();
										informUserAgentFlag = 0;
										//fprintf(HA_FD, "HA:addCID => userPref.indexOfMaxPrefHOIP[1]=%d.\n", userPref.indexOfMaxPrefHOIP[1]);
									}
									//--------------------------------------------------------------------------------------
								}
							}
							else if (IPIndex == -1)
							{
							fprintf(HA_FD, "\nHA : (ADD_CONNECTION) Host Agent received an IP(%s) which is NOT in IP address list. Command Aborted.\n\n", 
																																				toIP);
							}
						break;
						
						case REMOVE_CONNECTION:
						case AIDwise_REMOVE_FROM_BA_COMPLETE:
						case LD_AIDwise_REMOVE_FROM_BA_COMPLETE:

							// TLVValue
							// 8 Bytes (AID=1st 4Bytes, CID=2nd 4Bytes)

							TRID=0; CID=0;
							parseConnectionTLV(EMFC_TLV.TLVValue, &AID, &TRID, &CID, toIP);

							fprintf(HA_FD, "HA : (REMOVE_CONNECTION) TLVValue RECEIVED FROM Core is AID(%d), CID(%d).\n", AID, CID);
							removeFromBASource = 0;										// Must Initialize it with ZERO
							index = getRemoveFromBASource(AID, CID, &removeFromBASource);
							
							if (removeFromBASource == 1)								// Auto Remove From BA [User_Preference_Changed_IND]
							{
								portIndex = index;										// Copy portIndex
								
								//	Get indexOf CID
								for(index=0; index<INTERFACES; index++)
								{
									if (BATransaction[portIndex].AIDPointer->connectionID[index] == CID)
									{
										fromIndex = index;
										index = INTERFACES;							// Break the Loop
									}
								}
								index = fromIndex;
								
								// -------------------------Remove Connection in 'ConnInfo' Linked List ----------------------
								BATransaction[portIndex].AIDPointer->connectionID[index] = 0;
								BATransaction[portIndex].AIDPointer->BACount -= 1;

								if (userPref.indexOfMaxPrefHOIP[0] == index)		// Is connection made on the interface of MaxPrefHOIP?
								{
									userPref.indexOfMaxPrefHOIP[1] -= 1;			// We decrement it by ONE, so that userAgent can
																					// identify that at least a connection is through this IP
									updateUserPrefFile();							// Map changes to user preference file
									informUserAgent();
								}
								// -------------------------------------------------------------------------------------------
								addRemoveIPCountInBATransaction[portIndex][1] -= 1;			// Decrement 1, as One connection Removed.
								
								autoBATransaction(portIndex);					// Auto BA Messages to EMF Core are sent from inside this function
							}
							else if (removeFromBASource == 2)					// HO LinkDown
							{
								IPIndex = BATransactionInHOLinkDown[index].fromLinkIndex;

								// Remove Connection
								BATransactionInHOLinkDown[index].AIDPointer->connectionID[IPIndex] = 0;
								if (BATransactionInHOLinkDown[index].AIDPointer->BACount > 0)
									BATransactionInHOLinkDown[index].AIDPointer->BACount -= 1;
								
								//--------------------------------------------------------------------------------------
								if (userPref.indexOfMaxPrefHOIP[0] == IPIndex)		// Is connection made on the interface of MaxPrefHOIP?
								{
									userPref.indexOfMaxPrefHOIP[1] -= 1;			// We decrement it by ONE, so that userAgent can
																					// identify that at least a connection is through this IP
									updateUserPrefFile();							// Map changes to user preference file
									informUserAgent();
									//fprintf(HA_FD, "HA:RemoveConn => userPref.indexOfMaxPrefHOIP[1]=%d.\n", userPref.indexOfMaxPrefHOIP[1]);
								}
								//--------------------------------------------------------------------------------------

								BATransactionInHOLinkDown[index].AIDPointer = BATransactionInHOLinkDown[index].AIDPointer->nextAID;	// Move to Next AID
								fromIndex = BATransactionInHOLinkDown[index].fromLinkIndex;
								toIndex = BATransactionInHOLinkDown[index].toLinkIndex;
								TLVUnit.intValue = getNextAIDInHOLDTransaction(fromIndex);
								
								if (TLVUnit.intValue == -1)
								{
									//LD_AIDwiseRemoveFromBATransaction[index] = 0;			// Stop AIDwiseRemoveFromBA Transaction
									BATransactionInHOLinkDown[index].HOIdentifier = 0;		// Invalide Code
									LDTransactionLock[index] = 0;							// Unlock the index
								}
								else
								{
									EMFC_TLVTypeLength[0] = LD_AIDwise_REMOVE_FROM_BA_COMMIT;
									LD_ComposeRemoveFromBATLVInHO(fromIndex);
									
									if (vm_send(EMFC_FD, &EMFC_TLVTypeLength, 2, 0) == -1)
										perror("ERROR (Remove_Connection => AIDwise_BA_REMOVE_IN_LINK_DOWN) ");
									else
									{
										if (vm_send(EMFC_FD, EMFC_TLVValue, EMFC_TLVTypeLength[1], 0) == -1)
											perror("ERROR (LD_AIDwise_Remove_From_BA vm_send) ");
										else
										{
											fprintf(HA_FD, "HA : (AIDwise_BA_REMOVE_IN_LINK_DOWN) TLVValue SENT TO CORE is AID(%d), removeIP(%s).\n\n", 
																								 			TLVUnit.intValue, IPAddress[fromIndex]);
										}
									}
								}
							}
							else if (removeFromBASource == 3)							// HO LinkShift
							{
								IPIndex = BATransactionInHOLinkShift[index].fromLinkIndex;

								// Remove Connection
								BATransactionInHOLinkShift[index].AIDPointer->connectionID[IPIndex] = 0;
								if (BATransactionInHOLinkShift[index].AIDPointer->BACount > 0)
									BATransactionInHOLinkShift[index].AIDPointer->BACount -= 1;
								
								//--------------------------------------------------------------------------------------
								if (userPref.indexOfMaxPrefHOIP[0] == IPIndex)		// Is connection made on the interface of MaxPrefHOIP?
								{
									userPref.indexOfMaxPrefHOIP[1] -= 1;			// We decrement it by ONE, so that userAgent can
																					// identify that at least a connection is through this IP
									updateUserPrefFile();							// Map changes to user preference file
									informUserAgent();
									//fprintf(HA_FD, "HA:RemoveConn => userPref.indexOfMaxPrefHOIP[1]=%d.\n", userPref.indexOfMaxPrefHOIP[1]);									
								}
								//--------------------------------------------------------------------------------------

								BATransactionInHOLinkShift[index].AIDPointer = BATransactionInHOLinkShift[index].AIDPointer->nextAID;
								fromIndex = BATransactionInHOLinkShift[index].fromLinkIndex;
								toIndex = BATransactionInHOLinkShift[index].toLinkIndex;
								TLVUnit.intValue = getNextAIDInHOTransaction(fromIndex);	
								
								if (TLVUnit.intValue == -1)
								{
									// BA Transaction Completed for HO LinkShift
									BATransactionInHOLinkShift[index].HOIdentifier = 0;		// Invalide Code
								}
								else
								{
									EMFC_TLVTypeLength[0] = AIDwise_REMOVE_FROM_BA_COMMIT;
									composeRemoveFromBATLVInHO(index);
								
									if (vm_send(EMFC_FD, &EMFC_TLVTypeLength, 2, 0)==-1)
										perror("ERROR (Remove_Connection => AIDwise_BA_REMOVE_IN_LINK_SHIFT) ");
									else
									{
										if (vm_send(EMFC_FD, EMFC_TLVValue, EMFC_TLVTypeLength[1], 0) == -1)
											perror("ERROR (AIDwise_Remove_From_BA_COMMIT vm_send) ");
										else
										{
											fprintf(HA_FD, "HA : (AIDwise_Remove_From_BA_COMMIT) TLVValue SENT TO Core is AID(%d), removeIP(%s).\n\n",
																									 		TLVUnit.intValue, IPAddress[index]);
										}
									}
								}
							}
							else if (index == -1)					// Core called REMOVE_CONNECTION after ADD_ASSOCIATION
							{
								///// New Code /////////////////
								if (removeCID(AID, CID) == -1)
									fprintf(HA_FD, "\nHA : Either AID or CID is NOT present in connectionInfo Linked List.\n");
								else
								{
									//--------------------------------------------------------------------------------------
									if (informUserAgentFlag == 1)	// Did conInfLL.h set the flag?
									{
										updateUserPrefFile();
										informUserAgent();
										informUserAgentFlag = 0;
										//fprintf(HA_FD, "HA:RemoveConn => userPref.indexOfMaxPrefHOIP[1]=%d.\n", userPref.indexOfMaxPrefHOIP[1]);										
									}
									//--------------------------------------------------------------------------------------
								}
								
								///// Old Code ///////////////
								//if ((index=getIndexFromInterfaceIP(IP)) > -1)
								//	if (removeCID(AID, CID) == -1)
								//		fprintf(HA_FD, "\nHA : Either AID or CID is NOT present in connectionInfo Linked List.\n");
								//else
								//	fprintf(HA_FD, "\nHA : (REMOVE_CONNECTION) Host Agent RECEIVED Invalid IP(%s : %s). Command Aborted.\n\n", fromIP);
							}
								
							display(&connectionInfoHead);
						break;
						
						case HANDOVER:	// When HA sends HO_LINK_SHIFT_COMMIT message, a number of 'LINK_HANDOVER'
										// messages can be received, finally followed by a 'HO_LINK_SHIFT_COMPLETE' message.
							
							// TLVValue received is
							// 27Bytes (AID=4Bytes, oldCID=4Bytes, newCID=4Bytes, newIP=15Bytes)
							
							parseHOTLV(EMFC_TLV.TLVValue, &AID, &CID, &newCID, toIP);

							fprintf(HA_FD, "HA : (HANDOVER) TLVValue RECEIVED FROM Core is AID(%d), oldCID(%d), newCID(%d), toIP(%s).\n", AID, CID, newCID, toIP);
							
							index = getIndexFromInterfaceIP(toIP);
							if (index > -1)
							{
								if (HOUpdate(AID, CID, newCID, index)==-1)
									fprintf(HA_FD, "ERROR (HOUpdate)\n");
								else
								{
									display(&connectionInfoHead);

									//--------------------------------------------------------------------------------------
									if (informUserAgentFlag == 1)	// Did conInfLL.h set the flag?
									{
										updateUserPrefFile();
										informUserAgent();
										informUserAgentFlag = 0;
										//fprintf(HA_FD, "HA:Handover => userPref.indexOfMaxPrefHOIP[1]=%d.\n", userPref.indexOfMaxPrefHOIP[1]);										
									}
									//--------------------------------------------------------------------------------------

								}
							}
							else if (IPIndex == -1)
							{
								fprintf(HA_FD, "\nHA : (HANDOVER) Host Agent received invalid IP(%s). Command Aborted.\n\n", toIP);
							}
						break;
						
						case HO_LINK_SHIFT_COMPLETE:	// UA would have sent a Manual HO_LINK_SHIFT_COMMIT_REQ
														// in the past, and this case is its response.
							// TLVValue received is
							// 30Bytes (fromLink=1st 15Bytes, toLink=2nd 15Bytes)
							
							fprintf(HA_FD, "HA : (HO_LINK_SHIFT_COMPLETE) TLVValue RECEIVED FROM Core is (%s).\n", EMFC_TLV.TLVValue);
							
							if (EMFC_TLV.TLVValue[0] != '0')				// If connection on toHOIP did not get stable, Core will vm_send ZERO in
							{												// HO_LINK_SHIFT_COMPLETE TLV
							
								parseHOCompleteTLV(EMFC_TLV.TLVValue, 0, fromIP, toIP);		// Indicates function that portNo is NOT part of TLV.
								fromIndex = getIndexFromInterfaceIP(fromIP);
								toIndex = getIndexFromInterfaceIP(toIP);
								if ((fromIndex > -1) && (toIndex > -1))
								{
									index = getHOLinkShiftCompleteSource(fromIndex, toIndex, &HOCompleteSource);
									if (HOCompleteSource == 1)				// LinkDown HOLinkShiftComplete
									{
										LDTransactionLock[fromIndex] = 0;					// Unlock the Index							
										strcpy(userPref.adv_interfaces_name[index], "");	// Remove interface name
										IPAddress[index] = NULL;							// Remove Interface IP
									} 
							
									else if (HOCompleteSource == 2)			// Manual HOLinkShiftComplete
									{
										HOLinkShiftCompleteFlag[index] = 0;
										BATransactionInHOLinkShift[index].HOIdentifier = 0;		// Invalide Code								

										// Compose TLV for User Agent
										UA_TLV.TLVType = HO_LINK_SHIFT_COMMIT_CONF;
										strcpy(UA_TLV.TLVValue, "Handover completed from ");
										strcat(UA_TLV.TLVValue, userPref.adv_interfaces_name[fromIndex]);
										strcat(UA_TLV.TLVValue, " to ");
										strcat(UA_TLV.TLVValue, userPref.adv_interfaces_name[toIndex]);
										strcat(UA_TLV.TLVValue, ".\0");
										UA_TLV.TLVLength = strlen(UA_TLV.TLVValue);

										// No Need to do this in Manual HO Link Shift
										//strcpy(userPref.adv_interfaces_name[fromIndex], "");	// Remove interface name
										//IPAddress[fromIndex] = NULL;							// Remove Interface IP
								
										if (vm_send(UA_FD, &UA_TLV, sizeof(struct TLVInfo), 0)==-1)
											perror("ERROR (HO_LINK_SHIFT_COMPLETE vm_send) ");
										else
										{
											fprintf(HA_FD, "HA : (HO_LINK_SHIFT_COMPLETE) TLVValue SENT TO UserAgent is (%s).\n", UA_TLV.TLVValue);
										}
									}
									display(&connectionInfoHead);
								}
							}
							else
							{
							fprintf(HA_FD, "\nHA : (HO_LINK_SHIFT_COMPLETE) Host Agent RECEIVED Invalid IP(%s : %s). Command Aborted.\n\n", fromIP, toIP);
							}
						break;
						
						case HO_TRAFFIC_SHIFT_COMPLETE:	// UA would have sent a Manual HO_TRAFFIC_SHIFT_COMMIT_REQ
														// in the past, and this case is its response.
							
							// TLVValue received is
							// 34Bytes (portNo=4Bytes, fromIP=15Bytes, toIP=15Bytes, )
							
							parseHOCompleteTLV(EMFC_TLV.TLVValue, 1, fromIP, toIP);			// Indicates function that portNo is part of TLV

							fprintf(HA_FD, "HA : (HO_TRAFFIC_SHIFT_COMPLETE) TLV RECEIVED FROM CORE is port(%d), fromIP(%s), toIP(%s).\n", 
																									TLVUnit.intValue, fromIP, toIP);

							fromIndex = getIndexFromInterfaceIP(fromIP);
							toIndex = getIndexFromInterfaceIP(toIP);

							if ((fromIndex > -1) && (toIndex > -1))
							{
								UA_TLV.TLVType = HO_TRAFFIC_SHIFT_COMMIT_CONF;

								UA_TLV.TLVType = HO_LINK_SHIFT_COMMIT_CONF;
								strcpy(UA_TLV.TLVValue, "Handover completed from ");
								strcat(UA_TLV.TLVValue, userPref.adv_interfaces_name[fromIndex]);
								strcat(UA_TLV.TLVValue, " to ");
								strcat(UA_TLV.TLVValue, userPref.adv_interfaces_name[toIndex]);
								strcat(UA_TLV.TLVValue, " for ");
								strcat(UA_TLV.TLVValue, userPref.adv_protocol_name[getIndexFromPortNo(TLVUnit.intValue)]);
								strcat(UA_TLV.TLVValue, ".\0");
								UA_TLV.TLVLength = strlen(UA_TLV.TLVValue);

								if (vm_send(UA_FD, &UA_TLV, sizeof(struct TLVInfo), 0) == -1)
									perror("ERROR (HO_TRAFFIC_SHIFT_COMMIT_CONF Send) ");
								else
								{
									fprintf(HA_FD, "HA : (HO_TRAFFIC_SHIFT_COMPLETE) TLVValue SENT TO UserAgent is (%s).\n", UA_TLV.TLVValue);
								}
							}
							else
							{
							fprintf(HA_FD, "\nHA : (HO_TRAFFIC_SHIFT_COMPLETE) Host Agent received Invalid IPs(%s : %s). Command Aborted.\n\n", 
																																fromIP, toIP);
							}
						break;
						
						case BA_COMPLETE:			// UA would have sent either 'Manual' or 'Auto' BACOMMIT in the past,
													// this case is response of that request.
							// TLVValue received is
							// 23Bytes (AID=1st 4Bytes, CID=2nd 4Bytes, IP=Last 15Bytes)

							parseBACompleteTLV(EMFC_TLV.TLVValue, &AID, &CID, toIP);

							fprintf(HA_FD, "(BA_COMPLETE) TLVValue received from Core is AID(%d), CID(%d), toIP(%s).\n\n", AID, CID, toIP);

							fromIndex = getIndexFromInterfaceIP(toIP);
							
							// NOTE:
							// We Identify for which port this BA_COMPLETE is sent.
							for (portIndex=0; portIndex<PROTOCOLS; portIndex++)
							{
								if (BATransaction[portIndex].manualBAFlag != 2)		// '2' is invalid code (Neither Manual nor Auto BA)
								{
									if (BATransaction[portIndex].AIDPointer->associationID == AID)		// Does it contain the AID, we are looking for?
									{
										index = portIndex;
										portIndex = PROTOCOLS;		// Break the Loop
									}
								}
							}
							portIndex = index;						// Re-assign the value for Code Readability
							
							if (BATransaction[portIndex].manualBAFlag == 1)								// Case 1: Manual BA
							{
								//fromIndex = BATransaction[portIndex].fromLinkIndex;
							
								//-------------------Update ConnInfo Linked List-------------------------------------------------
								BATransaction[portIndex].AIDPointer->connectionID[fromIndex] = CID;		// Add CID to ConnInfo
								BATransaction[portIndex].AIDPointer->BACount += 1;						// Increment BACount

								if (userPref.indexOfMaxPrefHOIP[0] == fromIndex)	// Is connection made on the interface of MaxPrefHOIP?
								{
									userPref.indexOfMaxPrefHOIP[1] -= 1;			// We decrement it by ONE, so that userAgent can
																					// identify that at least a connection is through this IP
									updateUserPrefFile();							// Map changes to user preference file
									informUserAgent();
									//fprintf(HA_FD, "HA:BAComplete => userPref.indexOfMaxPrefHOIP[1]=%d.\n", userPref.indexOfMaxPrefHOIP[1]);									
								}
								//-----------------------------------------------------------------------------------------------

								BATransaction[portIndex].AIDPointer = BATransaction[portIndex].AIDPointer->nextAID;		// Increment AID Pointer
								TLVUnit.intValue = getNextAIDInBATransaction(portIndex);
							
								if (TLVUnit.intValue == -1)			// BA Transaction is Complete
								{
									BATransaction[portIndex].manualBAFlag = 2;									// Invalid Code
									UA_TLV.TLVType = BA_COMMIT_CONF;
									strcpy(UA_TLV.TLVValue, userPref.adv_interfaces_name[fromIndex]);			// IP
									strcat(UA_TLV.TLVValue, " is now part of Aggregation for ");
									strcat(UA_TLV.TLVValue, userPref.adv_protocol_name[portIndex]);	   			// Port
									strcat(UA_TLV.TLVValue, ".\0");
									
									
									if (vm_send(UA_FD, &UA_TLV, sizeof(struct TLVInfo), 0) == -1)
										perror("ERROR (BA_COMPLETE Send) ");
									else
									{
										fprintf(HA_FD, "HA : (BA_COMPLETE) TLVValue SENT TO UserAgent is (%s).\n\n", UA_TLV.TLVValue);
									}
								}
								else								// BA Transaction is Not Complete
								{
									EMFC_TLVTypeLength[0] = BA_COMMIT;
									composeBACommitTLV(portIndex);

									if (vm_send(EMFC_FD, &EMFC_TLVTypeLength, 2, 0) == -1)
										perror("ERROR (BA_COMPLETE Send) ");
									else
									{
										if (vm_send(EMFC_FD, EMFC_TLVValue, EMFC_TLVTypeLength[1], 0) == -1)
											perror("ERROR (BA_COMPLETE Send) ");
										else
										{
											fprintf(HA_FD, "HA : (BA_COMPLETE) TLVValue SENT TO Core is AID(%d), IP(%s).\n", 
																										TLVUnit.intValue, IPAddress[fromIndex]);
										}
									}
								}
							}
							
							else if (BATransaction[portIndex].manualBAFlag == 0)			// Case 2: Auto BA
							{
								//-------------------Update ConnInfo Linked List-------------------------------------------------
								BATransaction[portIndex].AIDPointer->connectionID[fromIndex] = CID;		// Add CID to ConnInfo
								BATransaction[portIndex].AIDPointer->BACount += 1;						// Increment BACount

								if (userPref.indexOfMaxPrefHOIP[0] == fromIndex)	// Is connection made on the interface of MaxPrefHOIP?
								{
									userPref.indexOfMaxPrefHOIP[1] -= 1;			// We decrement it by ONE, so that userAgent can
																					// identify that at least a connection is through this IP
									updateUserPrefFile();							// Map changes to user preference file
									informUserAgent();
									//fprintf(HA_FD, "HA:BAComplete => userPref.indexOfMaxPrefHOIP[1]=%d.\n", userPref.indexOfMaxPrefHOIP[1]);									
								}
								//-----------------------------------------------------------------------------------------------
								
								addRemoveIPCountInBATransaction[portIndex][1] -= 1;			// Decrement 1, as One connection Added.
								autoBATransaction(portIndex);					// Auto BA Messages to EMF Core are sent from inside this function
							}
							display(&connectionInfoHead);
						break;
					}
				}
			}
			else if (FD_ISSET(CL2I_FD, &tmpFDList))
			{
				bytesReceived = vm_recv(CL2I_FD, &CL2ITrigger, sizeof CL2ITrigger, 0/*MSG_WAITALL*/);
				if (bytesReceived == -1)
					perror("ERROR (CL2I Receive) ");
					
				else if (bytesReceived == 0)
				{
					fprintf(HA_FD, "\n******* Connection Closed by CL2I *******\n");
					FD_CLR(CL2I_FD, &baseFDList);
					vm_close(CL2I_FD);
				}
				else
				{
					switch(CL2ITrigger.triggerType)		// Process Request based on Trigger Type
					{
						case LINK_DETECTED:				// Link Detected Trigger
						break;

						case LINK_UP:					// Link Up trigger
							
							fprintf(HA_FD, "\nHA : (LINK_UP) TLVValue RECEIVED FROM CL2I with Interface(%s), IP(%s).\n", 
																								CL2ITrigger.interfaceName, CL2ITrigger.IP);
							// NOTE:
							// In the next phase, we can implement the following.
							// "A link went down in the past its LDTransaction is in progress. Suddenly same link goes up.
							// We can terminate such LDTransaction at the current point & clear the lock so that TCP can keep 
							// sending data on the remaining connections. The interface will be stored at the same index where it went down.
							// We can send HO_CANCLE(fromLink) message to EMFCore."
							
							if (index=sameInterfaceExists() > -1)		// either IndexValue='[0 - Interfaces]' or '-1'=DoesNotExist is returned.
							{
								if (LDTransactionLock[index] == 1)		// Is LD Processing going on?
								{
									// Send 'HO_CANCEL(fromIP)' message to EMFCore
									//EMFC_TLVTypeLength = HO_CANCEL;						// TLV Type
									//strcpy(EMFC_TLVValue, CL2ITrigger.IP);				// Composing TLV Value
									//if (send(EMFC_FD, &EMFC_TLV, sizeof(struct TLVInfo), 0)==-1)
									//	perror("ERROR (LINK_UP Send)");
									//else
									//	fprintf(HA_FD, "HO_CANCEL sent to UA.\n");
									LDTransactionLock[index] = 0;						// Unlock Interface
									strcpy(IPAddress[index], CL2ITrigger.IP);			// Update IP Address
								}
								//else if (LDTransactionLock[index] == 0)				// IP changed (Dynamic IP Acquisition)
							}
							else														// Interface does Not already exists.
							{
								index = getFreeIndex();
								if (index == -1)
								{
									fprintf(HA_FD, "\nHA : (LINK_UP) INFORMATION: NO CAPACITY TO ADJUST NEW INTERFACE.\n");
								}
								else
								{
									if (IPAddress[index] != NULL) free(IPAddress[index]);			// Free the memory, if held

									// Update User Agent structure 'adv_interfaces_name' at position=index
									strcpy(userPref.adv_interfaces_name[index], CL2ITrigger.interfaceName);

									length = strlen(CL2ITrigger.IP);
									IPAddress[index] = (char*) malloc (length+1);
									strcpy(IPAddress[index], CL2ITrigger.IP);
									IPAddress[length] = '\0';

									// Map changes to User Preference File
									updateUserPrefFile();

									////////////////////////////////////////////////////////////////////////////////////////////////////
//									if (userPref.generalEMFoptions[1] == 1)			// Is "Inform when New Link is available" checked?
//									{
										// Send Update Message to User Agent containing 'index' value at which
										// change has been made
										UA_TLV.TLVType = LINK_UP_Popup;								// TLV Type
										strcpy(UA_TLV.TLVValue, CL2ITrigger.interfaceName);			// TLV Value
										strcat(UA_TLV.TLVValue, " is UP.\0");						// TLV Value
									
										if (vm_send(UA_FD, &UA_TLV, sizeof(struct TLVInfo), 0) == -1)
											perror("\nERROR (LINK_UP Send)");
										else
										{
											fprintf(HA_FD, "HA : (LINK_UP) TLVValue SENT TO UserAgent is (%s).\n", UA_TLV.TLVValue);
										}
//									}

									//printf("\n\nBefore DNS Code...\n\n");

									////////////////////////////// Start (DNS Code) ////////////////////////////
									strcpy(DNSLocationUpdate.thisNodeIP, IPAddress[index]);
									DNSLocationUpdate.thisNodeIP[strlen(IPAddress[index])] = '\0';
									if(vm_create_thread(NULL, NULL, (void *)addDNSRecord, &DNSLocationUpdate,0,0) == -1)
									{
										addDNSRecord(&DNSLocationUpdate);
									}
									else
									{
										if (userPref.generalEMFoptions[4] == 1)
										{
											UA_TLV.TLVType = LOCATION_UPDATE_IND;				// TLV Type
											strcpy(UA_TLV.TLVValue, IPAddress[index]);			// TLV Value
											strcat(UA_TLV.TLVValue, " added to DNS.\0");		// TLV Value

											if (vm_send(UA_FD, &UA_TLV, sizeof(struct TLVInfo), 0) == -1)
												perror("\nERROR (LOCATION_UPDATE_IND Send)");
											else
											{
												//fprintf(HA_UA_FD, "%s, (LOCATION_UPDATE_IND) TLVValue sent to UserAgent is (%s).\n", 
																									//getMicroSecFormattedTime(), UA_TLV.TLVValue);
												fprintf(HA_FD, "HA : (LOCATION_UPDATE_IND) TLVValue SENT TO UserAgent is (%s).\n", UA_TLV.TLVValue);
											}
										}
									}	
									////////////////////////////// END (DNS Code) ////////////////////////////

									//printf("\n\nAfter DNS Code...\n\n");

									// NOTE:
									// (Only for this phase), HO/BA Algorithm is NOT written here. It will be placed here 
									// when UA will send USER_PREFERENCE_CHANGE_IND message to HostAgent.
								}
							}
						break;

						case LINK_DOWN:			// Link Down Trigger

							// NOTE:
							// Send Interface Name to UserAgent, and then Empty the values in All arrays against that index.
							// Now if an interface goes down and then goes UP, we store it at an Empty and Unlocked position.
							
							index = getIndexFromInterfaceName(CL2ITrigger.interfaceName);
							if (index == -1)
								fprintf(HA_FD, "\nHA : ERROR (LINK_DOWN) INTERFACE (%s : %s) NOT PRESENT.\n", CL2ITrigger.interfaceName, CL2ITrigger.IP);
							else
							{
								fprintf(HA_FD, "\nHA : (LINK_DOWN) TLVValue RECEIVED FROM CL2I is Interface(%s), IP(%s).\n", 
																										CL2ITrigger.interfaceName, CL2ITrigger.IP);

							   	// Deleting the information associated with the LinkDown Interface.
							   	strcpy(userPref.adv_interfaces_name[index], "");
							   	userPref.adv_interfaces[index] = 0;
								maxPrefIndex = getIndexOfMaxPrefIPInHOPref(-1);
								
								if (maxPrefIndex > -1)
								{
									userPref.indexOfMaxPrefHOIP[0] = maxPrefIndex;
									userPref.indexOfMaxPrefHOIP[1] = 0;					// No Connection Present right now									
							   	}
							   	else if (maxPrefIndex == -1)
							   	{
							   		userPref.indexOfMaxPrefHOIP[0] = -1;				// HO Preference NOT Found.
									userPref.indexOfMaxPrefHOIP[1] = 0;					// No Connection Present right now							   		
							   	}
							   	
							   	// Map changes to userPreference file
							   	updateUserPrefFile();
								informUserAgent();
								//fprintf(HA_FD, "HA:BAComplete => userPref.indexOfMaxPrefHOIP[1]=%d.\n", userPref.indexOfMaxPrefHOIP[1]);
								

								////////////////////////////////////////////////////////////////////////////////////////////////////
//								if (userPref.generalEMFoptions[1] == 1)			// Is "Inform when New Link is available" checked?
//								{
									// Send Update Message to User Agent containing 'index' value at which
									// change has been made
									UA_TLV.TLVType = LINK_DOWN_Popup;								// TLV Type
									strcpy(UA_TLV.TLVValue, CL2ITrigger.interfaceName);				// TLV Value
									strcat(UA_TLV.TLVValue, " is Down.\0");							// TLV Value
								
									if (vm_send(UA_FD, &UA_TLV, sizeof(struct TLVInfo), 0) == -1)
										perror("\nERROR (LINK_DOWN Send)");
									else
									{
										fprintf(HA_FD, "HA : (LINK_DOWN) TLVValue SENT TO UserAgent is (%s).\n", UA_TLV.TLVValue);
									}
//								}

								//printf("\n\nBefore DNS Code...\n\n");

								////////////////////////////// Start (DNS Code) ////////////////////////////
								strcpy(DNSLocationUpdate.thisNodeIP, IPAddress[index]);
								DNSLocationUpdate.thisNodeIP[strlen(IPAddress[index])] = '\0';
								if(vm_create_thread(NULL, NULL, (void *)deleteDNSRecord, &DNSLocationUpdate,0,0) == -1)
								{
									deleteDNSRecord(&DNSLocationUpdate);
								}
								else
								{
									if (userPref.generalEMFoptions[4] == 1)
									{
										UA_TLV.TLVType = LOCATION_UPDATE_IND;					// TLV Type
										strcpy(UA_TLV.TLVValue, IPAddress[index]);				// TLV Value
										strcat(UA_TLV.TLVValue, " removed from DNS.\0");		// TLV Value

										if (vm_send(UA_FD, &UA_TLV, sizeof(struct TLVInfo), 0) == -1)
											perror("\nERROR (LOCATION_UPDATE_IND Send)");
										else
										{
											fprintf(HA_FD, "HA : (LOCATION_UPDATE_IND) TLVValue SENT TO UserAgent is (%s).\n", UA_TLV.TLVValue);
										}
									}
								}
								////////////////////////////// END (DNS Code) ////////////////////////////
								
								//printf("\n\nAfter DNS Code...\n\n");

								// NOTE:
								// This code gets an interface which is NOT-Under EMF, to maintain Service Continuity.
								if (maxPrefIndex == -1)									// No IP found in Handover Preferences
								{
									if (userPref.forcedHO == 1)							// Did user select Forced Handover for Service Continuity?
										for (j=0; j < INTERFACES; j++)										// Traverse IPAddress
											if ((IPAddress[j] != NULL) && (userPref.adv_interfaces[j] == 0))	// Is Non-EMF Interface Running?
											{
												maxPrefIndex = j;								// Non-EMF running IP found
												strcpy(IPNotUnderEMF, IPAddress[j]);			// Store Non-EMF IP
												IPNotUnderEMF[strlen(IPAddress[j])] = '\0';		// Place '\0', on the safe side
												j = INTERFACES;									// Break the Loop
											}
								}
								
								// NOTE:
								// If neither HO_Preference IP nor Non-EMF IP is found, maxPrefIndex contains -1.
								if (maxPrefIndex > -1)
								{
									if (HOLinkStatus.isActive[index][0] == 1)					// Does fromHOLink contains connections?
									{
										//if (EMFC_TLVValue != NULL) free(EMFC_TLVValue);		// Free the memory, if held
										
										LDTransactionLock[index] = 1;							// Position Locked till LD processing is NOT Complete
									
										// Compose & Send ONE HO Command, also send 'AIDwise removeFromBAInHO' cases, if applicable.
										EMFC_TLVTypeLength[0] = HO_LINK_SHIFT_COMMIT;

										EMFC_TLVTypeLength[1] = strlen(IPAddress[index]) + strlen(IPAddress[maxPrefIndex]) + 2;
										EMFC_TLVValue = (char*) malloc (EMFC_TLVTypeLength[1]);
										strcpy(EMFC_TLVValue, IPAddress[index]);					// From IP Address
										strcat(EMFC_TLVValue, ":");									// ':' is IP Seperator
										strcat(EMFC_TLVValue, IPAddress[maxPrefIndex]);				// To IP Address
										EMFC_TLVValue[strlen(EMFC_TLVValue)] = '\0';
					
										if (vm_send(EMFC_FD, &EMFC_TLVTypeLength, 2, 0) == -1)
											perror("ERROR (HO_LINK_SHIFT_COMMIT_REQ Send) ");
										else
										{
											if (vm_send(EMFC_FD, EMFC_TLVValue, EMFC_TLVTypeLength[1], 0) == -1)
												perror("ERROR (HO_LINK_SHIFT_COMMIT_REQ Send) ");
											else
											{
												fprintf(HA_FD, "HA : (LINK_DOWN) TLVValue SENT TO Core is (%s).\n\n", EMFC_TLVValue);
											}
											// NOTE:
											// HA will receive EMF Core's per Association reply under 'case HANDOVER' & 'REMOVE_CONNECTION'.
											// Handover Trasaction will Complete under "case HO_LINK_SHIFT_COMPLETE" in EMF Core portion.
										}
									}

									// Step 2. For the downLink in BA, send AIDwise_Remove_From_BA
									BATransactionInHOLinkDown[index].AIDPointer = connectionInfoHead;		// One Time Assignment, at start

									BATransactionInHOLinkDown[index].fromLinkIndex = index;					// Copy HO fromLinkIndex/RemoveFromBAIndex
									BATransactionInHOLinkDown[index].toLinkIndex = maxPrefIndex;			// Copy HO toLinkIndex
									TLVUnit.intValue = getNextAIDInHOLDTransaction(index);			
					
									if (TLVUnit.intValue == -1)				// No Connection is available for removeFromBAInHO
									{
										//fprintf(HA_FD, "Noting for removeFromBA in LinkDown.\n");
									}
									else
									{
										BATransactionInHOLinkDown[index].HOIdentifier = 3;
										EMFC_TLVTypeLength[0] = LD_AIDwise_REMOVE_FROM_BA_COMMIT;
										LD_ComposeRemoveFromBATLVInHO(index);					// Down link index is passed

										if (vm_send(EMFC_FD, &EMFC_TLVTypeLength, 2, 0) == -1)
											perror("ERROR (LinkDown Send) ");
										else
										{
											if (vm_send(EMFC_FD, EMFC_TLVValue, EMFC_TLVTypeLength[1], 0) == -1)
												perror("ERROR (LinkDown Send) ");
											else
											{
												fprintf(HA_FD, "HA : (AIDwise_BA_REMOVE_IN_LINK_DOWN) TLVValue SENT TO Core is AID(%d), fromIP(%s).\n\n",  
																										TLVUnit.intValue, IPAddress[index]);
												
												userPref.BAPreference[BATransactionInHOLinkDown[index].AIDPointer->TRIDIndex][index] = 0;
											}
										}
									}
								}
							}
						break;

						case LINK_GOING_DOWN:	// Link Going Down Trigger

							// Execute Handover Algorithm
						break;
					}
				}
			}
		}
	}
	//return 0;
}

