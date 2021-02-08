#include	<string.h>
#include	<stdio.h>
#include	<sys/socket.h>
#include	<netinet/in.h>
#include	<unistd.h>

#define htonll(x)   ((((uint64_t)htonl(x&0xFFFFFFFF)) << 32) + htonl(x >> 32))

#define ntohll(x)   ((((uint64_t)ntohl(x&0xFFFFFFFF)) << 32) + ntohl(x >> 32))

int main(int argc, char *argv[])
{
	/*	STREAM	SERVER	*/
	int sd;	/*	socket	descriptor	*/
	int connected_sd;	/*	socket	descriptor	*/
	int rc;	/*	return	code	from	recvfrom */
	struct sockaddr_in server_address;
	struct sockaddr_in from_address;
	unsigned long long networkSize;
	unsigned long long filesize; 
	signed long long size = 0, originalsize=0;
	char dir_name[264] = "./recvd/";
	char fileName[256];
	char buffer[100];//read data that is being passed to server
	long portNumber;//used for portnumber
	int i = 7;
	int check;
	int check_name_length = 0;
	FILE *fp;
	char a;
	socklen_t fromLength;
	//Used for creating directory
    char* dirname = "recvd"; 
      
    check = mkdir(dirname,0777); 
    // check if directory is created or not 
    if (!check) 
		printf("Directory created\n"); 
    else { 
		printf("Unable to create directory because it's already created\n"); 
		
    } 
    //verifies that there are two input arguments
    if (argc != 2) {
        printf("Usage: ftps <remote-port> \n");
        return 0;
    }
    //sets socket for server
	portNumber = atol(argv[1]);
	//printf("Port number before conversion %s\n",argv[1]);
	printf("Port number %lu\n");
	sd = socket	(AF_INET,SOCK_STREAM,0);
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(portNumber);
	server_address.sin_addr.s_addr =INADDR_ANY;
	
	bind	(sd,(struct sockaddr *)&server_address,	sizeof(server_address));
	listen	(sd, 5);
	
	
	for(;;){
		connected_sd =	accept	(sd,	(struct sockaddr *)&from_address,	&fromLength);
		bzero (buffer,	100);
		rc = read(connected_sd,	&networkSize, 8);//retrieves the network file size
		filesize = ntohll(networkSize);

		size = (signed long long)filesize;
		originalsize = size;
		if (size>0){
			//printf ("received the following filesize: %lu %ld.\n", filesize,filesize);
			//Gets the name of the file
		    memset(fileName,'\0',256);
		    memset(dir_name,'\0',264);
			strcpy(dir_name,"./recvd/");
			check_name_length=0;
			while(check_name_length<256 ){//reads character by character
				while(read(connected_sd, &a, 1)<0);//waits for a char to read
		        
		        if(a == 0x00 || a== 0x0||a==00||a==0){
		           break;
		        }
		        fileName[check_name_length++] = a;
			}
			if(check_name_length==0){//checks if the first byte read was a file name that is just null
				printf("First char you read was a null char, not acceptable, clsoing connection:\n");
				goto close;
			}
			if(check_name_length>255){//checks if the file name length got exceeded
				printf("You have exceeded the file name size for file, closing connection: %s\n",fileName);
				goto close;
			}
			printf("Filename %s: \n", fileName);
			strcat(dir_name,fileName);
			printf("Directory %s: \n", dir_name);
			
			//attempt at reading file
			if ((fp = fopen(dir_name, "wb")) == NULL) {
				perror("Error opening input file\n");
				//goto close;
			}
			while((size)>0){//will only read bytes up until the file size given
			    memset(buffer,0,100);
		    	while(rc = read(connected_sd, &buffer, 100)<0){
		    	    if(connected_sd<0){//Verifies that the client connection is still on
		    	        close(connected_sd);
		    	        printf("Client disconneted\n");
		    	        goto close;
		    	    }
		    	}
			  	if(size-100<0){//used to get the exact number of bytes needs and to not write extra
			  		fwrite(buffer , 1 , size , fp );
			  		size = 0;
			  	}else{
			  		fwrite(buffer , 1 , 100 , fp );
			  		size-=100;
			  		
				}
			}
			printf("Total bytes read %ld out of %lu\n", originalsize,filesize);
			originalsize = originalsize-size; //gets the number of bytes that were read vs what should have been read
			filesize = htonll((unsigned long long)originalsize);//conversion to host to long
			rc = write(connected_sd, &filesize, 8);
			if (rc == -1) {//client was not able to recieve last information
				perror("Error sending filename\n");
				
			}
			originalsize = 0;
			fclose(fp);
    	}
    	close:
    	close(connected_sd);
	}
	return	0;
}
