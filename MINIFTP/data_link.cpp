//
//  data_link.cpp
//  MINIFTP
//
//  Created by Curtis Taylor on 3/21/13.
//  Copyright (c) 2013 WPI. All rights reserved.
//

#include "data_link.h"

static bool between(seq_nr a, seq_nr b, seq_nr c){
    /* Return true if a <= b < c circularly; false otherwise. */
    if (((a <= b) && (b < c)) || ((c < a) && (a <= b)) || ((b < c) && (c < a)))
        return(true);
    else
        return(false);
}
static void send_data(seq_nr frame_nr, seq_nr frame_expected, packet buffer[], int sock){
    /* Construct and send a data frame. */
    frame s; /* scratch variable */
    //s.info = buffer[frame_nr]; /* insert packet into frame */
    
    
    // 1. Byte stuffing here
    // 2. FRAGMENTATION OCCURS here
    //strcpy(s.info,buffer[frame_nr].data);
    
    s.seq = frame_nr; /* insert sequence number into frame */
    s.ack = (frame_expected + MAX_SEQ) % (MAX_SEQ + 1); /* piggyback ack */
    to_physical_layer(&s,sock); /* transmit the frame */
    start_timer(frame_nr); /* start the timer running */
}

void protocol5(int network_fd, int sock){
    seq_nr next_frame_to_send; /* MAX SEQ > 1; used for outbound stream */
    //pipe
    seq_nr ack_expected; /* oldest frame as yet unacknowledged */
    seq_nr frame_expected; /* next frame expected on inbound stream */
    frame r; /* scratch variable */
    packet buffer[MAX_SEQ + 1]; /* buffers for the outbound stream */
    seq_nr nbuffered; /* number of output buffers currently in use */
    seq_nr i; /* used to index into the buffer array */
    event_type event;
    enable_network_layer(); /* allow network layer ready events */
    ack_expected = 0; /* next ack expected inbound */
    next_frame_to_send = 0; /* next frame going out */
    frame_expected = 0; /* number of frame expected inbound */
    nbuffered = 0; /* initially no packets are buffered */
    while (true) {
        wait_for_event(&event); /* four possibilities: see event type above */
        switch(event) {
            case network_layer_ready: /* the network layer has a packet to send */
                /* Accept, save, and transmit a new frame. */
                from_network_layer(&buffer[next_frame_to_send]); /* fetch new packet */
                nbuffered = nbuffered + 1; /* expand the sender’s window */
                send_data(next_frame_to_send, frame_expected, buffer, sock);/* transmit the frame */
                inc(next_frame_to_send); /* advance sender’s upper window edge */
                break;
            case frame_arrival: /* a data or control frame has arrived */
                from_physical_layer(&r,sock); /* get incoming frame from physical layer */
                if (r.seq == frame_expected) {
                    /* Frames are accepted only in order. */
                    to_network_layer(r.info); /* pass packet to network layer */
                    inc(frame_expected); /* advance lower edge of receiver’s window */
                }
                /* Ack n implies n − 1, n − 2, etc. Check for this. */
                while (between(ack_expected, r.ack, next_frame_to_send)) {
                    /* Handle piggybacked ack. */
                    nbuffered = nbuffered - 1; /* one frame fewer buffered */
                    stop_timer(ack_expected); /* frame arrived intact; stop timer */
                    inc(ack_expected); /* contract sender’s window */
                }
                break;
            case cksum_err: break; /* just ignore bad frames */
            case timeout: /* trouble; retransmit all outstanding frames */
                next_frame_to_send = ack_expected; /* start retransmitting here */
                for (i = 1; i <= nbuffered; i++) {
                    send_data(next_frame_to_send, frame_expected, buffer, sock);/* resend frame */
                    inc(next_frame_to_send); /* prepare to send the next one */
                }
        }
        if (nbuffered < MAX_SEQ)
            enable_network_layer();
        else
            disable_network_layer();
    }
}
void wait_for_event(event_type *event){
///*
//#include <sys/types.h>
//#include <sys/time.h>
//    
//#define cchMax 256        
//    
//#define fdIn 0
//#define fdOut 1
//    
//    {
//        int pid;
//        int fdServer;
//        char sb[cchMax];
//        char ch;
//        int cch;
//        fd_set bvfdRead;
//        
//        
//        FD_ZERO(&bvfdRead);
//        FD_SET(fdServer, &bvfdRead);   /* socket for the server */
//        FD_SET(fdIn, &bvfdRead);            /* fd for stdin */
//        while (select (fdServer+1, &bvfdRead, 0, 0, 0) > 0) {
//            /* see what fd's have activity */
//            if (FD_ISSET(fdServer, &bvfdRead)) {
//                /* activity on socket (may be EOF) */
//                cch = read (fdServer, sb, cchMax);
//                if (cch <= 0)
//                    exit (0);        /* return of zero is EOF */
//                write (fdOut, sb, cch);  /* write to stdout */
//            }
//            if (FD_ISSET(fdIn, &bvfdRead)) {
//                /* activity on stdin (may be EOF) */
//                cch = read (fdIn, sb, cchMax);
//                if (cch <= 0)
//                    shutdown (fdServer, 1);                         
//                else
//                    write (fdServer, sb, cch);
//            }
//            /* get ready for another call to select */
//            FD_ZERO(&bvfdRead);
//            FD_SET(fdServer, &bvfdRead);   /* socket for the server */
//            FD_SET(fdIn, &bvfdRead);        /* fd for stdin */
//        }
//    }
}

/* Fetch a packet from the network layer for transmission on the channel */
void from_network_layer(packet *p){}
/* Deliver information from an inbound frame to the network layer. */

void to_network_layer(char *f){}
/* Go get an inbound frame from the physical layer and copy it to r. */

void from_physical_layer(frame *f, int sock){
    ssize_t bytesRec=recv(sock,f,sizeof(f),0); 
    if(bytesRec < 0){
        perror("Failed to recv frame in to_physical_layer\n");
        exit(1);
    }
    
}
/* Pass the frame to the physical layer for transmission. */

void to_physical_layer(frame *f, int sock){
    // zap frame
        // HERE
    
    // send via TCP
    // will be replaced with to_data_link()
    ssize_t bytesSent=send(sock,f,sizeof(f),0); 
    if(bytesSent<sizeof(f)){
        perror("Failed to send complete frame in to_physical_layer\n");
        exit(1);
    }
    
}

/* Start the clock running and enable the timeout event. */
void start_timer(seq_nr k){
}

/* Stop the clock and disable the timeout event. */
void stop_timer(seq_nr k){
}

/* Start an auxiliary timer and enable the ack timeout event. */
void start_ack_timer(void){}

/* Stop the auxiliary timer and disable the ack timeout event. */
void stop_ack_timer(void){}

/* Allow the network layer to cause a network layer ready event. */
void enable_network_layer(void){}

/* Forbid the network layer from causing a network layer ready event. */
void disable_network_layer(void){}



