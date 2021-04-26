/**********************************************************/
/* This program is a 'pass and play' version of tictactoe */
/* Two users, player 1 and player 2, pass the game back   */
/* and forth, on a single computer                        */
/**********************************************************/

/* include files go here */
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <errno.h>
#include <time.h> 
/* #define section, for now we will define the number of rows and columns */
#define ROWS  3
#define COLUMNS  3
#define VERSION 6
#define TIMETOWAIT 1
#define ITERATIONSTOWAIT 5
#define MAXCLIENT 10 //NEW
#define MC_PORT 1818 //new 
#define MC_GROUP "239.0.0.1" 
/* C language requires that you predefine all the routines you are writing */


struct GameSlot{ //Linked list used to take care of registering, activity and correctness
	int free_spot;
	char board[ROWS][COLUMNS];
	int player;
	int timeStamp; //used to get the time needed
	int sequenceNumber;
	int previousMove; //takes in the previous move incase a packet gets lost
	int timeOutCounter; //takes the number 30sec iterations it took before timing out
	int previousCommand; 
};

int resumeGame(struct GameSlot *game, char buffer[]);
void setGame(struct GameSlot *game, int i);
int checkwin(char board[ROWS][COLUMNS]);
void print_board(char board[ROWS][COLUMNS]);
int tictactoe(struct GameSlot *game, int gameNumber, char buff[], int sd);
int initSharedState(char board[ROWS][COLUMNS]);
int returnRandoms(int lower, int upper);

