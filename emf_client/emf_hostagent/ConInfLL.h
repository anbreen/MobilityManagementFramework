
#include "HACommon.h"					// Common Storage Here.

struct Node
{
	int associationID;						// EMF AID
	char TRIDIndex;							// IndexOf Protocol PortNo e.g. FTP(23)
	char BACount;							// [BACount=1] means 'No BA', [BACount >=2] means 'BA' 
	int connectionID[INTERFACES];			// Each CID is placed at the index where an IP address 
											// is present in IPAddress array maintained by Host Agent
	struct Node *nextAID;
};

// Purpose:
// When a connection is established on 'toHOIP' then this function increment #CIDs on this link.
// For the first time it marks is Active i.e. ONE or MORE connections are present on this "toHOIP" link.
void setActive(struct Node *currentNode, char indexOfIPAddress)
{
	// After Adding Connection
	char isPartOfBAPrefFlag = 0, index;
	
	if (currentNode->BACount == 1)			// Can be BA or HO Case
	{
		// NOTE:
		// If user wants to use BA, minimum TWO preferences should be specified in BA Preferences for a Protocol portNo.

		// We check in BA Preferences Matrix, if a preference other than the indexOfIPAddress is present?
		for(index=0; index<INTERFACES; index++)
		{
			if ((userPref.BAPreference[currentNode->TRIDIndex][index] == 1) && (index != indexOfIPAddress))	// BA Preference for this portNo present?
			{
				isPartOfBAPrefFlag = 1;			// BA Preference available for the requested port
				index = INTERFACES;				// Break the Loop
			}
		}
	
		if (isPartOfBAPrefFlag == 0)			// No BA Preference available, HO Preference
		{
			HOLinkStatus.isActive[indexOfIPAddress][0] = 1;			// Set Active Everytime
			HOLinkStatus.isActive[indexOfIPAddress][1] += 1;		// Increment Connections
			
			// NOTE:
			// We maintain "Number of connections" in [1] because, we only have one isActive bit for all the connections
			// in HO case. We increment its value when a connection is created on maxPrefHOIP. We decrement its value 
			// when a connection is removed closed and its entry is removed from connectionInfo Linded List.
		}	
	}
}

// Purpose:
// Add New Connection ID (CID) in "connectionInfo" LL against the AID passed as argument.
char addCID(int AID, int CID, char indexOfIPAddress)
{
	struct Node *tmp = connectionInfoHead;

	if (connectionInfoHead != NULL)
	{
		while ((tmp->associationID != AID) && (tmp->nextAID != NULL))	// Locate AID
			tmp = tmp->nextAID;
		
		if ((tmp->nextAID == NULL) && (tmp->associationID != AID))
			return -1;													// error code (AID Not Found)
		else
		{	
			// When AID is found, then, against this AID entry,
			// place CID in the connectionID array at the position specified by indexOfIPAddress
			// and increment available CIDs at index=0.
			if (tmp->connectionID[indexOfIPAddress] > 0)					// Connection is already present
			{
				tmp->connectionID[indexOfIPAddress] = CID;					// Replace CID
			}	
			else															// Connection not already present
			{
				tmp->connectionID[indexOfIPAddress] = CID;					// Add CID
				tmp->BACount += 1;											// Update #of CIDs

				if (userPref.indexOfMaxPrefHOIP[0] == indexOfIPAddress)		// Is connection made on the interface of MaxPrefHOIP?
				{
					userPref.indexOfMaxPrefHOIP[1] += 1;					// We decrement it by ONE, so that userAgent can
																			// identify that at least a connection is through this IP
					informUserAgentFlag = 1;								// Host Agent will check after function call, it this flag is set.
					//printf("addCID => userPref.indexOfMaxPrefHOIP[1]=%d.\n", userPref.indexOfMaxPrefHOIP[1]);
				}
			}
			
			setActive(tmp, indexOfIPAddress);							// Call setActive()
			return 0;
		}
	}
}

// Purpose:
// Add New Association ID (AID) in "connectionInfo" LL
char addAID(int AID, char TRIDIndex, int CID, char indexOfIPAddress)
{
	struct Node *tmp = connectionInfoHead;
	char index;

	if (connectionInfoHead == NULL)												// Link List is Empty
	{
		if ((connectionInfoHead = malloc (sizeof (struct Node))) == NULL)		// Allocate Memroy
		{
			perror("\nERROR (addAID) malloc ");						// Malloc returned ERROR
			fprintf(HA_FD, "\nAttempting to get Memory from OS...");
			
			for(index=1; index<11; index++)							// Make 10 Memory Requests
			{
				connectionInfoHead = malloc (sizeof (struct Node));
				if (connectionInfoHead != NULL)
					index = 11;				// Break the Loop
			}
		}
		if (connectionInfoHead != NULL)
			tmp = connectionInfoHead;
		else
		{
			fprintf(HA_FD, "\n +++++++Insufficient Memory Resources.+++++++ \n");
			return -1; 
		}
	}
	else															// Link List is Not Empty
	{
		tmp = connectionInfoHead;
		while (tmp->nextAID != NULL)
			tmp = tmp->nextAID;

		tmp->nextAID = malloc (sizeof (struct Node));				// Create Element
		tmp = tmp->nextAID;											// Move to the Last Element, we have just created
	}

	// Paste Data
	tmp->associationID = AID;
	tmp->TRIDIndex = TRIDIndex;
	tmp->BACount = 0;
	tmp->nextAID = NULL;

	// Initialize all the CIDs with ZERO
	for(index=0; index<INTERFACES; index++)
		tmp->connectionID[index] = 0;
	
	// Call addCID
	addCID (AID, CID, indexOfIPAddress);
	
	return 0;
}

