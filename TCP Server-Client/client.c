#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
char *handleStringRead(char * buffer);
int main(int argc, char *argv[])
{
  int sd;
  struct sockaddr_in server_address;
  char buffer[100];
  char serverBuffer[100];
  int portNumber;
  char serverIP[29];
  int rc = 0;
  int serverRc = 0; //assign to the server return for read
  if (argc < 4){
    printf ("usage is client <ipaddr> <port> <number>\n");
    exit(1);
  }

  sd = socket(AF_INET, SOCK_STREAM, 0);

  portNumber = strtol(argv[2], NULL, 10);
  strcpy(serverIP, argv[1]);
  
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(portNumber);
  server_address.sin_addr.s_addr = inet_addr(serverIP);
 
  if (connect (sd, (struct sockaddr *) &server_address, sizeof (struct sockaddr_in ))<0){
    close (sd);
    perror ("error connecting stream socket ");
    exit (1);
  }
  
  for (;;){
		while(fgets(buffer, 100, stdin) == NULL);//checks until user puts in input
		strcpy(buffer, handleStringRead(buffer));//gets correct formated string from function
    rc = write (sd, buffer, strlen(buffer));
    if (rc < 0)
      perror ("write");
    serverRc = read(sd, &serverBuffer, 100); //gets the number of bits to see if still connected
		if(serverRc>0){
			printf ("Response from server: %s \n", serverBuffer);
		}
		memset (buffer, 0, 100); //clear both buffers to avoid carry over
		memset (serverBuffer, 0, 100);
    sleep (3);
  }

  return 0 ; 

}

char *handleStringRead(char * buffer){
	char bufferReturn[100]; /*used to copy message from original buffer to access char by location*/
	char bufferCopy[100]; /*file in buffer to return*/
	char *bufferR; /*used to point to location of char array to be able to return*/
	char *list = "List";
	int j = 0; /*keeps track of position in array*/
	int i = 1; /*Keeps track of position on char pointer being return*/
	memset (bufferCopy, 0, 100);
	strcpy(bufferReturn,buffer); /*allows us to access char my location*/
	while(j<strlen(bufferReturn)){/*used to get the first word and recognize what to look for next*/
		if(bufferReturn[j] == ' '||bufferReturn[j] == '\n'){
			if(bufferReturn[j+1] != '<'){
				j++;
			}else {
				j += 2; /*moves to the position of the next first valid char by skiping over the next '<'*/
			}

			break;
		}
		bufferCopy[j] = bufferReturn[j]; /*Copies char by char until first space*/
	    j++;
	}
	if(strcmp(bufferCopy,"List")==0){ /*Checks if word is just list and responds right away*/
	    	memset (bufferCopy, 0, 100);
		bufferCopy[0] = '0';
		bufferR = &bufferCopy[0];
		return bufferR = &bufferCopy[0];
	}else if(strcmp(bufferCopy,"Register")==0){/*Verifies that Word is register and looks for username and password*/
		memset (bufferCopy, 0, 100);
		bufferCopy[0] = '1';
		while(bufferReturn[j] != '>' && bufferReturn[j] != ' ' && bufferReturn[j] != '\n' && i<21){ /*only allows for the username to be less than 20 characters*/
			bufferCopy[i] = bufferReturn[j];
			i++;
			j++;
		}
		bufferCopy[i] = '|';//terminal char
		i++;
		if(bufferReturn[j] == ' ' && bufferReturn[j+1] != '<'){
			j+=1;
		}else{
			j+=3; //skips unwanted characters
		}
		while(bufferReturn[j] != '>'&&i<41 && bufferReturn[j] != ' ' && bufferReturn[j] != '\n'){ /*only allows for the username to be less than 20 characters*/
			bufferCopy[i] = bufferReturn[j];
			i++;
			j++;
		}
		bufferR = &bufferCopy[0];
		bufferCopy[i] = '|';
		return bufferR = &bufferCopy[0];
	}else if(strcmp(bufferCopy,"Logout")==0){/*Looks for user name to logout*/
		memset (bufferCopy, 0, 100);//clears out bufferCopy to create new format
		bufferCopy[0] = '2';
		while(bufferReturn[j] != '>' && bufferReturn[j] != ' ' && bufferReturn[j] != '\n'){//Looks for final character to finish 
			bufferCopy[i] = bufferReturn[j];
			i++;
			j++;
		}
		bufferCopy[i] = '|';//terminal char
		bufferR = &bufferCopy[0];
		return bufferR = &bufferCopy[0];
	}else if(strcmp(bufferCopy,"Login")==0){//Checks for proper requirements for *char to be a login request
		memset (bufferCopy, 0, 100);//clears buffer to be used 
		bufferCopy[0] = '3';
		while(bufferReturn[j] != '>'&& i<21 && bufferReturn[j] != ' ' && bufferReturn[j] != '\n'){ /*only allows for the username to be less than 20 characters*/
			bufferCopy[i] = bufferReturn[j];
			i++;
			j++;
		}
		bufferCopy[i] = '|'; //termanl character
		i++;
		if(bufferReturn[j] == ' ' && bufferReturn[j+1] != '<'){
			j+=1;
		}else{
			j+=3; //skips unwanted characters
		}

		while(bufferReturn[j] != '>'&&i<41 && bufferReturn[j] != ' ' && bufferReturn[j] != '\n'){ /*only allows for the username to be less than 20 characters*/
			bufferCopy[i] = bufferReturn[j];
			i++;
			j++;
		}
		bufferR = &bufferCopy[0];
		bufferCopy[i] = '|';
		return bufferR = &bufferCopy[0];
	}else if(strcmp(bufferCopy,"Message")==0){//Checks for proper requirements for *char to be a message request
		memset (bufferCopy, 0, 100);
		bufferCopy[0] = '4';
		while(bufferReturn[j] != '>'&& i<21 && bufferReturn[j] != ' ' && bufferReturn[j] != '\n'){ /*only allows for the username to be less than 20 characters*/
			bufferCopy[i] = bufferReturn[j];
			i++;
			j++;
		}
		bufferCopy[i] = '|';//terminal char
		i++;
		if(bufferReturn[j] == ' ' && bufferReturn[j+1] != '<'){
			j+=1;
		}else{
			j+=3; //skips unwanted characters
		}

		while(bufferReturn[j] != '>'&&i<100 && bufferReturn[j] != '\n'){ /*only allows for the username to be less than 20 characters*/
			bufferCopy[i] = bufferReturn[j];
			i++;
			j++;
		}
		bufferCopy[i] = '|'; //added at the end as a teminal char
		bufferR = &bufferCopy[0];//done in order to be returned as a pointer
		return bufferR = &bufferCopy[0];
	}
	return "Command not found"; //returns the following if command isn't found. 


}