int main(int argc, char *argv[])
{
	/*	DGRAM	SERVER	*/
	int sd, dgramSocket, streamSocket;	/*	socket	descriptor	*/
	int connected_sd, cnt; //used for file descriptor set
  /* loop, first print the board, then ask player 'n' to make a move */
	struct sockaddr_in server_address;
	struct sockaddr_in from_address;
	char buffer[100]={0};//read data that is being passed to server
	char moves[9] = {0};
	socklen_t fromLength;
	struct timeval tv;
	int gameNumber = 0, choice = 0;
	int i=0,rc, flags = 0;
	int codeValue = 0; //used to check the return code from tictactoe function to verify if board needs to be re-initialized
	char board[ROWS][COLUMNS];
	socklen_t addrlen;
	int clientSDList[MAXCLIENT] = {0};//new
	fd_set socketFDS; //new
	int maxSD = 0; //new
	struct ip_mreq mreq; //new
	struct GameSlot games[10];//sets the ten slots ready for games
	int portNumber = atol(argv[1]);
	//verifies that there are two input arguments
	if (argc != 2) {
		printf("Usage: ftps <remote-port> \n");
		return 0;
	}

        //set up DGram socket
	dgramSocket = socket(AF_INET, SOCK_DGRAM, 0); // This is the non-MC socket	
	if (dgramSocket < 0) {    
		perror("socket");    
		exit(1);  
	}  // Since I am a server, I will bind to a port/IP for the NON multi-cast socket   
	bzero((char *)&server_address, sizeof(server_address));  
	server_address.sin_family = AF_INET;  
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);  
	server_address.sin_port = htons(portNumber);  
	addrlen = sizeof(server_address);
	if(bind(dgramSocket, (struct sockaddr*) &server_address, sizeof(server_address))<0){ //bind
                perror("socket");    
		exit(1);
        }

        //set up Stream socket
	streamSocket = socket(AF_INET, SOCK_STREAM, 0); // This is the non-MC socket	
	if (streamSocket < 0) {    
		perror("socket");    
		exit(1);  
	}  // Since I am a server, I will bind to a port/IP for the NON multi-cast socket   
	bzero((char *)&server_address, sizeof(server_address));  
	server_address.sin_family = AF_INET;  
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);  
	server_address.sin_port = htons(portNumber);  
	addrlen = sizeof(server_address);
	if(bind(streamSocket, (struct sockaddr*) &server_address, sizeof(server_address))<0){
		perror("bind");    
		exit(1);
	}//bind
	
	//set up Multicast
	sd = socket(AF_INET,SOCK_DGRAM,0);
	if (sd < 0) {    
		perror("socket");    
		exit(1);  
	}
	bzero((char *)&server_address, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(MC_PORT);
	server_address.sin_addr.s_addr = INADDR_ANY;
	addrlen = sizeof(server_address);
	if(bind(sd, (struct sockaddr*) &server_address, sizeof(server_address))<0){//bind
	        perror("socket");    
	        exit(1);  
	}
	mreq.imr_multiaddr.s_addr = inet_addr(MC_GROUP);   
	mreq.imr_interface.s_addr = htonl(INADDR_ANY); 
        if (setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP,&mreq, sizeof(mreq)) < 0) {    
                perror("setsockopt mreq");    
                exit(1);  
        }
	
        listen(streamSocket, 10);
	maxSD = sd;
        fromLength = sizeof(struct sockaddr_in);

	//initialize all the boards
	for(i = 0; i<10; i++){
		setGame(&games[i],0);
	}
        printf("Done initiallizing games \n");

	for(;;){
	        memset(buffer,0,100);
		FD_ZERO(&socketFDS);								
		FD_SET(sd, &socketFDS); //sets the multicast socket				new
		FD_SET(streamSocket, &socketFDS); //sets the streamSocket			new
		for(i=0; i<MAXCLIENT;i++){ //interates through the games that are not zero	
                        if(clientSDList[i]>0){
                                FD_SET(clientSDList[i], &socketFDS);
                                if(clientSDList[i] >maxSD)
                                        maxSD = clientSDList[i];
                        }
		}
		rc = select(maxSD+1, &socketFDS, NULL, NULL, NULL);

		gameNumber = -1;
                if(FD_ISSET(sd, &socketFDS)){
                        cnt = recvfrom(sd, buffer, sizeof(buffer), 0, (struct sockaddr*) &from_address, &fromLength); //searching for client from multicast
			printf("Recieved a new game request from a client, answering with open game \n");

                        for(i=0;i<MAXCLIENT;i++)//used to assign a new game
                                if(clientSDList[i]==0){ //will only check if a game is availible but will not set it aside since the client can select any server it wants. 
					gameNumber = i;
                                        break;
                                }
			if(gameNumber>-1){//else ignores the client
			        buffer[1] = 5;//sends over a game avaliable command
				printf("Sent UDP socket to client with game number %d being availiable \n", gameNumber);
			        rc = sendto(dgramSocket, &buffer, 4,0,(struct sockaddr *)&from_address, sizeof(from_address));
			}
                        
                }
		gameNumber = -1;
		if(FD_ISSET(streamSocket, &socketFDS)){
			fromLength = sizeof(struct sockaddr_in);
                        connected_sd =  accept(streamSocket, (struct sockaddr *)&from_address, &fromLength);
			printf("Recieved stream socket from client  \n");
                        for(i=0;i<MAXCLIENT;i++)//used to assign a new game
                                if(clientSDList[i]==0){
					gameNumber = i;
                                        games[i].free_spot = 0;
                                        clientSDList[i] = connected_sd;
                                        break;
                                }
                }

                for(i=0;i<MAXCLIENT;i++){
                        if(FD_ISSET(clientSDList[i], &socketFDS)){
                                rc = read(clientSDList[i], &buffer, 4);
                                if(rc==0){//client disconnected
                                        printf("Game %d has disconnected \n",i);
                                        close(clientSDList[i]);
                                        clientSDList[i]=0;
                                        setGame(&games[i],0);
					continue;//continues to next iteration
                                }
                                if(rc>0)
			                printf("\n\nCurrent player %d, game number %d, move %d, command number %d\n",games[i].player,buffer[3], buffer[2], buffer[1]);
		                if(rc==4){
		                        if(buffer[0] == VERSION && buffer[1] == 0) {//Starts a new game
			                        printf("Looking for new game. Version %d, sequence number %d command %d and game number %d\n", buffer[0],buffer[1], buffer[2],i);
			                        gameNumber = i;
		                                codeValue = tictactoe(&games[gameNumber], gameNumber,buffer,sd);
		                                buffer[1] = 1;//Initializes game command to 1
                                    		buffer[2] = games[gameNumber].previousMove;
                                    		buffer[3] = gameNumber;
				                sleep(1);
                                    		rc = write(clientSDList[i], buffer, 4);
			                        if(rc==0){//client disconnected
			                                printf("Game %d has disconnected in start of game\n",i);
                                                        close(clientSDList[i]);
                                                        clientSDList[i]=0;
                                                        setGame(&games[i],0);
					                continue;//continues to next iteration
                                                }
                                                printf("Was able to send out start of game successfully for game number %d: ", i);
                            		}else if(buffer[0] == VERSION && buffer[1] == 2){//game over command, clear out spot
                            		        printf("The client at game %d has sent back game over command and the game will be cleared\n", buffer[3]);
                                                setGame(&games[i],0);
                            		}
                            		else if(buffer[0] == VERSION && (buffer[1] == 1||buffer[1]==3)){//checks that the next sequence number is the correct one
                            		        if(games[buffer[3]].free_spot==0){//checks if the board hasn't been re-initialized after a client took to long and timed out on the server side && buffer[1] == (1 + games[buffer[4]].sequenceNumber)
                            		                if(buffer[1] ==3){
                            		                        printf("Entered Resume Game state\n");
                            		                        rc = read(clientSDList[i], &moves, 9);
                            		                        resumeGame(&games[i], moves);
                            		                        buffer[3] = i; //sets the game number
                            		                }
                                    		        gameNumber = buffer[3];
                                    		        codeValue = tictactoe(&games[gameNumber], gameNumber,buffer,sd);
                                    		        buffer[1] = 1; //needed this because a buffer may carry over and not have this value replaced
                                            		buffer[2] = games[gameNumber].previousMove;
                                            		printf("Current player %d \n", games[gameNumber].player);
                                    		        if(codeValue == 0 && games[gameNumber].player==2){//checks to see if the game is over
                                    		                buffer[1] = 2;//sends the command that the game is over
                                    		                setGame(&games[i],0);
                                    		                printf("\n Sending out game over stream command, game number %d, move %d, command number %d\n\n",buffer[3], buffer[2], buffer[1]);
                                    		                rc = write(clientSDList[gameNumber], &buffer, 4);
                                    		                close(clientSDList[i]);
                                                                clientSDList[i]=0;
                                    		        }else{
                                            		        printf("\n Sending out stream command to client, game number %d, move %d, command number %d\n\n",buffer[3], buffer[2], buffer[1]);
                                                    	        rc = write(clientSDList[gameNumber], &buffer, 4);
                                                    	        if(rc<0){//client disconnected
                                                    	                printf("Game %d has disconnected in middle of game\n",i);
                                                                        close(clientSDList[i]);
                                                                        clientSDList[i]=0;
                                                                        setGame(&games[i],0);
                                                                }
                                                        }
                                                }
			                //ignore if sequence number is < than 1 (previous sequence number) or > than the previous sequence number by more than 2
                            		}
                            	}
                        }
                }
	}
	return 0; 
}

