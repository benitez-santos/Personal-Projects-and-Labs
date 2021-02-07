#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h> 



#define MAXCLIENTS 10  //NEW

struct Node{ //Linked list used to take care of registering, activity and correctness
	char fileName[20];
	struct sockaddr_in from_address;
	struct Node *next;
};

char *excuteBuffer(struct Node **head, char *buffer, struct sockaddr_in from_address, int sd);
char *shareCommand(char *buffer,struct Node **head, struct sockaddr_in from_address);
char *downloadCommand(struct Node **head, char *buffer,struct sockaddr_in file_address,int sd);
char *listCommand(struct Node **head,struct sockaddr_in file_address,int sd);
char *searchCommand(struct Node **head, char *buffer);

int main()
{
	struct Node tempHeadNode; //dummy Node
	tempHeadNode.next =NULL; //Assist with not checking empty list
	struct Node* head = &tempHeadNode;

	int sd; /*socket descriptor*/
	int connected_sd; /*socket descriptor*/
	int rc; /*return code form recvfrom*/
	struct sockaddr_in server_address;
	struct sockaddr_in from_address;
	char buffer[100];
	int flags = 0;
	socklen_t fromLength;

	int clientSDList[MAXCLIENTS] = {0};
	fd_set socketFDS; //NEW
	int maxSD = 0; //NEW
	int i = 0;

	sd = socket(AF_INET, SOCK_DGRAM, 0);


	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(24000);
	server_address.sin_addr.s_addr = INADDR_ANY;

	bind (sd, (struct sockaddr *)&server_address, sizeof(server_address));
	fromLength = sizeof(struct sockaddr_in);
	
	while(1){
		rc = recvfrom(sd,&buffer,100,flags, (struct sockaddr *)&from_address, &fromLength);
		strcpy(buffer,excuteBuffer(&head,buffer, from_address,sd));
		rc = sendto(sd,&buffer,100,flags, (struct sockaddr *)&from_address, fromLength);
		memset(buffer, '\0',100);
	}	
	
	
	return 0;
}

char *excuteBuffer(struct Node **head, char *buffer, struct sockaddr_in from_address, int sd){
	char copyOfBuffer[100];
	strcpy(copyOfBuffer, buffer);
	if(copyOfBuffer[0] == '0'){
		return listCommand(head,from_address, sd);
	}else if(copyOfBuffer[0] == '1'){
		return shareCommand(buffer, head, from_address);
	}else if(copyOfBuffer[0] == '2'){
		return searchCommand(head,buffer);
	}else if(copyOfBuffer[0] == '3'){
		return downloadCommand(head,buffer,from_address,sd);
	}
}

char *shareCommand(char *buffer,struct Node **head, struct sockaddr_in from_address){
	struct Node *traverse = *head;
	struct Node* nodeToAdd;
	int i = 1; //used to traverse buffer
	int j = 0; //used to check position on Username and password
	char copyOfBuffer[100];
	char nameOfFile[20];
	memset(nameOfFile, '\0',20);
	strcpy(copyOfBuffer,buffer);

	while(traverse->next != NULL ){ //traverses node to be able to add new node;
		traverse = (*traverse).next;
	}

	while(i<strlen(copyOfBuffer)){
		if(copyOfBuffer[i] == '|'){
			nodeToAdd = (struct Node*)malloc(sizeof(struct Node));//allocates memory
			(*traverse).next = nodeToAdd; //adds node to end of list
			(*nodeToAdd).next = NULL; //sets next to null to know when to stop
			strcpy(nodeToAdd -> fileName, nameOfFile);//copies the fileName to node
			nodeToAdd -> from_address = from_address; //gets the file IP address and port number
			traverse = (*traverse).next; //goes to end of list 
			memset(nameOfFile, '\0',20);//clears char[] for next file;
			j=0;
		}else{
			nameOfFile[j] = copyOfBuffer[i];
			j++;
		}
		i++;
	}

	return "Shared";
}

char *downloadCommand(struct Node **head, char *buffer,struct sockaddr_in file_address,int sd){
	struct Node *traverse = *head;
	char fileLookUp[21];
	char copyOfBuffer[100];
	strcpy(copyOfBuffer,buffer);
	socklen_t fromLength;
	char * returnBuff;
	int rc = 0;
	int i = 1; //used as position in char pointer
	char str[INET_ADDRSTRLEN];
	memset(fileLookUp, '\0',21);
	fromLength = sizeof(struct sockaddr_in);
	while(copyOfBuffer[i] != '\n' && copyOfBuffer[i] !='|'){
		fileLookUp[i-1] = copyOfBuffer[i];
		i++;
	}
	while(traverse!=NULL){ //traverse linkedlist
		if(strcmp(fileLookUp,traverse -> fileName) == 0){ //checks if user is active
			break;
		}
		traverse = traverse -> next;
	}
	memset(copyOfBuffer,'\0',100);
	sprintf(copyOfBuffer, "%d", (*traverse).from_address.sin_port);
	rc = sendto(sd,&(*traverse).fileName,20,0, (struct sockaddr *)&file_address, fromLength);//sends the file name
	rc = sendto(sd,&copyOfBuffer,100,0, (struct sockaddr *)&file_address, fromLength); //sends the file port number that is associated to it
	inet_ntop(AF_INET, &((*traverse).from_address.sin_addr), str, INET_ADDRSTRLEN);
	rc = sendto(sd,&str,sizeof(str),0, (struct sockaddr *)&file_address, fromLength); //sends the IP address of the file being downloaded
	returnBuff = &str[0];
	return "File is ready to download";

}

char *listCommand(struct Node **head,struct sockaddr_in file_address,int sd){
	struct Node *traverse = *head;
	int clientRC;
	char newLine[100];
	int rc;
	socklen_t fromLength;
	fromLength = sizeof(struct sockaddr_in);
	printf("\n");
	while(traverse!=NULL){ //traverse linkedlist
		printf("%s \n", (*traverse).fileName); //prints file names to server to make sure it is working properly 
		/*clientRC = write (sd, traverse -> fileName, strlen(traverse -> fileName));
		clientRC = write (sd, newLine, strlen(newLine));*/
		rc = sendto(sd,&(*traverse).fileName,20,0, (struct sockaddr *)&file_address, fromLength);
		traverse = (*traverse).next;
	}
	memset(newLine,'\0',100);
	newLine[0] = '_';
	rc = sendto(sd,&newLine,100,0, (struct sockaddr *)&file_address, fromLength);
	return "List of file on server is provided above\n";
}

char *searchCommand(struct Node **head, char *buffer){
	struct Node *traverse = *head;
	char fileLookUp[21];
	char copyOfBuffer[100];
	strcpy(copyOfBuffer,buffer);
	int i = 1; //used as position in char pointer

	memset(fileLookUp, '\0',21);
	while(copyOfBuffer[i] != '\n' && copyOfBuffer[i] !='|'){
		fileLookUp[i-1] = copyOfBuffer[i];
		i++;
	}
	while(traverse!=NULL){ //traverse linkedlist
		if(strcmp(fileLookUp,traverse -> fileName) == 0){ //checks if user is active
			return "File found\n";
		}
		traverse = traverse -> next;
	}
	return "File not found, Please try again\n";
}


