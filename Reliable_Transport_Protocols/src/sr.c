#include "../include/simulator.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

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

struct Node{
  bool  received;
  struct pkt sndpkt;
};

struct QNode{
  float timeout;      // system current time + increment
  struct pkt sndpkt;
  struct QNode *next;
};

struct pkt *a_Make_Packet(struct msg message, int rear);
struct pkt *b_Make_Packet(int expectedseqnum);
int get_checksum(struct pkt packet);
struct QNode* newQNode(struct Node element);
int enQueue(struct QNode *Aq_head, struct QNode element);
int queueLength(struct QNode *Aq_head);
struct QNode *deQueue_acknum(struct QNode *Aq_head, int acknum);
struct QNode *deQueue(struct QNode *Aq_head);

int window_size;
int A_head = 0;
int A_nextseqnum = 0;
int A_rear = -1;
int A_w_last; // last node of window
struct Node A_buffer[BUFFER_SIZE];
struct QNode *Aq_head;

struct QNode A_QNode;

/* called from layer 5, passed the data to be sent to other side */

void A_output(message)
  struct msg message;
{
  // save message in buffer
  printf("***********************into A_output********************\n");
  A_rear += 1;
  A_buffer[A_rear].sndpkt = *a_Make_Packet(message, A_rear);
  A_buffer[A_rear].received = false;

