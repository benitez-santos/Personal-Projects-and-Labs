#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h> 

struct Node{ //Linked list used to take care of registering, activity and correctness
    int active;
    char username[40];
    char password[40];
    int sd;
    struct Node *next;
};
void readFile(struct Node **head);
void printActiveUsers(struct Node **head, int sd);
char *checkIfRegistered(struct Node **head,char* buffer,int sd);
int checkIfLogin(struct Node **head, char username[], char password[]);
char * logOut(struct Node **head, char *buffer);
char * checkIfClientLoginIn(struct Node **head, char *buffer, int sd);
int returnSD(struct Node **head, char *buffer);
char * messageReturned(struct Node **head, char *buffer);
#define MAXCLIENTS 10  //NEW
int main()
{
  int sd; /* socket descriptor */
  int connected_sd; /* socket descriptor */
  int rc; /* return code from recvfrom */
  struct sockaddr_in server_address;
  struct sockaddr_in from_address;
  char buffer[100];
  int flags = 0;
  socklen_t fromLength;
  int clientSDList[MAXCLIENTS] = {0}; // NEW
  fd_set socketFDS; // NEW
  int maxSD = 0;//NEW
  int i;
  int clientRC = 0;
  int sendOutRc = 0;
  char *pointAtChar; 
  struct Node tempNode; //dummy Node
  tempNode.active = 0; //Dummy variable
  tempNode.sd = 0; //dummy sd value
  tempNode.next =NULL; //Assist with not checking empty list
  struct Node* head = &tempNode;
  char username[20];
  char password[20];
  int count = 1;
  int j = 0;
  int returnedSd = 0;
  sd = socket (AF_INET, SOCK_STREAM, 0);
  FILE *fp;
  fp = fopen("clientHistory.txt", "r");
  if( fp!=NULL) { 
		fclose(fp);
		readFile(&head);
	}


  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(24000);
  server_address.sin_addr.s_addr = INADDR_ANY;

  bind (sd, (struct sockaddr *)&server_address, sizeof(server_address));

  listen (sd, 8);//is it okay to set it to 10?
  maxSD = sd; // NEW so far only socket descriptor
  for(;;){// NEW
    memset (buffer, 0, 100);
    FD_ZERO(&socketFDS);// NEW
    FD_SET(sd, &socketFDS); //NEW - sets the bit for the initial SD
    for (i=0;i<MAXCLIENTS;i++) //NEW
      if (clientSDList[i]>0){ //NEW ckecks for clients
	FD_SET(clientSDList[i], &socketFDS); //NEW pass in new socket Descriptor 
	if (clientSDList[i] > maxSD) //NEW
	  maxSD = clientSDList[i]; //NEW stores biggest socket desciptor 
      }
    rc = select (maxSD+1, &socketFDS, NULL, NULL, NULL); // NEW block until something arrives, checks up to (maxSD +1)

    if (FD_ISSET(sd, &socketFDS)){ // NEW
			connected_sd = accept (sd, (struct sockaddr *) &from_address, &fromLength);
			for (i=0;i<MAXCLIENTS;i++)//NEW
				if (clientSDList[i] ==0){ //NEW
					clientSDList [i] = connected_sd; //NEW
					break; // NEW
				}
    }
    for (i=0;i<MAXCLIENTS;i++) // NEW
			if (FD_ISSET(clientSDList[i], &socketFDS)){ //NEW
				rc = read(clientSDList[i], &buffer, 100);//NEW
				if (rc ==0 ){ // NEW - the client disconnected
					close (clientSDList[i]); // NEW close the socket
					clientSDList[i] = 0; // reuse the slot
				}
				else{
					printf ("rc from read  %d received the following %s\n", rc, buffer);
					if(buffer[0]=='0'){ //This will check when Print the list
						printActiveUsers(&head, clientSDList[i]);//Printing list function
						strcpy(buffer,"The List is Provided Above \n");
					}else if(buffer[0]=='1'){ //adds person that is being registered if not yet registered
						pointAtChar = &buffer[0];
		//############################################################################################################################################################################
						strcpy(buffer,checkIfRegistered(&head,pointAtChar,clientSDList[i])); //returns an appropriate value to the client
		//############################################################################################################################################################################
					}else if(buffer[0]=='2'){//Logout
						pointAtChar = &buffer[0];
						strcpy(buffer,logOut(&head,pointAtChar));
					}
					else if(buffer[0]=='3'){//checks for login
						pointAtChar = &buffer[0];
						strcpy(buffer,checkIfClientLoginIn(&head,pointAtChar,clientSDList[i]));
					}
		//############################################################################################################################################################################
					else if(buffer[0]=='4'){
						pointAtChar = &buffer[0];
						returnedSd = returnSD(&head, pointAtChar);

						if(returnedSd != 0){
							sendOutRc = write (returnedSd, buffer, strlen(buffer));
							if (sendOutRc < 0){
									//checks 
							 perror ("write");
							}
							strcpy(buffer,"Message Delivered\n");
						}else{
							strcpy(buffer,"Message not Delivered\n");
						}
					}
		//#############################################################################################################################################################################
					clientRC = write (clientSDList[i], buffer, strlen(buffer));
					if (clientRC < 0)//checks 
						perror ("write");
					printf ("sent  %d bytes\n", rc);
					}
			}// NEW

 	}// NEW
  return 0;
}
void readFile(struct Node **head){
		struct Node *traverse = *head;//traverse node to go thru list
		struct Node *nodeToAdd; //Node that gets added to the end
		FILE *fp;
		fp = fopen("clientHistory.txt", "a+"); //file opener
		nodeToAdd = (struct Node*)malloc(sizeof(struct Node));//allocates memory
		while(fscanf(fp,"%d %s %s %d", &(*nodeToAdd).active,&(*nodeToAdd).username, &(*nodeToAdd).password, &(*nodeToAdd).sd) == 4){ //checks until end of file
			traverse -> next = nodeToAdd; //adds new node to list
			nodeToAdd -> next = NULL; // gets new node equal to null just in case it's the end
			traverse = traverse -> next; //sets traverse to next node
			nodeToAdd = (struct Node*)malloc(sizeof(struct Node));//allocates memory
		}
		fclose(fp); //closes file
}

