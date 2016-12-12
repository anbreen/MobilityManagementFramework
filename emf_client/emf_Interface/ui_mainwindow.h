/********************************************************************************
** Form generated from reading ui file 'mainwindow.ui'
**
** Created: Mon Mar 12 09:29:33 2009
**      by: Qt User Interface Compiler version 4.3.5
**
** WARNING! All changes made in this file will be lost when recompiling ui file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QTimer>
#include <QtGui/QMenu>
#include <QtGui/QLabel>
#include <QtGui/QFrame>
#include <QtGui/QAction>
#include <QtGui/QWidget>
#include <QtCore/QLocale>
#include <QtGui/QSpinBox>
#include <QtGui/QGroupBox>
#include <QtGui/QCheckBox>
#include <QtGui/QLineEdit>
#include <QtGui/QComboBox>
#include <QtGui/QTableView>
#include <QtGui/QTabWidget>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QGridLayout>
#include <QtGui/QMainWindow>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QMessageBox>
#include <QtGui/QHeaderView>
#include <QtGui/QApplication>
#include <QtGui/QRadioButton>
#include <QtGui/QTableWidget>
#include <QtGui/QSystemTrayIcon>
#include <QtGui/QTableWidgetItem>

class Ui_MainWindow
{
public:
    QLabel *intfacHeadLabel, *label_2 , *selectKey_Label, *prtNamePrtNoLabel, *availablePrt_Lable, *selectedPrt_Lable , *label_8, *label_5, *label_4;
    QLabel *label_9, *label_10, *selectTwoNetworks, *fromLinkLabel, *toLinkLabel, *fromLink_TrafficLabel, *toLink_TrafficLabel, *toTraffic_TrafficLabel;
    QLabel *perforLabel, *manualBALabel, *label_11, *sltLinkLabel, *appFileSlabel, *label_15, *label_16, *label_17, *label_18, *label_6, *label_7;
    QLabel *cost_Label, *dataRate_Label, *parameter_Label, *value_Label, *unit_Label, *handover_HeadingLabel, *selectNet_Label, *preference_Label;
    QWidget *centralwidget, *generalTab, *advancedTab, *interfacestab, *authTab, *manualConfTab, *protocolTab, *handoverTab, *bwAggTab;
    QWidget *manualHO_TrafficTab, *manualHO_LinkTab, *helpTab, *layoutWidget, *layoutWidget1, *layoutWidget2, *layoutWidget3, *layoutWidget4;
    QTabWidget *tabWidget, *advanceTabWidget, *ManualHO_tabWidget;
    QCheckBox *enDisableEMF, *g_startOnstartUp, *g_informInterfaceAvail, *g_informHandoverComp, *g_informBWAvail, *g_enableLocUpdate;
    QCheckBox *adv_HO_enForcedHOCBox, *interfaces_chkBox[20];
    QFrame *adv_Mon_Frame, *manual_BAFrame, *manual_HOFrame, *adv_prefProtocolFrame, *adv_prefBWFrame, *adv_HO_Frame, *adv_interfacesFrame;
    QFrame *adv_authenticationFrame, *labelFrame, *labelFrame_2;
    QRadioButton *adv_auth_AnonRB, *adv_auth_EcurveRB, *adv_auth_DHelRB, *adv_auth_useCertRB;
    QLineEdit *adv_auth_CertPathLEdit, *adv_auth_KeyPathLEdit, *costLineEdit, *dataRateLineEdit;
    QPushButton *adv_auth_selectCertButton, *adv_auth_selectKeyButton, *adv_pref_AddProtocolButton, *adv_pref_removePrtButton, *adv_Mon_MHOButton;
    QPushButton *adv_Mon_MBWButton, *help_ConnectUpdateButton, *help_OpenManlButton, *cancelButton, *okPushButton, *applyButton;
    QPushButton *forward_Button, *backward_Button, *BWforward_Button, *BWbackward_Button;
    QComboBox *adv_HO_selectNetComBox, *adv_BW_selectPrtComBox, *fromLinkComboBOx, *toLinkComboBOx, *fromLink_TrafficComboBOx,*toTraffic_TrafficComboBOx;
    QComboBox *toLink_TrafficComboBOx, *adv_Mon_selectAppComBox, *adv_Mon_selectLinkComBox, *adv_HO_costUnitComBox, *adv_HO_datarateUnitComBox;
    QTableWidget *totalApp_table, *selectedApp_Table, *BWtotalApp_table, *BWselectedApp_Table;
    QTableWidgetItem *totalApp_tableItem[40], *selectedApp_tableItem[40], *BWtotalApp_tableItem[20], *BWselectedApp_tableItem[20];    
    QGridLayout *gridLayout, *gridLayout1, *gridLayout2, *gridLayout3, *gridLayout4;
    QSpacerItem *spacerItem, *spacerItem1, *spacerItem2, *spacerItem3, *spacerItem4, *spacerItem5;
    QAction *minimizeAction, *maximizeAction, *restoreAction, *quitAction, *toTryAct, *exitAct;
    QGroupBox *elliptic_Deffi_GrpBox;
    QSystemTrayIcon *trayIcon;
    QSpinBox *integerSpinBox;
    QVBoxLayout *vboxLayout;
    QHBoxLayout *hboxLayout;
    QMenu *trayIconMenu;
    QMenu *fileMenu;
    QTimer *timer;

    void setupUi(QMainWindow *MainWindow)
    {
    if (MainWindow->objectName().isEmpty())
        MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
    MainWindow->resize(680, 425);
    MainWindow->setMinimumSize(QSize(680, 425));
    MainWindow->setMaximumSize(QSize(680, 425));
    centralwidget = new QWidget(MainWindow);
    centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
    tabWidget = new QTabWidget(centralwidget);
    tabWidget->setObjectName(QString::fromUtf8("tabWidget"));
    tabWidget->setGeometry(QRect(10, 5, 661, 350));
//**************************************General Tab*************************************
    generalTab = new QWidget();
    generalTab->setObjectName(QString::fromUtf8("generalTab"));
    enDisableEMF = new QCheckBox(generalTab);
    enDisableEMF->setObjectName(QString::fromUtf8("enDisableEMF"));
    enDisableEMF->setGeometry(QRect(20, 16, 200, 23));
    enDisableEMF->setChecked(true);
    g_startOnstartUp = new QCheckBox(generalTab);
    g_startOnstartUp->setObjectName(QString::fromUtf8("g_startOnstartUp"));
    g_startOnstartUp->setGeometry(QRect(50, 70, 600, 23));
    g_informInterfaceAvail = new QCheckBox(generalTab);
    g_informInterfaceAvail->setObjectName(QString::fromUtf8("g_informInterfaceAvail"));
    g_informInterfaceAvail->setGeometry(QRect(50, 120, 600, 23));
    g_informHandoverComp = new QCheckBox(generalTab);
    g_informHandoverComp->setObjectName(QString::fromUtf8("g_informHandoverComp"));
    g_informHandoverComp->setGeometry(QRect(50, 170, 600, 23));
    g_informBWAvail = new QCheckBox(generalTab);
    g_informBWAvail->setObjectName(QString::fromUtf8("g_informBWAvail"));
    g_informBWAvail->setGeometry(QRect(50, 220, 600, 23));
    g_enableLocUpdate = new QCheckBox(generalTab);
    g_enableLocUpdate->setObjectName(QString::fromUtf8("g_enableLocUpdate"));
    g_enableLocUpdate->setGeometry(QRect(50, 270, 600, 23));
//********************************advacned (Interface) Tab*********************************
    tabWidget->addTab(generalTab, QString());
    advancedTab = new QWidget();
    advancedTab->setObjectName(QString::fromUtf8("advancedTab"));
    advanceTabWidget = new QTabWidget(advancedTab);
    advanceTabWidget->setObjectName(QString::fromUtf8("advanceTabWidget"));
    advanceTabWidget->setGeometry(QRect(-10, 0, 671, 355));
    advanceTabWidget->setLocale(QLocale(QLocale::English, QLocale::Pakistan));
    interfacestab = new QWidget();
    interfacestab->setObjectName(QString::fromUtf8("interfacestab"));
    intfacHeadLabel = new QLabel(interfacestab);
    intfacHeadLabel->setObjectName(QString::fromUtf8("intfacHeadLabel"));
    intfacHeadLabel->setGeometry(QRect(20, 16, 550, 19));
    adv_interfacesFrame = new QFrame(interfacestab);
    adv_interfacesFrame->setObjectName(QString::fromUtf8("adv_interfacesFrame"));
    adv_interfacesFrame->setGeometry(QRect(20, 40, 632, 240));
    adv_interfacesFrame->setFrameShape(QFrame::WinPanel);
    adv_interfacesFrame->setFrameShadow(QFrame::Plain);
    for (int i = 0; i < 20 ; i++)
    {
        interfaces_chkBox[i] = new QCheckBox("",adv_interfacesFrame);
        interfaces_chkBox[i]->setObjectName(QString::fromUtf8("Interfaces"));
        interfaces_chkBox[i]->setGeometry(QRect(1000, 1000, 2 , 4));
    }
    advanceTabWidget->addTab(interfacestab, QString());
//******************************advacned (Authentication) Tab*****************************
    authTab = new QWidget();
    authTab->setObjectName(QString::fromUtf8("authTab"));
    adv_authenticationFrame= new QFrame(authTab);
    adv_authenticationFrame->setObjectName(QString::fromUtf8("adv_authenticationFrame"));
    adv_authenticationFrame->setGeometry(QRect(20, 40, 632, 240));
    adv_authenticationFrame->setFrameShape(QFrame::WinPanel);
    adv_authenticationFrame->setFrameShadow(QFrame::Plain);
    adv_auth_AnonRB = new QRadioButton(adv_authenticationFrame);
    adv_auth_AnonRB->setObjectName(QString::fromUtf8("adv_auth_AnonRB"));
    adv_auth_AnonRB->setGeometry(QRect(60, 10, 250, 25));
    adv_auth_AnonRB->setChecked(true);
    adv_auth_EcurveRB = new QRadioButton(adv_authenticationFrame);
    adv_auth_EcurveRB->setObjectName(QString::fromUtf8("adv_auth_EcurveRB"));
    adv_auth_EcurveRB->setGeometry(QRect(110, 35, 200, 26));
    adv_auth_DHelRB = new QRadioButton(adv_authenticationFrame);
    adv_auth_DHelRB->setObjectName(QString::fromUtf8("adv_auth_DHelRB"));
    adv_auth_DHelRB->setGeometry(QRect(110, 130, 200, 24));
    elliptic_Deffi_GrpBox = new QGroupBox(adv_authenticationFrame);
    QVBoxLayout *Elliptic_Dhel_Layout = new QVBoxLayout;
    Elliptic_Dhel_Layout->addWidget(adv_auth_EcurveRB);
    Elliptic_Dhel_Layout->addWidget(adv_auth_DHelRB);
    adv_auth_EcurveRB->setChecked(true);
    elliptic_Deffi_GrpBox->setLayout(Elliptic_Dhel_Layout);
    elliptic_Deffi_GrpBox->setGeometry(QRect(110, 35, 250, 60));
    adv_auth_useCertRB = new QRadioButton(adv_authenticationFrame);
    adv_auth_useCertRB->setObjectName(QString::fromUtf8("adv_auth_useCertRB"));
    adv_auth_useCertRB->setGeometry(QRect(60, 100, 250, 25));
    adv_auth_CertPathLEdit = new QLineEdit(adv_authenticationFrame);
    adv_auth_CertPathLEdit->setObjectName(QString::fromUtf8("adv_auth_CertPathLEdit"));
    adv_auth_CertPathLEdit->setGeometry(QRect(140, 150, 381, 26));
    adv_auth_KeyPathLEdit = new QLineEdit(adv_authenticationFrame);
    adv_auth_KeyPathLEdit->setObjectName(QString::fromUtf8("adv_auth_KeyPathLEdit"));
    adv_auth_KeyPathLEdit->setGeometry(QRect(140, 200, 381, 26));
    label_2 = new QLabel(adv_authenticationFrame);
    label_2->setObjectName(QString::fromUtf8("label_2"));
    label_2->setGeometry(QRect(140, 130, 250, 19));
    selectKey_Label = new QLabel(adv_authenticationFrame);
    selectKey_Label->setObjectName(QString::fromUtf8("selectKey_Label"));
    selectKey_Label->setGeometry(QRect(140, 180, 250, 19));    
    adv_auth_selectCertButton = new QPushButton(adv_authenticationFrame);
    adv_auth_selectCertButton->setObjectName(QString::fromUtf8("adv_auth_selectCertButton"));
    adv_auth_selectCertButton->setGeometry(QRect(530, 150, 85, 28));
    adv_auth_selectKeyButton = new QPushButton(adv_authenticationFrame);
    adv_auth_selectKeyButton->setObjectName(QString::fromUtf8("adv_auth_selectKeyButton"));
    adv_auth_selectKeyButton->setGeometry(QRect(530, 200, 85, 28));
    advanceTabWidget->addTab(authTab, QString());
//******************************advacned (Authentication) Tab*****************************
    protocolTab = new QWidget();
    protocolTab->setObjectName(QString::fromUtf8("protocolTab"));
    prtNamePrtNoLabel = new QLabel(protocolTab);
    prtNamePrtNoLabel->setObjectName(QString::fromUtf8("prtNamePrtNoLabel"));
    prtNamePrtNoLabel->setGeometry(QRect(20, 16, 600, 19));
    adv_prefProtocolFrame = new QFrame(protocolTab);
    adv_prefProtocolFrame->setObjectName(QString::fromUtf8("adv_prefProtocolFrame"));
    adv_prefProtocolFrame->setGeometry(QRect(20, 40, 632, 240));
    adv_prefProtocolFrame->setFrameShape(QFrame::WinPanel);
    adv_prefProtocolFrame->setFrameShadow(QFrame::Plain);
    QFont font;
    font.setPointSize(9);
    QStringList Istlabels;
    Istlabels << ("Protocol Name") << ("Port Number");
    availablePrt_Lable = new QLabel(adv_prefProtocolFrame);
    availablePrt_Lable->setObjectName(QString::fromUtf8("availablePrt_Lable"));
    availablePrt_Lable->setGeometry(QRect(10, 12, 250, 20));
    availablePrt_Lable->setAlignment( Qt::AlignCenter );
    availablePrt_Lable->setFont(font);
    totalApp_table = new QTableWidget(adv_prefProtocolFrame);
    totalApp_table->setObjectName(QString::fromUtf8("totalApp_table"));
    totalApp_table->setGeometry(QRect(10, 40, 256, 150));
    totalApp_table->setFont(font);
    totalApp_table->setSelectionMode(QAbstractItemView::SingleSelection);
    totalApp_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    totalApp_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    totalApp_table->setColumnCount(2);
    totalApp_table->setRowCount(20);
    totalApp_table->setHorizontalHeaderLabels(Istlabels);
    totalApp_table->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
    totalApp_table->horizontalHeader()->resizeSection(0, 150);
    QStringList scndlabels;
    scndlabels << ("Protocol Name") << ("Port Number");
    selectedPrt_Lable = new QLabel(adv_prefProtocolFrame);
    selectedPrt_Lable->setObjectName(QString::fromUtf8("selectedPrt_Lable"));
    selectedPrt_Lable->setGeometry(QRect(365, 12, 250, 20));
    selectedPrt_Lable->setAlignment( Qt::AlignCenter );
    selectedPrt_Lable->setFont(font);
    selectedApp_Table = new QTableWidget(adv_prefProtocolFrame);
    selectedApp_Table->setObjectName(QString::fromUtf8("selectedApp_Table"));
    selectedApp_Table->setGeometry(QRect(365, 40, 256, 150));
    selectedApp_Table->setFont(font);
    selectedApp_Table->setSelectionMode(QAbstractItemView::SingleSelection);
    selectedApp_Table->setSelectionBehavior(QAbstractItemView::SelectRows);
    selectedApp_Table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    selectedApp_Table->setColumnCount(2);
    selectedApp_Table->setRowCount(20);
    selectedApp_Table->setHorizontalHeaderLabels(scndlabels);
    selectedApp_Table->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
    selectedApp_Table->horizontalHeader()->resizeSection(0, 150);
    forward_Button = new QPushButton(adv_prefProtocolFrame);
    forward_Button->setObjectName(QString::fromUtf8("forward_Button"));
    forward_Button->setGeometry(QRect(302, 80, 31, 28));
    backward_Button = new QPushButton(adv_prefProtocolFrame);
    backward_Button->setObjectName(QString::fromUtf8("backward_Button"));
    backward_Button->setGeometry(QRect(302, 120, 31, 28));
    adv_pref_AddProtocolButton = new QPushButton(adv_prefProtocolFrame);
    adv_pref_AddProtocolButton->setObjectName(QString::fromUtf8("adv_pref_AddProtocolButton"));
    adv_pref_AddProtocolButton->setGeometry(QRect(10, 200, 120, 26));
    adv_pref_AddProtocolButton->setFont(font);
    adv_pref_removePrtButton = new QPushButton(adv_prefProtocolFrame);
    adv_pref_removePrtButton->setObjectName(QString::fromUtf8("adv_pref_removePrtButton"));
    adv_pref_removePrtButton->setGeometry(QRect(140, 200, 120, 26));
    adv_pref_removePrtButton->setFont(font);
    advanceTabWidget->addTab(protocolTab, QString());
//******************************advacned (Handover) Tab*****************************
    handoverTab = new QWidget();
    handoverTab->setObjectName(QString::fromUtf8("handoverTab"));
    adv_HO_enForcedHOCBox = new QCheckBox(handoverTab);
    adv_HO_enForcedHOCBox->setObjectName(QString::fromUtf8("adv_HO_enForcedHOCBox"));
    adv_HO_enForcedHOCBox->setGeometry(QRect(20, 16, 600, 20));
    adv_HO_Frame = new QFrame(handoverTab);
    adv_HO_Frame->setObjectName(QString::fromUtf8("adv_HO_Frame"));
    adv_HO_Frame->setGeometry(QRect(20, 40, 632, 240));
    adv_HO_Frame->setFrameShape(QFrame::WinPanel);
    adv_HO_Frame->setFrameShadow(QFrame::Plain);
    label_8 = new QLabel(adv_HO_Frame);
    label_8->setObjectName(QString::fromUtf8("label_8"));
    label_8->setGeometry(QRect(10, 95, 200, 19));
    adv_HO_selectNetComBox = new QComboBox(adv_HO_Frame);
    adv_HO_selectNetComBox->setObjectName(QString::fromUtf8("adv_HO_selectNetComBox"));
    adv_HO_selectNetComBox->setGeometry(QRect(40, 54, 120, 25));
    integerSpinBox = new QSpinBox(adv_HO_Frame);
    integerSpinBox->setRange(0, 20);
    integerSpinBox->setSingleStep(1);
    integerSpinBox->setValue(0);
    integerSpinBox->setGeometry(QRect(350, 54, 120, 25));
    label_5 = new QLabel(adv_HO_Frame);
    label_5->setObjectName(QString::fromUtf8("label_5"));
    label_5->setGeometry(QRect(40, 35, 111, 19));
    label_5->setFont(font);
    label_4 = new QLabel(adv_HO_Frame);
    label_4->setObjectName(QString::fromUtf8("label_4"));
    label_4->setGeometry(QRect(10, 10, 400, 19));
    label_4->setFont(font);
    labelFrame_2 = new QFrame(adv_HO_Frame);
    labelFrame_2->setObjectName(QString::fromUtf8("labelFrame_2"));
    labelFrame_2->setGeometry(QRect(10, 150, 610, 80));
    labelFrame_2->setFrameShape(QFrame::Panel);
    labelFrame_2->setFrameShadow(QFrame::Plain);
    cost_Label = new QLabel(labelFrame_2);
    cost_Label->setObjectName(QString::fromUtf8("cost_Label"));
    cost_Label->setGeometry(QRect(30, 10, 85, 19));
    dataRate_Label = new QLabel(labelFrame_2);
    dataRate_Label->setObjectName(QString::fromUtf8("dataRate_Label"));
    dataRate_Label->setGeometry(QRect(30, 50, 85, 19));
    costLineEdit = new QLineEdit(labelFrame_2);
    costLineEdit->setObjectName(QString::fromUtf8("costLineEdit"));
    costLineEdit->setGeometry(QRect(220, 10, 113, 25));
    dataRateLineEdit = new QLineEdit(labelFrame_2);
    dataRateLineEdit->setObjectName(QString::fromUtf8("dataRateLineEdit"));
    dataRateLineEdit->setGeometry(QRect(220, 42, 113, 25));
    adv_HO_costUnitComBox = new QComboBox(labelFrame_2);
    adv_HO_costUnitComBox->setObjectName(QString::fromUtf8("adv_HO_costUnitComBox"));
    adv_HO_costUnitComBox->setGeometry(QRect(420, 10, 113, 25));
    adv_HO_costUnitComBox->addItem("Select Unit");
    adv_HO_costUnitComBox->addItem("/MB");
    adv_HO_costUnitComBox->addItem("/month");
    adv_HO_costUnitComBox->setFont(font);
    adv_HO_datarateUnitComBox = new QComboBox(labelFrame_2);
    adv_HO_datarateUnitComBox->setObjectName(QString::fromUtf8("adv_HO_datarateUnitComBox"));
    adv_HO_datarateUnitComBox->setGeometry(QRect(420, 42, 113, 25));
    adv_HO_datarateUnitComBox->addItem("Select Unit");
    adv_HO_datarateUnitComBox->addItem("kbps");
    adv_HO_datarateUnitComBox->addItem("Mbps");
    adv_HO_datarateUnitComBox->setFont(font);
    labelFrame = new QFrame(adv_HO_Frame);
    labelFrame->setObjectName(QString::fromUtf8("labelFrame"));
    labelFrame->setGeometry(QRect(10, 120, 610, 28));
    labelFrame->setFrameShape(QFrame::Panel);
    labelFrame->setFrameShadow(QFrame::Plain);
    parameter_Label = new QLabel(labelFrame);
    parameter_Label->setObjectName(QString::fromUtf8("parameter_Label"));
    parameter_Label->setGeometry(QRect(30, 7, 80, 19));
    value_Label = new QLabel(labelFrame);
    value_Label->setObjectName(QString::fromUtf8("value_Label"));
    value_Label->setGeometry(QRect(250, 7, 68, 19));
    unit_Label = new QLabel(labelFrame);
    unit_Label->setObjectName(QString::fromUtf8("unit_Label"));
    unit_Label->setGeometry(QRect(450, 7, 68, 19));
    handover_HeadingLabel = new QLabel(adv_HO_Frame);
    handover_HeadingLabel->setObjectName(QString::fromUtf8("handover_HeadingLabel"));
    handover_HeadingLabel->setGeometry(QRect(10, 20, 400, 19));
    selectNet_Label = new QLabel(adv_HO_Frame);
    selectNet_Label->setObjectName(QString::fromUtf8("selectNet_Label"));
    selectNet_Label->setGeometry(QRect(30, 60, 116, 19));
    preference_Label = new QLabel(adv_HO_Frame);
    preference_Label->setObjectName(QString::fromUtf8("preference_Label"));
    preference_Label->setGeometry(QRect(350, 35, 111, 19));
    preference_Label->setFont(font);
    advanceTabWidget->addTab(handoverTab, QString());
//******************************advacned (Bandwidth Aggrg) Tab*****************************
    bwAggTab = new QWidget();
    bwAggTab->setObjectName(QString::fromUtf8("bwAggTab"));
    label_9 = new QLabel(bwAggTab);
    label_9->setObjectName(QString::fromUtf8("label_9"));
    label_9->setGeometry(QRect(20, 16, 450, 19));
    adv_prefBWFrame = new QFrame(bwAggTab);
    adv_prefBWFrame->setObjectName(QString::fromUtf8("adv_prefBWFrame"));
    adv_prefBWFrame->setGeometry(QRect(20, 40, 632, 240));
    adv_prefBWFrame->setFrameShape(QFrame::WinPanel);
    adv_prefBWFrame->setFrameShadow(QFrame::Plain);
    adv_BW_selectPrtComBox = new QComboBox(bwAggTab);
    adv_BW_selectPrtComBox->setObjectName(QString::fromUtf8("adv_BW_selectPrtComBox"));
    adv_BW_selectPrtComBox->setGeometry(QRect(460, 10, 121, 25));
    label_10 = new QLabel(adv_prefBWFrame);
    label_10->setObjectName(QString::fromUtf8("label_10"));
    label_10->setGeometry(QRect(10, 10, 250, 40));
    label_10->setAlignment( Qt::AlignCenter );
    font.setPointSize(9);
    label_10->setFont(font);
    selectTwoNetworks = new QLabel(adv_prefBWFrame);
    selectTwoNetworks->setObjectName(QString::fromUtf8("selectTwoNetworks"));
    selectTwoNetworks->setGeometry(QRect(360, 10, 260, 40));
    selectTwoNetworks->setAlignment( Qt::AlignCenter );
    selectTwoNetworks->setFont(font);
    QStringList Istlabels1;
    Istlabels1 << ("Available Interfaces");
    BWtotalApp_table = new QTableWidget(adv_prefBWFrame);
    BWtotalApp_table->setObjectName(QString::fromUtf8("BWtotalApp_table"));
    BWtotalApp_table->setGeometry(QRect(10, 63, 256, 172));
    BWtotalApp_table->setFont(font);
    BWtotalApp_table->setSelectionMode(QAbstractItemView::SingleSelection);
    BWtotalApp_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    BWtotalApp_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    BWtotalApp_table->setColumnCount(1);
    BWtotalApp_table->setRowCount(20);
    BWtotalApp_table->setHorizontalHeaderLabels(Istlabels1);
    BWtotalApp_table->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
    QStringList scndlabels1;
    scndlabels1 << ("Selected Interfaces");
    BWselectedApp_Table = new QTableWidget(adv_prefBWFrame);
    BWselectedApp_Table->setObjectName(QString::fromUtf8("BWselectedApp_Table"));
    BWselectedApp_Table->setGeometry(QRect(360, 63, 256, 172));
    BWselectedApp_Table->setFont(font);
    BWselectedApp_Table->setSelectionMode(QAbstractItemView::SingleSelection);
    BWselectedApp_Table->setSelectionBehavior(QAbstractItemView::SelectRows);
    BWselectedApp_Table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    BWselectedApp_Table->setColumnCount(1);
    BWselectedApp_Table->setRowCount(20);
    BWselectedApp_Table->setHorizontalHeaderLabels(scndlabels1);
    BWselectedApp_Table->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
    BWforward_Button = new QPushButton(adv_prefBWFrame);
    BWforward_Button->setObjectName(QString::fromUtf8("BWforward_Button"));
    BWforward_Button->setGeometry(QRect(300, 120, 31, 28));
    BWbackward_Button = new QPushButton(adv_prefBWFrame);
    BWbackward_Button->setObjectName(QString::fromUtf8("BWbackward_Button"));
    BWbackward_Button->setGeometry(QRect(300, 160, 31, 28));
    advanceTabWidget->addTab(bwAggTab, QString());
//******************************advacned (RunTime Config) Tab*****************************
    manualConfTab = new QWidget();
    manualConfTab->setObjectName(QString::fromUtf8("manualConfTab"));
    label_11 = new QLabel(manualConfTab);
    label_11->setObjectName(QString::fromUtf8("label_11"));
    label_11->setGeometry(QRect(20, 16, 550,19));
    adv_Mon_Frame = new QFrame(manualConfTab);
    adv_Mon_Frame->setObjectName(QString::fromUtf8("adv_Mon_Frame"));
    adv_Mon_Frame->setGeometry(QRect(20, 40, 632, 240));
    adv_Mon_Frame->setFrameShape(QFrame::WinPanel);
    adv_Mon_Frame->setFrameShadow(QFrame::Plain);
    manual_BAFrame= new QFrame(adv_Mon_Frame);
    manual_BAFrame->setObjectName(QString::fromUtf8("manual_BAFrame"));
    manual_BAFrame->setGeometry(QRect(10, 10, 300, 222));
    manual_BAFrame->setFrameShape(QFrame::WinPanel);
    manual_BAFrame->setFrameShadow(QFrame::Plain);
    perforLabel = new QLabel(manual_BAFrame);
    perforLabel->setObjectName(QString::fromUtf8("perforLabel"));
    perforLabel->setGeometry(QRect(50, 5, 270, 19));
    appFileSlabel = new QLabel(manual_BAFrame);
    appFileSlabel->setObjectName(QString::fromUtf8("appFileSlabel"));
    appFileSlabel->setGeometry(QRect(10, 55, 140, 19));
    adv_Mon_selectAppComBox = new QComboBox(manual_BAFrame);
    adv_Mon_selectAppComBox->setObjectName(QString::fromUtf8("adv_Mon_selectAppComBox"));
    adv_Mon_selectAppComBox->setGeometry(QRect(145, 55, 130, 28));
    sltLinkLabel = new QLabel(manual_BAFrame);
    sltLinkLabel->setObjectName(QString::fromUtf8("sltLinkLabel"));
    sltLinkLabel->setGeometry(QRect(10, 115, 140, 19));
    adv_Mon_selectLinkComBox = new QComboBox(manual_BAFrame);
    adv_Mon_selectLinkComBox->setObjectName(QString::fromUtf8("adv_Mon_selectLinkComBox"));
    adv_Mon_selectLinkComBox->setGeometry(QRect(145, 115, 130, 28));
    adv_Mon_MBWButton = new QPushButton(manual_BAFrame);
    adv_Mon_MBWButton->setObjectName(QString::fromUtf8("adv_Mon_MBWButton"));
    adv_Mon_MBWButton->setGeometry(QRect(95, 185, 120, 30));
    manual_HOFrame= new QFrame(adv_Mon_Frame);
    manual_HOFrame->setObjectName(QString::fromUtf8("manual_HOFrame"));
    manual_HOFrame->setGeometry(QRect(310, 10, 312, 222));
    manual_HOFrame->setFrameShape(QFrame::WinPanel);
    manual_HOFrame->setFrameShadow(QFrame::Plain);
    manualBALabel= new QLabel(manual_HOFrame);
    manualBALabel->setObjectName(QString::fromUtf8("manualBALabel"));
    manualBALabel->setGeometry(QRect(100, 5, 220, 19));
    ManualHO_tabWidget= new QTabWidget(manual_HOFrame);
    manualHO_LinkTab= new QWidget();
    manualHO_TrafficTab= new QWidget();
    ManualHO_tabWidget->setGeometry(QRect(13, 28, 285, 155));
    ManualHO_tabWidget->addTab(manualHO_LinkTab, QString());
    ManualHO_tabWidget->setTabText(0,"Link");
    ManualHO_tabWidget->addTab(manualHO_TrafficTab, QString());
    ManualHO_tabWidget->setTabText(1,"Traffic");
    fromLinkLabel  = new QLabel("Current Links:", manualHO_LinkTab);
    fromLinkLabel->setGeometry(QRect(10, 20, 145, 20));
    toLinkLabel  = new QLabel("New Links:", manualHO_LinkTab);
    toLinkLabel->setGeometry(QRect(10, 70, 145, 20));
    fromLinkComboBOx = new QComboBox(manualHO_LinkTab);
    fromLinkComboBOx->setGeometry(QRect(135, 20, 120, 28));
    toLinkComboBOx = new QComboBox(manualHO_LinkTab);
    toLinkComboBOx->setGeometry(QRect(135, 65, 120, 28));
    fromLink_TrafficLabel= new QLabel("Current Links:", manualHO_TrafficTab);
    fromLink_TrafficLabel->setGeometry(QRect(15, 10, 145, 20));
    toLink_TrafficLabel= new QLabel("Traffic:", manualHO_TrafficTab);
    toLink_TrafficLabel->setGeometry(QRect(15, 45, 145, 20));
    toTraffic_TrafficLabel= new QLabel("New Links:", manualHO_TrafficTab);
    toTraffic_TrafficLabel->setGeometry(QRect(15, 85, 145, 20));
    fromLink_TrafficComboBOx = new QComboBox(manualHO_TrafficTab);
    fromLink_TrafficComboBOx->setGeometry(QRect(135, 5, 120, 28));
    toTraffic_TrafficComboBOx= new QComboBox(manualHO_TrafficTab);
    toTraffic_TrafficComboBOx->setGeometry(QRect(135, 45, 120, 28));
    toLink_TrafficComboBOx= new QComboBox(manualHO_TrafficTab);
    toLink_TrafficComboBOx->setGeometry(QRect(135, 85, 120, 28));
    adv_Mon_MHOButton = new QPushButton(manual_HOFrame);
    adv_Mon_MHOButton->setObjectName(QString::fromUtf8("adv_Mon_MHOButton"));
    adv_Mon_MHOButton->setGeometry(QRect(95, 185, 120, 30));
    advanceTabWidget->addTab(manualConfTab, QString());
    tabWidget->addTab(advancedTab, QString());
    helpTab = new QWidget();
    helpTab->setObjectName(QString::fromUtf8("helpTab"));
    label_15 = new QLabel(helpTab);
    label_15->setObjectName(QString::fromUtf8("label_15"));
    label_15->setGeometry(QRect(80, 150, 68, 19));
    label_16 = new QLabel(helpTab);
    label_16->setObjectName(QString::fromUtf8("label_16"));
    label_16->setGeometry(QRect(50, 190, 591, 20));
    label_17 = new QLabel(helpTab);
    label_17->setObjectName(QString::fromUtf8("label_17"));
    label_17->setGeometry(QRect(260, 224, 151, 20));
    label_18 = new QLabel(helpTab);
    label_18->setObjectName(QString::fromUtf8("label_18"));
    label_18->setGeometry(QRect(250, 260, 181, 20));
    help_ConnectUpdateButton = new QPushButton(helpTab);
    help_ConnectUpdateButton->setObjectName(QString::fromUtf8("help_ConnectUpdateButton"));
    help_ConnectUpdateButton->setGeometry(QRect(350, 90, 85, 28));
    help_OpenManlButton = new QPushButton(helpTab);
    help_OpenManlButton->setObjectName(QString::fromUtf8("help_OpenManlButton"));
    help_OpenManlButton->setGeometry(QRect(350, 50, 85, 28));
    layoutWidget = new QWidget(helpTab);
    layoutWidget->setObjectName(QString::fromUtf8("layoutWidget"));
    layoutWidget->setGeometry(QRect(82, 52, 261, 22));
    gridLayout = new QGridLayout(layoutWidget);
    gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
    gridLayout->setContentsMargins(0, 0, 0, 0);
    label_6 = new QLabel(layoutWidget);
    label_6->setObjectName(QString::fromUtf8("label_6"));
    gridLayout->addWidget(label_6, 0, 0, 1, 1);
    spacerItem = new QSpacerItem(158, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    gridLayout->addItem(spacerItem, 0, 1, 1, 1);
    layoutWidget1 = new QWidget(helpTab);
    layoutWidget1->setObjectName(QString::fromUtf8("layoutWidget1"));
    layoutWidget1->setGeometry(QRect(81, 91, 255, 27));
    gridLayout1 = new QGridLayout(layoutWidget1);
    gridLayout1->setObjectName(QString::fromUtf8("gridLayout1"));
    gridLayout1->setContentsMargins(0, 0, 0, 0);
    label_7 = new QLabel(layoutWidget1);
    label_7->setObjectName(QString::fromUtf8("label_7"));
    gridLayout1->addWidget(label_7, 0, 0, 1, 1);
    spacerItem1 = new QSpacerItem(165, 25, QSizePolicy::Expanding, QSizePolicy::Minimum);
    gridLayout1->addItem(spacerItem1, 0, 1, 1, 1);
    tabWidget->addTab(helpTab, QString());
    layoutWidget2 = new QWidget(centralwidget);
    layoutWidget2->setObjectName(QString::fromUtf8("layoutWidget2"));
    layoutWidget2->setGeometry(QRect(0, 360, 684, 32));
    hboxLayout = new QHBoxLayout(layoutWidget2);
    hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
    hboxLayout->setContentsMargins(0, 0, 0, 0);
    spacerItem2 = new QSpacerItem(405, 25, QSizePolicy::Expanding, QSizePolicy::Minimum);
    hboxLayout->addItem(spacerItem2);
    cancelButton = new QPushButton(layoutWidget2);
    cancelButton->setObjectName(QString::fromUtf8("cancelButton"));
    hboxLayout->addWidget(cancelButton);
    spacerItem3 = new QSpacerItem(11, 25, QSizePolicy::Expanding, QSizePolicy::Minimum);
    hboxLayout->addItem(spacerItem3);
    okPushButton = new QPushButton(layoutWidget2);
    okPushButton->setObjectName(QString::fromUtf8("okPushButton"));
    hboxLayout->addWidget(okPushButton);
    spacerItem4 = new QSpacerItem(15, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    hboxLayout->addItem(spacerItem4);
    gridLayout2 = new QGridLayout();
    gridLayout2->setObjectName(QString::fromUtf8("gridLayout2"));
    applyButton = new QPushButton(layoutWidget2);
    applyButton->setObjectName(QString::fromUtf8("applyButton"));
    gridLayout2->addWidget(applyButton, 0, 0, 1, 1);
    spacerItem5 = new QSpacerItem(12, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    gridLayout2->addItem(spacerItem5, 0, 1, 1, 1);
    hboxLayout->addLayout(gridLayout2);
    MainWindow->setCentralWidget(centralwidget);
    retranslateUi(MainWindow);
    tabWidget->setCurrentIndex(0);
    advanceTabWidget->setCurrentIndex(0);
    QObject::connect(cancelButton, SIGNAL(clicked()), MainWindow, SLOT(cancelClose()));
    QObject::connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),MainWindow, SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));
    trayIcon->show();
    QObject::connect(enDisableEMF, SIGNAL(clicked()), MainWindow, SLOT(setting_InterfaceOptions()));
    QObject::connect(g_startOnstartUp, SIGNAL(clicked()), MainWindow, SLOT(setunset_GeneralTabOptions()));
    QObject::connect(g_informInterfaceAvail, SIGNAL(clicked()), MainWindow, SLOT(setunset_GeneralTabOptions()));
    QObject::connect(g_informHandoverComp, SIGNAL(clicked()), MainWindow, SLOT(setunset_GeneralTabOptions()));
    QObject::connect(g_informBWAvail, SIGNAL(clicked()), MainWindow, SLOT(setunset_GeneralTabOptions()));
    QObject::connect(g_enableLocUpdate, SIGNAL(clicked()), MainWindow, SLOT(setunset_GeneralTabOptions()));
    QObject::connect(interfaces_chkBox[0], SIGNAL(clicked()), MainWindow, SLOT(addInterfacesComboBox()));
    QObject::connect(interfaces_chkBox[1], SIGNAL(clicked()), MainWindow, SLOT(addInterfacesComboBox()));
    QObject::connect(interfaces_chkBox[2], SIGNAL(clicked()), MainWindow, SLOT(addInterfacesComboBox()));
    QObject::connect(interfaces_chkBox[3], SIGNAL(clicked()), MainWindow, SLOT(addInterfacesComboBox()));
    QObject::connect(interfaces_chkBox[4], SIGNAL(clicked()), MainWindow, SLOT(addInterfacesComboBox()));
    QObject::connect(interfaces_chkBox[5], SIGNAL(clicked()), MainWindow, SLOT(addInterfacesComboBox()));
    QObject::connect(interfaces_chkBox[6], SIGNAL(clicked()), MainWindow, SLOT(addInterfacesComboBox()));
    QObject::connect(interfaces_chkBox[7], SIGNAL(clicked()), MainWindow, SLOT(addInterfacesComboBox()));
    QObject::connect(interfaces_chkBox[8], SIGNAL(clicked()), MainWindow, SLOT(addInterfacesComboBox()));
    QObject::connect(interfaces_chkBox[9], SIGNAL(clicked()), MainWindow, SLOT(addInterfacesComboBox()));
    QObject::connect(interfaces_chkBox[10], SIGNAL(clicked()), MainWindow, SLOT(addInterfacesComboBox()));
    QObject::connect(interfaces_chkBox[11], SIGNAL(clicked()), MainWindow, SLOT(addInterfacesComboBox()));
    QObject::connect(interfaces_chkBox[12], SIGNAL(clicked()), MainWindow, SLOT(addInterfacesComboBox()));
    QObject::connect(interfaces_chkBox[13], SIGNAL(clicked()), MainWindow, SLOT(addInterfacesComboBox()));
    QObject::connect(interfaces_chkBox[14], SIGNAL(clicked()), MainWindow, SLOT(addInterfacesComboBox()));
    QObject::connect(interfaces_chkBox[15], SIGNAL(clicked()), MainWindow, SLOT(addInterfacesComboBox()));
    QObject::connect(interfaces_chkBox[16], SIGNAL(clicked()), MainWindow, SLOT(addInterfacesComboBox()));
    QObject::connect(interfaces_chkBox[17], SIGNAL(clicked()), MainWindow, SLOT(addInterfacesComboBox()));
    QObject::connect(interfaces_chkBox[18], SIGNAL(clicked()), MainWindow, SLOT(addInterfacesComboBox()));
    QObject::connect(interfaces_chkBox[19], SIGNAL(clicked()), MainWindow, SLOT(addInterfacesComboBox()));
    QObject::connect( costLineEdit, SIGNAL(textChanged (const QString&)),MainWindow, SLOT(cost_textChanged()) );
    QObject::connect( dataRateLineEdit, SIGNAL(textChanged (const QString&)),MainWindow, SLOT( dataRate_textChanged() ) );
    QObject::connect( integerSpinBox, SIGNAL(valueChanged (int)),MainWindow, SLOT(prefLevel_textChanged()));
    QObject::connect(forward_Button, SIGNAL(clicked()),MainWindow, SLOT(forwardTotal_SelectedTable()));
    QObject::connect(backward_Button, SIGNAL(clicked()),MainWindow, SLOT(backwardSelected_Table()));
    QObject::connect(BWforward_Button, SIGNAL(clicked()),MainWindow, SLOT(forwardBW_SelectedTable()));
    QObject::connect(BWbackward_Button, SIGNAL(clicked()),MainWindow, SLOT(backwardBW_SelectedTable()));
    QObject::connect(adv_HO_selectNetComBox, SIGNAL(currentIndexChanged(int)),MainWindow, SLOT(getSetHandoverPref()));
    QObject::connect(adv_BW_selectPrtComBox, SIGNAL(currentIndexChanged(int)),MainWindow, SLOT(retriveBWEntrTable()));
    QObject::connect(adv_HO_costUnitComBox, SIGNAL(currentIndexChanged(int)),MainWindow, SLOT(setCostUnitComboBox()));
    QObject::connect(adv_HO_datarateUnitComBox, SIGNAL(currentIndexChanged(int)),MainWindow, SLOT(setRateUnitComboBox()));
    QObject::connect(adv_Mon_MHOButton, SIGNAL(clicked()),MainWindow, SLOT(sentmanualHandoverCmd()));
    QObject::connect(adv_Mon_MBWButton, SIGNAL(clicked()),MainWindow, SLOT(sentmanualBandAggrgCmd()));
    QObject::connect(adv_auth_AnonRB, SIGNAL(clicked()), MainWindow, SLOT(anonymousRButton()));
    QObject::connect(adv_auth_EcurveRB, SIGNAL(clicked()), MainWindow, SLOT(ellipticRButton()));
    QObject::connect(adv_auth_DHelRB, SIGNAL(clicked()), MainWindow, SLOT(deffieHellmenRButton()));
    QObject::connect(adv_auth_useCertRB, SIGNAL(clicked()), MainWindow, SLOT(certificateRButton()));
    QObject::connect(adv_auth_selectCertButton, SIGNAL(clicked()), MainWindow, SLOT(selectFile()));
    QObject::connect(adv_auth_selectKeyButton, SIGNAL(clicked()), MainWindow, SLOT(selectKey()));
    QObject::connect(okPushButton, SIGNAL(clicked()), MainWindow, SLOT(saveANDclose()));
    QObject::connect(applyButton, SIGNAL(clicked()), MainWindow, SLOT(applyANDsave()));
    QObject::connect(adv_pref_AddProtocolButton, SIGNAL(clicked()), MainWindow, SLOT(addProtocol()));
    QObject::connect(adv_pref_removePrtButton, SIGNAL(clicked()), MainWindow, SLOT(removeProtocol()));
    QObject::connect(help_OpenManlButton, SIGNAL(clicked()), MainWindow, SLOT(openManual()));
    QObject::connect(help_ConnectUpdateButton, SIGNAL(clicked()), MainWindow, SLOT(updateEMF()));
    QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
    MainWindow->setWindowTitle(QApplication::translate("MainWindow", "EMF-Beta", 0, QApplication::UnicodeUTF8));
    enDisableEMF->setText(QApplication::translate("MainWindow", "Enable EMF", 0, QApplication::UnicodeUTF8));
    g_startOnstartUp->setText(QApplication::translate("MainWindow", "Always start on system startup", 0, QApplication::UnicodeUTF8));
    g_informInterfaceAvail->setText(QApplication::translate("MainWindow", "Inform when new Interface becomes available", 0, QApplication::UnicodeUTF8));
    g_informHandoverComp->setText(QApplication::translate("MainWindow", "Inform handover completion process", 0, QApplication::UnicodeUTF8));
    g_informBWAvail->setText(QApplication::translate("MainWindow", "Inform when new Interface for Bandwidth Aggregation is available", 0, QApplication::UnicodeUTF8));
    g_enableLocUpdate->setText(QApplication::translate("MainWindow", "Enable Location Updates", 0, QApplication::UnicodeUTF8));
    tabWidget->setTabText(tabWidget->indexOf(generalTab), QApplication::translate("MainWindow", "General", 0, QApplication::UnicodeUTF8));
    prtNamePrtNoLabel->setText(QApplication::translate("MainWindow", "Select Application Protocol against Port Number for use in EMF", 0, QApplication::UnicodeUTF8));
        
    intfacHeadLabel->setText(QApplication::translate("MainWindow", "Select from the available interfaces for use in EMF", 0, QApplication::UnicodeUTF8));
    cost_Label->setText(QApplication::translate("MainWindow", "Cost", 0, QApplication::UnicodeUTF8));
    dataRate_Label->setText(QApplication::translate("MainWindow", "Data Rate", 0, QApplication::UnicodeUTF8));
    parameter_Label->setText(QApplication::translate("MainWindow", "Parameter", 0, QApplication::UnicodeUTF8));
    value_Label->setText(QApplication::translate("MainWindow", "Value", 0, QApplication::UnicodeUTF8));
    unit_Label->setText(QApplication::translate("MainWindow", "Unit", 0, QApplication::UnicodeUTF8));
    preference_Label->setText(QApplication::translate("MainWindow", "Preference", 0, QApplication::UnicodeUTF8));
    advanceTabWidget->setTabText(advanceTabWidget->indexOf(interfacestab), QApplication::translate("MainWindow", "Interfaces", 0, QApplication::UnicodeUTF8));
    adv_auth_AnonRB->setText(QApplication::translate("MainWindow", "Anonymous", 0, QApplication::UnicodeUTF8));
    adv_auth_EcurveRB->setText(QApplication::translate("MainWindow", "Elliptic Curve", 0, QApplication::UnicodeUTF8));
    adv_auth_DHelRB->setText(QApplication::translate("MainWindow", "Deffi-Helman", 0, QApplication::UnicodeUTF8));
    adv_auth_useCertRB->setText(QApplication::translate("MainWindow", "Use Certificate", 0, QApplication::UnicodeUTF8));
    label_2->setText(QApplication::translate("MainWindow", "Select Certificate", 0, QApplication::UnicodeUTF8));
    selectKey_Label->setText(QApplication::translate("MainWindow", "Select Private Key", 0, QApplication::UnicodeUTF8));
    adv_auth_selectCertButton->setText(QApplication::translate("MainWindow", "Browse...", 0, QApplication::UnicodeUTF8));
    adv_auth_selectKeyButton->setText(QApplication::translate("MainWindow", "Browse...", 0, QApplication::UnicodeUTF8));
    advanceTabWidget->setTabText(advanceTabWidget->indexOf(authTab), QApplication::translate("MainWindow", "Authentication", 0, QApplication::UnicodeUTF8));
    forward_Button->setText(QApplication::translate("MainWindow", ">", 0, QApplication::UnicodeUTF8));
    backward_Button->setText(QApplication::translate("MainWindow", "<", 0, QApplication::UnicodeUTF8));
    BWforward_Button->setText(QApplication::translate("MainWindow", ">", 0, QApplication::UnicodeUTF8));
    BWbackward_Button->setText(QApplication::translate("MainWindow", "<", 0, QApplication::UnicodeUTF8));
    adv_pref_AddProtocolButton->setText(QApplication::translate("MainWindow", "Add", 0, QApplication::UnicodeUTF8));
    adv_pref_removePrtButton->setText(QApplication::translate("MainWindow", "Remove", 0, QApplication::UnicodeUTF8));
    advanceTabWidget->setTabText(advanceTabWidget->indexOf(protocolTab), QApplication::translate("MainWindow", "Protocol", 0, QApplication::UnicodeUTF8));
    adv_HO_enForcedHOCBox->setText(QApplication::translate("MainWindow", "Enable forced Handover to available network for service continuity", 0, QApplication::UnicodeUTF8));
    label_8->setText(QApplication::translate("MainWindow", "Specify values", 0, QApplication::UnicodeUTF8));
    label_5->setText(QApplication::translate("MainWindow", "Select Network", 0, QApplication::UnicodeUTF8));
    label_4->setText(QApplication::translate("MainWindow", "Specify Threshold values for Handover parameters", 0, QApplication::UnicodeUTF8));
    advanceTabWidget->setTabText(advanceTabWidget->indexOf(handoverTab), QApplication::translate("MainWindow", " Handover ", 0, QApplication::UnicodeUTF8));
    label_9->setText(QApplication::translate("MainWindow", "Select protocols that will use Bandwidth Aggregation", 0, QApplication::UnicodeUTF8));
    label_10->setText(QApplication::translate("MainWindow", "Available networks for <br> BandWidth Aggregation", 0, QApplication::UnicodeUTF8));
    selectTwoNetworks->setText(QApplication::translate("MainWindow", "Select <b>Minimum Two </b> Interfaces for <br>BandWidth Aggregation</br>", 0, QApplication::UnicodeUTF8));
availablePrt_Lable->setText(QApplication::translate("MainWindow", "Network Protocols", 0, QApplication::UnicodeUTF8));
selectedPrt_Lable->setText(QApplication::translate("MainWindow", "Protocols under<b> EMF </b>", 0, QApplication::UnicodeUTF8));
    advanceTabWidget->setTabText(advanceTabWidget->indexOf(bwAggTab), QApplication::translate("MainWindow", "B/W  Aggregation", 0, QApplication::UnicodeUTF8));
    label_11->setText(QApplication::translate("MainWindow", "Check Availablity of Mannual Willful Handover", 0, QApplication::UnicodeUTF8));
    adv_Mon_MHOButton->setText(QApplication::translate("MainWindow", "Handover", 0, QApplication::UnicodeUTF8));
    adv_Mon_MBWButton->setText(QApplication::translate("MainWindow", "Aggregate", 0, QApplication::UnicodeUTF8));
    appFileSlabel->setText(QApplication::translate("MainWindow", "Select Traffic:", 0, QApplication::UnicodeUTF8));
    perforLabel->setText(QApplication::translate("MainWindow", "Manual Bandwidth Aggregation", 0, QApplication::UnicodeUTF8));
    manualBALabel->setText(QApplication::translate("MainWindow", "Manual Handover", 0, QApplication::UnicodeUTF8));
    sltLinkLabel->setText(QApplication::translate("MainWindow", "Select Link:", 0, QApplication::UnicodeUTF8));
    advanceTabWidget->setTabText(advanceTabWidget->indexOf(manualConfTab), QApplication::translate("MainWindow", "RunTime Conf", 0, QApplication::UnicodeUTF8));
    tabWidget->setTabText(tabWidget->indexOf(advancedTab), QApplication::translate("MainWindow", "Advanced", 0, QApplication::UnicodeUTF8));
    label_15->setText(QApplication::translate("MainWindow", "About Us", 0, QApplication::UnicodeUTF8));
    label_16->setText(QApplication::translate("MainWindow", "End to End Mobility Management Framework (EMF) for Multihomed Mobile Devices", 0, QApplication::UnicodeUTF8));
    label_17->setText(QApplication::translate("MainWindow", "www.emfproject.com", 0, QApplication::UnicodeUTF8));
    label_18->setText(QApplication::translate("MainWindow", "support@emfproject.com", 0, QApplication::UnicodeUTF8));
    help_ConnectUpdateButton->setText(QApplication::translate("MainWindow", "Connect", 0, QApplication::UnicodeUTF8));
    help_OpenManlButton->setText(QApplication::translate("MainWindow", "Open", 0, QApplication::UnicodeUTF8));
    label_6->setText(QApplication::translate("MainWindow", "User Mannual", 0, QApplication::UnicodeUTF8));
    label_7->setText(QApplication::translate("MainWindow", "Update EMF", 0, QApplication::UnicodeUTF8));
    tabWidget->setTabText(tabWidget->indexOf(helpTab), QApplication::translate("MainWindow", "Help", 0, QApplication::UnicodeUTF8));
    okPushButton->setText(QApplication::translate("MainWindow", "&OK", 0, QApplication::UnicodeUTF8));
    applyButton->setText(QApplication::translate("MainWindow", "&Apply", 0, QApplication::UnicodeUTF8));
    cancelButton->setText(QApplication::translate("MainWindow", "&Cancel", 0, QApplication::UnicodeUTF8));
    Q_UNUSED(MainWindow);
    } // retranslateUi
};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

#endif // UI_MAINWINDOW_H
