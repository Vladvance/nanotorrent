CC = gcc

CCFLAGS = -Wall -g 
INCLUDEDIR = -Iinclude
LIBDIR = -Llibs 
LIBS = -lbencode -lcrypto -lm
TARGET = nt
SRC = main.c

$(TARGET): main.o
	$(CC) $(CCFLAGS) main.o -o $@ $(LIBDIR) $(LIBS) $(INCLUDEDIR) $(INCLUDES) 

main.o: main.c 
	$(CC) $(CCFLAGS) -c main.c -o $@ $(INCLUDEDIR) $(INCLUDES) 

clean:
	rm -f $(TARGET) *.o *~
