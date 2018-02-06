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

/********* STUDENTS WRITE THE NEXT SIX ROUTINES *********/

#define BUFFER_SIZE 1000
float increment = 100.0; // starttimer(0,increment);

int get_checksum(struct pkt packet);
struct pkt *a_Make_Packet(char A_queue[][20], int A_head);
struct pkt *b_Make_Packet(int B_w_seqnum);

char A_queue[BUFFER_SIZE][20]; 
int head = 0;   //the head of A_queue. seqnum = head%2. add one when A_input get right ack
int rear = -1;  //the rear of A_queue, rear-head+1: number of the message in A_queue
int A_wait_ack = 0; // odd wait 0, even wait 1
int A_wait_seq = 0; // odd wait 0, even wait 1

struct pkt A_sndpkt;
struct pkt B_sndpkt;


//////////////////start A/////////////////////////////////////////
/* called from layer 5, passed the data to be sent to other side */
void A_output(message)
  struct msg message;
{
  printf("************************into A_output****************************\n");
  printf("A_wait_ack: %d\nA_wait_seq %d\n", A_wait_ack, A_wait_seq);
  if(A_wait_ack == A_wait_seq){
    if(head <= rear) {  //queue is not empty
      printf("queue is not empty, save message\n");
      rear+=1;
      strncpy(A_queue[rear], message.data, 20);
    }else{  //queue is empty. in this situation rear == head - 1
      printf("queue is empty, ready to send packet\n");
      rear = head;               
      strncpy(A_queue[rear], message.data, 20);
      A_sndpkt = *a_Make_Packet(A_queue, head);
      tolayer3(0,A_sndpkt);
      printf("send packet %d and starttimer\n", head % 2);
      starttimer(0,increment);
      A_wait_seq = (head + 1) % 2;  // next sequence
    }
  }else{    //A_wait_ack != A_wait_seq
    printf("a packet in link, save message\n");
    rear+=1;
    strncpy(A_queue[rear], message.data, 20);
  }
  printf("A_wait_ack: %d\nA_wait_seq %d\n", A_wait_ack, A_wait_seq);
  printf("************************END A_output****************************\n");
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;
{ 
  printf("************************into A_input****************************\n");   
  printf("A_wait_ack: %d\nA_wait_seq %d\n", A_wait_ack, A_wait_seq);
  if(get_checksum(packet) == packet.checksum && packet.acknum == A_wait_ack) {  
    printf("A get right ack %d\n", packet.acknum); 
    stoptimer(0);
    printf("stoptimer\n");
    head+=1;
    A_wait_ack = head % 2;   // next ack
    if( head <= rear ){ //queue is not empty   
      printf("queue is not empty\n");
      A_sndpkt = *a_Make_Packet(A_queue, head);
      tolayer3(0, A_sndpkt);
      starttimer(0,increment); 
      printf("Send packet %d and starttimer\n", head % 2);
      A_wait_seq = (head + 1) % 2;
      printf("a_wait_seq: %d\n", A_wait_seq);
    }else{
      printf("the queue is empty\n");
    }
  }else{
    printf("packet corrupted or wrong packet. ignore it\n");
  } 
  printf("A_wait_ack: %d\nA_wait_seq %d\n", A_wait_ack, A_wait_seq);
  printf("************************END A_input****************************\n");
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  printf("************************into A_timerinterrupt****************************\n");
  printf("resent packet %d\n", A_sndpkt.seqnum);
  tolayer3(0,A_sndpkt); 
  printf("starttimer\n");
  starttimer(0,increment);
  printf("************************END A_timerinterrupt****************************\n");
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{

}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

////////////////END A//////////////////////////////////

///////////////START B////////////////////////////////
int B_wait_seqnum = 0;

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
{ 
  printf("************************into B_input****************************\n");
  printf("B_wait_seqnum: %d\n", B_wait_seqnum);
  if(get_checksum(packet) == packet.checksum && packet.seqnum == B_wait_seqnum ){
    printf("B get expected packet %d\n", packet.seqnum);
    tolayer5(1,packet.payload);
    B_sndpkt = *b_Make_Packet( B_wait_seqnum );
    printf("B send right ack %d back\n", B_wait_seqnum % 2 );
    tolayer3(1,B_sndpkt);
    B_wait_seqnum = (B_wait_seqnum + 1) % 2;
  }else{
    printf("B get corrupted or wrong packet\n");
    B_sndpkt = *b_Make_Packet(((B_wait_seqnum + 1) % 2));
    tolayer3(1, B_sndpkt); 
    printf("B send ack %d back\n", ((B_wait_seqnum + 1) % 2) );
  }
  printf("B_wait_seqnum: %d\n", B_wait_seqnum);
  printf("************************END B_input****************************\n");
}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{

}
///////////B END HERE////////////////////////
///////////FUNCTION/////////////////

struct pkt *a_Make_Packet(char A_queue[][20], int A_head){
  struct pkt *packet;
  packet = (struct pkt *)malloc(sizeof(struct pkt));
  packet->seqnum = A_head % 2;
  packet->acknum = A_head % 2;
  strncpy(packet->payload, A_queue[head], 20);
  packet->checksum = get_checksum(*packet);
  return packet;
}

struct pkt *b_Make_Packet(int B_w_seqnum){
  struct pkt *packet;
  packet = (struct pkt *)malloc(sizeof(struct pkt));
  packet->seqnum = B_w_seqnum % 2;
  packet->acknum = B_w_seqnum % 2;
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


