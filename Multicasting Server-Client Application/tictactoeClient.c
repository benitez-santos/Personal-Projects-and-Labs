/**********************************************************/
/* This program is a 'pass and play' version of tictactoe */
/* Two users, player 1 and player 2, pass the game back   */
/* and forth, on a single computer                        */
/**********************************************************/

/* include files go here */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

/* #define section, for now we will define the number of rows and columns */
#define ROWS  3
#define COLUMNS  3

#define TIMEOUT 15
#define FINDGAMECHANCES 3

#define MC_ADDRESS "239.0.0.1"
#define MC_PORT "1818"

#define VERSION 6

/* C language requires that you predefine all the routines you are writing */
int checkwin(char board[ROWS][COLUMNS]);
void print_board(char board[ROWS][COLUMNS]);
int tictactoe(char board[ROWS][COLUMNS],char *argv[]);
int initSharedState(char board[ROWS][COLUMNS]);

int main(int argc, char *argv[])
{
  int rc;
  char board[ROWS][COLUMNS]; 

  if (argc > 1) {
        printf("Usage: tictactoeClient\n");
        exit(1);
  }

  rc = initSharedState(board); // Initialize the 'game' board
  rc = tictactoe(board,argv); // call the 'game'

  return 0; 
}


int tictactoe(char board[ROWS][COLUMNS],char *argv[])
{
  /* this is the meat of the game, you'll look here for how to change it up */
  int player = 1; // keep track of whose turn it is
  int i = -1;
  int row, column, holder;
  char mark, choice;      // either an 'x' or an 'o'and user choice or choice recieved
  int rc;   //return code
  int sd, sdgram;
  int portnumber;
  struct sockaddr_in server_address;
  struct sockaddr_in mc_address;
  struct sockaddr_in from_address;
  socklen_t fromLength = sizeof(struct sockaddr_in);
  int flags = 0;
  char serverIP[29];
  char buffer[100] = {0};
  char gameNumber = -1;
  int receivedGameNum = 0;
  int receivedGameOver = 0;
  char foundGame = 0;
  int timer = 0;
  int chances = FINDGAMECHANCES;
  int j = 0;

  int turnCount = 0;

  portnumber = strtol(MC_PORT, NULL, 10);
  strcpy(serverIP, MC_ADDRESS);

  sdgram = socket(AF_INET, SOCK_DGRAM, 0);
  if (sdgram == -1) {
      perror("Unable to open socket\n");
      exit(1);
  }

  fcntl(sdgram, F_SETFL, O_NONBLOCK);

  mc_address.sin_family = AF_INET;
  mc_address.sin_addr.s_addr = inet_addr(serverIP);
  mc_address.sin_port = htons(portnumber);

  
  buffer[0] = VERSION;
  buffer[1] = 4;

  timer = 0;
  chances = FINDGAMECHANCES;
  foundGame = 0;

  while(!foundGame) {
    if(timer == 0 && chances >= 0) {
      printf("Sending game request...\n");
      rc = sendto(sdgram, buffer, 2, 0, (struct sockaddr *)&mc_address, sizeof(mc_address));
      timer = TIMEOUT;
      chances--;
    } else if(chances < 0) {
      perror("Ran out of conection chance requests. Exiting\n");
      exit(1);
    }
    
    //reads all incoming dgrams
    while(!foundGame && recvfrom(sdgram, &buffer, 2, flags, (struct sockaddr *)&from_address, &fromLength) > 0) {
      if(buffer[0] == VERSION && buffer[1] == 5) {
        //found game
        foundGame = 1;

        //save server information into struct
        server_address.sin_family = AF_INET;
        server_address.sin_port = from_address.sin_port;
        server_address.sin_addr.s_addr = from_address.sin_addr.s_addr;
      }
    }
    if(!foundGame) {
      sleep(1);
      timer--;
    }

  }

  sd = socket(AF_INET, SOCK_STREAM, 0);

  if (connect (sd, (struct sockaddr *) &server_address, sizeof(struct sockaddr_in)) < 0) {
    close(sd);
    perror("Error connecting stream socket.\n");
    exit(1);
  }

  printf("Connection was successful\n");
  //send command to start game to server
  buffer[0] = VERSION;
  buffer[1] = 0;    //new game command

  rc = write(sd, &buffer, 4);
  if(rc < 0) {
      perror("Error sending start game command to server\n");
      close(sd);
      exit(1);
  }
  printf("First packet was successful\n");

  /* loop, first print the board, then ask player 'n' to make a move */

  while (i == -1){    //while no player has won
    print_board(board); // call function to print the board on the screen
    player = (player % 2) ? 1 : 2;  // Mod math to figure out who the player is

    if(player == 2) {   //if local player's turn
      printf("Player %d, enter a number:  ", player); // print out player so you can pass game
      scanf("%c", &choice); //using scanf to get the choice
      scanf("%c", (char*)&holder); //consume newline

      mark = (player == 1) ? 'X' : 'O'; //depending on who the player is, either us x or o

      holder = choice - '0';      //convert choice to int
      printf("binary value: %i\n", holder);
      row = (int)((holder-1) / ROWS); 
      column = (holder-1) % COLUMNS;

      if (board[row][column] == (holder+'0')) {
        turnCount++;
        board[row][column] = mark;  //update local board

        buffer[0] = VERSION;
        buffer[1] = 1;          //move command
        buffer[2] = choice;
        buffer[3] = gameNumber;

        //send move to server
        printf("Sending: Version %i, Command %i, Move %i, GameNum %i\n", buffer[0], buffer[1], buffer[2], buffer[3]);
        rc = write(sd, &buffer, 4);
        if (rc != 4) {
          printf("Connection to server lost. Attempting to find a new one...\n");
          close(sd);

          buffer[0] = VERSION;
          buffer[1] = 4;

          timer = 0;
          chances = FINDGAMECHANCES;
          foundGame = 0;

          while(!foundGame) {
           if(timer == 0 && chances >= 0) {
              printf("Sending game request...\n");
              rc = sendto(sdgram, buffer, 2, 0, (struct sockaddr *)&mc_address, sizeof(mc_address));
              timer = TIMEOUT;
              chances--;
            } else if(chances < 0) {
              perror("Ran out of conection chance requests. Exiting\n");
              exit(1);
            }
    
            //reads all incoming dgrams
            while(!foundGame && recvfrom(sdgram, &buffer, 2, flags, (struct sockaddr *)&from_address, &fromLength) > 0) {
              if(buffer[0] == VERSION && buffer[1] == 5) {
                //found game
                foundGame = 1;

                //save server information into struct
                server_address.sin_family = AF_INET;
                server_address.sin_port = from_address.sin_port;
                server_address.sin_addr.s_addr = from_address.sin_addr.s_addr;
              }
            }
            if(!foundGame) {
              sleep(1);
              timer--;
            }

          }

          sd = socket(AF_INET, SOCK_STREAM, 0);

          if (connect (sd, (struct sockaddr *) &server_address, sizeof(struct sockaddr_in)) < 0) {
            close(sd);
            perror("Error connecting stream socket.\n");
            exit(1);
          }

          //send resume command
          buffer[0] = VERSION;
          buffer[1] = 3;  //resume command
          for(j = 0; j < 9; j++) {
            row = j / ROWS; 
            column = j % COLUMNS;
            if(board[row][column] == 'X' || board[row][column] == 'O') {
              buffer[j + 4] = board[row][column];
            } else {
              buffer[j + 4] = 0;
            }
          }
          rc = write(sd, &buffer, 13);
        }

      } else {
	      printf("Invalid move ");
	      player--;
	      getchar();
      }
      /* after a move, check to see if someone won! (or if there is a draw */
      i = checkwin(board);
    
      player++;
    } else {      //if server player's turn
      //wait for server player's move
      printf("Waiting for server's move...\n");
      buffer[0] = 0; buffer[1] = 0;
      rc = read(sd, &buffer, 4);
      if(rc > 0) { //move probably recieved
        printf("Received: Version %i, Command %i, Move %i, GameNum %i\n", buffer[0], buffer[1], buffer[2], buffer[3]);
        if(buffer[0] == VERSION && buffer[1] == 1) {      //if move recieved
          if(receivedGameNum == 0) {
	          receivedGameNum = 1;
            choice = buffer[2];
            gameNumber = buffer[3];
            printf("Recieved first move from server, gamenumber received %i\n", gameNumber);
          } else if(buffer[3] != gameNumber) {
            perror("Move for other game recieved from server\n");
            close(sd);
            exit(1);
          } else {
            choice = buffer[2];
          }
          printf("Correct format from server, move recieved %c\n", buffer[2]);
        } else {
            perror("Move recieved from server is in the wrong format (version num or not a move command)\n");
            close(sd);
            exit(1);
        }

        printf("Char recieved from server: %c\n", choice);

        mark = (player == 1) ? 'X' : 'O'; //depending on who the player is, either us x or o

        holder = choice - '0';      //convert choice to int
        printf("holder value: %i\n", holder);
        row = (int)((holder-1) / ROWS); 
        column = (holder-1) % COLUMNS;

        if (board[row][column] == (holder+'0')) {
          turnCount++;
          board[row][column] = mark;
        } else {
	        perror("Invalid move received from server\n");
          close(sd);
          exit(1);
        }

        /* after a move, check to see if someone won! (or if there is a draw */
        i = checkwin(board);
    
        player++;
      } else {  //rc <= 0
        close(sd);
        printf("Connection to server lost. Attempting to find a new one...\n");
        buffer[0] = VERSION;
        buffer[1] = 4;

        timer = 0;
        chances = FINDGAMECHANCES;
        foundGame = 0;

        while(!foundGame) {
          if(timer == 0 && chances >= 0) {
            printf("Sending game request...\n");
            rc = sendto(sdgram, buffer, 2, 0, (struct sockaddr *)&mc_address, sizeof(mc_address));
            timer = TIMEOUT;
            chances--;
          } else if(chances < 0) {
            perror("Ran out of conection chance requests. Exiting\n");
            exit(1);
          }
    
          //reads all incoming dgrams
          while(!foundGame && recvfrom(sdgram, &buffer, 2, flags, (struct sockaddr *)&from_address, &fromLength) > 0) {
            if(buffer[0] == VERSION && buffer[1] == 5) {
              //found game
              foundGame = 1;

              //save server information into struct
              server_address.sin_family = AF_INET;
              server_address.sin_port = from_address.sin_port;
              server_address.sin_addr.s_addr = from_address.sin_addr.s_addr;
           }
          }
          if(!foundGame) {
            sleep(1);
            timer--;
          }

        }

        sd = socket(AF_INET, SOCK_STREAM, 0);

        if (connect (sd, (struct sockaddr *) &server_address, sizeof(struct sockaddr_in)) < 0) {
          close(sd);
          perror("Error connecting stream socket.\n");
          exit(1);
        }

        //send resume command
        buffer[0] = VERSION;
        buffer[1] = 3;  //resume command
        for(j = 0; j < 9; j++) {
          row = j / ROWS; 
          column = j % COLUMNS;
          if(board[row][column] == 'X' || board[row][column] == 'O') {
            buffer[j + 4] = board[row][column];
          } else {
            buffer[j + 4] = 0;
          }
        }
        rc = write(sd, &buffer, 13);
      }
    }
  }
    
  /* print out the board again */
  print_board(board);
    
  if (i == 1) {
    printf("==>\aPlayer %d wins\n ", --player);
  } // means a player won!! congratulate them
  else {
    printf("==>\aGame draw\n"); // ran out of squares, it is a draw
  }

  player = (player % 2) ? 1 : 2;

  if(player == 2 && turnCount < 9) {   //was just client's turn, means client just made final move

    //resend wait for gameover from server

    printf("Waiting for gameover message from server...\n");
    
    rc = read(sd, &buffer, 4);
    if(rc == 4) { //prob received command
      if(buffer[0] == VERSION && buffer[1] == 2 && buffer[3] == gameNumber) {
        printf("Received gameover from server, quiting.\n");
        receivedGameOver = 1;
      } else {
        perror("Received something other than the gameover message, quitting.");
        close(sd);
        exit(1);
      }
    }
  } else {  //was just server's turn, server just made last move

    //send gameover to server

    buffer[0] = VERSION;
    buffer[1] = 2;    //gameover command
    buffer[2] = 0;
    buffer[3] = gameNumber;

    printf("Sending gameover message to server.\n");

    rc = write(sd, &buffer, 4);

    if(rc != 4) {
      perror("Error sending gameover to server\n");
      close(sd);
      exit(1);
    }
  }
  close(sd);
  return 0;
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