char * messageReturned(struct Node **head, char *buffer){
	struct Node *traverse = *head; //traverses node
	int i = 1; //used to traverse buffer
	int j = 0; //used to check position on Username and password
	char message[99];//allows message to be 99characters
	char bufferCopy[100];
	char *returnBuff;
	strcpy(bufferCopy, buffer); 
	while(bufferCopy[i] != '|'){ //goes thru string until it reaches the message part
		i++;
	}
	while(bufferCopy[i] != '|'){ //gets all the characters for the string
		message[j] = bufferCopy[i];
		i++;
		j++;
	}
	returnBuff = &message[0];
	return returnBuff;
} 
int returnSD(struct Node **head, char *buffer){ //returns the appropriate socket Descriptor
	struct Node *traverse = *head; 
	int i = 1; //used to traverse buffer
	int j = 0; //used to check position on Username and password
	char username[20];
	char bufferCopy[100];
	strcpy(bufferCopy, buffer); 
	while(bufferCopy[i] != '|'){ //first gets user name
		username[j] = bufferCopy[i];
		i++;
		j++;
	}
	while(traverse!=NULL){//traverses linkedlist
		if(strcmp(traverse -> username, username)== 0 && traverse -> active== 1){//checks that the user actually exist 
			return traverse -> sd; //returns accurate sd
		}
		traverse = traverse -> next;
	}
	return 0; //returns zero if not found
}

