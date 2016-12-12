/*
Application:	This application is a frontend for End to End Mobility Management Frame work
Author: 			Muhammad Omer 
Date: 			12nd Feb 2009
*/

#include "mainwindowimpl.h"

int nINTERFACES = 0;		      //nINTERFACES is used to create Checkboxes in Interfaces Tab

#define SO_REUSEPORT 15

//////////////////////////////////////////////////////////////////////
// 					User Agent Related Types					    //
// 			Request Types from User Agent are in Odd numbers,		//
// 				while from Host Agent are in Event numbers.			//
//////////////////////////////////////////////////////////////////////

enum UA_HA_TLV_Types
{HO_LINK_SHIFT_COMMIT_REQ=51, HO_LINK_SHIFT_COMMIT_CONF=52, HO_TRAFFIC_SHIFT_COMMIT_REQ=53,
HO_TRAFFIC_SHIFT_COMMIT_CONF=54,BA_COMMIT_REQ=55, BA_COMMIT_CONF=56, LINK_UP_Popup=57, LINK_DOWN_Popup=58,
USER_PREFERENCE_CHANGED_IND=59, INTERFACE_CHANGE_IND=60, LOCATION_UPDATE_IND=61, READ_USER_PREF_FILE = 62};

MainWindowImpl::MainWindowImpl( QWidget * parent, Qt::WFlags f) : QMainWindow(parent, f)
{
    trayIcon = new QSystemTrayIcon(this);		// Tray Icon to disply Bubble messages
//*********************************1__CREATE SOCKET***********************************
    if ((socketFD=socket(AF_INET, SOCK_STREAM, 0))==-1)
    {
        perror("Socket");
        exit(0);
    }
// Populating UserAgent Address Structure
    uAgentAddr.sin_family = AF_INET;         									// host byte order
    uAgentAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    uAgentAddr.sin_port = htons(USERAGENT_PORT);     								// short, network byte order
    memset(uAgentAddr.sin_zero, '\0', sizeof uAgentAddr.sin_zero);

//*********************************2__BIND THE UA WITH PORT**************************
    int opt=1;
    setsockopt (socketFD, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof (opt));
    setsockopt (socketFD, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt));
    
    if (bind(socketFD, (struct sockaddr*) &uAgentAddr, sizeof(uAgentAddr)) == -1)
    {
        perror("BIND:");
    }
    
//*********************************3__CONNECTS WITH H.A AT H.A LISTENING PORT********

    hostAgent.sin_family = AF_INET;												//Populating Host Agent Address Structure
    hostAgent.sin_addr.s_addr = inet_addr("127.0.0.1");
    hostAgent.sin_port = htons(HOSTAGENT_PORT);
    memset(hostAgent.sin_zero, '\0', sizeof hostAgent.sin_zero);
    int inc = 0;
    while (::connect(socketFD, (struct sockaddr*)&hostAgent, sizeof(hostAgent))==-1)
    {
        if (inc > 3)
        {
            perror("Connect");
            FD_CLR(socketFD, &baseFDList);
            ::close(socketFD);
            exit(0);
        }
        else
        {
            inc++;
            sleep(1);
        }
    }
//************************************************************************************
    createActions();							// Contains Tray Menu Actions    
    createMenus();								// Creating System Tray Icon Menu
    createTrayIcon();							// Creating System Tray Icon
    setupUi(this);								// Creating Widgets
    initialValues();							// Setting up initial Values
    retriveRepository();					   // Reading the File Contents
    initially_setgnrlOption();
    init();												// Contains the Validation Function of LineEdit
    createInterfaceCheckBoxes(nINTERFACES);		// Creating the CheckBoxes
    addInterfacesComboBox();			         // Adding the Checked Interfaces in the ComboBox (Interface Tab)
//********************Values are used in createConnection Function********************
    tv.tv_usec = 10;
    FD_ZERO(&baseFDList);
    FD_ZERO(&tmpFDList);
    FD_SET(socketFD, &baseFDList);					// Add socketFD to baseFDList
    maxFDValue = socketFD;
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(receiveBytesFunction()));///Timer
    timer->start(100);
//************************************************************************************
}

//This function is used to receive Data from Host Agent
void MainWindowImpl::receiveBytesFunction()
{
    fflush(NULL);
    tmpFDList = baseFDList;						// To keep baseFDList away from 'select'
    tv.tv_sec = 0;
    tv.tv_usec = 10;
    select(maxFDValue+1, &tmpFDList, NULL, NULL, &tv);
    fflush(NULL);
    if (FD_ISSET(socketFD, &tmpFDList))
    {
        bytesReceived = recv(socketFD, &UA_TLV, sizeof(struct TLVInfo), 0);
        if (bytesReceived ==-1)
            perror("Receive ERROR :");
        else if (bytesReceived == 0)			// TCP Connection Closed
        {
            FD_CLR(socketFD, &baseFDList);
            ::close(socketFD);
            timer->stop();
        }
        else  									// Data is available
        {
            switch(UA_TLV.TLVType)			// Process Request based on TLV Type
            {
                case HO_LINK_SHIFT_COMMIT_CONF:	// Confirmation Msg Link Shift Handover ID = 2;
                    UA_TLV.TLVValue[strlen(UA_TLV.TLVValue)] = '\0';
                    //printf("%s\n", UA_TLV.TLVValue);
                    showMessage(UA_TLV.TLVValue);
                break;

                case HO_TRAFFIC_SHIFT_COMMIT_CONF:	// Confirmation Msg Traffic Shift Handover ID = 4;
                    UA_TLV.TLVValue[strlen(UA_TLV.TLVValue)] = '\0';
                    //printf("%s\n", UA_TLV.TLVValue);
                    showMessage(UA_TLV.TLVValue);
                break;
    
                case BA_COMMIT_CONF:				// Confirmation Msg Bandwidth Aggregation Traffic Shift B.Aggrg ID = 6;
                    UA_TLV.TLVValue[strlen(UA_TLV.TLVValue)] = '\0';
                    ////printf("%s\n", UA_TLV.TLVValue);
                    showMessage(UA_TLV.TLVValue);
                break;
    
                case LINK_UP_Popup:				// Confirmation Msg Link is Up ID = 12;
                    nINTERFACES = 0;
                    spinBoxOldValue = 0;
                    setInterfaceChkBox();
                    readInterface_File();
                    addInterfacesComboBox();   // Added After Thesis task
                    if (nINTERFACES >= spinBoxOldValue)
                        integerSpinBox->setRange(0, nINTERFACES);
                    else
                        integerSpinBox->setRange(0, spinBoxOldValue);
                    if (up.grl_options[1] == 1)
                    {
                        UA_TLV.TLVValue[strlen(UA_TLV.TLVValue)] = '\0';
                        //printf("%s\n", UA_TLV.TLVValue);
                        showMessage(UA_TLV.TLVValue);
                    }
                break;

                case LINK_DOWN_Popup:				// No Confirmation Msg Link is Down ID = 13;
                    setInterfaceChkBox();
                    readInterface_File();
                    addInterfacesComboBox();   // Added After Thesis task
                    if (up.grl_options[1] == 1)
                    {
                        UA_TLV.TLVValue[strlen(UA_TLV.TLVValue)] = '\0';
                        //printf("%s\n", UA_TLV.TLVValue);
                        showMessage(UA_TLV.TLVValue);
                    }					
                break;

                case LOCATION_UPDATE_IND:		// Confirmation Msg File Updated ID = 32;
                    UA_TLV.TLVValue[strlen(UA_TLV.TLVValue)] = '\0';
                    //printf("%s\n", UA_TLV.TLVValue);
                    showMessage(UA_TLV.TLVValue);
                break;

                case READ_USER_PREF_FILE://Chothay Ya
                    setInterfaceChkBox();
                    readInterface_File();
                    addInterfacesComboBox();   // Added After Thesis task
            }
        }
    }
}