  if(A_nextseqnum <= A_w_last && A_nextseqnum <= A_rear){
    A_QNode = *newQNode(A_buffer[A_nextseqnum]);
    printf("send #%d packet to B\n", A_nextseqnum);
    tolayer3(0,A_QNode.sndpkt);
    A_QNode.timeout = get_sim_time() + increment;
    if( enQueue(Aq_head ,A_QNode) == 1 ){
      printf("insert #%d packet into Linklist\n", A_nextseqnum);
    }
    if(A_head == A_nextseqnum){
      printf("A_head == A_nextseqnum, starttimer\n");
      starttimer(0, increment);   //have problem here. see note. its simple do tomorrow
    }
    A_nextseqnum += 1;
  }else{
    printf("A_nextseqnum >= A_w_last OR >= A_rear\n");
  }
  printf("head: %d\nnextseqnum: %d\nrear: %d\nw_last: %d\n", A_head, A_nextseqnum, A_rear, A_w_last);
  printf("***********************out A_output********************\n");
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;
{
printf("***********************into A_input********************\n");

  float gap = 0.0;
  if(get_checksum(packet) == packet.checksum){  // not corrupted
    printf("get packet #%d\n", packet.seqnum);
    A_buffer[packet.seqnum].received = true;  // packet.acknum is same as packet.seqnum
    
    if(Aq_head->next->sndpkt.seqnum == packet.acknum){
      stoptimer(0);
      printf("delete first node in linklist\n");
      deQueue(Aq_head);
      if(Aq_head->next != NULL){
        printf("start next node timer\n");
        gap = Aq_head->next->timeout - get_sim_time();
        starttimer(0, gap);
      }else{
        printf("Linklist is empty\n");
      }
    }else{
      if(deQueue_acknum(Aq_head, packet.acknum) != NULL){
        printf("delete QNode #%d\n in Linklist", packet.seqnum);
      }
    }

    if(A_head == packet.seqnum){
      while(A_buffer[A_head].received == true){
        printf("Move A_head from %d \n", A_head);
        A_head += 1;
        A_w_last += 1;
        if(A_nextseqnum <= A_w_last && A_nextseqnum <= A_rear){
          // Send packet to B
          A_QNode = *newQNode(A_buffer[A_nextseqnum]);
          printf("send #%d packet to B\n", A_nextseqnum);
          tolayer3(0,A_QNode.sndpkt);
          A_QNode.timeout = get_sim_time() + increment;
          if(Aq_head->next ==NULL){ //Linklist is empty
            starttimer(0, increment);
            enQueue(Aq_head ,A_QNode);
            printf("insert #%d packet to be the first node of linklist\n", A_nextseqnum);
          }
          enQueue(Aq_head ,A_QNode);
          printf("attarch #%d packet to the rear of linklist\n", A_nextseqnum);
          A_nextseqnum += 1;
          // End send packet to B
        }      
      } // end while
      printf("head stop at #%d\n", A_head);
      if(A_head <= A_rear){
        printf("has packets not been acked\n");
      }else{
        printf("A_buffer is empty\n");
      }
    } // end if
  }else{  
    printf("packet corrupted\n");
  }

  printf("***********************out A_input********************\n");
}

/* called when A's timer goes off */

void A_timerinterrupt()
{
  printf("***********************into A_timerinterrupt********************\n");
  float gap = 0.0;
  struct QNode *temp;
  temp = deQueue(Aq_head);
  printf("packet #%d timeout\n", temp->sndpkt.seqnum);
  if(temp !=NULL){
    tolayer3(0,temp->sndpkt);
    temp->timeout = get_sim_time() + increment;
    temp->next = NULL;
    enQueue(Aq_head, *temp);
    gap = Aq_head->next->timeout - get_sim_time();
    printf("start packet #%d timer\n", Aq_head->next->sndpkt.seqnum);
    starttimer(0, gap);
  }else{
    printf("ERROR\n");
  }
  printf("***********************out A_timerinterrupt********************\n");
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  printf("***********************into A_init********************\n");
  window_size = getwinsize();
  printf("window size %d\n", window_size);
  A_w_last = A_head +window_size - 1;
  Aq_head = (struct QNode *) malloc(sizeof(struct QNode));
  Aq_head->next = NULL;
  int i = 0;
  for(i=0; i<BUFFER_SIZE; i++){
    A_buffer[i].received = false;
  }
  printf("***********************out A_init********************\n");
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */


int B_head = 0;
int B_w_last;  // last node of window
struct pkt B_sndpkt;
struct Node B_buffer[BUFFER_SIZE];
/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
{
  printf("***********************into B_input********************\n");
  if(get_checksum(packet) == packet.checksum){  // not corrupted
    if( B_head<= packet.seqnum && packet.seqnum <= B_w_last){
      //Send Ack back
      printf("Send ACK %d back\n", packet.seqnum);
      B_sndpkt = *b_Make_Packet( packet.seqnum );
      tolayer3(1,B_sndpkt);
      // save in B_buffer
      if( B_buffer[packet.seqnum].received == false){  //not previously received
        B_buffer[packet.seqnum].received = true;
        B_buffer[packet.seqnum].sndpkt = packet;
        printf("buffer packet\n");
      }else{
        printf("previously received packet\n");
      }
      if(packet.seqnum == B_head){
        printf("Send packets to layer5 from #%d packet\n", B_head);
        while(B_buffer[B_head].received == true){
          tolayer5(1, B_buffer[B_head].sndpkt.payload);
          B_head += 1;
          B_w_last += 1;
        }
        printf("Expected #%d packet\n", B_head);
      }
    }else if( B_head - window_size <= packet.seqnum && packet.seqnum <= B_head - 1){
      //Send Ack back
      B_sndpkt = *b_Make_Packet( packet.seqnum );
      tolayer3(1,B_sndpkt);
    }else{
      printf("ignore packet\n");
    }
  }else{  //corrupted
    printf("packet corrupted\n");
  }
  printf("***********************out B_input********************\n");
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
  printf("***********************into B_init********************\n");
  window_size = getwinsize();
  printf("window size %d\n", window_size);
  B_w_last = B_head + window_size - 1;
  int i;
  for(i=0; i<BUFFER_SIZE; i++){
    B_buffer[i].received = false;
  }
  printf("***********************out B_init********************\n");
}


//////////Function start here////////////////

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
// Linklist queue learned from http://www.geeksforgeeks.org/queue-set-2-linked-list-implementation/

struct QNode* newQNode(struct Node element){
  struct QNode *temp = (struct QNode*)malloc(sizeof(struct QNode));
  temp->sndpkt = element.sndpkt;
  temp->next = NULL;
  return temp; 
}

int enQueue(struct QNode *Aq_head, struct QNode element){
  struct QNode *temp = (struct QNode*)malloc(sizeof(struct QNode));
  *temp = element;
  temp->next = NULL;

  struct QNode *p, *q;
  p = Aq_head;
  q = Aq_head->next;
  while(q){
    p=p->next;
    q=q->next;
  }
  temp->next = p->next;
  p->next = temp;
  return 1;
}

int queueLength(struct QNode *Aq_head){
  struct QNode *p;
  p = Aq_head->next;
  int i;
  while(p){
    p = p->next;
    i++;
  }
  return i;
}

struct QNode *deQueue_acknum(struct QNode *Aq_head, int acknum){
  struct QNode *p, *q;
  p = Aq_head;
  q = Aq_head->next;

  while(q && q->sndpkt.seqnum != acknum){
    p = p->next;
    q = q->next;
  }
  if(q == NULL){
    printf("Do not have Node acknum equal to %d\n", acknum);
    return q;
  }else{
    p->next = q->next;
    q->next = NULL;
    return q;
  }
}

struct QNode *deQueue(struct QNode *Aq_head){
  struct QNode *p, *q;
  p = Aq_head;
  q = Aq_head->next;

  if(q == NULL){
    printf("Linklist is empty, nothing to delete\n");
    return q;
  }

  p->next = q->next;
  q->next = NULL;
  return q;
}




