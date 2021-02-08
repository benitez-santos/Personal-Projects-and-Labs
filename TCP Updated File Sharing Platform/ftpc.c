#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define htonll(x)   ((((uint64_t)htonl(x&0xFFFFFFFF)) << 32) + htonl(x >> 32))
#define ntohll(x)   ((((uint64_t)ntohl(x&0xFFFFFFFF)) << 32) + ntohl(x >> 32))

unsigned long long filesize(FILE* file);

int main(int argc, char *argv[]) {
    int sd;
    int rc;
    int connect_attempts_left = 5;
    int portnumber;
    unsigned long long file_size, data_recieved = 0;
    unsigned long long data_read, count;
    struct sockaddr_in server_address;
    char serverIP[29], filename[255], buffer[2000];
    FILE *inputfile;

    //Checks for corect number of args
    if (argc < 4 || argc > 4) {
        printf("Usage: ftpc <remote-IP> <remote-port> <local-file-to-transfer>\n");
        exit(1);
    }

    //manipulate args as strings into variables
    portnumber = strtol(argv[2], NULL, 10);
    strcpy(serverIP, argv[1]);
    strcpy(filename, argv[3]);

    //checks for slashes in the file name
    if ((strchr(filename, '/') != NULL) || (strchr(filename, '\\') != NULL)) {
        perror("Filename must not contain \'/\' or \'\\\'");
        exit(1);
    }

    //attempt to open file
    if ((inputfile = fopen(filename, "rb")) == NULL) {
        perror("Error opening input file\n");
        exit(1);
    }

    //attempt to open socket
    sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd == -1) {
        perror("Unable to open socket\n");
        fclose(inputfile);
        exit(1);
    }

    //save server information into struct
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(portnumber);
    server_address.sin_addr.s_addr = inet_addr(serverIP);

    //make connection attempts to the server, tries a few times, then closes if it can't connect
    while(connect_attempts_left > 0 && connect(sd, (struct sockaddr*) &server_address, sizeof(struct sockaddr_in)) < 0) {
        connect_attempts_left--;
        if(connect_attempts_left > 0) {
            printf("Unable to connect to server, waiting 3 seconds before attempting again\n");
            sleep(3);
        } else {
            close(sd);
            fclose(inputfile);
            perror("Unable to establish connection to server\n");
            exit(1);
        }
    }

    //get file size of the file
    file_size = filesize(inputfile);
    file_size = htonll(file_size);

    //send filesize
    rc = write(sd, &file_size, 8);
    if (rc == -1 || rc != 8) {
        perror("Error sending filesize\n");
        close(sd);
        fclose(inputfile);
        exit(1);
    }

    //send filename
    rc = write(sd, filename, strlen(filename) + 1); //sends data at filename including the null character at the end of the filename
    if (rc == -1 || rc != strlen(filename) + 1) {
        perror("Error sending filename\n");
        close(sd);
        fclose(inputfile);
        exit(1);
    }

    //send data
    count = 0;
    data_read = fread(buffer, 1, sizeof(buffer), inputfile);
    while(data_read > 0) {
        count += data_read;
        printf("sending %llu bytes, total is %llu\n", data_read, count);
        rc = write(sd, buffer, data_read);
        
        if (rc == -1 || rc != data_read) {
            perror("Error sending file data\n");
            close(sd);
            fclose(inputfile);
            exit(1);
        }

        data_read = fread(buffer, 1, sizeof(buffer), inputfile);
    }
    
    fclose(inputfile);

    //read size of data revieved from server
    rc = read(sd, buffer, sizeof(buffer));
    close(sd);

    //format data_recieved from the server, check if server reported back correct amount of data
    memcpy(&data_recieved, buffer, sizeof(unsigned long long));
    data_recieved = ntohll(data_recieved);

    if(data_recieved != file_size) {
        perror("Server reported back an incorrect amount of data recieved");
        exit(1);
    } else {
        printf("Server recieved %llu bytes\n", data_recieved);
    }

    return 0;
}


    /*Counts the size of the file at fp file, returns as a unsigned long long*/
unsigned long long filesize(FILE* file) {
    int size = 0;
    char *buffer[1];

    while(fread(buffer, 1, 1, file) == 1) { //count file bytes
        size++;
    }

    fseek(file, 0, SEEK_SET); //reset file position

    return size;
}