//This Function is used to send the Mannual Commands regarding HO towards Host Agent, Error Checking has also been done in this function
void MainWindowImpl::sentmanualHandoverCmd()
{
    if (advanceTabWidget->currentIndex() == 5)
    {
        if (ManualHO_tabWidget->currentIndex() == 0) // means Link Tab has been selected
        {
            printf("Link Shift Tab is Selected\n");
            QString memo_text1 = "";
            memo_text1= fromLinkComboBOx->currentText();
            QByteArray textTemp1 = memo_text1.toUtf8() ;
            QString memo_text2 = "";
            memo_text2= toLinkComboBOx->currentText();
            QByteArray textTemp2 = memo_text2.toUtf8() ;
            if (strcmp(textTemp1, "") == 0)
            {
                QMessageBox::information(this, tr("Mannual Handover"),tr("No Interface under EMF"));
            }
            else if (strcmp(textTemp2, "") == 0)
            {
                QMessageBox::information(this, tr("Mannual Handover"),tr("No Interface under EMF"));
            }			
            else if (strcmp(textTemp1, textTemp2) == 0)
            {
                QMessageBox::information(this, tr("Mannual Handover"),tr("Please Select Different Links"));				
            }
            else
            {
                int IntindexFrom = -1;
                int IntindexTo = -1;
                char i;
                for(i = 0; i < (20); i++) // 20 has to be replaced by 20
                    if (strcmp(up.adv_interfaces_name[i], textTemp1)== 0)   //  fromLinkComboBOx
                        IntindexFrom = i;
                for(i = 0; i < (20); i++) // 20 has to be replaced by 20
                    if (strcmp(up.adv_interfaces_name[i], textTemp2)== 0)   //  toLinkComboBOx
                        IntindexTo = i;
                if (  (up.HOPreferences[IntindexFrom][2] > 0) && (up.HOPreferences[IntindexTo][2] > 0)  )
                {
                    UA_TLV.TLVType = HO_LINK_SHIFT_COMMIT_REQ; // 1;
                    UA_TLV.TLVValue[0] = IntindexFrom;
                    UA_TLV.TLVValue[1] = IntindexTo;

                    UA_TLV.TLVLength = strlen(UA_TLV.TLVValue);		// TLV Length
                    if (send(socketFD, &UA_TLV, sizeof (struct TLVInfo), 0)==-1)
                    {
                        perror("HO_LINK_SHIFT_COMMIT_REQ Send");
                        QMessageBox::information(this, tr("Mannual Handover"),tr("Host Agent is Not Listening"));
                    }
                    else
                        printf("U.A: (HandOver Link Shift Commit Request Sent to H.A) From %s To %s \n\n", up.adv_interfaces_name[IntindexFrom],        up.adv_interfaces_name[IntindexTo]);
                }
                else
                {
                    QMessageBox::information(this, tr("Mannual Handover"),tr("Apply Handover Preference for requested Interfaces"));
                    tabWidget->setCurrentIndex(1);
                    advanceTabWidget->setCurrentIndex(3);
                }
            }
        }
        else if (ManualHO_tabWidget->currentIndex() == 1) // means Traffic Tab has been selected
        {
            printf("Traffic Shift Tab is Selected\n");
            QString memo_text1 = "";
            memo_text1= fromLink_TrafficComboBOx->currentText();
            QByteArray textTemp1 = memo_text1.toUtf8() ;
            QString memo_text2 = "";
            memo_text2= toTraffic_TrafficComboBOx->currentText();
            QByteArray textTemp2 = memo_text2.toUtf8() ;
            QString memo_text3 = "";
            memo_text3= toLink_TrafficComboBOx->currentText();
            QByteArray textTemp3 = memo_text3.toUtf8() ;
            if (strcmp(textTemp1, "") == 0)
            {
                QMessageBox::information(this, tr("Mannual Handover"),tr("No Interface under EMF"));
            }
            else if (strcmp(textTemp2, "") == 0)
            {
                QMessageBox::information(this, tr("Mannual Handover"),tr("Please Enter Traffic in Protocol Tab"));
            }
            else if (strcmp(textTemp3, "") == 0)
            {
                QMessageBox::information(this, tr("Mannual Handover"),tr("No Interface under EMF"));
            }						
            else if (strcmp(textTemp1, textTemp3) == 0)
            {
                QMessageBox::information(this, tr("Mannual Handover"),tr("Please Select Different Links"));				
            }
            else
            {
                int IntindexFrom = -1;
                int IntindexTo  = -1;
                int Prtindex = -1;
                char i;
                for(i = 0; i < (20); i++) // 20 has to be replaced by up.TotalInterfaces
                    if (strcmp(up.adv_interfaces_name[i], textTemp1)== 0)
                        IntindexFrom = i;
                for(i = 0; i < (20); i++) // 20 has to be replaced by up.TotalInterfaces
                    if (strcmp(up.adv_interfaces_name[i], textTemp3)== 0)
                        IntindexTo = i;
                for(i = 0; i < (20); i++)
                    if (strcmp(up.adv_protocol_name[i], textTemp2)== 0)
                        Prtindex = i;
                if (  (up.HOPreferences[IntindexFrom][2] > 0) && (up.HOPreferences[IntindexTo][2] > 0)  )
                {
                    UA_TLV.TLVType = HO_TRAFFIC_SHIFT_COMMIT_REQ; // 5;				
                    UA_TLV.TLVValue[0] = Prtindex;
                    UA_TLV.TLVValue[1] = IntindexFrom;
                    UA_TLV.TLVValue[2] = IntindexTo;
                    UA_TLV.TLVLength = strlen(UA_TLV.TLVValue);		// TLV Length
                    if (send(socketFD, &UA_TLV, sizeof (struct TLVInfo), 0)==-1)
                    {
                        perror("HO_TRAFFIC_SHIFT_COMMIT_REQ Send");
                        QMessageBox::information(this, tr("Mannual Handover"),tr("Host Agent is Not Listening"));
                    }
                    else
                        printf("U.A: (HandOver Traffic Shift Commit Request Sent to H.A) From %s To %s For %s Traffic\n\n", up.adv_interfaces_name[IntindexFrom], up.adv_interfaces_name[IntindexTo], up.adv_protocol_name[Prtindex]);
                }
                else
                {
                    QMessageBox::information(this, tr("Mannual Handover"),tr("Handover Preference Must be greater than zero (0) for requested Handover Interfaces"));
                    tabWidget->setCurrentIndex(1);
                    advanceTabWidget->setCurrentIndex(3);
                }
            }
        }		
    }
}

//Function send the Mannual Commands regarding BW.Aggrg towards Host Agent
void MainWindowImpl::sentmanualBandAggrgCmd()
{
    if (advanceTabWidget->currentIndex() == 5)
    {
        QString memo_text1 = "";
        memo_text1= adv_Mon_selectLinkComBox->currentText();
        QByteArray textTemp1 = memo_text1.toUtf8() ;
        QString memo_text2 = "";
        memo_text2= adv_Mon_selectAppComBox->currentText();
        QByteArray textTemp2 = memo_text2.toUtf8() ;

        if (strcmp(textTemp1, "") == 0)
        {
            QMessageBox::information(this, tr("BandWidth Aggregation"),tr("No Interface under EMF"));
        }
        else if (strcmp(textTemp2, "") == 0)
        {
            QMessageBox::information(this, tr("BandWidth Aggregation"),tr("No Protocol under EMF"));
        }
        else
        {
            int Prtindex = -1;
            int FromLink = 0;
            char i, localVar = 0, flag = 0;
            int INDEX = 0;
            for(i = 0; i < 20 ; i++)
            if (strcmp(up.adv_protocol_name[i], textTemp2)== 0)
            {
                Prtindex = i;
                break; 
            }
            else
            {
                if (up.adv_protocol[i] == 0)
                    INDEX++;
            }
            for(i = 0; i < (20); i++) // 20 has to be replaced by up.TotalInterfaces
                if (strcmp(up.adv_interfaces_name[i], textTemp1)== 0)
                {
                    FromLink = i;
                    break;
                }
            for (i = 0 ;i < 20 ; i++)
            {
                if (up.BAPreference[Prtindex][i] == 1)
                {
                    localVar ++;
                }
                else
                    if (localVar >= 2)
                    {
                        flag = 1;
                        break;
                    }
                    else
                        flag = 0;
            }
            if (flag == 1)
            {
                if (up.BAPreference[Prtindex][FromLink] == 0)
                {
                    UA_TLV.TLVType = BA_COMMIT_REQ; // 1;
                    UA_TLV.TLVValue[0] = Prtindex;
                    UA_TLV.TLVValue[1] = FromLink;
                    UA_TLV.TLVLength = strlen(UA_TLV.TLVValue);		// TLV Length
                    if (send(socketFD, &UA_TLV, sizeof (struct TLVInfo), 0)==-1)
                    {
                        perror("BA_COMMIT_REQ Send");
                        QMessageBox::information(this, tr("Mannual BandWidth Aggregation"),tr("Host Agent is Not Listening"));
                    }
                    else
                    {
                        printf("U.A: (BandWidth Aggregation Commit Request sent to H.A) Add %s Traffic to %s as well\n\n", up.adv_protocol_name[Prtindex] , up.adv_interfaces_name[FromLink]);
                    }
                }
                else
                    QMessageBox::information(this, tr("Selected Link"),tr("Already under Bandwidth Aggregation"));
            }
            else
            {
                QMessageBox::information(this, tr("Bandwidth Aggregation"),tr("Add minimum two interfaces in Bandwidth Aggregation Preferences to fulfill requested command against selected protocol"));
                tabWidget->setCurrentIndex(1);
                advanceTabWidget->setCurrentIndex(4);
                adv_BW_selectPrtComBox->setCurrentIndex(Prtindex - INDEX);
            }
//Again Initialize Values
            Prtindex = -1;
            FromLink = 0;
            localVar = 0, flag = 0;
        }
    }
}

//Function is used to initialize the values with appropriate value
void MainWindowImpl::initialValues()
{
    char i, j;
    flag_sysUp = false;
    flag_newInterface = false;
    flag_hoCmpl = false;
    flag_bwAggrg = false;
    flag_locUpdate = false;
    EnDisInterface = true;
    emfEnDisable = 1;
    up.TotalInterfaces = 0;
    yesNoOption = 0;
    advanceTabWidget->setTabEnabled(1, false);
    flag_save = false;
    
    if (adv_auth_EcurveRB->isChecked())
    {
        adv_auth_AnonRB->setChecked(true);
        adv_auth_useCertRB->setChecked(false);
        adv_auth_selectCertButton->setEnabled(false);
        adv_auth_selectKeyButton->setEnabled(false);
    }

    adv_auth_useCertRB->setChecked(false);
    fileNotfound = false;
    for (i = 0; i < 40 ; i ++)
    {
        totalApp_tableItem[i] = new QTableWidgetItem();
        selectedApp_tableItem[i] = new QTableWidgetItem();
    }
    for (i = 0; i < 20 ; i ++)
    {  
        BWtotalApp_tableItem[i] = new QTableWidgetItem();
        BWselectedApp_tableItem[i] = new QTableWidgetItem();
        strcpy(IntPref.interfaceName[i], "");
        IntPref.intefaceStatus[i] = 0;
        for (j = 0; j < 7 ; j++)
            IntPref.HandoverPref[i][j] = 0;
        for (j = 0; j < 20 ; j++)
            IntPref.BWAggrgPref[i][j] = 0;
        if (interfaces_chkBox[i]->text() != "")
        {
            interfaces_chkBox[i]->setChecked(false);
        }
        up.HOPreferences[i][0] = 0;
        up.HOPreferences[i][1] = 0;
        up.HOPreferences[i][2] = 0;
        up.HOPreferences[i][3] = 0;
        up.HOPreferences[i][4] = 0;
        up.HOPreferences[i][5] = 0;
        up.HOPreferences[i][6] = 0;		
    }
    interfaceID = 0;
    integerSpinBox->setValue(0);
    costLineEdit->setText("");
    dataRateLineEdit->setText("");
    adv_HO_costUnitComBox->setCurrentIndex(0);
    adv_HO_datarateUnitComBox->setCurrentIndex(0);
}


void MainWindowImpl::setVisible(bool visible)
{
    minimizeAction->setEnabled(visible);
    restoreAction->setEnabled(isMaximized() || !visible);
    QWidget::setVisible(visible);
}

void MainWindowImpl::showEvent(QShowEvent *event)
{
    event->setAccepted(true);
    readSettings();
}

void MainWindowImpl::closeEvent(QCloseEvent *event)
{
    if (trayIcon->isVisible())
    {
    //		QMessageBox::information(this, tr("Systray"),tr("The program will keep running in the ""system tray. To terminate the program, ""choose <b>Quit</b> in the context menu ""of the system tray entry."));
        hide();
        event->ignore();
    }
}

void MainWindowImpl::writeSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope,"CLPC-Software", "changeFileTime");
    settings.setValue("geometry", geometry());
    settings.setValue("path", pathName);
}

void MainWindowImpl::minimizeToSystemTray()
{
    if (trayIcon->isVisible())
    {
        saveStructure();
        hide();
    }
}

void MainWindowImpl::readSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope,"CLPC-Software", "changeFileTime");
    // Open our settings file
    QRect rect = settings.value("geometry", QRect(80, 200, 400, 160)).toRect();	// Read the last position of our window
    move(rect.topLeft());
    // Set our window to its last position
    pathName = settings.value("path", "").toString();
    // Set the current path to the last used path
}

void MainWindowImpl::setIcon()
{}