// Purpose:
// Remove an Association ID (AID)
char removeAID(int AID)
{
	struct Node *tmp = connectionInfoHead, *prevNode;

	while ((tmp->associationID != AID) && (tmp->nextAID != NULL))
	{
		prevNode = tmp;
		tmp = tmp->nextAID;
	}
	if ((tmp->nextAID == NULL) && (tmp->associationID != AID))	// AID Not Found
		return -1;												// error code

	else
	{
		if (tmp == connectionInfoHead)				// First Node
		{
			if (tmp->nextAID == NULL)				// If First = Last Node, Delete it
			{
				//free(connectionInfoHead);
				//free(tmp);						// Duplicate effort on the local variable
				tmp = NULL;
				
				return 1;							// AID List Deleted.
			}
			else
				connectionInfoHead = tmp->nextAID;
		}
		else if ((tmp->nextAID == NULL) && (tmp->associationID == AID))		// Last Node
			prevNode->nextAID=NULL;
		else
			prevNode->nextAID = tmp->nextAID;		// Middle Node

		tmp->nextAID = NULL;
		//free(tmp);
		return 0;									// Success Code
	}
}

// Purpose:
// When a connection is removed from 'fromHOIP' then this function decrements #CIDs on this link
// and if #CIDs equals ZERO, it sets 'fromHOIP' as InActive i.e. NOW NO connection is present on it.
void setInActive(struct Node *currentNode, char indexOfIPAddress)
{
	// After Removing Connection
	char isPartOfBAPrefFlag = 0, index;
	
	if (currentNode->BACount == 0)		// Can be BA or HO case, we Identify it.
	{
		// NOTE:
		// If user wants to use BA, minimum TWO preferences should be specified in BA Preferences for a Protocol portNo.

		// We check in BA Preferences Matrix, if a preference other than the indexOfIPAddress is present?
		for(index=0; index<INTERFACES; index++)
		{
			if ((userPref.BAPreference[currentNode->TRIDIndex][index] == 1) && (index != indexOfIPAddress))	// BA Preference for this portNo present?
			{
				isPartOfBAPrefFlag = 1;			// BA Preference available for the requested port
				index = INTERFACES;				// Break the Loop
			}
		}

		if (isPartOfBAPrefFlag == 0)			// No BA Preference available, HO Preference
		{
			HOLinkStatus.isActive[indexOfIPAddress][1] -= 1;			// Decrement Connections
			
			if (HOLinkStatus.isActive[indexOfIPAddress][1] == 0)		// Only One Connection Present
				HOLinkStatus.isActive[indexOfIPAddress][0] = 0;			// Set InActive
			
			// NOTE:
			// We maintain "Number of connections" in [1] because, we only have one isActive bit for all the connections
			// in HO case. We increment its value when a connection is created on maxPrefHOIP. We decrement its value 
			// when a connection is removed closed and its entry is removed from connectionInfo Linded List.
		}	
	}
}

// Purpose:
// Remove a Connection ID (CID)
int removeCID(int AID, int CID)
{
	struct Node *tmp = connectionInfoHead;
	int index;

	while ((tmp->associationID != AID) && (tmp->nextAID != NULL))	// Locate AID
		tmp = tmp->nextAID;

	if ((tmp->nextAID == NULL) && (tmp->associationID != AID))		// Invalid AID
		return -1;													// error code
	else
	{
		for(index=0; index<INTERFACES; index++)
		{
			if (tmp->connectionID[index] == CID)
			{
				tmp->connectionID[index] = 0;						// Delete it
				tmp->BACount -= 1;									// Update #of CIDs
				
				if (userPref.indexOfMaxPrefHOIP[0] == index)		// Is connection made on the interface of MaxPrefHOIP?
				{
					userPref.indexOfMaxPrefHOIP[1] -= 1;			// We decrement it by ONE, so that userAgent can
																	// identify that at least a connection is through this IP
					informUserAgentFlag = 1;						// Host Agent will check after function call, it this flag is set.
					//printf("removeCID => userPref.indexOfMaxPrefHOIP[1]=%d.\n", userPref.indexOfMaxPrefHOIP[1]);
				}

				setInActive(tmp, index);							// call setInActive()
				return 0;
			}
		}
	}
	return -1;
}

