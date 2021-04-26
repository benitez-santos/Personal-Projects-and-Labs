#makefile for tictactoe game

CC=gcc
CGLAGS = -g -Wall

all: server client

server: server.c
	$(CC) $(CFLAGS) -o tictactoeServer server.c

client: tictactoeClient.c
	$(CC) $(CFLAGS) -o tictactoeClient tictactoeClient.c
	
clean:
	rm tictactoeClient
	rm tictactoeServer 