//Function is used to To Create Actions against the Context Menu in System Tray and in UserWindow
void MainWindowImpl::createActions()
{
    toTryAct = new QAction(tr("&Minimize to System Tray"), this);
    toTryAct->setShortcut(tr("Ctrl+M"));
    toTryAct->setStatusTip(tr("Minimize to System Tray"));
    connect(toTryAct, SIGNAL(triggered()), this, SLOT(minimizeToSystemTray()));
    exitAct = new QAction(tr("E&xit"), this);
    exitAct->setShortcut(tr("Ctrl+Q"));
    exitAct->setStatusTip(tr("Exit the application"));
    connect(exitAct, SIGNAL(triggered()), this, SLOT(slotQUIT()));
    connect(exitAct, SIGNAL(triggered()), qApp, SLOT(quit()));
    minimizeAction = new QAction(tr("Mi&nimize"), this);  
    connect(minimizeAction, SIGNAL(triggered()), this, SLOT(hide()));
    minimizeAction->setEnabled(true);
    restoreAction = new QAction(tr("&Restore"), this);
    connect(restoreAction, SIGNAL(triggered()), this, SLOT(showNormal()));
    quitAction = new QAction(tr("&Quit"), this);
    connect(quitAction, SIGNAL(triggered()), this, SLOT(slotQUIT()));
    connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
}

void MainWindowImpl::createTrayIcon()
{
    trayIconMenu = new QMenu(tr("EMF"), this);
    trayIconMenu->addAction(minimizeAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(restoreAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);
    QPixmap icon(":/images/icon.png");
    QIcon trayIconImage;	 
    trayIconImage= QIcon(icon);
    trayIcon->setIcon(trayIconImage);
    trayIcon->setToolTip("EMF");
    trayIcon->setContextMenu(trayIconMenu);
    setWindowIcon(trayIconImage);
    trayIcon->show();
}

void MainWindowImpl::showMessage(char *str)
{
    QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::MessageIcon(QSystemTrayIcon::Information);
    trayIcon->showMessage("End to End Mobility Management", str, icon, 4000);
}

void MainWindowImpl::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason)
    {
        case QSystemTrayIcon::Trigger:
        case QSystemTrayIcon::DoubleClick:
        if (trayIcon->isVisible())
        {
            if (minimizeAction->isEnabled() == false)
                showNormal();
            else
                hide();
        }
        case QSystemTrayIcon::MiddleClick:
        break;
        default:
        ;
    }
}


void MainWindowImpl::setting_InterfaceOptions()
{
//General Tab    
    if(enDisableEMF->isChecked())
    {
        tabWidget->setTabEnabled(1, true);
        emfEnDisable = 1;
        if (flag_sysUp == true)
            g_startOnstartUp->setChecked(true);
        else
            g_startOnstartUp->setChecked(false);
        if (flag_newInterface == true)
            g_informInterfaceAvail->setChecked(true);
        else
            g_informInterfaceAvail->setChecked(false);
        if (flag_hoCmpl == true)
            g_informHandoverComp->setChecked(true);
        else
            g_informHandoverComp->setChecked(false);	
        if (flag_bwAggrg == true)
            g_informBWAvail->setChecked(true);
        else
            g_informBWAvail->setChecked(false);	
        if (flag_locUpdate == true)
            g_enableLocUpdate->setChecked(true);
        else
            g_enableLocUpdate->setChecked(false);
    }
    else
    {
        emfEnDisable = 0;	
        g_startOnstartUp->setChecked(false);
        g_informInterfaceAvail->setChecked(false);
        g_informHandoverComp->setChecked(false);
        g_informBWAvail->setChecked(false);
        g_enableLocUpdate->setChecked(false);
        tabWidget->setTabEnabled(1, false);
    }
    yesNoOption = 0;
}

void MainWindowImpl::initially_setgnrlOption()
{
    if (g_startOnstartUp->isChecked())
        flag_sysUp = true;
    else
        flag_sysUp = false;
    if (g_informInterfaceAvail->isChecked())
        flag_newInterface = true;
    else
        flag_newInterface = false;    
    if (g_informHandoverComp->isChecked())
        flag_hoCmpl = true;
    else
        flag_hoCmpl = false;    
    if (g_informBWAvail->isChecked())
        flag_bwAggrg = true;
    else
        flag_bwAggrg = false;    
    if (g_enableLocUpdate->isChecked())
        flag_locUpdate = true;
    else
        flag_locUpdate = false; 	
}

void MainWindowImpl::setunset_GeneralTabOptions()
{
    if (g_startOnstartUp->isChecked())
    {
        flag_sysUp = true;
        enDisableEMF->setChecked(true);
        emfEnDisable = 1;	
    }
    else
        flag_sysUp = false;
    if (g_informInterfaceAvail->isChecked())
    {
        flag_newInterface = true;
        enDisableEMF->setChecked(true);
        emfEnDisable = 1;		
    }
    else
        flag_newInterface = false;    
    if (g_informHandoverComp->isChecked())
    {
        flag_hoCmpl = true;
        enDisableEMF->setChecked(true);
        emfEnDisable = 1;		
    }
    else
        flag_hoCmpl = false;    
    if (g_informBWAvail->isChecked())
    {
        flag_bwAggrg = true;
        enDisableEMF->setChecked(true);
        emfEnDisable = 1;		
    }
    else
        flag_bwAggrg = false;    
    if (g_enableLocUpdate->isChecked())
    {
        flag_locUpdate = true;
        enDisableEMF->setChecked(true);
        emfEnDisable = 1;		
    }
    else
        flag_locUpdate = false; 	
}

//Event Based Function used to set/unSet the CheckBox Options in Advance Authentication Tab
void MainWindowImpl::anonymousRButton()
{
    adv_auth_selectCertButton->setEnabled(false);
    adv_auth_selectKeyButton->setEnabled(false);
    adv_auth_CertPathLEdit->setEnabled(false);
    adv_auth_KeyPathLEdit->setEnabled(false);
    elliptic_Deffi_GrpBox->setEnabled(true);
    if (adv_auth_useCertRB->isChecked())
    {
        adv_auth_AnonRB->setChecked(true);
        adv_auth_useCertRB->setChecked(false);
    }
    else
        adv_auth_AnonRB->setChecked(true);
}

void MainWindowImpl::certificateRButton()
{
    adv_auth_selectCertButton->setEnabled(true);
    adv_auth_selectKeyButton->setEnabled(true);
    adv_auth_CertPathLEdit->setEnabled(true);
    adv_auth_KeyPathLEdit->setEnabled(true);
    if (adv_auth_AnonRB->isChecked())
    {
        adv_auth_useCertRB->setChecked(true);
        adv_auth_AnonRB->setChecked(false);
    }
    else
        adv_auth_useCertRB->setChecked(true);
    elliptic_Deffi_GrpBox->setEnabled(false);
}

void MainWindowImpl::ellipticRButton()
{
    elliptic_Deffi_GrpBox->setEnabled(true);
    if (adv_auth_DHelRB->isChecked())
    {
        adv_auth_EcurveRB->setChecked(true);
        adv_auth_DHelRB->setChecked(false);
    }
    else    
        adv_auth_EcurveRB->setChecked(true);
    adv_auth_AnonRB->setChecked(true);
    adv_auth_useCertRB->setChecked(false);
    adv_auth_selectCertButton->setEnabled(false);    
    adv_auth_selectKeyButton->setEnabled(false);
}

void MainWindowImpl::deffieHellmenRButton()
{
    elliptic_Deffi_GrpBox->setEnabled(true);
    if (adv_auth_EcurveRB->isChecked())
    {
        adv_auth_DHelRB->setChecked(true);
        adv_auth_EcurveRB->setChecked(false);
    }
    else
        adv_auth_DHelRB->setChecked(true);
    adv_auth_AnonRB->setChecked(true);    
    adv_auth_useCertRB->setChecked(false);
    adv_auth_selectCertButton->setEnabled(false);    
    adv_auth_selectKeyButton->setEnabled(false);
}

void MainWindowImpl::selectFile()
{
    QString directory = QFileDialog::getOpenFileName(this, tr("Select Certificate"),"/home",tr("File (*.pem *.crt)"));
    if (!directory.isEmpty())
    {
        adv_auth_CertPathLEdit->setText(directory);
    } 
    else if (adv_auth_CertPathLEdit->text() == "")
        QMessageBox::critical(0, tr("EMF"),tr("Select Certificate to Proceed"));
}

void MainWindowImpl::selectKey()
{
    QString directory = QFileDialog::getOpenFileName(this, tr("Select Key"),"/home",tr("File (*.key *.pem)"));
    if (!directory.isEmpty())
    {
        adv_auth_KeyPathLEdit->setText(directory);
    }
    else if (adv_auth_KeyPathLEdit->text() == "")
        QMessageBox::critical(0, tr("EMF"),tr("Select Private Key to Proceed"));
}


void MainWindowImpl::populateStructure()
{
    char i,j;
    if(g_startOnstartUp->isChecked())
        up.grl_options[0] = 1;
    else
        up.grl_options[0] = 0;    
    if(g_informInterfaceAvail->isChecked())
        up.grl_options[1] = 1;	
    else 
        up.grl_options[1] = 0;	
    if(g_informHandoverComp->isChecked())
        up.grl_options[2] = 1;	
    else
        up.grl_options[2] = 0;
    if(g_informBWAvail->isChecked())
        up.grl_options[3] = 1;	
    else
        up.grl_options[3] = 0;
    if(g_enableLocUpdate->isChecked())
        up.grl_options[4] = 1;	
    else
        up.grl_options[4] = 0;
    for (i = 0; i < 20 ; i++)
    {
        if (strlen(up.adv_interfaces_name[i]) > 0)
        {  
            for (j = 0 ; j < 20 ; j++)
            {
                QString memo_text = "";
                memo_text=interfaces_chkBox[j]->text();
                QByteArray textTemp = memo_text.toUtf8();
                if (strcmp(textTemp, up.adv_interfaces_name[i]) == 0)
                {
                    strcpy(up.adv_interfaces_name[i], textTemp);
                    if (interfaces_chkBox[j]->isChecked())
                        up.adv_interfaces[j] = 1;
                    else
                        up.adv_interfaces[j] = 0;
                }
            }
        }
    }
    if (adv_auth_AnonRB->isChecked())
    up.anony_cert[0] = 1;		// Means Anonymous has been selected
    else if (adv_auth_useCertRB->isChecked())
    {
        if (adv_auth_CertPathLEdit->text() == "")       //Defualt Value are Anonymous and Elliptic Curve
        {
            up.anony_cert[0] = 1;		// Means Anonymous has been selected
            up.anony_cert[1] = 1;		// Means Anonymous (Elliptic curve) has been selected if not already selected the Diffie-Hellman
        }
        else
        {
            up.anony_cert[0] = 0;		// Means Certificate has been checked
            QString memo_text=adv_auth_CertPathLEdit->text();
            QByteArray textTemp = memo_text.toUtf8() ;
            strcpy(up.certificatePath, textTemp);
            QString memo_text1=adv_auth_KeyPathLEdit->text();
            QByteArray textTemp1 = memo_text1.toUtf8() ;
            strcpy(up.key, textTemp1);
        }
    }
    if (adv_auth_EcurveRB->isChecked())
        up.anony_cert[1] = 1;		// Means Anonymous (Elliptic curve) has been selected
    else if (adv_auth_DHelRB->isChecked())
        up.anony_cert[1] = 0;		// Means Anonymous (Diffie-Hellman) has been checked
    saveProtocolTotalEntrTable();
    if (adv_HO_enForcedHOCBox->isChecked())
        up.forcedHO = 1;
    else
        up.forcedHO = 0;
}

