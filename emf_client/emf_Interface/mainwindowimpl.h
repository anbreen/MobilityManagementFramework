#ifndef MAINWINDOWIMPL_H
#define MAINWINDOWIMPL_H

#include <QtGui>
#include <stdio.h>   		/* printf, stderr, fprintf */
#include <errno.h>   		/* errno */
#include <stdlib.h>  		/* exit */
#include <unistd.h>
#include <arpa/inet.h>
#include "../commonFiles/emf_ports.h"
#include "ui_mainwindow.h"
#include <sys/time.h>	// for 'timeval' structure


#define TLVValueSize		75

class MainWindowImpl : public QMainWindow, public Ui::MainWindow
{
Q_OBJECT
	public:
		MainWindowImpl( QWidget * parent = 0, Qt::WFlags f = 0 );

		void createInterfaceCheckBoxes(int s);
		void retriveProtocolTotalEntrTable();
		void portNumberAgainstProtocols();
		void saveProtocolTotalEntrTable();
		void saveSelectedInftEntrTable();
		void populateManualWinComboBox();
		void setVisible(bool visible);
		void setInterfaceChkBox();
		void populateStructure();
 		void retriveRepository();
 		void saveInterfacePref();
 		void saveDuplicateData();
 		void readInterface_File();
 		void readInterfacePref();
		void initialValues();
        bool preference_BA();
        void displayAll();
		bool maybeSave();
        void setIcon();
		void convert();
		void init();

	protected:
		void closeEvent(QCloseEvent *event);
      void showEvent(QShowEvent *event);
      
	private slots:
		void iconActivated(QSystemTrayIcon::ActivationReason reason);
		void setunset_GeneralTabOptions();
		void forwardTotal_SelectedTable();
		void setting_InterfaceOptions();
		void backwardBW_SelectedTable();
 		void forwardBW_SelectedTable();
		void sentmanualBandAggrgCmd();
		void backwardSelected_Table();
 		void addInterfacesComboBox();
		void prefLevel_textChanged();
		void sentmanualHandoverCmd();
		void dataRate_textChanged();
		void receiveBytesFunction();
		void deffieHellmenRButton();
		void minimizeToSystemTray();
		void showMessage(char *str);
		void setCostUnitComboBox();
		void setRateUnitComboBox();
		void certificateRButton();
		void getSetHandoverPref();										 				 						
		void retriveBWEntrTable();
		void cost_textChanged();
		void anonymousRButton();
		void ellipticRButton();
		void removeProtocol();
		void saveStructure();
		void applyANDsave();
		void StartStartUP();
		void saveANDclose();
        void cancelClose();
		void addProtocol();
		void openManual();
		void selectFile();
		void selectKey();
		void updateEMF();
		void slotQUIT();
		
	private:
 		void initially_setgnrlOption();
 		void retrive_setGnrlOptions();
        void interfaceChkBoxLoc();
		void createTrayIcon();
		void createActions();
		void writeSettings();
		void readSettings();
		void createMenus();
//Variables
	 QString pathName;   // Delete this variable, Before that check in .cpp
//Variables against Options tab

		bool 	   flag_newInterface;	//Inform when new Interface becomes available
		bool 	   flag_bwAggrg;	//Inform When BA available
		bool 	   flag_hoCmpl;		//Inform Handover Completion process
		bool	   fileNotfound;
		bool     flag_save;
		fd_set   baseFDList, tmpFDList;
		bool 	   flag_locUpdate;				//Location Update
		bool	   flag_sysUp;					//Always start at system startup
		bool 	   EnDisInterface;
		int 	   bytesReceived;
		char 	   emfEnDisable;								//As it has been enabled as Default
		int 	   maxFDValue;
//		int	   nProtocols;
		int 	   socketFD;
		int 	   interfaceID;//This Variable contains the index of the selected interface, and is used as the reference for HO_Preferences
		int      spinBoxMaxValue;
		int      spinBoxOldValue;
		char    yesNoOption;
		struct timeval tv;// This Structure is used to set the time interval in select command
		struct sockaddr_in hostAgent, uAgentAddr;	
		//This Structure is used to send and receive data between H.A and U.A
		struct TLVInfo
		{
			char TLVType;
			char TLVLength;
			char TLVValue[TLVValueSize];
		}
		UA_TLV;

