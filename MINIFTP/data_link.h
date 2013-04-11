//
//  data_link.h
//  MINIFTP

#ifndef data_link
#define data_link


#include "event_queue.h"
typedef enum{frame_arrival, cksum_err, timeout, network_layer_ready, dl_die} event_type;

#define CHECK_SUM_LENGTH 2
#define MAX_FRAME_SPLIT 4


/* From protocol.h */
//typedef unsigned int seq_nr; // moved to header
typedef enum{data,ack} frame_kind;

typedef struct{
    frame_kind kind;
    seq_nr seq;
    seq_nr ack;
    char info[PAYLOAD_SIZE];
    char checkSum[2];
    int remaining;
} frame;

extern int toDL[2];
extern int fromDL[2];
extern int signalFromDL[2];
extern int networkEnabled;
extern int errorRate;



/* Wait for an event to happen; return its type in event */
void wait_for_event(event_type *event, int sock);
/* Fetch a packet from the network layer for transmission on the channel */
void from_network_layer(packet *p);
/* Deliver information from an inbound frame to the network layer. */
void to_network_layer(vector<frame> framesToNL);
/* Go get an inbound frame from the physical layer and copy it to r. */
void from_physical_layer(frame *r, int sock);
/* Pass the frame to the physical layer for transmission. */
void to_physical_layer(frame *s, int sock);
/* Start the clock running and enable the timeout event. */
void start_timer(seq_nr k);
/* Stop the clock and disable the timeout event. */
void stop_timer(seq_nr k);
/* Allow the network layer to cause a network layer ready event. */
void enable_network_layer(void);
/* Forbid the network layer from causing a network layer ready event. */
void disable_network_layer(void);
/* Macro inc is expanded in-line: increment k circularly. */
#define inc(k) if (k < MAX_SEQ) k = k + 1; else k = 0
/* End protocol.h */


#define MAX_SEQ 7

void protocol5(int sock); //removed network_fd see .cpp file
static bool between(seq_nr a, seq_nr b, seq_nr c);
int byteStuff(char *input, char *output);
void deStuff(vector <frame> partialPackets, packet *p);
int checksum(const char* input, int size, char result[CHECK_SUM_LENGTH]);
int makeFrame(packet packetData, frame framesArr[MAX_FRAME_SPLIT]);
void bzzzzzzzuppp(frame *f);
int checksumFrame(frame f);
int fragment(char *stuffedPacket, frame rawFrames[MAX_FRAME_SPLIT], int size);
int split(packet *p, frame buffer[], vector<frame> & reserved, int next_frame_to_send, seq_nr nbuffered);

#endif
