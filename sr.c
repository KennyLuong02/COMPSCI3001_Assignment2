#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "emulator.h"
#include "sr.h"

/* ******************************************************************
   Go Back N protocol.  Adapted from J.F.Kurose
   ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.2  

   Network properties:
   - one way network delay averages five time units (longer if there
   are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
   or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
   (although some can be lost).

   Modifications: 
   - removed bidirectional GBN code and other code not used by prac. 
   - fixed C style to adhere to current programming style
   - added GBN implementation
**********************************************************************/

#define RTT  16.0       /* round trip time.  MUST BE SET TO 16.0 when submitting assignment */
#define WINDOWSIZE 6    /* the maximum number of buffered unacked packet */
#define SEQSPACE 12      /* the min sequence space for GBN must be at least windowsize + 1 */
                        /* The minimum for selective repeat is WINDOWSIZE * 2*/
#define NOTINUSE (-1)   /* used to fill header fields that are not being used */

/* generic procedure to compute the checksum of a packet.  Used by both sender and receiver  
   the simulator will overwrite part of your packet with 'z's.  It will not overwrite your 
   original checksum.  This procedure must generate a different checksum to the original if
   the packet is corrupted.
*/
int ComputeChecksum(struct pkt packet)
{
  int checksum = 0;
  int i;

  checksum = packet.seqnum;
  checksum += packet.acknum;
  for ( i=0; i<20; i++ ) 
    checksum += (int)(packet.payload[i]);

  return checksum;
}

bool IsCorrupted(struct pkt packet)
{
  if (packet.checksum == ComputeChecksum(packet))
    return (false);
  else
    return (true);
}


/********* Sender (A) variables and functions ************/

static struct pkt buffer[WINDOWSIZE];  /* array for storing packets waiting for ACK */
static int windowfirst, windowlast;    /* array indexes of the first/last packet awaiting ACK */
static int windowcount;                /* the number of packets currently awaiting an ACK */
static int A_nextseqnum;               /* the next sequence number to be used by the sender */
int ACKarray[WINDOWSIZE];
int send_base;

/* called from layer 5 (application layer), passed the message to be sent to other side */
/*message is a structure containing data to be sent to B. This routine will be called 
whenever the upper layer application at the sending side (A) has a message to send.  
It is the job of the reliable transport protocol to insure that the data in such a message 
is delivered in-order, and correctly, to the receiving side upper layer. */
void A_output(struct msg message)
{
  struct pkt sendpkt;
  int i;

  /* if not blocked waiting on ACK */
  if ( windowcount < WINDOWSIZE) {

    /*Keep this the same*/

    if (TRACE > 1) /*This is for the level of detail in the terminal*/
      printf("----A: New message arrives, send window is not full, send new messge to layer3!\n");

    /* create packet */
    sendpkt.seqnum = A_nextseqnum; /*The current sequence number of the new packet becomes the next sequence number*/
    sendpkt.acknum = NOTINUSE;
    /*Load data into payload*/
    for (i=0; i<20 ; i++ ) 
      sendpkt.payload[i] = message.data[i];
    sendpkt.checksum = ComputeChecksum(sendpkt); /*Get the checksum of the packet*/





    /*//////////////////////////// Not sure about this bit
    Done*/

    /* put packet in window buffer */
    /* windowlast will always be 0 for alternating bit; but not for GoBackN */
    /*windowlast = (windowlast + 1) % WINDOWSIZE;*/
    /*To add the packet into the buffer and ACKarray to keep track
    of the ACK*/
    buffer[A_nextseqnum % WINDOWSIZE] = sendpkt;
    ACKarray[A_nextseqnum % WINDOWSIZE] = 0;
    windowcount++;

    /*////////////////////////////////////////


    // Keep this the same*/

    /* send out packet */
    if (TRACE > 0)
      printf("Sending packet %d to layer 3\n", sendpkt.seqnum);
    tolayer3 (A, sendpkt);
    

    /*////////////////////// Not sure about this bit
    // May need different timer, or use 1 timer as multiples*/

    /* start timer if first packet in window */
    if (windowcount == 1)
      starttimer(A,RTT);

    /*///////////////////////////////////*/


    /* get next sequence number, wrap back to 0 */
    A_nextseqnum = (A_nextseqnum + 1) % SEQSPACE; /*//Get the next sequence number for the next packet
                                                  //Sequence number has to be larger than window size 
                                                  //+1 to prevent confusion
                                                  //But for selective repeat, the SEQSPACE has to be double
                                                  //the window size*/



  }
  /* if blocked,  window is full*/
  /*// Keep this the same*/
  else {
    if (TRACE > 0)
      printf("----A: New message arrives, send window is full\n");
    window_full++;
  }
}