// Purpose:
// When 'Handover' message is received from EMFCore, 'HOUpdate()' is executed
// so that 'HOFromLink' can be removed and 'HOToLink' can be added
int HOUpdate(int AID, int oldCID, int newCID, char indexOfIPAddress)
{
	int index;
	struct Node *tmp = connectionInfoHead;

	while ((tmp->associationID != AID) && (tmp->nextAID != NULL))	// Locate AID
		tmp = tmp->nextAID;

	if ((tmp->nextAID == NULL) && (tmp->associationID != AID))		// AID Not Found
		return -1;													// error code 

	for (index=0; index<INTERFACES; index++)
	{
		if (tmp->connectionID[index] == oldCID)
		{
			tmp->connectionID[index] = 0; 						// Delete CID

			HOLinkStatus.isActive[index][1] -= 1;				// Decrement #CIDs
			if (HOLinkStatus.isActive[index][1] == 0)			// Only One Connection Present
				HOLinkStatus.isActive[index][0] = 0;			// Set InActive

			// Is connection made on the interface of MaxPrefHOIP?
			if ((userPref.indexOfMaxPrefHOIP[0] == index) && (userPref.indexOfMaxPrefHOIP[1] > 0))	// From Link is maxPrefHOIP.
			{
				userPref.indexOfMaxPrefHOIP[1] -= 1;			// We decrement it by ONE, so that userAgent can
																// identify that at least a connection is through this IP
				informUserAgentFlag = 1;						// Host Agent will check after function call, it this flag is set.
				//printf("HOUpdate => userPref.indexOfMaxPrefHOIP[1]=%d.\n", userPref.indexOfMaxPrefHOIP[1]);
			}
			else if (userPref.indexOfMaxPrefHOIP[0] == indexOfIPAddress)							// To Link is maxPrefHOIP.
			{
				userPref.indexOfMaxPrefHOIP[1] += 1;			// We decrement it by ONE, so that userAgent can
																// identify that at least a connection is through this IP
				informUserAgentFlag = 1;						// Host Agent will check after function call, it this flag is set.
				//printf("HOUpdate => userPref.indexOfMaxPrefHOIP[1]=%d.\n", userPref.indexOfMaxPrefHOIP[1]);
			}

			index = INTERFACES;
		}
	}
	tmp->connectionID[indexOfIPAddress] = newCID; 				// Add New CID at the Index where IP is present in IPAddress array.

	HOLinkStatus.isActive[indexOfIPAddress][0] = 1;				// Set Active
	HOLinkStatus.isActive[indexOfIPAddress][1] += 1;			// Increment #CIDs
	
	return 0;
}

// Delete connectionInfo Linked List when EMFCore TCP Connection is closed.
void deleteConnInfoLL()
{
	struct Node *nextNode, *tmpNode = connectionInfoHead;
	while (tmpNode->nextAID != NULL)
	{
		fprintf(HA_FD, "AID = %d ", tmpNode->associationID);
		nextNode = tmpNode->nextAID;
		tmpNode->nextAID = NULL;
		free(tmpNode);													// Delete tmpNode
		fprintf(HA_FD, " Deleted.\n");
		tmpNode = nextNode;
	}
	fprintf(HA_FD, "AID = %d ", tmpNode->associationID);
	tmpNode->nextAID = NULL;
	free(tmpNode);														// Delete tmpNode
	fprintf(HA_FD, " Deleted.\n");
}

// Display
int display()
{
	struct Node *tmp = connectionInfoHead;
	int index, n=0, flag=0, flag2=0;

	if (connectionInfoHead != NULL)
	{
		do
		{
			fprintf(HA_FD, "\nAID=%d : port=%d : CIDs=(", tmp->associationID,  userPref.adv_protocol_PortNo[tmp->TRIDIndex]); 
			n = 0;
			for (index=0; index < (tmp->BACount + n); index++)
			{
				if (tmp->connectionID[index] > 0) 							// has value
				{
					if (flag == 0)
					{
						fprintf(HA_FD, "%d:%d", tmp->connectionID[index], index);
						flag = 1;
					}
					else
						fprintf(HA_FD, ", %d:%d", tmp->connectionID[index], index);
				}
				else														// is empty
					n++;
			}
			if (flag==1)
				fprintf(HA_FD, ") : %u\n",tmp->nextAID);
			else
				fprintf(HA_FD, "-) : %u\n",tmp->nextAID);
			flag = 0;
			
			tmp = tmp->nextAID;
		}
		while (tmp != NULL);
	}
	else
		fprintf(HA_FD, "\nNothing To Display.\n");
	fprintf(HA_FD, "(Display Ends here)\n\n");
}