//Save the Structure (User Info/Preferences)
void MainWindowImpl::saveStructure()
{
    applyButton->setEnabled(false);
    populateStructure();
    saveDuplicateData();
    FILE *fp;
    size_t count,count2;
    fp = fopen("../emf_hostagent/userPref.txt", "w + r");
    if(fp == NULL)
    {
        printf("User Preferences are not available");
    }

    count = fwrite(&emfEnDisable, 1, 1,fp);
    count2 = fwrite (&up, sizeof(up),1,fp); 
    fflush(fp);
    fseek(fp,0,SEEK_SET);
    fclose (fp);
    applyButton->setEnabled(true);
}

//Function is used to Read the Stored Information from File (User Preferences)
void MainWindowImpl::retriveRepository()
{
    FILE *fp;
    size_t count1,count3;
    fp = fopen("../emf_hostagent/userPref.txt", "r");
    if(fp == NULL)
    {
        printf("User Preferences are not available");
        fileNotfound = false;
    }
    else
    {
        fileNotfound = true;
        count1 = fread(&emfEnDisable, 1, 1,fp);
        count3 = fread(&up,sizeof(up), 1,fp);
        fclose (fp);
        retrive_setGnrlOptions();
    }
}

void MainWindowImpl::retrive_setGnrlOptions()
{
    if (emfEnDisable == 1)
        enDisableEMF->setChecked(true);  
    else
        enDisableEMF->setChecked(false);
    if (up.grl_options[0] == 1)
        g_startOnstartUp->setChecked(true);
    else
        g_startOnstartUp->setChecked(false);
    if (up.grl_options[1] == 1)
        g_informInterfaceAvail->setChecked(true);
    else 
        g_informInterfaceAvail->setChecked(false);
    if (up.grl_options[2] == 1)
        g_informHandoverComp->setChecked(true);
    else 
        g_informHandoverComp->setChecked(false);
    if (up.grl_options[3] == 1)
        g_informBWAvail->setChecked(true);
    else 
        g_informBWAvail->setChecked(false);
    if (up.grl_options[4] == 1)
        g_enableLocUpdate->setChecked(true);
    else 
        g_enableLocUpdate->setChecked(false);  
    char i;
    nINTERFACES = 0;
    spinBoxOldValue = 0;
    for (i = 0; i < (20); i++)
    {
        if (strcmp(up.adv_interfaces_name[i], "") != 0)
            createInterfaceCheckBoxes(nINTERFACES);
    }

    if (up.anony_cert[0] == 1)		// Means Anonymous has been selected
    {
        adv_auth_AnonRB->setChecked(true);
        elliptic_Deffi_GrpBox->setEnabled(true);
        adv_auth_selectCertButton->setEnabled(false);
        adv_auth_selectKeyButton->setEnabled(false);
    }
    if (up.anony_cert[0] == 0)		// Means Certificate has been checked
    {
        adv_auth_useCertRB->setChecked(true);
        elliptic_Deffi_GrpBox->setEnabled(false);
        adv_auth_selectCertButton->setEnabled(true);
        adv_auth_selectKeyButton->setEnabled(true);
        adv_auth_CertPathLEdit->setText((up.certificatePath));
        adv_auth_KeyPathLEdit->setText((up.key));
    }
    if (up.anony_cert[1] == 1) 
        adv_auth_EcurveRB->setChecked(true);		// Means Anonymous has been selected
    else if (up.anony_cert[1] == 0)
        adv_auth_DHelRB->setChecked(true);		// Means Certificate has been checked
    if (up.forcedHO == 1)
        adv_HO_enForcedHOCBox->setChecked(true);
    else
        adv_HO_enForcedHOCBox->setChecked(false);
    retriveProtocolTotalEntrTable();
    retriveBWEntrTable();
}

//Interface Tab, Function is used to Create the CheckBoxes in case of userPreference file is available. else initialize with Appropriate Value
void MainWindowImpl::createInterfaceCheckBoxes(int totl)
{
    char i, j;

    if (!fileNotfound)
    {
        for (i = 0; i < 20; i++)
        {
            interfaces_chkBox[i]->setText("");
            up.HOPreferences[i][0] = 0;
            up.HOPreferences[i][1] = 0;
            up.HOPreferences[i][2] = 0;
            up.HOPreferences[i][3] = 0;
            up.HOPreferences[i][4] = 0;
            up.HOPreferences[i][5] = 0;
            up.HOPreferences[i][6] = 0;
        }
        for (i = 0; i < 20; i++)
        {
            for (j = 0; j < 20; j++)
                up.BAPreference[i][j] = 0;
            strcpy(up.adv_interfaces_name[i], "");
            up.adv_protocol[i] = 0;
            up.adv_interfaces[i] = 0;
            strcpy(up.adv_protocol_name[i], "");			
            up.adv_protocol_PortNo[i] = 0;
        }
    }
    else
    {
        if (up.adv_interfaces[totl] == 1)
        {
            interfaces_chkBox[totl]->setText(up.adv_interfaces_name[totl]);
            interfaces_chkBox[totl]->setChecked(true);
        }
        else if (up.adv_interfaces[totl] == 0)
        {
            interfaces_chkBox[totl]->setText(up.adv_interfaces_name[totl]);
            interfaces_chkBox[totl]->setChecked(false);
        }
        nINTERFACES++;
    }
    interfaceChkBoxLoc();
}

//Setting up the Location/Placement of the Interfaces
void MainWindowImpl::interfaceChkBoxLoc()
{
    int dos = 0;
    int dos1 = 0;
    int dos2 = 0;
    int dos3 = 0;
    char i;
    for (i = 0; i < 20 ; i++)   // 20 has to be replaced by up.TotalInterfaces
    {
        if ((interfaces_chkBox[i]->text() != ""))
        {
            if (i >= 0 && i <= 5)
            {
                interfaces_chkBox[i]->setGeometry(QRect(25, 15 +(35*dos), 251 , 23));
                dos++;
            }
            else if (i >= 6 && i <= 11)
            {
                interfaces_chkBox[i]->setGeometry(QRect(150, 15 +(35*dos1), 251 , 23));
                dos1++;
            }
            else if (i >= 12 && i <= 17)
            {
                interfaces_chkBox[i]->setGeometry(QRect(275, 15 +(35*dos2), 251 , 23));
                dos2++;
            }
            else if (i >= 18 && i <= 19)
            {
                interfaces_chkBox[i]->setGeometry(QRect(400, 15 +(35*dos3), 251 , 23));
                dos3++;
            }
        }
        else
            interfaces_chkBox[i]->setGeometry(QRect(0, -100, 0 , 0));
    }
}

//Function is used the set the value of selected interfaces in the ComboBox and Table
void MainWindowImpl::addInterfacesComboBox()
{
    char i;
    flag_save = false;
    adv_HO_selectNetComBox->clear();
    adv_Mon_selectLinkComBox->clear();
    fromLinkComboBOx->clear();
    toLinkComboBOx->clear();
    fromLink_TrafficComboBOx->clear();
    toLink_TrafficComboBOx->clear();

    for(i = 0; i < 20; i++)
        BWtotalApp_tableItem[i]->setText("");
    int settext = 0;
    for (i = 0; i < 20; i++) // 20 has to be replaced by up.TotalInterfaces
    {
        if (interfaces_chkBox[i]->text() != "")
        {
            QString memo_text1 = "";
            memo_text1=(interfaces_chkBox[i]->text());
            QByteArray textTemp1 = memo_text1.toUtf8();
            if (interfaces_chkBox[i]->isChecked())
            {
                adv_HO_selectNetComBox->addItem(textTemp1);
                BWtotalApp_tableItem[settext]->setText(textTemp1);
                BWtotalApp_table->setItem(settext, 0, BWtotalApp_tableItem[settext]);
                adv_Mon_selectLinkComBox->addItem(textTemp1);
                fromLinkComboBOx->addItem(textTemp1); 
                toLinkComboBOx->addItem(textTemp1); 
                fromLink_TrafficComboBOx->addItem(textTemp1);  
                toLink_TrafficComboBOx->addItem(textTemp1);
                if (up.HOPreferences[i][2] <= 0)
                {
                    up.HOPreferences[i][2] = 1;
                    integerSpinBox->setValue(up.HOPreferences[i][2]);
                }
                settext++;
            }
            else
            {
                if ((up.indexOfMaxPrefHOIP[0] != -1) && (up.indexOfMaxPrefHOIP[0] >= 0))
                {
                    if (up.indexOfMaxPrefHOIP[1] > 0)
                    {
                        if (up.HOPreferences[up.indexOfMaxPrefHOIP[0]][2] > 0)
                        {
                            if (up.adv_interfaces_name[up.indexOfMaxPrefHOIP[0]] == textTemp1)
                            {
                                if (emfEnDisable == 1)
                                {
                                    QMessageBox::information(this, tr("Handover Preferences") , QString ("Connection is established on the currently running Interface Either set link with low prefernce or Perform Handover"));
                                    integerSpinBox->setValue(up.HOPreferences[interfaceID][2]);
                                    interfaces_chkBox[i]->setChecked(true);
                                    adv_HO_selectNetComBox->addItem(textTemp1);
                                    BWtotalApp_tableItem[settext]->setText(textTemp1);
                                    BWtotalApp_table->setItem(settext, 0, BWtotalApp_tableItem[settext]);
                                    adv_Mon_selectLinkComBox->addItem(textTemp1);
                                    fromLinkComboBOx->addItem(textTemp1); 
                                    toLinkComboBOx->addItem(textTemp1); 
                                    fromLink_TrafficComboBOx->addItem(textTemp1);  
                                    toLink_TrafficComboBOx->addItem(textTemp1);
                                }
                            }
                        }
                    }
                }
                else
                {
                    char q, t;
                    for (q = 0; q < 20; q++)
                        if (strlen (up.adv_protocol_name[q]) > 0)
                            for (t = 0; t < 20; t++)
                                if (strlen(up.adv_interfaces_name[t]) > 0)
                                {
                                    if (up.adv_interfaces_name[t] == textTemp1)
                                        up.BAPreference[q][t] = 0;
                                }
                                retriveBWEntrTable();
                }
            }
        }
    }
}

//Function is used to set the Validator of LindEdit, so that Only Integer values can be entered
void MainWindowImpl::init()
{
    costLineEdit->setValidator( new QIntValidator( costLineEdit ) );
    dataRateLineEdit->setValidator( new QIntValidator( dataRateLineEdit ) );
    convert();
}