/* called from layer 3, when a packet arrives for layer 4 
   In this practical this will always be an ACK as B never sends data.
*/
/*This routine will be called whenever a packet sent from B 
(i.e., as a result of a tolayer3() being called by a B procedure) 
arrives at A. packet is the (possibly corrupted) packet sent from B.*/
void A_input(struct pkt packet)
{ /*//This is for A receiving a packet from B*/
  int ackcount = 0;
  int i;

  /*//If an ACK is received, the SR sender marks that packet as having been received,
  //provided it is in the window. If the packet’s sequence number is equal to send_
  //base , the window base is moved forward to the unacknowledged packet with the 
  //smallest sequence number. If the window moves and there are untransmitted packets
  //with sequence numbers that now fall within the window, these packets are transmitted.

  if received ACK is not corrupted 
  // Keep this*/
  if (!IsCorrupted(packet)) {
    if (TRACE > 0)
      printf("----A: uncorrupted ACK %d is received\n",packet.acknum);


    total_ACKs_received++; /*Not sure about this*/
    
    int ACKnum = packet.acknum;

    /*Check if this ACK is within window range*/
    if ((send_base <= ACKnum && ACKnum < send_base + WINDOWSIZE) ||
    ((send_base + WINDOWSIZE >= SEQSPACE) && (ACKnum < (send_base + WINDOWSIZE) % SEQSPACE))) {
      if (ACKarray[ACKnum] == 0) {
        ACKarray[ACKnum] = 1;
      }
    }


    /* check if new ACK or duplicate */
    if (windowcount != 0) { /*If there are still packets awaiting ACK*/

          int seqfirst = buffer[windowfirst].seqnum;
          int seqlast = buffer[windowlast].seqnum;
          /* check case when seqnum has and hasn't wrapped */
          if (((seqfirst <= seqlast) && (packet.acknum >= seqfirst && packet.acknum <= seqlast)) ||
              ((seqfirst > seqlast) && (packet.acknum >= seqfirst || packet.acknum <= seqlast))) {

            /* packet is a new ACK */
            if (TRACE > 0)
              printf("----A: ACK %d is not a duplicate\n",packet.acknum);
            new_ACKs++; /*This is for the final result so keep  it


            ////////////////////// Not sure about this

            // Does Selective repeat requires cumulative ACKs?
            Can comment this out but it does not do anything so ignore it*/

            /* cumulative acknowledgement - determine how many packets are ACKed */
            if (packet.acknum >= seqfirst) /*???*/
              ackcount = packet.acknum + 1 - seqfirst;
            else
              ackcount = SEQSPACE - seqfirst + packet.acknum;



            /*To turn the bit in the ACKarray for that packet to 1*/
            ACKarray[packet.acknum] = 1;

            /* Instead of slide window equating the number of ACKs have received,
            //in Selective repeat, the window only slides to the correctly
            //ordered ACKed*/
            for (i = 0; i<WINDOWSIZE; i++) {
              /*If the packet is ACKed*/
              if (ACKarray[i] == 1) {

                /* slide window until the last packet ACKed */
                /*windowfirst = (windowfirst + 1) % WINDOWSIZE;*/
                
                /* Move the buffer forward*/
                buffer[i] = buffer[i+1];

                
                /* delete the acked packets from window buffer */
                windowcount--;
                
                /* Move the ACKarray forward*/
                if ((i+1) == WINDOWSIZE) {
                  ACKarray[i] = 0;
                } else {
                  ACKarray[i] = ACKarray[i+1];
                }

              } else if (ACKarray[i] == 0) {
                /*If the packet is not ACKed*/
                break;
              }
            }


            /*If the window moves and there are unstranmitted packets with sequence numbers that are now fall
            into the window these packets are transmitted
            This has not been implemented in GobackN*/







            /*if (ACKflag == 1) {*/
              /*Then move the window*/
              /* slide window by the number of packets ACKed */
              /*windowfirst = (windowfirst + packet.acknum) % WINDOWSIZE;
            }*/
            /* delete the acked packets from window buffer */
            /*for (i=0; i<packet.acknum; i++)
              windowcount--; How many packets awaiting ACK*/

            
            /*// Implement 1 timer for multiples*/
	    /* start timer again if there are still more unacked packets in window */
            stoptimer(A);
            if (windowcount > 0)
              starttimer(A, RTT);

          }
        }
        
        /*//////////////////////////////////

        // Keep this*/
        else
          if (TRACE > 0)
        printf ("----A: duplicate ACK received, do nothing!\n");
  }
  else 
    if (TRACE > 0)
      printf ("----A: corrupted ACK is received, do nothing!\n");
}

