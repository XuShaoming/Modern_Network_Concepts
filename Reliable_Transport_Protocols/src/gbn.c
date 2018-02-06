#include "../include/simulator.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional data transfer 
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/
#define BUFFER_SIZE 1000
float increment = 100.0; // starttimer(0,increment);

struct A_node {
  float timeout;
  struct pkt sndpkt;
};

struct pkt *a_Make_Packet(struct msg message, int rear);
struct pkt *b_Make_Packet(int expectedseqnum);
int get_checksum(struct pkt packet);

int window_size;
int head = 0;
int nextseqnum = 0;
int rear = -1;
int w_last;  // last node of window
struct A_node A_queue[BUFFER_SIZE];
struct pkt B_sndpkt;

/* called from layer 5, passed the data to be sent to other side */
void A_output(message)
  struct msg message;
{
  printf("***********************into A_output********************\n");
  rear += 1;
  A_queue[rear].sndpkt = *a_Make_Packet(message, rear);
  if(nextseqnum <= w_last && nextseqnum <= rear){
    printf("send #%d packet to B\n", nextseqnum);
    tolayer3(0,A_queue[nextseqnum].sndpkt);
    A_queue[nextseqnum].timeout = get_sim_time() + increment;
    printf("packet #%d timeout at %f\n", nextseqnum, A_queue[nextseqnum].timeout);
    if(head == nextseqnum){  // here the nextseqnum must be equal rear
      starttimer(0,increment);
    }
    nextseqnum += 1;
  }
  printf("head: %d\tnextseqnum: %d\trear: %d\nw_last: %d\n", head, nextseqnum, rear, w_last);
  printf("***********************out A_output********************\n");
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;
{
  printf("***********************into A_input********************\n");
  float gap = 0.0;
  if( get_checksum(packet) == packet.checksum ){
    printf("packet.acknum: %d\nhead: %d\n", packet.acknum, head);
    head = packet.acknum + 1;
    w_last = head + window_size - 1;
    if(head == nextseqnum){
      stoptimer(0);
      printf("A_input head==nextseqnum\nhead= %d\nnextseqnum = %d\n rear=%d\n", head, nextseqnum, rear);
      if(head <= rear){
        starttimer(0, increment);
        while(nextseqnum <= w_last && nextseqnum <= rear){
          printf("send #%d packet to B\n", nextseqnum);
          tolayer3(0,A_queue[nextseqnum].sndpkt);
          A_queue[nextseqnum].timeout = get_sim_time() + increment;
          printf("packet #%d timeout at %f\n", nextseqnum ,A_queue[nextseqnum].timeout);
          nextseqnum += 1;
        }
      }else{
        printf("A_queue is empty now\n");
      }
    }else{   // some packets still not been acked
      stoptimer(0);
      printf("node #%d timeout at %f\ncurrent time is %f\n", head, A_queue[head].timeout, get_sim_time());
      gap = A_queue[head].timeout - get_sim_time();
      starttimer(0, gap);
      while(nextseqnum <= w_last && nextseqnum <= rear){
        printf("send #%d packet to B\n", nextseqnum);
        tolayer3(0,A_queue[nextseqnum].sndpkt);
        A_queue[nextseqnum].timeout = get_sim_time() + increment;
        printf("packet #%d timeout at %f\n", A_queue[nextseqnum].timeout);
        nextseqnum += 1;
      }
    }
  }else{
    printf("packet is corrupted\n");
  }
  printf("head %d\tnextseqnum: %d\trear: %d\n", head, nextseqnum, rear);
  printf("***********************out A_input********************\n");
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  printf("***********************into A_timerinterrupt********************\n");
  printf("packet #%d timeout at %f\n get_sim_time = %f\n",head, A_queue[head].timeout ,get_sim_time() );
  starttimer(0,increment);
  int i = head;
  while(i < nextseqnum){
    printf("i %d send# %d packet to B\n", i, A_queue[i].sndpkt.seqnum);
    tolayer3(0,A_queue[i].sndpkt);
    A_queue[i].timeout = get_sim_time() + increment;
    i++;
  }
  printf("head: %d\nnextseqnum: %d\nrear: %d\nw_last: %d\n", head, nextseqnum, rear, w_last);
  printf("***********************out A_timerinterrupt********************\n");
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  printf("***********************into A_init********************\n");
  window_size = getwinsize();
  w_last = head + window_size - 1;
  printf("***********************out A_init********************\n");
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */
int expectedseqnum = 0;
/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
{
  printf("***********************into B_input********************\n");

  if(get_checksum(packet) == packet.checksum){  // not corrupted
    if(packet.seqnum == expectedseqnum){
      printf("\nB get right packet\n");
      printf("packet.seqnum: %d\nexpectedseqnum: %d\n", packet.seqnum, expectedseqnum);
      tolayer5(1,packet.payload);
      B_sndpkt = *b_Make_Packet( expectedseqnum );
      tolayer3(1,B_sndpkt);
      printf("Send ack %d back to A_input\n", expectedseqnum);
      expectedseqnum += 1;
    }else{
      printf("\nB get wrong packet %d\n", packet.seqnum);
      B_sndpkt = *b_Make_Packet( expectedseqnum - 1 );
      printf("B return ack: %d\n", expectedseqnum - 1 );
      tolayer3(1, B_sndpkt);
    }   
  }else{  //corrupted
    printf("B get corrupted packet\n");
  }
  printf("expectedseqnum: %d\n", expectedseqnum);
  printf("***********************out B_input********************\n");
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
  printf("***********************into B_init********************\n");
  printf("***********************out B_init********************\n");
}


struct pkt *a_Make_Packet(struct msg message, int rear){
  struct pkt *packet;
  packet = (struct pkt *)malloc(sizeof(struct pkt));
  packet->seqnum = rear;
  packet->acknum = rear;
  strncpy(packet->payload, message.data, 20);
  packet->checksum = get_checksum(*packet);
  return packet;
}

struct pkt *b_Make_Packet(int expectedseqnum){
  struct pkt *packet;
  packet = (struct pkt *)malloc(sizeof(struct pkt));
  packet->seqnum = expectedseqnum;
  packet->acknum = expectedseqnum;
  packet->checksum = get_checksum(*packet);
  return packet;
}

int get_checksum(struct pkt packet){
  int checksum = 0;
  int i;
  checksum = packet.seqnum + packet.acknum;
  char temp[21];
  memset(temp,'\0', 21);
  strncpy(temp, packet.payload, 20);
  for(i=0; i <20 && temp[i] != '\0'; i++){
    checksum += temp[i];
  }
  return checksum;
}