//HO_preferences conatains , Cost, Preferences, Value and Data Rate , This function is used to set the appropriate value against each Field
void MainWindowImpl::convert()
{
    costLineEdit->setText(QString::number(up.HOPreferences[interfaceID][5]));
    dataRateLineEdit->setText(QString::number(up.HOPreferences[interfaceID][6]));
    integerSpinBox->setValue(up.HOPreferences[interfaceID][2]);
    adv_HO_costUnitComBox->setCurrentIndex(up.HOPreferences[interfaceID][3]);
    adv_HO_datarateUnitComBox->setCurrentIndex(up.HOPreferences[interfaceID][4]);
    int input = costLineEdit->text().toInt();
    up.HOPreferences[interfaceID][5] = input;
    int input2 = dataRateLineEdit->text().toInt();
    up.HOPreferences[interfaceID][6] = input2;
    int input3 = integerSpinBox->value();
    up.HOPreferences[interfaceID][2] = input3;
    up.HOPreferences[interfaceID][3] = adv_HO_costUnitComBox->currentIndex();
    up.HOPreferences[interfaceID][4] = adv_HO_datarateUnitComBox->currentIndex();
    costLineEdit->setText(QString::number(up.HOPreferences[interfaceID][5]));
    dataRateLineEdit->setText(QString::number(up.HOPreferences[interfaceID][6]));
    integerSpinBox->setValue(up.HOPreferences[interfaceID][2]);
    adv_HO_costUnitComBox->setCurrentIndex(up.HOPreferences[interfaceID][3]);
    adv_HO_datarateUnitComBox->setCurrentIndex(up.HOPreferences[interfaceID][4]);
}

//Funciton is used to read/get the HO_Preferences Values
void MainWindowImpl::getSetHandoverPref()
{
    QString memo_text = "";
    memo_text=adv_HO_selectNetComBox->currentText();
    QByteArray textTemp = memo_text.toUtf8() ;
    if (strcmp(textTemp, "") != 0)
    {
        char i;
        for (i = 0; i < (20); i++) // 20 has to be replaced by up.TotalInterfaces $-$-$-$-$-
        {
            if (up.adv_interfaces_name[i] == memo_text.toAscii())
            {
                interfaceID = i;
                costLineEdit->setText(QString::number(up.HOPreferences[interfaceID][5]));
                dataRateLineEdit->setText(QString::number(up.HOPreferences[interfaceID][6]));
                integerSpinBox->setValue(up.HOPreferences[interfaceID][2]);
                adv_HO_costUnitComBox->setCurrentIndex(up.HOPreferences[interfaceID][3]);
                adv_HO_datarateUnitComBox->setCurrentIndex(up.HOPreferences[interfaceID][4]);
            }
        }
    }
}

void MainWindowImpl::cost_textChanged()
{
    float input = costLineEdit->text().toInt();
    float input2 = dataRateLineEdit->text().toInt();
    up.HOPreferences[interfaceID][5] = costLineEdit->text().toDouble();
    if (adv_HO_costUnitComBox->currentIndex() == 2)
    {
        if (adv_HO_datarateUnitComBox->currentIndex() == 1)
            up.HOPreferences[interfaceID][0] = input/( ((input2*3600)/8192) *720 );   //if Kbps[1]then      input/( ((input2*3600)/8192) *720 )
        else if (adv_HO_datarateUnitComBox->currentIndex() == 2)//Mbps
            up.HOPreferences[interfaceID][0] = input/( ((input2*3600)/8) *720 );    //if Mbps[2]then      input/( ((input2*3600)/8) *720 )
    }
    else
        up.HOPreferences[interfaceID][0] = input;
    if (adv_HO_datarateUnitComBox->currentIndex() == 2)
        up.HOPreferences[interfaceID][1] = (input2)*1024;
    else
        up.HOPreferences[interfaceID][1] = (input2);
}

void MainWindowImpl::dataRate_textChanged()
{
    float input = costLineEdit->text().toInt();
    float input2 = dataRateLineEdit->text().toInt();
    up.HOPreferences[interfaceID][6] = dataRateLineEdit->text().toDouble();
    if (adv_HO_costUnitComBox->currentIndex() == 2)
    {
        if (adv_HO_datarateUnitComBox->currentIndex() == 1)
            up.HOPreferences[interfaceID][0] = input/( ((input2*3600)/8192) *720 );   //if Kbps[1]then      input/( ((input2*3600)/8192) *720 )
        else if (adv_HO_datarateUnitComBox->currentIndex() == 2)//Mbps
            up.HOPreferences[interfaceID][0] = input/( ((input2*3600)/8) *720 );    //if Mbps[2]then      input/( ((input2*3600)/8) *720 )
    }
    else
        up.HOPreferences[interfaceID][0] = input;
    if (adv_HO_datarateUnitComBox->currentIndex() == 2)
        up.HOPreferences[interfaceID][1] = (input2)*1024;
    else
        up.HOPreferences[interfaceID][1] = (input2);
}

void MainWindowImpl::prefLevel_textChanged()
{
    up.HOPreferences[interfaceID][2] = integerSpinBox->value();
    if (up.indexOfMaxPrefHOIP[0] != -1)
    {
        if (up.indexOfMaxPrefHOIP[1] > 0)
        {
            if (up.HOPreferences[interfaceID][2] == 0)
            {           
                QString memo_text = "";
                memo_text=adv_HO_selectNetComBox->currentText();
                QByteArray textTemp = memo_text.toUtf8() ;
                if (up.adv_interfaces_name[up.indexOfMaxPrefHOIP[0]] == textTemp)
                {
                    QMessageBox::information(this, tr("Handover Preferences") , QString ("Connection is established on this Link Either change the prefernce or Perform Handover"));
                    integerSpinBox->setValue(1);
                    tabWidget->setCurrentIndex(1);
                    advanceTabWidget->setCurrentIndex(3);
                }
            }
        }
    }
}

void MainWindowImpl::setCostUnitComboBox()
{
    float input = costLineEdit->text().toInt();
    float input2 = dataRateLineEdit->text().toInt();
    up.HOPreferences[interfaceID][3] = adv_HO_costUnitComBox->currentIndex();
    if (adv_HO_costUnitComBox->currentIndex() == 2)
    {
        if (adv_HO_datarateUnitComBox->currentIndex() == 1)
            up.HOPreferences[interfaceID][0] = input/( ((input2*3600)/8192) *720 );   //if Kbps[1]then      input/( ((input2*3600)/8192) *720 )
        else if (adv_HO_datarateUnitComBox->currentIndex() == 2)//Mbps
            up.HOPreferences[interfaceID][0] = input/( ((input2*3600)/8) *720 );    //if Mbps[2]then      input/( ((input2*3600)/8) *720 )
    }
    else
        up.HOPreferences[interfaceID][0] = input;
    if (adv_HO_datarateUnitComBox->currentIndex() == 2)
        up.HOPreferences[interfaceID][1] = (input2)*1024;
    else
        up.HOPreferences[interfaceID][1] = (input2);
}

void MainWindowImpl::setRateUnitComboBox()
{
    float input = costLineEdit->text().toInt();
    float input2 = dataRateLineEdit->text().toInt();
    up.HOPreferences[interfaceID][4] = adv_HO_datarateUnitComBox->currentIndex();
    if (adv_HO_datarateUnitComBox->currentIndex() == 2)
        up.HOPreferences[interfaceID][1] = (input2)*1024;
    else
        up.HOPreferences[interfaceID][1] = (input2);
    if (adv_HO_costUnitComBox->currentIndex() == 2)
    {
        if (adv_HO_datarateUnitComBox->currentIndex() == 1)
            up.HOPreferences[interfaceID][0] = input/( ((input2*3600)/8192) *720 );   //if Kbps[1]then      input/( ((input2*3600)/8192) *720 )
        else if (adv_HO_datarateUnitComBox->currentIndex() == 2)//Mbps
            up.HOPreferences[interfaceID][0] = input/( ((input2*3600)/8) *720 );    //if Mbps[2]then      input/( ((input2*3600)/8) *720 )
    }
    else
        up.HOPreferences[interfaceID][0] = input;
}

//Function is used to add the Protocol in Protocol Tab
void MainWindowImpl::addProtocol()
{
    bool addPr_Name, addP_no, donotPass = false;
    QString pName;
    int pNo = 0;
    char i;
    pName = QInputDialog::getText(this, tr("Add Protocol"),tr("Protocol Name:"), QLineEdit::Normal,pName, &addPr_Name);
    QString memo_text = "";
    memo_text=pName;
    QByteArray textTemp = memo_text.toUtf8() ;
    textTemp = textTemp.trimmed();
    if ((strcmp(textTemp, "") != 0))
    {
        if (addPr_Name && !pName.isEmpty() && pName != "")
        {
            for (i = 0; i < 20; i++)
            {
                if (up.adv_protocol_name[i] == textTemp)
                {
                    QMessageBox::information(this, tr("Add Protocol") , QString (textTemp + " Already exists in the List"));
                    pName = "";
                    pNo = 0;
                    textTemp = "";
                    donotPass = true;
                    break;
                }
            }
            if (donotPass == false)
            {
                pNo = QInputDialog::getDouble(this, tr("Add Protocol"),tr("Protocol No:"), QLineEdit::Normal, pNo, 10000, 0, &addP_no);
                if (addP_no && pNo<=0)
                    addProtocol();
                else
                    for (i = 0; i < 20; i++)
                    {
                        if (up.adv_protocol_PortNo[i] == pNo)
                        {
                            QMessageBox::information(this, tr("Add Protocol") , QString ("Port Number already exists in the List"));
                            pName = "";
                            pNo = 0;
                            textTemp = "";
                            donotPass = true;
                            break;
                        }
                    }
            }
        }
        else if (addPr_Name && pName.isEmpty())
        {
            pName = "";
            pNo = 0;
            addProtocol();  
        }
        if ( (pName != "" ) && (pNo > 0 ) && addP_no)
        {
            for (i = 0; i < 20; i++)
            {
                if (strcmp(up.adv_protocol_name[i], "") == 0)//if u donot want to remove the protocol from its position used nProtocol in [????]
                {
                    QString memo_text = "";
                    memo_text=pName;
                    QByteArray textTemp = memo_text.toUtf8() ;
                    strcpy(up.adv_protocol_name[i], textTemp);
                    up.adv_protocol_PortNo[i] = pNo;
                    up.adv_protocol[i] = 0;
                    retriveProtocolTotalEntrTable();
                    break;
                }
            }
        }
    }
    else if ((addPr_Name == true) && (donotPass == false))
    {
        pName = "";
        pNo = 0;
        addProtocol();
    }  
}