/* called when A's timer goes off */
/*This routine will be called when A's timer expires
 (thus generating a timer interrupt). This routine controls 
 the retransmission of packets. See starttimer() and stoptimer()  
 below for how the timer is started and stopped.*/
void A_timerinterrupt(void)
{
  int i;

  if (TRACE > 0)
    printf("----A: time out,resend packets!\n");

    /*Gotta fix this for Selective repeat, only sends the unACKed ones
    No, only send the packet that is timeout, not all unACKed packets*/

  for(i=0; i<WINDOWSIZE; i++) {
    if (ACKarray[i]==0)  {/*Only send the packet that is not ACKed*/

      if (TRACE > 0)
        printf ("---A: resending packet %d\n", (buffer[(windowfirst+i) % WINDOWSIZE]).seqnum);

      tolayer3(A,buffer[(windowfirst+i) % WINDOWSIZE]); /*Buffer is the array that has the
                                                        //packets waiting for ACKs*/
      
      /*/////////////////////////////////Not sure about this*/
      packets_resent++;
      
      if (i==0) starttimer(A,RTT); /*Need to redo this timer probably*/

      /*///////////////////////////////////////////*/

    }
  }
}       



/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init(void)
{
  int i;

  /* initialise A's window, buffer and sequence number */
  A_nextseqnum = 0;  /* A starts with seq num 0, do not change this */
  windowfirst = 0;
  windowlast = -1;   /* windowlast is where the last packet sent is stored.  
		     new packets are placed in winlast + 1 
		     so initially this is set to -1
		   */
  windowcount = 0;

  send_base = 0;
  
  for (i = 0; i< WINDOWSIZE; i++) {
    ACKarray[i] = 0; /*This array is used for keeping track of al the ACKs
                                    0: is not ACKed and 1: is ACKed*/
  }
  
}



/********* Receiver (B)  variables and procedures ************/

static int expectedseqnum; /* the sequence number expected next by the receiver */
static int B_nextseqnum;   /* the sequence number for the next packets sent by B */

static struct pkt buffer_for_B[WINDOWSIZE+1];  /* array for storing packets waiting for ACK */
int ACKarray_for_B[WINDOWSIZE];