int resumeGame(struct GameSlot *game, char buffer[]){    
  /* this just initializing the shared state aka the board */
  
  printf ("Resume board\n");
  (*game).board[0][0] = (buffer[0] == 'X' || buffer[0]  == 'O')?buffer[0]:'1';
  (*game).board[0][1] = (buffer[1] == 'X' || buffer[1]  == 'O')?buffer[1]:'2';
  (*game).board[0][2] = (buffer[2] == 'X' || buffer[2]  == 'O')?buffer[2]:'3';
  
  (*game).board[1][0] = (buffer[3] == 'X' || buffer[3]  == 'O')?buffer[3]:'4';
  (*game).board[1][1] = (buffer[4] == 'X' || buffer[4]  == 'O')?buffer[4]:'5';
  (*game).board[1][2] = (buffer[5] == 'X' || buffer[5]  == 'O')?buffer[5]:'6';
  
  (*game).board[2][0] = (buffer[6] == 'X' || buffer[6]  == 'O')?buffer[6]:'7';
  (*game).board[2][1] = (buffer[7] == 'X' || buffer[7]  == 'O')?buffer[7]:'8';
  (*game).board[2][2] = (buffer[8] == 'X' || buffer[8]  == 'O')?buffer[8]:'9';
  
  return 0;

}

void setGame(struct GameSlot *game, int i){
	(*game).free_spot = 1;//sets all board to be availiable
	initSharedState((*game).board);//sets all boards
	(*game).player = 1;
	(*game).sequenceNumber = 1;
	(*game).previousCommand = 1;//after a game is started, it will always be initialized to 1 until the game is over
	(*game).timeOutCounter = 0;//sets the timeout counter to 0 since there isn't any timeouts yet
}

