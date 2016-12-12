#! /bin/bash
# EMF Application
 chmod +x end2end.sh
 cd  /home/ambreen/Desktop/100114/emf_client/emf_hostagent/;   ./emfHost   &
 cd  /home/ambreen/Desktop/100114/emf_client/emf_Interface/;   ./emf_Interface   &
 sleep "20" 
 cd  /home/ambreen/Desktop/100114/emf_client/emf_cl2i/;   ./cl2i   &
 sleep "1" 
 cd  /home/ambreen/Desktop/100114/emf_client/emf_core/;   ./emf_client