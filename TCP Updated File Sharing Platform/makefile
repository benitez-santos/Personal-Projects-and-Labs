#makefile for ftpc.c
#makefile for ftps.c

CC=gcc
CGLAGS = -g -Wall

all: ftps ftpc

ftps: ftps.c
	$(CC) $(CFLAGS) -o ftps ftps.c

ftpc: ftpc.c
	$(CC) $(CFLAGS) -o ftpc ftpc.c
	
clean:
	rm ftps 
	rm ftpc
