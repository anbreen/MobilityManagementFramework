#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main (int argc, char *argv[])
{ 
	FILE *filepointer;
	filepointer = fopen("../emf_client/emf_hostagent/emfHost", "r");
	if(filepointer == NULL)
		printf("File Not Available\n");
	else
	{
	    system("clear");
		printf("\nExecuting EMF Client...............\n\n");
		system("cd emf_hostagent/;    ./emfHost  &");
		system("cd ..");

		sleep(1);

		system("cd emf_Interface/;    ./emf_Interface  &");
		system("cd ..");

		sleep(2);

		system("cd emf_cl2i/;    ./cl2i  &");
		system("cd ..");
		
		sleep(1);

		system("cd emf_core/;    ./emf_client");
		fclose(filepointer);
	}
}
