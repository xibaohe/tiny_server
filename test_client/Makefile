CC=gcc
CFLAGS=-I./include -I../common 
LD=-lpthread
HPATH=../common
SRC=src
INCLUDE=include


test_client: test_client.o csapp.o
	$(CC) -o test_client test_client.o csapp.o $(LD)

test_client.o: test_client.c
	$(CC) -c test_client.c $(CFLAGS)

csapp.o: $(HPATH)/csapp.c
	$(CC) -c $(HPATH)/csapp.c $(CFLAGS)

clean:
	rm -rf test_client
	rm -rf *.o
send:
	scp test_client mclab@192.168.2.104:/media/mclab/linux-data2/JJ_SERVER

