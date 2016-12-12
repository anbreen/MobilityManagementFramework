///////////////////////////////////////////////////////////////////
//
// Host Agent Managed Structure
//
#ifdef _WIN32

#include <stdio.h>

#else

#include <stdio.h>
#include <time.h>

#endif

#define INTERFACES 20
#define PROTOCOLS  INTERFACES

//******************************************************************************************************

// This structure is also maintained by User Agent
struct userPreferences
{
	// Host Agent will use it 
	char generalEMFoptions[5]; 			// [0, 1=Interface_Available_Popup, 2=Auto Handover_Complete_Popup, 
										// 3=Multiple_Interfaces_Available_for_BA_Popup, 4=Location_Update_Popup]
	// HA will use it.
	char adv_interfaces[20];			// Advanced Tab -> Interfaces,
										// [0] => 0(Interface NOT under EMF)/1(Interface under EMF)
	// HA will use it.
	char adv_interfaces_name[20][20];	// Advanced Tab -> Interfaces
										// [0][0] => eth0, [1][0] => wlan0, etc.
										// Fixed => eth0(0), wlan0(1), wlan1(2), wimax0(3), gprs0(4), umts(5)
	// HA will use these.
 	char anony_cert[2];					// 1st Value Anonymous(0)/Certificate(1), If Anonymous selected use [2nd Elliptic Curve(0)/D-Hellman(1)]
	char certificatePath[200];			// Certificate Path, if Certificate is selected use Key
	char key[100];						// Path of Private Key
	
	// HA will use it.
	char adv_protocol[20];				// Advanced Tab -> Protocol
										// [0] => 0 (Protocol NOT under EMF), [0] => 1 (Protocol under EMF)
	// HA will NOT use it.
	char adv_protocol_name[20][20];		// Advanced Tab -> Protocol
										// e.g. [0][0] => FTP, [1][0] => HTTP
	// HA will use it.
	int adv_protocol_PortNo[20];		// Advanced Tab -> Protocol, contains Port Number against each Protocol
										// [0][0] => 20 (e.g.), [1][0] => 8080 (e.g.)
											
	char forcedHO;						// Forced Handover char contains either 0 or 1
	
	// This is Missing
	//int HOProtocolWisePref[20][20];	// [0][0] => [indexOf adv_protocol] [indexOf adv_interfaces], etc.

	// HA will use it.
	//int HOPreference[20][5];			// [0=costValue, 1=DRValue, 2=Pref, 3=costUnit, 4=DRUnit] => HA will scale these values

	float HOPreference[20][7];			// [0=costValue, 1=DRValue, 2=Pref, 3=costUnit, 4=DRUnit] => HA will scale these values
										// Host Agent requires following infromation against HO Preferences
										// 1.InterfaceID, 2.IPAddress, 3.Preference, 4.down/up/running=0/1/2, 
										// 5.Scaled value of Cost & Data Rate, 6. isActive
	// HA will use it. 
	char BAPreference[20][20];			// [0][0] => [indexOf adv_protocol] [indexOf adv_interfaces]
										//		=> value is '1'(under BA) or '0'(Not under BA)
										// [0][1] => [indexOf adv_protocol] [indexOf adv_interfaces]
										//		=> value is '1'(under BA) or '0'(Not under BA)
										
	char TotalInterfaces;				// Holds Currently Stored Running Interfaces 
	char indexOfMaxPrefHOIP[2];			// [0] = > Contains the index of max preferred IP in Handover Preferences
										// [1] = > Contains the no. of connection at this IP
}
userPref;

//*************************************************************************************************************

/*
struct HOLinkStatusInfo
{
	char isActive[2];						// Value '1' at index [0] means Data is communicating over it [from EMF Core],
											// index [1] contains number of connections made on this interface.
	char isProtocolSet[PROTOCOLS];			// We will just set/unset corresponding protocol's entry indicating that
											// among so many connections, atleast one is present using this protocol as well.
}
HOLinkStatus[INTERFACES];
*/

struct HOLinkStatusInfo
{
	char isActive[INTERFACES][2];						// Value '1' at index [0] means Data is communicating over it [from EMF Core],
											// index [1] contains number of connections made on this interface.
	char isProtocolSet[PROTOCOLS];			// We will just set/unset corresponding protocol's entry indicating that
											// among so many connections, atleast one is present using this protocol as well.
}
HOLinkStatus;


// Purpose:
// To diffrentiate interface data transfer, port-wise. E.g. for FTP eth0 is isActive but for HTTP it is NOT.
struct BAPortWiseLinkStatusInfo
{
	char isActive[INTERFACES];				// For each Traffic, we have isActive status equal to INTERFACES
}
BAPortWiseLinkStatus[INTERFACES];			// Each entry points to a portNo


int socketFD, acceptFD, clientSize, bytesReceived, maxPrefIndex, minPrefIndex;
int UA_FD, EMFC_FD, CL2I_FD;				// Stores descriptors for UA, EMFCore, CL2I. HA uses them to identify data source.
struct Node *connectionInfoHead = NULL;
char formattedString[100];					// contains formatted string to be printed on File and Terminal.

FILE *HA_FD, *HA_UA_FD, *HA_CL2I_FD, *HA_AH_FD, *HA_DH_FD, *HA_UA_DH_AH_CL2I_FD;

// Variables used for getting system time
time_t myTime;
const struct tm *tm = NULL;
struct timeval microTime;
static char timeStamp[28], milliMicroTime[8];
short milliSeconds;
long int microSeconds;
int seqNo=1;

// DNS Related Variables
char DNSIP[16], *EMFDomain;

// Helps to Identify which Interface is passing through LinkDown(LD) Processing
char LDTransactionLock[INTERFACES];

// Handover-to-BA case IPs. 
// Explanation:A Single link is not in BA preferences and connection(s) are establised on it. Now user adds it in BA Preferences along other
// links, so we include in the variable defined below, all other interface IPs except this link IP.
char *HOtoBAIPs=NULL, extractedIP[16];

// stores Previous BAPreferences
char prevBAPref[INTERFACES][INTERFACES];

// Variable used by connectionInfoLL file for informing User Agent
char informUserAgentFlag = 0;

// Holds IP Addresses
char *IPAddress[INTERFACES], *addToBAIPs[PROTOCOLS], *removeFromBACIDs[INTERFACES], *removeFromBAIPs[PROTOCOLS];
char IPNotUnderEMF[16];

char manualBACommit = 2, manualHOCommit=0, LD_HOCommit=0, HOLinkShiftCompleteFlag[INTERFACES], LD_HOLinkShiftCompleteFlag[INTERFACES];
char addRemoveIPCountInBATransaction[PROTOCOLS][2], LD_AIDwiseRemoveFromBATransaction[INTERFACES];
char addToBAMessageFlag[PROTOCOLS], removeFromBAMessageFlag[PROTOCOLS], localComlpianceReq[PROTOCOLS];

//char index, j;						// Loop variables used frequently inside functions
