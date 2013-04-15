//
// data_link.h
// MINIFTP
// CS513-TEAM2

#ifndef data_link
#define data_link


#include "event_queue.h"
typedef enum{frame_arrival, timeout, network_layer_ready, dl_die} event_type;

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
extern int killDL[2];
extern int networkEnabled;
extern int errorRate;
extern string activeUser;



//Primary Author: Salman
/* Wait for an event to happen; return its type in event */
void wait_for_event(event_type *event, int sock);

//Primary Author: Curtis
/* Fetch a packet from the network layer for transmission on the channel */
void from_network_layer(packet *p);

//Primary Author: Curtis
/* Deliver information from an inbound frame to the network layer. */
void to_network_layer(vector<frame> framesToNL);

//Primary Author: Curtis
/* Go get an inbound frame from the physical layer and copy it to r. */
void from_physical_layer(frame *r, int sock);

//Primary Author: Curtis
/* Pass the frame to the physical layer for transmission. */
void to_physical_layer(frame *s, int sock);

//Primary Author: Salman
/* Start the clock running and enable the timeout event. */
void start_timer(seq_nr k);

//Primary Author: Salman
/* Stop the clock and disable the timeout event. */
void stop_timer(seq_nr k);

//Primary Author: Curtis
/* Allow the network layer to cause a network layer ready event. */
void enable_network_layer(void);

//Primary Author: Curtis
/* Forbid the network layer from causing a network layer ready event. */
void disable_network_layer(void);


/* Macro inc is expanded in-line: increment k circularly. */
#define inc(k) if (k < MAX_SEQ) k = k + 1; else k = 0
#define dec(k) if (k > 0) k = k - 1; else k = MAX_SEQ
/* End protocol.h */


#define MAX_SEQ 7
#define FRAME_SIZE 120

//Primary Author: Curtis
void protocol5(int type, int sock); //removed network_fd see .cpp file

//Primary Author: Curtis
static bool between(seq_nr a, seq_nr b, seq_nr c);

//Primary Author: Salman
int byteStuff(char *input, char *output);

//Primary Author: Salman
void deStuff(vector <frame> partialPackets, packet *p);

//Primary Author: Salman
int checksum(int seq, int kind, int remaining, const char* input, int size, char result[CHECK_SUM_LENGTH]);

//Primary Author: Curtis
int makeFrame(packet packetData, frame framesArr[MAX_FRAME_SPLIT]);

//Primary Author: Curtis
void bzzzzzzzuppp(frame *f);

//Primary Author: Salman
int checksumFrame(frame f);

//Primary Author: Salman
int fragment(char *stuffedPacket, frame rawFrames[MAX_FRAME_SPLIT], int size);

//Primary Author: Salman
int split(packet *p, frame buffer[], vector<frame> & reserved, int next_frame_to_send, seq_nr nbuffered);

//Primary Author: Curtis
void sendAck(frame *r, int sock);

//Primary Author: Curtis
void log(int type);

#endif
