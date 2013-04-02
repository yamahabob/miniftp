//
//  data_link.h
//  MINIFTP

#ifndef __MINIFTP__data_link__
#define __MINIFTP__data_link__

#include "header.h"
#include <iostream>

typedef enum{frame_arrival, cksum_err, timeout, network_layer_ready} event_type;

#define CHECK_SUM_LENGTH 2
#define MAX_FRAME_SPLIT 4


/* From protocol.h */
typedef unsigned int seq_nr;
typedef enum{data,ack} frame_kind;

typedef struct{
    frame_kind kind;
    seq_nr seq;
    seq_nr ack;
    char info[PAYLOAD_SIZE];
} frame;


/* Wait for an event to happen; return its type in event */
void wait_for_event(event_type *event);
/* Fetch a packet from the network layer for transmission on the channel */
void from_network_layer(packet *p);
/* Deliver information from an inbound frame to the network layer. */
void to_network_layer(char *p);
/* Go get an inbound frame from the physical layer and copy it to r. */
void from_physical_layer(frame *r, int sock);
/* Pass the frame to the physical layer for transmission. */
void to_physical_layer(frame *s, int sock);
/* Start the clock running and enable the timeout event. */
void start_timer(seq_nr k);
/* Stop the clock and disable the timeout event. */
void stop_timer(seq_nr k);
/* Start an auxiliary timer and enable the ack timeout event. */
void start_ack_timer(void);
/* Stop the auxiliary timer and disable the ack timeout event. */
void stop_ack_timer(void);
/* Allow the network layer to cause a network layer ready event. */
void enable_network_layer(void);
/* Forbid the network layer from causing a network layer ready event. */
void disable_network_layer(void);
/* Macro inc is expanded in-line: increment k circularly. */
#define inc(k) if (k < MAX_SEQ) k = k + 1; else k = 0
/* End protocol.h */


#define MAX_SEQ 7

void protocol5(int fd, int sock);
static bool between(seq_nr a, seq_nr b, seq_nr c);
int byteStuff(char *input, char *output);
int checksum(const char* input, int size, char result[3]);
int makeFrame(packet packetData, frame framesArr[MAX_FRAME_SPLIT]);

#endif 