//Remove the Entered/Selected Protocol from the Table in Protocol Tab
void MainWindowImpl::removeProtocol()
{
    if (totalApp_tableItem[0]->text() != "")
    {
        if (totalApp_table->currentRow() >= 0)
        {
            if (totalApp_tableItem[totalApp_table->currentRow()]->text() != "")
            {
                QString memo_text = "";
                memo_text = totalApp_table->item(totalApp_table->currentRow(),0)->text();
                QByteArray textTemp = memo_text.toUtf8() ;
                char i, j;
                for (i = 0; i < 20; i++)
                {
                    if (strcmp(up.adv_protocol_name[i], "") != 0)
                    {
                        if (up.adv_protocol_name[i] == textTemp)
                        {
                            up.adv_protocol[i] = 0;
                            strcpy(up.adv_protocol_name[i], "");
                            up.adv_protocol_PortNo[i] = 0;
                            for (j=0; j<20; j++)
                                up.BAPreference[i][j] = 0;
                            for(j = 0; j < 40; j++)
                            {
                                totalApp_tableItem[j]->setText("");
                                selectedApp_tableItem[j]->setText("");
                            }
                            retriveProtocolTotalEntrTable();
                            break;
                        }
                    }
                }
            }
        }
    }
}

void MainWindowImpl::retriveProtocolTotalEntrTable()
{
    int inc = 0;
    int inc_s = 0;
    char i;
    adv_BW_selectPrtComBox->clear();
    toTraffic_TrafficComboBOx->clear();
    adv_Mon_selectAppComBox->clear();
    for (i = 0; i < 20; i++)
    {
        if (strcmp(up.adv_protocol_name[i], "") != 0)
        {
            QString memo_text1 = "";
            memo_text1=(up.adv_protocol_name[i]);
            QByteArray textTemp1 = memo_text1.toUtf8() ;
            if (up.adv_protocol[i] == 0)
            {
                totalApp_tableItem[inc]->setText(textTemp1);
                inc++;
            }
            else if (up.adv_protocol[i] == 1)
            {
                selectedApp_tableItem[inc_s]->setText(textTemp1);
                adv_BW_selectPrtComBox->addItem(textTemp1);
                toTraffic_TrafficComboBOx->addItem(textTemp1);
                adv_Mon_selectAppComBox->addItem(textTemp1);
                inc_s++;
            }
        }
    }
    for (i = 0; i < 20; i++)
    {
        totalApp_table->setItem(i, 0, totalApp_tableItem[i]);
        selectedApp_Table->setItem(i, 0, selectedApp_tableItem[i]);
    }
    inc = 20;
    inc_s = 20;
    for (i = 0; i < 20; i++)
    {
        if (up.adv_protocol_PortNo[i] > 0)
        {
            QString memo_text1 = "";
            memo_text1=(QString::number(up.adv_protocol_PortNo[i]));
            QByteArray textTemp1 = memo_text1.toUtf8() ;
            if (up.adv_protocol[i] == 0)
            {
                totalApp_tableItem[inc]->setText(textTemp1);
                inc++;
            }
            else  if(up.adv_protocol[i] == 1)
            {
                selectedApp_tableItem[inc_s]->setText(textTemp1);
                inc_s++;
            }
        }
    }
    for (i = 0; i < 20; i++)
    {
        totalApp_table->setItem(i, 1, totalApp_tableItem[i+20]);
        selectedApp_Table->setItem(i, 1, selectedApp_tableItem[i+20]);
    }
}

void MainWindowImpl::saveProtocolTotalEntrTable()
{
    int inc_total = 0;
    int inc_selected = 0;
    char i;
    for (i = 0; i < 20; i++)
    {
        if (up.adv_protocol[i] == 0)
        {
            if (totalApp_tableItem[0]->text() != "")
            {			
                QString memo_text = "";
                memo_text=totalApp_table->item(inc_total,0)->text();
                QByteArray textTemp = memo_text.toUtf8() ;
                strcpy(up.adv_protocol_name[i], textTemp); 
                QString memo_text1 = "";
                memo_text1=totalApp_table->item(inc_total,1)->text();
                QByteArray textTemp1 = memo_text1.toUtf8() ;
                up.adv_protocol_PortNo[i] = textTemp1.toInt();
                inc_total++;
            }
        }
        else if (up.adv_protocol[i] == 1)
        {
            if (totalApp_tableItem[0]->text() != "")
            {
                QString memo_text = "";
                memo_text=selectedApp_Table->item(inc_selected,0)->text();
                QByteArray textTemp = memo_text.toUtf8() ;
                strcpy(up.adv_protocol_name[i], textTemp); 
                QString memo_text1 = "";
                memo_text1=selectedApp_Table->item(inc_selected,1)->text();
                QByteArray textTemp1 = memo_text1.toUtf8() ;
                up.adv_protocol_PortNo[i] = textTemp1.toInt();
                inc_selected++;
            }
        }
        else
        {
            strcpy(up.adv_protocol_name[i], "");
            up.adv_protocol_PortNo[i] = 0;
        }
    }
}

void MainWindowImpl::forwardTotal_SelectedTable()
{
    if (totalApp_tableItem[0]->text() != "")
    {
        if (totalApp_table->currentRow() >= 0)
        {
            if (totalApp_tableItem[totalApp_table->currentRow()]->text() != "")
            {
                char i, j;
                QString memo_text = "";
                memo_text = totalApp_table->item(totalApp_table->currentRow(),0)->text();
                QByteArray textTemp = memo_text.toUtf8() ;
                for (i = 0; i < 20; i++)
                {
                    if (strcmp(up.adv_protocol_name[i], "") != 0)
                    {
                        if (up.adv_protocol_name[i] == textTemp)
                        {
                            up.adv_protocol[i] = 1;
                            for(j = 0; j < 40; j++)
                            {
                                totalApp_tableItem[j]->setText("");
                                selectedApp_tableItem[j]->setText("");
                            }
                                retriveProtocolTotalEntrTable();
                        }
                    }
                }
            }
        }
    }
}


void MainWindowImpl::backwardSelected_Table()
{
    if (selectedApp_tableItem[0]->text() != "")
    {
        if (selectedApp_Table->currentRow() >= 0)
        {
            if (selectedApp_tableItem[selectedApp_Table->currentRow()]->text() != "")
            {
                QString memo_text = "";
                memo_text = selectedApp_Table->item(selectedApp_Table->currentRow(),0)->text();
                QByteArray textTemp = memo_text.toUtf8() ;
                char i, j;
                for (i = 0; i < 20; i++)
                {
                    if (strcmp(up.adv_protocol_name[i], "") != 0)
                    {
                        if (up.adv_protocol_name[i] == textTemp)
                        {
                            up.adv_protocol[i] = 0;
                            for(j = 0; j < 40; j++)
                            {
                                totalApp_tableItem[j]->setText("");
                                selectedApp_tableItem[j]->setText("");
                            }
                            retriveProtocolTotalEntrTable();
                        }
                    }
                }
            }
        }
    }
}

void MainWindowImpl::forwardBW_SelectedTable()
{
    if (BWtotalApp_tableItem[0]->text() != "")
    {
        if (BWtotalApp_table->currentRow() >= 0)
        {
            if (BWtotalApp_tableItem[BWtotalApp_table->currentRow()]->text() != "")
            {
                QString memo_text = "";
                memo_text=adv_BW_selectPrtComBox->currentText();
                QByteArray textTemp = memo_text.toUtf8() ;
                char i, j;
                for (i = 0; i < 20; i++)
                {
                    if (strlen(up.adv_protocol_name[i]) > 0)
                    {
                        if (up.adv_protocol_name[i] == textTemp)
                        {
                            QString memo_text1 = "";
                            memo_text1 = BWtotalApp_table->item(BWtotalApp_table->currentRow(),0)->text();
                            QByteArray textTemp1 = memo_text1.toUtf8() ;
                            for(j = 0; j < 20; j++)
                            {
                                if (strlen(up.adv_interfaces_name[j]) > 0)
                                    if (up.adv_interfaces_name[j] == textTemp1)
                                        up.BAPreference[i][j] = 1;
                            }
                        }
                    }
                }
                retriveBWEntrTable();
            }
        }
    }
}


void MainWindowImpl::retriveBWEntrTable()
{
    QString memo_text = "";
    memo_text=adv_BW_selectPrtComBox->currentText();
    QByteArray textTemp = memo_text.toUtf8() ;
    char i, j;
    for(i = 0; i < 20; i++)
        BWselectedApp_tableItem[i]->setText("");
    int inc = 0;
    if (strcmp(textTemp, "") != 0)
    {
        for (i = 0; i < 20; i++)
        {
            if (up.adv_protocol_name[i] == memo_text.toAscii())
            {
                for (j = 0; j < 20; j++)
                {
                    if (strlen(up.adv_interfaces_name[j]) > 0)
                    {                
                        if (up.BAPreference[i][j] == 1)
                        {
                            QString memo_text1 = "";
                            memo_text1=(up.adv_interfaces_name[j]);
                            QByteArray textTemp1 = memo_text1.toUtf8() ;
                            BWselectedApp_tableItem[inc]->setText(textTemp1);
                            BWselectedApp_Table->setItem(inc, 0, BWselectedApp_tableItem[inc]);
                            inc++;
                        }
                    }
                }
            }
        }
    }
}

void MainWindowImpl::backwardBW_SelectedTable()
{
    if (BWselectedApp_tableItem[0]->text() != "")
    {
        if (BWselectedApp_Table->currentRow() >= 0)
        {
            if (BWselectedApp_tableItem[BWselectedApp_Table->currentRow()]->text() != "")
            {
                char i, j;
                QString memo_text = "";
                memo_text=adv_BW_selectPrtComBox->currentText();
                QByteArray textTemp = memo_text.toUtf8() ;
                for (i = 0; i < 20; i++)
                {
                    if (strcmp(up.adv_protocol_name[i], "") != 0)
                    {
                        if (up.adv_protocol_name[i] == textTemp)
                        {
                            QString memo_text1 = "";
                            memo_text1 = BWselectedApp_Table->item(BWselectedApp_Table->currentRow(),0)->text();
                            QByteArray textTemp1 = memo_text1.toUtf8() ;
                            for(j = 0; j < 20; j++)
                            {
                                if (strlen(up.adv_interfaces_name[j]) > 0)
                                    if (up.adv_interfaces_name[j] == textTemp1)
                                        up.BAPreference[i][j] = 0;
                            }
                        }
                    }
                }
                for(i = 0; i < 20; i++)
                    BWselectedApp_tableItem[i]->setText("");
                retriveBWEntrTable();
            }
        }
    }
}

void MainWindowImpl::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(toTryAct);
    fileMenu->addAction(exitAct);
}