char *checkIfRegistered(struct Node **head, char* buffer,int sd){
	struct Node *traverse = *head;
	struct Node* nodeToAdd;
	int i = 1; //used to traverse buffer
	int j = 0; //used to check position on Username and password
	char username[20]; //used to store the suer name
	char password[20]; //used to store and transfer the password
	char bufferCopy[100]; //gets copy of buffer to access char by position
	FILE *fp;
	fp = fopen("clientHistory.txt", "a+"); //name of file to open and write to
	strcpy(bufferCopy, buffer); 
	while(bufferCopy[i] != '|'){ //keeps going until it gets the username
		username[j] = bufferCopy[i];
		i++;
		j++;
	}
	i++;
	j = 0;
	while(bufferCopy[i] != '|'){ //keeps going until it gets the password
		password[j] = bufferCopy[i];
		i++;
		j++;
	}
	if(checkIfLogin(head, username, password)==1){ //checks if user already exist
		return "\n User already exists please try again\n"; //fails to add new user
	}
	while(traverse->next != NULL ){ //traverses node to be able to add new node;
		traverse = (*traverse).next;
	};
	nodeToAdd = (struct Node*)malloc(sizeof(struct Node));//allocates memory
	(*traverse).next = nodeToAdd; //adds node to end of list
	(*nodeToAdd).next = NULL; //sets next to null to know when to stop
	strcpy((*nodeToAdd).username, username);//copies the username to node
	strcpy((*nodeToAdd).password, password);//copies the password to node
	(*nodeToAdd).sd = sd;

	(*nodeToAdd).active = 1;
	traverse = (*traverse).next;
	fprintf(fp,"%d %s %s %d \n",0,(*nodeToAdd).username,(*nodeToAdd).password,(*nodeToAdd).sd);//writes to file to update client info
	fclose(fp);
	return "Success\n";
}

int checkIfLogin(struct Node **head, char username[], char password[]){//purpose of fromFunction is to that it won't print the print statement
	struct Node *traverse = *head;
	while(traverse != NULL){//traverse linkedlist
		if(strcmp((*traverse).username, username)== 0 && strcmp((*traverse).password, password)==0){//checks if user matches input by user
			return 1;//returns true if user already login
		}
		traverse = (*traverse).next;
	}
	return 0;
}

char * checkIfClientLoginIn(struct Node **head, char *buffer, int sd){
	struct Node *traverse = *head;
	int i = 1; //used to traverse buffer
	int j = 0; //used to check position on Username and password
	char username[20]; //stores password
	char password[20]; //stores username
	char bufferCopy[100]; //stores buffer copy
	strcpy(bufferCopy, buffer); 
	while(bufferCopy[i] != '|'){  //goes thru buffer to get username
		username[j] = bufferCopy[i]; 
		i++;
		j++;
	}
	i++;
	j = 0;
	while(bufferCopy[i] != '|'){  //gets password be traversing
		password[j] = bufferCopy[i];
		i++;
		j++;
	}
	while(traverse != NULL){
		if(strcmp(username,traverse->username)==0 && strcmp(password,traverse->password)==0 && traverse -> active == 1){ //checks for the user to already be login by checking active bit
			return "Already Login\n";
		}
		if(strcmp(username,traverse->username)==0 && strcmp(password,traverse->password)==0 && traverse -> active == 0){//checks for the user to already not login by checking active bit
			traverse -> sd = sd;
			traverse ->active = 1;
			return "Login successful\n";
		}
		traverse = traverse ->next;
	}
	return "Wrong username or password\n";
}
void printActiveUsers(struct Node **head, int sd){
	struct Node *traverse = *head;
	int clientRC;
	char newLine[2];
	strcpy(newLine,"\n");

	clientRC = write (sd, newLine, strlen(newLine));
	if (clientRC < 0){
			//checks 
	 perror ("write");
	}
	while(traverse!=NULL){ //traverse linkedlist
		if((*traverse).active == 1){ //checks if user is active
			clientRC = write (sd, (*traverse).username, strlen((*traverse).username));
			if (clientRC < 0){
					//checks 
			 perror ("write");
			}
			clientRC = write (sd, newLine, strlen(newLine));
			if(clientRC <0){
				perror("write");
			}
		}
		traverse = (*traverse).next;
	}

}
char * logOut(struct Node **head, char *buffer){
	struct Node *traverse = *head; 
	int i = 1; //used to traverse buffer
	int j = 0; //used to check position on Username and password
	char username[20]; //stores username
	char bufferCopy[100]; //gets copy of buffer
	strcpy(bufferCopy, buffer); 
	while(bufferCopy[i] != '|'){
		username[j] = bufferCopy[i];
		i++;
		j++;
	}
	while(traverse!=NULL){//traverses linkedlist
		if(strcmp(traverse -> username, username)== 0 && traverse -> active== 1){//checks that the user actually exist 
			(*traverse).active = 0; //sets the user to not active
			return "Logout Successful\n"; //returns true for success
		}
		traverse = traverse -> next;
	}
	return "\n Username not found or account not login\n";
}




