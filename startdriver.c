
#include "BoundedBuffer.h"
#include "packetdescriptor.h"
#include "destination.h"
#include "pid.h"
#include "freepacketdescriptorstore.h"

#include "networkdevice.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define SEND_BUFFER_SIZE 50
#define RECV_BUFFER_SIZE 50
#define RECSIZE 10

NetworkDevice *networkdev;
FreePacketDescriptorStore *fpds;

BoundedBuffer *toSend, toReceive;
BoundedBuffer *buf[MAX_PID+1] //do i need +1



void do_nanosleep(int nseconds) /* helper sleep function.*///FROM LAB
{
	struct timespec time, time2;
	time.tv_sec = 0;
	time.tv_nsec = nseconds;
	time2.tv_sec =0;
	time2.tv_nsec = 0;
	nanosleep( &time, &time2);
}

/* These are the calls to be implemented by the students */

void blocking_send_packet(PacketDescriptor *pd){
	blockingWriteBB(toSend, pd);
	return;
}

int  nonblocking_send_packet(PacketDescriptor *pd){
	return nonblockingWriteBB(toSend, pd);
}
/* These calls hand in a PacketDescriptor for dispatching */
/* The nonblocking call must return promptly, indicating whether or */
/* not the indicated packet has been accepted by your code          */
/* (it might not be if your internal buffer is full) 1=OK, 0=not OK */
/* The blocking call will usually return promptly, but there may be */
/* a delay while it waits for space in your buffers.                */
/* Neither call should delay until the packet is actually sent      */

void blocking_get_packet(PacketDescriptor **pd, PID pid){
	*pd = blockingReadBB(buf[pid]);
}
int  nonblocking_get_packet(PacketDescriptor **pd, PID pid){
	int i;
	i = nonblockingReadBB(buf[pid], pd);
	return i;
}
/* These represent requests for packets by the application threads */
/* The nonblocking call must return promptly, with the result 1 if */
/* a packet was found (and the first argument set accordingly) or  */
/* 0 if no packet was waiting.                                     */
/* The blocking call only returns when a packet has been received  */
/* for the indicated process, and the first arg points at it.      */
/* Both calls indicate their process number and should only be     */
/* given appropriate packets. You may use a small bounded buffer   */
/* to hold packets that haven't yet been collected by a process,   */
/* but are also allowed to discard extra packets if at least one   */
/* is waiting uncollected for the same PID. i.e. applications must */
/* collect their packets reasonably promptly, or risk packet loss. */

void init_network_driver(NetworkDevice               *nd, 
                         void                        *mem_start, 
                         unsigned long               mem_length,
                         FreePacketDescriptorStore **fpds_ptr){


	networkdev = nd;
	pthread_t sendThread, receiveThread;
	fpds = create_fpds();
	create_free_packet_descriptors(fpds, mem_start, mem_length);
	*fpds_ptr = fpds;
	PacketDescriptor *pd;



	/*CREATE PTHREADS*/
	pthread_create(&sendThread, NULL, sthread, NULL);
	pthread_create(&receiveThread, NULL, rthread, NULL);


	/*CREATE BUFFERS*/ //most from lab
	int i;

	toSend = createBB(SEND_BUFFER_SIZE);
	toReceive = createBB(RECV_BUFFER_SIZE);

	for(i=0; i<MAX_PID; i++){
		buf[i]= createBB(RECV_BUFFER_SIZE)
	}
	for(i=0;i<RECSIZE;i++){//from lab //can i just use send_receive_size?
		blocking_get_pd(fpds, &pd);
		blockingWriteBB(toReceive);
	}

}
/* Called before any other methods, to allow you to initialise */
/* data structures and start any internal threads.             */ 
/* Arguments:                                                  */
/*   nd: the NetworkDevice that you must drive,                */
/*   mem_start, mem_length: some memory for PacketDescriptors  */
/*   fpds_ptr: You hand back a FreePacketDescriptorStore into  */
/*             which you have put the divided up memory        */
/* Hint: just divide the memory up into pieces of the right size */
/*       passing in pointers to each of them                     */ 


void* receive_thread(){
	PID curPID;
	int counter, temp;
	counter = 0;
	PacketDescriptor *current;
	PacketDescriptor *todo;
	PacketDescriptor *previous;
	blocking_get_pd(fpds, &current);
	init_packet_descriptor(current);
	register_receiving_packetdescriptor(netdev, current);





	while(true){
		await_incoming_packet(netdev);
		counter++;
		todo = current;
		DIAGNOSTICS("Received! Total count: %d\n", counter);
		//need to check if went through
		temp = nonblocking_get_pd(fpds, &current)
		if(temp==1){
			init_packet_descriptor(current);
			register_receiving_packetdescriptor(netdev, current);
			curPID = packet_descriptor_get_pid(todo);
			temp = nonblockingWriteBB(buf[curPID], todo);
			if(temp !=1){
				DIAGNOSTICS("RThread: Read failed on %u, buffer full, trying fpds\n", curPID);
				temp = nonblocking_put_pd(fpds, todo);
				if(temp!=1){
					DIAGNOSTICS("RThread: Packet Descriptor store failed on %u\n", curPID);
				}
			}
		}




		//need else if cuz could be both
		else if(nonblocking_get_pd(fpds, &current)){
			//literally do the exact same thing?
			init_packet_descriptor(current);
			register_receiving_packetdescriptor(netdev, current);
			curPID = packet_descriptor_get_pid(todo);
			temp = nonblockingWriteBB(buf[curPID], todo);
			if(temp !=1){
				DIAGNOSTICS("RThread: Read failed on %u, buffer full, trying fpds\n", curPID);
				temp = nonblocking_put_pd(fpds, todo);
				if(temp!=1){
					DIAGNOSTICS("RThread: Packet Descriptor store failed on %u\n", curPID);
				}
			}

		}



		else{
			temp = nonblocking_put_pd(fpds, todo);
			if(temp!=1){
				DIAGNOSTICS("RThread: Packet Descriptor store failed on %u\n", curPID);
			}
			current = todo;
			register_receiving_packetdescriptor(netdev, current);
		}



	}



	return NULL;
}




void* send_thread(){
	PacketDescriptor *current;
	int i, temp;

	while(true){
		current = blockingReadBB(toSend);
		temp = send_packet(netdev, current);
		if (temp == 1){
			DIAGNOSTICS("SThread: Packet sent!\n");
		}

		else{
			for(i=0; i<10;i++){

				do_nanosleep(100000);
				temp = send_packet(netdev, current);
				if (temp == 1){
					DIAGNOSTICS("SThread: Packet sent!\n");
					break;
				}
			}
		}


		temp = nonblockingWriteBB(toReceive, current);
		if(temp!=1){
			temp = nonblocking_put_pd(fpds,current);
			if(temp!=1){
				DIAGNOSTICS("RThread: Packet Descriptor store failed\n");
			}
		}
		return NULL;
	}}
}