void MainWindowImpl::populateManualWinComboBox()
{
    adv_Mon_selectLinkComBox->clear();
    adv_Mon_selectAppComBox->clear();
    toTraffic_TrafficComboBOx->clear();
    adv_Mon_selectLinkComBox->clear();
    fromLinkComboBOx->clear();
    toLinkComboBOx->clear();
    fromLink_TrafficComboBOx->clear();
    toLink_TrafficComboBOx->clear();
    char i;
    for (i = 0; i < 20 ; i++)
    {
        if (strcmp(up.adv_protocol_name[i], "") != 0)
        {
            if (up.adv_protocol[i] == 1)
            {
                QString memo_text1 = "";
                memo_text1=(up.adv_protocol_name[i]);
                QByteArray textTemp1 = memo_text1.toUtf8() ;
                adv_Mon_selectAppComBox->addItem(textTemp1); 
                toTraffic_TrafficComboBOx->addItem(textTemp1);
            }
        }
    }
    for (i = 0; i < (20); i++)
    {
        if (strcmp(up.adv_interfaces_name[i], "") != 0)
        {
            if (up.adv_interfaces[i] == 1)
            {
                QString memo_text1 = "";
                memo_text1=(up.adv_interfaces_name[i]);
                QByteArray textTemp1 = memo_text1.toUtf8() ;
                adv_Mon_selectLinkComBox->addItem(textTemp1);
                fromLinkComboBOx->addItem(textTemp1); 
                toLinkComboBOx->addItem(textTemp1); 
                fromLink_TrafficComboBOx->addItem(textTemp1);  
                toLink_TrafficComboBOx->addItem(textTemp1);
            }
        }
    }
}

void MainWindowImpl::saveANDclose()
{
    char index, j;
    if(enDisableEMF->isChecked() == false)
    {
        if (maybeSave())
        {
            if (preference_BA() == false)
            {
                minimizeToSystemTray();
                UA_TLV.TLVType = USER_PREFERENCE_CHANGED_IND; // 59;
                UA_TLV.TLVLength = strlen(UA_TLV.TLVValue);		// TLV Length
                if (send(socketFD, &UA_TLV, sizeof (struct TLVInfo), 0)==-1)
                    perror("Error: USER_PREFERENCE_CHANGED_IND Send");
                else
                    displayAll();
            }
        }
    }
    else
    {
        if (preference_BA() == false)
        {
            minimizeToSystemTray();
            UA_TLV.TLVType = USER_PREFERENCE_CHANGED_IND; // 59;
            UA_TLV.TLVLength = strlen(UA_TLV.TLVValue);		// TLV Length
            if (send(socketFD, &UA_TLV, sizeof (struct TLVInfo), 0)==-1)
                perror("Error: USER_PREFERENCE_CHANGED_IND Send");
            else
                displayAll();
            printf("Ok Applied\n");
        }
    }
}


void MainWindowImpl::applyANDsave()
{
    char index, j;
    if(enDisableEMF->isChecked() == false)
    {
        if (maybeSave())
        {
            if (preference_BA() == false)
            {
                saveStructure();
                UA_TLV.TLVType = USER_PREFERENCE_CHANGED_IND; // 59;
                UA_TLV.TLVLength = strlen(UA_TLV.TLVValue);		// TLV Length
                if (send(socketFD, &UA_TLV, sizeof (struct TLVInfo), 0)==-1)
                    perror("Error: On Apply USER_PREFERENCE_CHANGED_IND Send");
                else
                    displayAll();
            }
        }
    }
    else
    {
        if (preference_BA() == false)
        {
            saveStructure();
            UA_TLV.TLVType = USER_PREFERENCE_CHANGED_IND; // 59;
            UA_TLV.TLVLength = strlen(UA_TLV.TLVValue);		// TLV Length
            if (send(socketFD, &UA_TLV, sizeof (struct TLVInfo), 0)==-1)
                perror("Error: On Apply USER_PREFERENCE_CHANGED_IND Send");
            else
                displayAll();
            printf("Applied\n");
        }
    }
}

void MainWindowImpl::setInterfaceChkBox()
{
    FILE *filepointer;
    filepointer = fopen("../emf_hostagent/userPref.txt", "r");
    if(filepointer == NULL)
    {
        fileNotfound = false;
        printf("User Preferences are not available");
    }
    else
    {
        fileNotfound = true;
        fread(&emfEnDisable, 1, 1,filepointer);
        fread(&up,sizeof(up), 1,filepointer);
        fclose (filepointer);
    }
    char i;
    for (i = 0; i < 20; i++)
        interfaces_chkBox[i]->setText("");
    nINTERFACES = 0;
    for (i = 0; i < 20; i++)
    {	
        if (strcmp(up.adv_interfaces_name[i], "") != 0)
        {
            if (up.adv_interfaces[i] == 1)
            {
                interfaces_chkBox[nINTERFACES]->setText(up.adv_interfaces_name[i]);
                interfaces_chkBox[nINTERFACES]->setChecked(true);
            }
            else if (up.adv_interfaces[i] == 0)
            {
                interfaces_chkBox[nINTERFACES]->setText(up.adv_interfaces_name[i]);
                interfaces_chkBox[nINTERFACES]->setChecked(false);
            }
            nINTERFACES++;
        }
    }
    interfaceChkBoxLoc();
}

void MainWindowImpl::slotQUIT()
{
    if (::close(socketFD) != -1)
    {
        timer->stop();
        FD_CLR(socketFD, &baseFDList);
        FD_CLR(maxFDValue, &baseFDList);
        ::close(socketFD);
        ::close(maxFDValue);
        socketFD = NULL;
        maxFDValue = NULL;
    }
    StartStartUP();
}

bool MainWindowImpl::maybeSave()
{
    if (yesNoOption == 0)
    {
        QMessageBox::StandardButton ret;
        ret = QMessageBox::warning(this, tr("EMF Application"),
        tr("You are going to Disable the EMF Application\n""Do you want to proceed?"),
        QMessageBox::Yes | QMessageBox::No);
        yesNoOption = 1;
        if (ret == QMessageBox::Yes)
            return true;
        else if (ret == QMessageBox::No)
        {
            tabWidget->setCurrentIndex(0);
            enDisableEMF->setChecked(true);
            setting_InterfaceOptions();
            return false;
        }
        return false;
    }
    return true;
}

bool MainWindowImpl::preference_BA()
{  
    char i, j, lvar;
    int INDEX = 0;
    for (i = 0; i < 20; i++)
        if (up.adv_protocol[i] == 1)
        {
            lvar = 0;
            for (j = 0; j < 20; j++)
            {
                if (up.BAPreference[i][j] == 1)
                    lvar++;
                if (lvar >= 2)
                    break;
            }
            INDEX++;
            if (lvar == 1)
            {
                if (emfEnDisable == 1)
                {
                    QMessageBox::information(this, tr("BandWidth Aggregation Preferences") , QString ("Add Minimum two Interfaces against each selected Protocol in Bandwidth Aggregation"));
                    tabWidget->setCurrentIndex(1);
                    advanceTabWidget->setCurrentIndex(4);
                    adv_BW_selectPrtComBox->setCurrentIndex(INDEX-1);
                    return true;
                }
            }
        }
        return false;
}


void MainWindowImpl::cancelClose()
{
    if (trayIcon->isVisible())
    {
        initialValues();
        retriveRepository();
        getSetHandoverPref();
        addInterfacesComboBox();
        hide();
    }   
}


void MainWindowImpl::saveDuplicateData()
{
    saveInterfacePref();
    FILE *fp;
    size_t count;
    fp = fopen("../emf_hostagent/InterfaceInfo.txt", "w + r");
    if(fp == NULL)
    {
        //perror("1 failed to open ../emf_hostagent/InterfaceInfo.txt");
        fclose (fp);
    }
    count = fwrite (&IntPref, sizeof(IntPref),1,fp);
    fflush(fp);
    fseek(fp,0,SEEK_SET);
    fclose (fp);
}