int tictactoe(struct GameSlot *game, int gameNumber, char buff[], int sd)
{
	/* this is the meat of the game, you'll look here for how to change it up */
	int i, choice, iterations=0;  // used for keeping track of choice user makes
	int row, column;
	char mark;      // either an 'x' or an 'o'
	int rc = -1;
	int flags = 0;
	char buffer[100]= {0};//read data that is being passed to server
	strcpy (buffer, buff);

	do{
		
		(*game).player = ((*game).player % 2) ? 1 : 2;  // Mod math to figure out who the player is
		if((*game).player==1){
			choice = returnRandoms(1,9); // print out player so you can pass game
			printf("Player 1 choice = %d\n", choice);       
		}else if((*game).player==2){
			printf("Player %d, entered a number:  \n", (*game).player);
			choice = buffer[2]-'0';
			printf("Correct format from client, recieved %c\n", buffer[3]);
		}
		mark = ((*game).player == 1) ? 'X' : 'O'; //depending on who the player is, either us x or o
		/******************************************************************/
		/** little math here. you know the squares are numbered 1-9, but  */
		/* the program is using 3 rows and 3 columns. We have to do some  */
		/* simple math to conver a 1-9 to the right row/column            */
		/******************************************************************/
		row = (int)((choice-1) / ROWS); 
		column = (choice-1) % COLUMNS;
		/* first check to see if the row/column chosen is has a digit in it, if it */
		/* square 8 has and '8' then it is a valid choice                          */

		if ((*game).board[row][column] == (choice+'0')){
			if((*game).player==1){
                                (*game).previousMove = choice+'0';//places the previous mo
			        (*game).timeStamp = (unsigned)time(NULL);
			}
	                (*game).board[row][column] = mark;
	                /* after a move, check to see if someone won! (or if there is a draw */
	                i = checkwin(game->board);
	                if(i != -1)
	                        break;
                        if((*game).player==1){
                                (*game).player++;
                                break;   
                        }
	                (*game).player++;
		}else
		  {
			printf("Invalid move enter again: ");
			(*game).player--;
		  }
	}while (i ==  - 1); // -1 means no one won
	/* print out the board again */
	print_board(game->board);

	if (i == 1) // means a player won!! congratulate them
	        printf("==>\aPlayer %d wins\n game %d\n", (*game).player, gameNumber);
	else if(i==0)
	        printf("==>\aGame draw"); // ran out of squares, it is a draw
	return (i==1||i==0)?0:1;
}

int checkwin(char board[ROWS][COLUMNS])
{
  /************************************************************************/
  /* brute force check to see if someone won, or if there is a draw       */
  /* return a 0 if the game is 'over' and return -1 if game should go on  */
  /************************************************************************/
  if (board[0][0] == board[0][1] && board[0][1] == board[0][2] ) // row matches
    return 1;
        
  else if (board[1][0] == board[1][1] && board[1][1] == board[1][2] ) // row matches
    return 1;
        
  else if (board[2][0] == board[2][1] && board[2][1] == board[2][2] ) // row matches
    return 1;
        
  else if (board[0][0] == board[1][0] && board[1][0] == board[2][0] ) // column
    return 1;
        
  else if (board[0][1] == board[1][1] && board[1][1] == board[2][1] ) // column
    return 1;
        
  else if (board[0][2] == board[1][2] && board[1][2] == board[2][2] ) // column
    return 1;
        
  else if (board[0][0] == board[1][1] && board[1][1] == board[2][2] ) // diagonal
    return 1;
        
  else if (board[2][0] == board[1][1] && board[1][1] == board[0][2] ) // diagonal
    return 1;
        
  else if (board[0][0] != '1' && board[0][1] != '2' && board[0][2] != '3' &&
	   board[1][0] != '4' && board[1][1] != '5' && board[1][2] != '6' && 
	   board[2][0] != '7' && board[2][1] != '8' && board[2][2] != '9')

    return 0; // Return of 0 means game over
  else
    return  - 1; // return of -1 means keep playing
}


void print_board(char board[ROWS][COLUMNS])
{
  /*****************************************************************/
  /* brute force print out the board and all the squares/values    */
  /*****************************************************************/

  printf("\n\n\n\tCurrent TicTacToe Game\n\n");

  printf("Player 1 (X)  -  Player 2 (O)\n\n\n");


  printf("     |     |     \n");
  printf("  %c  |  %c  |  %c \n", board[0][0], board[0][1], board[0][2]);

  printf("_____|_____|_____\n");
  printf("     |     |     \n");

  printf("  %c  |  %c  |  %c \n", board[1][0], board[1][1], board[1][2]);

  printf("_____|_____|_____\n");
  printf("     |     |     \n");

  printf("  %c  |  %c  |  %c \n", board[2][0], board[2][1], board[2][2]);

  printf("     |     |     \n\n");
}

int initSharedState(char board[ROWS][COLUMNS]){    
  /* this just initializing the shared state aka the board */
  int i, j, count = 1;
  printf ("in sharedstate area\n");
  for (i=0;i<3;i++)
    for (j=0;j<3;j++){
      board[i][j] = count + '0';
      count++;
    }
  return 0;

}

int returnRandoms(int lower, int upper) 
{ 
  return (rand() % (upper - lower + 1)) + lower; 
} 
