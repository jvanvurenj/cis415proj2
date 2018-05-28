CFLAGS=-W -Wall -pedantic -g
OBJECTS=BoundedBuffer.o diagnostics.o fakeapplications.o \
        freepacketdescriptorstore.o \
        networkdevice.o networkdriver.o packetdescriptor.o \
        queue.o testharness.o

mydemo: $(OBJECTS)
	gcc -o mydemo $(CFLAGS) $(OBJECTS) -lpthread

clean:
	rm -f *.o mydemo

networkdriver.o: networkdriver.c
