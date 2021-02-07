#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
char *handlingString(char * buffer);
int main(int argc, char *argv[])
{
	int sd; 
	struct sockaddr_in server_address;
	struct sockaddr_in from_address;
	struct sockaddr_in from_client;//searches the client address
	char buffer[100];
	char fileName[100]; //stores the file name being downloaded
	char fileIpAddress[100];//stores the ip address of the file wanted to be downloaded
	int filePortNumber;//stores the number of the portNumber from the file
	int portNumber;
	int filesize; //stores the size of file to be downloaded
	char serverIP[29]; //stores server ip address
	int rc = 0;
	struct stat st;
	int indicator = 0;//used to indicate if user would like to download or share
	char copyBuffer[] = "copy-";
	socklen_t fromLength;
	FILE *fp;//file pointer to transfer file
  	fp = fopen("clientHistory.txt", "r");
	int timer = 0;
	if (argc < 3){
		printf("usage is client <ipaddr> <port>\n");
		exit(1);
	}
	
	if(rc < 0){
		perror ("sendto");
	}
	sd = socket(AF_INET, SOCK_DGRAM, 0);

	portNumber = strtol(argv[2], NULL,10);
	strcpy(serverIP, argv[1]);
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(portNumber);
	server_address.sin_addr.s_addr = inet_addr(serverIP);

	for(;;){
		while(fgets(buffer, 100, stdin) == NULL);//checks until user puts in input
		strcpy(buffer, handlingString(buffer));//gets correct formated string from function
		rc = sendto(sd,buffer,strlen(buffer),0,(struct sockaddr *) &server_address, sizeof(server_address));
		if(buffer[0] == '1'){//checks if command is "Share", to open a coket and wait for requests 
			indicator = 1;
		}
		if(buffer[0] == '0'){//checks if the client asked for the list
			memset(buffer, '\0',100);//clears the buffer to start accepting the list
			printf("\n");
			while(buffer[0] != '_'){
				rc = recvfrom(sd,&buffer,100,0, (struct sockaddr *)&from_address, &fromLength);
				printf("%s\n",buffer);
			}
		}
		if(buffer[0] == '3'){//checks if the user wants to download the file 
			rc = recvfrom(sd,&fileName,100,0, (struct sockaddr *)&from_address, &fromLength);
			memset(buffer, '\0',100);//clears the buffer to start accepting the list
			rc = recvfrom(sd,&buffer,100,0, (struct sockaddr *)&from_address, &fromLength);
			filePortNumber = strtol(buffer, NULL,10);
			rc = recvfrom(sd,&fileIpAddress,100,0, (struct sockaddr *)&from_address, &fromLength);
			indicator = 3;
		}
		rc = recvfrom(sd,&buffer,100,0, (struct sockaddr *)&from_address, &fromLength);
		printf("%s\n",buffer);
		memset (buffer, '\0', 100); //clears buffer to not reuse data
		
		
		if(indicator == 1){//Opens sockets for file sharing
			printf("waiting to share\n");
			rc = recvfrom(sd,&buffer,100,0, (struct sockaddr *)&from_address, &fromLength);//retrieves the name of the file being downloaded
			fp = fopen(buffer, "rb");
			//fseek(fp,0L,SEEK_END);//moves pointer to end to get file size
			//filesize = ftell(fp); // used to store size of file
			//fseek(fp,0L,SEEK_SET); //restores the pointer to read from the beginning
			stat(buffer,&st);
			filesize = st.st_size;
			memset(buffer,0,100); //clears the buffer to start sending data
			sprintf(buffer, "%d", filesize);//converts the filesize to a string
			rc = sendto(sd,buffer,strlen(buffer),0,(struct sockaddr *) &from_address, sizeof(server_address));//sends size of string
			printf("have started sharing data\n");
			while(filesize>0){//continues reading until the end of file 
				filesize = filesize-100;
				//memset(buffer,'\0',100); //clears the buffer to start sending data
				if(filesize<0){
					fread(buffer,1,100+filesize,fp);//recalculates the accurate number of bits to send
					rc = sendto(sd,buffer,strlen(buffer),0,(struct sockaddr *) &from_address, sizeof(server_address)); //sends bits in buffer
					//memset(buffer,'\0',100);
					break;
				}else if(filesize>0){
					fread(buffer,1,100,fp);
					rc = sendto(sd,buffer,strlen(buffer),0,(struct sockaddr *) &from_address, sizeof(server_address));
					//memset(buffer,'\0',100);
				}else{
					break;
				}
			}
			indicator = 0; //resets the counter
			fclose(fp);
			printf("have closed sharing data\n\n");
		}else if(indicator == 3){//iniciated download 
			from_client.sin_family = AF_INET;
			from_client.sin_port = filePortNumber;
			from_client.sin_addr.s_addr = inet_addr(fileIpAddress);
			
			rc = sendto(sd,fileName,strlen(fileName),0,(struct sockaddr *) &from_client, sizeof(from_client));//sends the name of the file it wants
			memset(buffer,'\0',100);
			rc  = recvfrom(sd,&buffer,100,0, (struct sockaddr *)&from_client, &fromLength);//retrieves the size of the file being downloaded
			filesize = atoi(buffer);
			printf("have finished successfully creating socket\n");
			strcat(copyBuffer,fileName);
			fp = fopen(copyBuffer,"wb");
			strcpy(copyBuffer, "copy-");
			printf("About to start downloading\n");
			memset(buffer,'\0',100);
			
			while(filesize>0){
				filesize = filesize-100;
				memset(buffer,'\0',100); //clears the buffer to start sending data
				if(filesize<0){
					rc  = recvfrom(sd,&buffer,100,0, (struct sockaddr *)&from_client, &fromLength);//retrieves the bytes
					fwrite(buffer, sizeof(char), 100+filesize, fp); 
					memset(buffer,'\0',100); //clears the buffer to start sending data
					break;
				}else if(filesize>0){
					rc  = recvfrom(sd,&buffer,100,0, (struct sockaddr *)&from_client, &fromLength);//retrieves the bytes
					fwrite(buffer, sizeof(char), 100, fp);
					memset(buffer,'\0',100); //clears the buffer to start sending data
				}else{
					break;
				}
			}
			fclose(fp);
			indicator = 0; //resets the counter
			printf("have finished downloading\n\n");
		}
	}
	return 0;
}

