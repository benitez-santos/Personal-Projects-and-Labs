## Objective of this project was to simulate a real world problem where a server may disconnect and a client must search for a new server.
 - Should be accomplished through multicasting
 - Utlizes DGram socket as first response to client (UDP socket)
 - Client will initialize a connect through another stream socket (TCP socket)
 - Carries out game
 - Server can handle various clients that may disconnect from other servers
 - A game can either be resumed or started from the beginning 

To clean: make clean

To compile: make

To run the client: tictactoeClient

To run the server: tictactoeServer <port-number>