/* called from layer 3, when a packet arrives for layer 4 at B*/
/*This routine will be called whenever a packet sent from A 
(i.e., as a result of a  tolayer3() being called by a A-side procedure) 
arrives at B. The packet is the (possibly corrupted) packet sent from A. 

It is important to note that in Step 2 in Figure 3.25, the receiver reacknowledges
(rather than ignores) already received packets with certain sequence numbers below
the current window base.*/
void B_input(struct pkt packet)
{
  struct pkt sendpkt;
  int i;

  /* if not corrupted and received packet is in order 
  The SR receiver will acknowledge a correctly received packet whether or not it is in
  order. Out-of-order packets are buffered until any missing packets (that is, 
  packets with lower sequence numbers) are received, at which point a batch of 
  packets can be delivered in order to the upper layer.*/
  
  /*Save the packet straight into the array,
  then if the packet is in order, send it straight to layer 5,
  if it is out of order, then save it in its place in the array,
  then check the flag for the ACK, and only send the all to layer 5
  when receives the previous packets*/

  buffer_for_B[packet.seqnum - expectedseqnum] = packet; /*Save the packet into the buffer*/
  ACKarray_for_B[packet.seqnum - expectedseqnum] = 1;
  
  if  ( (!IsCorrupted(packet))  && (packet.seqnum == expectedseqnum) ) {
    if (TRACE > 0)
      printf("----B: packet %d is correctly received, send ACK!\n",packet.seqnum);
    packets_received++;

    /* Send all the packets that are in order*/
    for (i = 0; i<WINDOWSIZE; i++) {
      /*If the packet is ACKed*/
      if (ACKarray_for_B[i] == 1) {

        /* deliver to receiving application */
        tolayer5(B, buffer_for_B[i].payload); /*This is to layer 5*/

        /* Move the buffer forward*/
        buffer_for_B[i] = buffer_for_B[i+1];
        
        
        /* Move the ACKarray forward*/
        if ((i+1) == WINDOWSIZE) {
          ACKarray_for_B[i] = 0;
        } else {
          ACKarray_for_B[i] = ACKarray_for_B[i+1];
        }

      } else if (ACKarray_for_B[i] == 0) {
        /*If the packet is not ACKed*/
        break;
      }
    }



    /* send an ACK for the received packet */
    sendpkt.acknum = expectedseqnum;

    /* update state variables */
    expectedseqnum = (expectedseqnum + 1) % SEQSPACE; ???
  }
  else if ((IsCorrupted(packet))){
    /* packet is corrupted or out of order resend last ACK
    
    But if the packet is corrupted, do nothing*/
    /*if (TRACE > 0) 
      printf("----B: packet corrupted or not expected sequence number, resend ACK!\n");
    if (expectedseqnum == 0) /*If the next expected sequence number is 0,
                             //it is relooped and send 6*/
    /*  sendpkt.acknum = SEQSPACE - 1;
    else
    /* If it is not 0, then just send the last ACK*/
    /*  sendpkt.acknum = expectedseqnum - 1;*/

    if (TRACE > 0) 
      printf("----B: packet corrupted, do nothing!\n");
    
  } else {
    /*Else if the packet is out of order
    if it is out of order, then save it in its place in the array,
    then check the flag for the ACK, and only send the all to layer 5
    when receives the previous packets*/


    
    
    if (expectedseqnum == 0) /*If the next expected sequence number is 0,
                             //it is relooped and send 6*/
      sendpkt.acknum = SEQSPACE - 1;
    else
    /* If it is not 0, then just send the last ACK*/
      sendpkt.acknum = expectedseqnum - 1;

    A_nextseqnum = (A_nextseqnum + 1) % SEQSPACE;
  }

  /* create packet */
  sendpkt.seqnum = B_nextseqnum;
  B_nextseqnum = (B_nextseqnum + 1) % 2;


  
  /*Keep this*/
  /* we don't have any data to send.  fill payload with 0's */
  for ( i=0; i<20 ; i++ ) 
    sendpkt.payload[i] = '0';  

  /* computer checksum */
  sendpkt.checksum = ComputeChecksum(sendpkt); 

  /* send out packet */
  tolayer3 (B, sendpkt);
}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init(void)
{
  int i;
  expectedseqnum = 0;
  B_nextseqnum = 1;

  for (i = 0; i< WINDOWSIZE; i++) {
    ACKarray_for_B[i] = 0; /*This array is used for keeping track of al the ACKs
                                    0: is not ACKed and 1: is ACKed*/
  }
}

/******************************************************************************
 * The following functions need be completed only for bi-directional messages *
 *****************************************************************************/

/* Note that with simplex transfer from a-to-B, there is no B_output() */
void B_output(struct msg message)  
{
}

/* called when B's timer goes off */
void B_timerinterrupt(void)
{
}


/*The unit of data passed between the application layer and the
 transport layer protocol is a message, which is declared as: 
 5 to 4
*/
/*struct msg {
  char data[20]; //A char is 1 byte
                 //Array of 20 chars is initialised, thus 20 bytes
};
*/

/*That is, data is stored in a msg structure which contains
 an array of 20 chars. A char is one byte. The sending entity 
 will thus receive data in 20-byte chunks from the sending application, 
 and the receiving entity should deliver 20-byte chunks to the receiving application.

The unit of data passed between the transport layer and the network layer is a 
packet, which is declared as: 
4 to 3
*/
/*struct pkt {
   int seqnum;
   int acknum;
   int checksum;
   char payload[20];
};
*/

/*The A_output() routine fills in the payload field from the message 
data passed down from the Application layer. The other packet fields 
must be filled in the a_output routine so that they can be used by the 
reliable transport protocol to insure reliable delivery, as we've seen in class.
*/