char *handlingString(char * buffer){
	char bufferReturned[100]; /*used to copy message from original buffer to access char by location*/
	char bufferCopy[100]; /*file in buffer to return*/
	char *bufferR; /*used to point to location of char array to be able to return*/
	char *list = "List";
	int j = 0; /*keeps track of position in array*/
	int i = 1; /*Keeps track of position on char pointer being return*/
	strcpy(bufferReturned,buffer); /*allows us to access char my location*/
	memset (bufferCopy, '\0', 100);
	while(j<strlen(bufferReturned)){/*used to get the first word and recognize what to look for next*/
		if(bufferReturned[j] == ' '|| bufferReturned[j] == '\n'){
			break;
		}
		bufferCopy[j] = bufferReturned[j]; /*Copies char by char until first space*/
	    j++;
	}

	j++;
	if(strcmp(bufferCopy,"List")==0){ /*DONE - Checks if word is just list and responds right away*/
	    	memset (bufferCopy, 0, 100);
		bufferCopy[0] = '0';
		return bufferR = &bufferCopy[0];
	}else if(strcmp(bufferCopy,"Share")==0){/*Verifies that Word is "Share and starts reading all the files from that"*/
		memset (bufferCopy, 0, 100);//clears out bufferCopy to create new format
		bufferCopy[0] = '1';
		while(1){
			if(bufferReturned[j] == ' '){
				bufferCopy[i] = '|';
			}else if (bufferReturned[j] == '\n'){
				break;
			}else{
				bufferCopy[i] = bufferReturned[j];
			}
			i++;
			j++;
		}
		bufferCopy[i] = '|';//terminal char for server to know when to shop
		return bufferR = &bufferCopy[0];
	}else if(strcmp(bufferCopy,"search")==0){/*DONE - Looks for user looking for a file*/
		memset (bufferCopy, 0, 100);//clears out bufferCopy to create new format
		bufferCopy[0] = '2';
		while(bufferReturned[j] != ' ' && bufferReturned[j] != '\n'){//Looks for final character to finish 
			bufferCopy[i] = bufferReturned[j];
			i++;
			j++;
		}
		bufferCopy[i] = '|';//terminal char for server to know when to shop
		return bufferR = &bufferCopy[0];
	}else if(strcmp(bufferCopy,"download")==0){//Checks for proper requirements for *char to be a download
		memset (bufferCopy, 0, 100);//clears out bufferCopy to create new format
		bufferCopy[0] = '3';
		while(bufferReturned[j] != ' ' && bufferReturned[j] != '\n'){//Looks for final character to finish 
			bufferCopy[i] = bufferReturned[j];
			i++;
			j++;
		}
		bufferCopy[i] = '|';//terminal char for server to know when to stop
		return bufferR = &bufferCopy[0];
	}
	return "Command not found\n"; //returns the following if command isn't found. 
}