void MainWindowImpl::saveInterfacePref()
{
    char index, i, j, check,q;
    for (index = 0; index < 20 ; index++)
    {
        for (i = 0; i < 20 ; i++)// Go to total number of Interfaces iNtfiles //oldInterfaces
        {
            if (strlen(IntPref.interfaceName[i]) > 0)
            {           //New Interfaces                        Old Interfaces
                if (strcmp(up.adv_interfaces_name[index], IntPref.interfaceName[i]) == 0)
                {
                    strcpy(IntPref.interfaceName[i], up.adv_interfaces_name[index]);
                    IntPref.intefaceStatus[i] = up.adv_interfaces[index];
                    for (j = 0; j < 7 ; j++)
                        IntPref.HandoverPref[i][j] = up.HOPreferences[index][j];
                    break;
                }
            }
            else
            {
                strcpy(IntPref.interfaceName[i], up.adv_interfaces_name[index]);
                IntPref.intefaceStatus[i] = up.adv_interfaces[index];
                for (j = 0; j < 7 ; j++)
                    IntPref.HandoverPref[i][j] = up.HOPreferences[index][j];
                break;                
            }
        }
    }
    for (j = 0; j < 20 ; j++)//New Protocols
    {
        if (strlen(up.adv_protocol_name[j]) > 0)
        {
            check = 0;
            for (index = 0; index < 20 ; index++)
            {
                if (strlen(up.adv_interfaces_name[index]) > 0)
                {
                    for (i = 0; i < 20 ; i++)// Go to total number of Interfaces iNtfiles //oldInterfaces
                    {
                        if (strlen(IntPref.interfaceName[i]) > 0)
                        {           //New Interfaces                        Old Interfaces
                            if (strcmp(up.adv_interfaces_name[index], IntPref.interfaceName[i]) == 0)
                            {
                                IntPref.BWAggrgPref[j][i] = up.BAPreference[j][index];
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
}


void MainWindowImpl::readInterface_File()
{
    FILE *fp;
    size_t count3;
    fp = fopen("../emf_hostagent/InterfaceInfo.txt", "r");
    if(fp == NULL)
    {
        //perror("2 failed to open ../emf_hostagent/InterfaceInfo.txt");
    }
    else
    {
        count3 = fread(&IntPref,sizeof(IntPref), 1,fp);
        fclose (fp);
        readInterfacePref();
    }
}   

void MainWindowImpl::readInterfacePref()
{
    char index, i, j;
    spinBoxOldValue = 0;
    for (index = 0; index < 20 ; index++)
    {
        if (strlen(up.adv_interfaces_name[index]) > 0)        
        {
            spinBoxOldValue = 1;
            for (i = 0; i < 20 ; i++)// Go to total number of Interfaces iNtfiles //oldInterfaces
            {
                if (strlen(IntPref.interfaceName[i]) > 0)
                {           //New Interfaces                        Old Interfaces
                    if (strcmp(up.adv_interfaces_name[index], IntPref.interfaceName[i]) == 0)
                    {
                        up.adv_interfaces[index] = IntPref.intefaceStatus[i];
                        for (j = 0; j < 7 ; j++)
                            up.HOPreferences[index][j] = IntPref.HandoverPref[i][j];
                        if (up.adv_interfaces[index] == 1)
                            interfaces_chkBox[index]->setChecked(true);
                        else
                            interfaces_chkBox[index]->setChecked(false);
                        break;
                    }
                    spinBoxOldValue++;
                }
            }
        }
        else
        {
            strcpy(up.adv_interfaces_name[index], "");
            up.adv_interfaces[index] = 0;
            for (j = 0; j < 7 ; j++)
                up.HOPreferences[index][j] = 0;
        }
    }
    for (j = 0; j < 20 ; j++)//New Protocols
    {
        if (strlen(up.adv_protocol_name[j]) > 0)
        {
            for (index = 0; index < 20 ; index++)
            {
                if (strlen(up.adv_interfaces_name[index]) > 0)
                {
                    for (i = 0; i < 20 ; i++)// Go to total number of Interfaces iNtfiles //oldInterfaces
                    {
                        if (strlen(IntPref.interfaceName[i]) > 0)
                        {           //New Interfaces                        Old Interfaces
                            if (strcmp(up.adv_interfaces_name[index], IntPref.interfaceName[i]) == 0)
                            {
                                up.BAPreference[j][index] = IntPref.BWAggrgPref[j][i];
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    for (i = 0; i < 20; i++)
        BWselectedApp_tableItem[i]->setText("");
    char inc = 0;    
    for (i = 0; i < 20; i++)
        if (strlen (up.adv_protocol_name[i]) > 0)
    for (j = 0; j < 20; j++)
        if (up.BAPreference[i][j] == 1)
        {
            if (strlen(up.adv_interfaces_name[j]) > 0)
            {
                if (interfaces_chkBox[j]->isChecked())
                {
                    QString memo_text1 = "";
                    memo_text1=(up.adv_interfaces_name[j]);
                    QByteArray textTemp1 = memo_text1.toUtf8();
                    BWselectedApp_tableItem[inc]->setText(textTemp1);
                    BWselectedApp_Table->setItem(inc, 0, BWselectedApp_tableItem[inc]);
                    inc++;
                }
                else
                    up.BAPreference[i][j] = 0;
            }
            else
                up.BAPreference[i][j] = 0;
        }
        retriveBWEntrTable();
        saveStructure();
        UA_TLV.TLVType = USER_PREFERENCE_CHANGED_IND; // 59;
        UA_TLV.TLVLength = strlen(UA_TLV.TLVValue);		// TLV Length
        if (send(socketFD, &UA_TLV, sizeof (struct TLVInfo), 0)==-1)
            perror("Error: USER_PREFERENCE_CHANGED_IND Send");
        else
            displayAll();
}

void MainWindowImpl::StartStartUP()
{
    FILE *file = NULL;
    if ((file = fopen("name.txt", "r")) == NULL) file = fopen("name.txt", "w"); // Create ../emf_hostagent/userPref.txt File
    char mainPATH[100];char DesktopPATH[100];unsigned cnum = 0;int ch;
    system("whoami > name.txt");
    fclose(file);
    file = fopen("name.txt", "r");
    memset(mainPATH,'\0',100);while ((ch = fgetc(file)) != EOF){if (ch == '\n'){break;}else{mainPATH[cnum] = ch;cnum++;}}
    mainPATH[cnum] = '\0';
    memset(DesktopPATH,'\0',100);
    strcat(DesktopPATH, "mkdir  -p ");
    printf("User:%s\n",mainPATH);
    char str[80];//bzero(str, 80);
    memset(str,'\0',80);
    if (strcmp(mainPATH, "root") == 0){
    strcpy(str, "/");strcat(str, mainPATH);strcat(str, "/.config/autostart");}
    else
    {
        strcpy(str, "/home/");
        strcat(str, mainPATH);
        strcat(str, "/.config/autostart");
    }
    strcat(DesktopPATH, str);
    if(system(DesktopPATH) == 0)
    {
        printf("\n");
    }
    strcat(str, "/EMF.desktop");
    str[strlen(str)] = '\0';
    fclose(file); 
    file = fopen ( "name.txt", "r + w" );                               // ************ File Open ************ name.txt
    system("pwd > name.txt");
    ch = 0;cnum = 0;
    while ((ch = fgetc(file)) != EOF){if (ch == '\n'){break;}else{if (ch == ' '){mainPATH[cnum] = '\\';mainPATH[++cnum] = ' ';}
    else mainPATH[cnum] = ch;cnum++;}}
    mainPATH[cnum] = '\0';
    fclose(file);                                           // ____________ File Close ___________ name.txt
    char emfCltSer[11];memset(emfCltSer,'\0',11);
    FILE *filepointer;filepointer = fopen("../emf_core/emf_client", "r");if(filepointer == NULL)strcpy(emfCltSer, "emf_server");else{strcpy(emfCltSer,"emf_client");
    fclose(filepointer);}           // ____________ File Close ___________ name.txt
    emfCltSer[strlen(emfCltSer)] = '\0';
    char emfSH[100];bzero(emfSH, 100);strncpy(emfSH, mainPATH, strlen(mainPATH) - 13); strcat(emfSH, "end2end.sh");
    emfSH[strlen(emfSH)] = '\0';
    file = fopen (str, "w" );                                           // ************ File Open ************ path.txt
    fprintf(file, "[Desktop Entry]\n");
    fprintf(file, "Type=Application\n");
    fprintf(file, "Name=EMF\n");
    fprintf(file, "Exec=");
    fprintf(file, emfSH);
    fprintf(file, "\nIcon=system-run\n");
    fprintf(file, "Comment=End to End Mobility Mangement Frame Work\n");
    fprintf(file, "X-GNOME-Autostart-enabled=true\n");
    fclose (file);                                                     // ____________ File Close ___________ name.txt
    file = fopen ( "end2end.sh", "w" );// ************ File Open ************ end2end.txt
    system("chmod +x end2end.sh");
    if ( file != NULL )
    {
        memset(emfSH,'\0',100);
        strncpy(emfSH, mainPATH, strlen(mainPATH) - 14);
        emfSH[strlen(emfSH)] = '\0';
        if (up.grl_options[0] == 0)
        {
            fprintf(file, "#! /bin/bash\n");
            fprintf(file, "# EMF Application\n");
            fprintf(file, "#chmod +x end2end.sh\n");
            fprintf(file, "#cd  ");
            fprintf(file, emfSH);
            fprintf(file, "/emf_hostagent/;   ./emfHost   &\n");
            fprintf(file, "#cd  ");
            fprintf(file, emfSH);
            fprintf(file, "/emf_Interface/;   ./emf_Interface   &\n");
            fprintf(file, "#sleep \"20\" \n");
            fprintf(file, "#cd  ");
            fprintf(file, emfSH);
            fprintf(file, "/emf_cl2i/;   ./cl2i   &\n");
            fprintf(file, "#sleep \"1\" \n");
            fprintf(file, "#cd  ");
            fprintf(file, emfSH);
            fprintf(file, "/emf_core/;   ./");
            fprintf(file, emfCltSer);
        }
        else
        {
            fprintf(file, "#! /bin/bash\n");
            fprintf(file, "# EMF Application\n");
            fprintf(file, " chmod +x end2end.sh\n");
            fprintf(file, " cd  ");
            fprintf(file, emfSH);
            fprintf(file, "/emf_hostagent/;   ./emfHost   &\n");
            fprintf(file, " cd  ");
            fprintf(file, emfSH);
            fprintf(file, "/emf_Interface/;   ./emf_Interface   &\n");
            fprintf(file, " sleep \"20\" \n");
            fprintf(file, " cd  ");
            fprintf(file, emfSH);
            fprintf(file, "/emf_cl2i/;   ./cl2i   &\n");
            fprintf(file, " sleep \"1\" \n");
            fprintf(file, " cd  ");
            fprintf(file, emfSH);
            fprintf(file, "/emf_core/;   ./");
            fprintf(file, emfCltSer);
        }
    }
    else
    {
        perror ( "Failed to Start On StartUp" );
    }
    fclose ( file );                    // ____________ File Close ___________ name.txt
    memset(mainPATH,'\0',100);
    strcpy(mainPATH, "cp end2end.sh ../end2end.sh");
    mainPATH[strlen(mainPATH)] = '\0';
    system(mainPATH);
    system("rm end2end.sh");
    system("rm name.txt");
}


void MainWindowImpl::displayAll()
{
/*
    char index, j=0;
    printf("\n EMF Enable \t\t\t[");
    printf("%d", emfEnDisable);
    printf("]\n");

    printf("\n General EMF Options\t\t[");
    printf("( %d, ", up.grl_options[0]);
    printf("%d , ", up.grl_options[1]);
    printf("%d , ", up.grl_options[2]);
    printf("%d , ", up.grl_options[3]);
    printf("%d )", up.grl_options[4]);
    printf("]\n");

    printf("\n Interfaces\t\t\t[");
    for (index=0; index<20; index++)
    if (strlen(up.adv_interfaces_name[index]) > 0)
    {
    printf("(%s) ", up.adv_interfaces_name[index]);
    j=1;
    }
    if (j==1)
    printf("\b]\n");
    else if (j==0)
    printf("]\n");


    printf("\n EMF_Compliance_of_Interfaces   [");
    for (index=0; index<20; index++)
    printf("%d ", up.adv_interfaces[index]);
    printf("\b]\n");


    j=0;
    printf("\n HO_Preferences\t\t\t[");
    for (index=0; index<20; index++)
    if (strlen(up.adv_interfaces_name[index]) > 0)
    {
    printf("(%.0f, ", up.HOPreferences[index][2]);
    printf("%.2f, ", up.HOPreferences[index][0]);
    printf("%.0f, ", up.HOPreferences[index][1]);
    printf("%.0f, ", up.HOPreferences[index][5]);
    printf("%.0f), ", up.HOPreferences[index][6]);
    j=1;
    }
    if (j==1)
    printf("\b\b]\n");
    else if (j==0)
    printf("]\n");


    j=0;
    printf("\n Protocols\t\t\t[");
    for (index=0; index<20; index++)
    if (strlen(up.adv_protocol_name[index]) > 0)
    {
    printf("%s, ", up.adv_protocol_name[index]);
    j=1;
    }
    if (j==1)
    printf("\b\b]\n");
    else if (j==0)
    printf("]\n");


    printf("\n EMF_Compliance_of_Protocols\t[");
    for (index=0; index<20; index++)
    printf("%d ", up.adv_protocol[index]);
    printf("\b]\n");


    printf("\n BA_Preferences");
    for (index=0; index<20; index++)
    {
    if (strlen(up.adv_protocol_name[index]) > 0)
    {
    if(index == 0)
    printf("\t\t\t[");
    else
    printf("\t\t\t\t[");
    printf("%s:\t(", up.adv_protocol_name[index]);
    for (j=0; j<20; j++)
    if (strlen(up.adv_interfaces_name[j]) > 0)
    if (up.BAPreference[index][j] == 1)
    printf("  %s  ", up.adv_interfaces_name[j]);
    printf(")]\n");
    }
    }

    printf("\n indexOfMaxPrefHOIndex \t\t[%d][%d]\n", up.indexOfMaxPrefHOIP[0], up.indexOfMaxPrefHOIP[1]);

    printf("\n----------------------------------------------------------------------------------------------------------------------------------------\n");
*/
}

void MainWindowImpl::openManual()
{
    system("evince '../../Documentation/PDF Version/2_Quick Start EMF Guide.pdf' &");
}

void MainWindowImpl::updateEMF()
{
    QMessageBox::information(this, tr("EMF Update") , QString ("You are currently running an Update Version"));
}