    struct userPreferences
    {
			char 	grl_options[5]; 					   //General Options Tab (CheckBoxes), 
			// [0, 1=Interface_Available_Popup, 2=Handover_Complete_Popup,Normalized_ 4=Multiple_Interfaces_Available_for_BA_Popup, 5=Location_Update_Popup]
			
			char	adv_interfaces[20];							//Advanced Tab with Sub Tab as Interfaces, Cotains the ID against each Interface set/unset
			
			char	adv_interfaces_name[20][20];		      //Advanced Tab with Sub Tab as Interfaces, Cotains name against each Interface
			// [0][0] => eth0, [1][0] => wlan0, etc.
			
			char	anony_cert[2];									//Ist Value Anonymous/Certificate and 2nd Elliptic Curve/D-Hellman
			// 1st Value Anonymous(0)/Certificate(1), 2nd Elliptic Curve(0)/D-Hellman(1)
			
	  	   char	certificatePath[200];						//Path of the Certificate if Certificate is selected
	  	   // Certificate Path, if Certificate is selected

	  	   char	key[100];						            //Key for the Certificate if Certificate is selected
	  	   // Key Path, if Certificate is selected
	  	   	  	   
			char	adv_protocol[20];					         //Advanced Tab with Sub Tab as Preferences, Conatins ID against each Protocol
			// [0] => 0(Protocol NOT under EMF)/1(Protocol under EMF)
			
			char	adv_protocol_name[20][20];	            //Sub Tab as Preferences, Contains Name against each protocol
			// [0][0] => FTP (e.g.), [1][0] => HTTP (e.g.)
			
			int	adv_protocol_PortNo[20];		         //Sub Tab as Preferences, contains Port Number against each Protocol
			
			char 	forcedHO;										//Forced Handover char contains either 0 or 1
			
			float	HOPreferences[20][7];						//Contains Values and Weights against each Interface
			// [interfaceID][0=Normalized_costValue,1=Normalized_DRValue,2=Pref,3=costUnit,4=DRUnit,5=costValue,6 = DRValue] => HA will scale these values
			
		 	char	BAPreference[20][20];			         //Ist Index Contains the ProtocolID and 2nd Index Contains the Interface ID
		 	
			// [0][0] => [indexOf adv_protocol] [indexOf adv_interfaces] => [0/1]	(e.g. FTP => Ethernet)
			// [0][1] => [indexOf adv_protocol] [indexOf adv_interfaces] => [0/1]	(e.g. FTP => WLAN0)
		 	char 	TotalInterfaces;//This Variable contains the total number of Interfaces
		 	char  indexOfMaxPrefHOIP[2];
    };
    struct userPreferences up;


    struct InterfacePref
    {
        char	intefaceStatus[20];			//Cotains the ID against each Interface set/unset
        char	interfaceName[20][20];		//Cotains name against each Interface [0][0] => eth0, [1][0] => wlan0, etc.
        float	HandoverPref[20][7];		//Contains Values and Weights against each Interface
                                            //[interfaceID][PrefValues]             
                                            //PrefValues => 0 = Normalized_costValue,
                                                        //  1 = Normalized_DRValue
                                                        //  2 = Pref
                                                        //  3 = costUnit
                                                        //  4 = DRUnit
                                                        //  5 = costValue
                                                        //  6 = DRValue
        char	BWAggrgPref[20][20];			        //  Ist Index Contains the ProtocolID and 2nd Index Contains the Interface ID
    };
    struct InterfacePref IntPref;
};
#endif
