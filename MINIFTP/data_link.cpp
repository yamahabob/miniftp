//
//  data_link.cpp
//  MINIFTP
//
//  Created by Curtis Taylor on 3/21/13.
//  Copyright (c) 2013 WPI. All rights reserved.
//

#include "header.h"
#include "data_link.h"
#include <algorithm> // for max

eventEntry* queueHead=NULL;

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
    
    strcpy(s.info,buffer[frame_nr].data);
    cout << "frame info being send=" << s.info << endl;
    s.kind=data;
    s.seq = frame_nr; /* insert sequence number into frame */
    s.ack = (frame_expected + MAX_SEQ) % (MAX_SEQ + 1); /* piggyback ack */
    to_physical_layer(&s,sock); /* transmit the frame */
    start_timer(frame_nr); /* start the timer running */
}

void protocol5(int sock){ // removed network_fd because it's now global
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
#ifdef DEBUG
        cout << "WAITING FOR EVENT IN DL\n";
#endif
        wait_for_event(&event,sock); /* four possibilities: see event type above */
#ifdef DEBUG
        cout << "EVENT RECEIVED -- " << event <<endl;
#endif
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
                cout << "r.seq=" << r.seq << " r.kind=" << r.kind << endl;
                if (r.seq == frame_expected && r.kind==data) {
                    /* Frames are accepted only in order. */
                    to_network_layer(r.info); /* pass packet to network layer */
                    inc(frame_expected); /* advance lower edge of receiver’s window */
                    
                    // SEND ACK FOR THIS FRAME
                    cout << "SENDING ACK FRAME1\n";
                    frame ackFrame;
                    memcpy(&ackFrame, &r,sizeof(r));
                    ackFrame.kind=ack;
                    to_physical_layer(&ackFrame, sock);
                }
                else if(r.kind==data){
                    // SEND ACK FOR LAST SUCCESSFUL FRAME
                    cout << "SENDING ACK FRAME2\n";
                    frame ackFrame;
                    memcpy(&ackFrame, &r,sizeof(r));
                    ackFrame.seq=ackFrame.seq-1;
                    ackFrame.kind=ack;
                    to_physical_layer(&ackFrame, sock);
                }
                
                /* Ack n implies n − 1, n − 2, etc. Check for this. */
                cout << "CHECKING between(" << ack_expected << "," << r.ack << "," << next_frame_to_send <<")\n";
                while (between(ack_expected, r.ack, next_frame_to_send)) {
                    /* Handle piggybacked ack. */
                    cout << "stopping timer for "<< ack_expected << endl;
                    nbuffered = nbuffered - 1; /* one frame fewer buffered */
                    stop_timer(ack_expected); /* frame arrived intact; stop timer */
                    inc(ack_expected); /* contract sender’s window */
                }
                //int temp;
                //cin>>temp;
                break;
            case cksum_err: break; /* just ignore bad frames */
            case timeout: /* trouble; retransmit all outstanding frames */
                next_frame_to_send = ack_expected; /* start retransmitting here */
                //cout << "Retransmitting " << next_frame_to_send << endl;
                for (i = 1; i <= nbuffered; i++) {
                    send_data(next_frame_to_send, frame_expected, buffer, sock);/* resend frame */
                    inc(next_frame_to_send); /* prepare to send the next one */
                }
                break;
            case dl_die: exit(0);
        }
        if (nbuffered < MAX_SEQ)
            enable_network_layer();
        else
            disable_network_layer();
    }
}

void wait_for_event(event_type *event, int sock){ // dl_die!!
    fd_set bvfdRead;
    //*event=timeout;

    FD_ZERO(&bvfdRead);
    FD_SET(sock, &bvfdRead);   /* socket from receiver */
    FD_SET(toDL[0], &bvfdRead);            /* pipe from network layer */
    struct timeval *timeToWait;
    timeToWait=(struct timeval *)malloc(sizeof(struct timeval));
    timeToWait->tv_sec=0;
    timeToWait->tv_usec=0;
    int curTime=(int)time(NULL);
    if(queueHead!=NULL){
        if((FRAME_TIMEOUT-(curTime-queueHead->timestamp))<0){
            remove_byTime(queueHead, &queueHead, curTime);
            *event=timeout;
            return;
        }
        else{
            timeToWait->tv_sec=FRAME_TIMEOUT-(curTime-queueHead->timestamp);
            //cout << "TIME DIFFERENCE2=" <<FRAME_TIMEOUT-(curTime-queueHead->timestamp)<<endl;
        }
    }
    else{
#ifdef DEBUG
        cout << "Nothing in window\n";
#endif
    }
    
    int maxVal=max(toDL[0], sock);
    cout << "Time to wait=" << timeToWait->tv_sec << "+" << timeToWait->tv_usec << endl;
    if(timeToWait->tv_sec<=0){
        free(timeToWait);
        timeToWait=NULL;
    }
    if(select(maxVal+1, &bvfdRead, NULL, NULL, timeToWait)) {
        /* see what fd's have activity */
        if (FD_ISSET(sock, &bvfdRead)) {
            *event=frame_arrival;
            //frame temp;
            //ssize_t bytesRec=recv(sock,&temp,sizeof(temp),0);
            //if(bytesRec < 0){
             //   perror("Failed to recv frame in to_physical_layer\n");
              //  exit(1);
            //}
//            cout << "bytesRec=" << bytesRec<<endl;
//            cout << "frame info received=" <<temp.info << endl;
//            cout << "frame kind received=" << temp.kind << endl;
//            int temp2;
//            cin>>temp2;
            if(!(timeToWait)){
                free(timeToWait);
                timeToWait=NULL;
            }
            return;
            
        }
        if (FD_ISSET(toDL[0], &bvfdRead)) {
            cout << "Network layer ready returning from wait for event\n";
            *event=network_layer_ready;
        }
        //FD_ZERO(&bvfdRead);
        
    }
    else{
        int curTime2=(int)time(NULL);
        *event=timeout;
        
        //remove all timeouts based on curTime
#ifdef DEBUG
        cout << "removing by time\n";
#endif
        remove_byTime(queueHead, &queueHead, curTime2);

        if(!(timeToWait)){
            free(timeToWait);
            timeToWait=NULL;
        }
        return;
    }
    
}

/* Fetch a packet from the network layer for transmission on the channel */
void from_network_layer(packet *p){
    char temp[PACKET_SIZE];
    int bytesRec=(int)read(toDL[0],temp,PACKET_SIZE);
    strncpy(p->data,temp,bytesRec);
    cout << "DL read from network layer -- " << temp << endl;
}
/* Deliver information from an inbound frame to the network layer. */

void to_network_layer(char *f){
    int bytesSent=(int)write(fromDL[1],f,strlen(f));
    if(bytesSent<0){
        perror("well, this sucks...");
        exit(1);
    }
}
/* Go get an inbound frame from the physical layer and copy it to r. */

void from_physical_layer(frame *f, int sock){
#ifdef DEBUG
    cout << "Entering from_physical\n";
#endif
    frame temp;
    ssize_t bytesRec=recv(sock,&temp,sizeof(frame),0);
    if(bytesRec < 0){
        perror("Failed to recv frame in to_physical_layer\n");
        exit(1);
    }
    f->kind=temp.kind;
    f->seq=temp.seq;
    f->ack=temp.ack;
    strcpy(f->info,temp.info);
    cout << "bytesRec=" << bytesRec<<endl;
    cout << "frame info received=" << f->info << endl;
    cout << "frame received=" << f << endl;

#ifdef DEBUG
    cout << "Leaving from_physical\n";
#endif
    
}
/* Pass the frame to the physical layer for transmission. */

void to_physical_layer(frame *f, int sock){
    // zap frame
        // HERE
    
    // send via TCP
    // will be replaced with to_data_link() --> I dunno why this coment is here?
    frame temp;
    temp.kind=f->kind;
    temp.seq=f->seq;
    temp.ack=f->ack;
    strcpy(temp.info,f->info);
    cout << "sizeof(temp)=" << sizeof(frame) << endl;
    ssize_t bytesSent=send(sock,&temp,sizeof(frame),0);
    if(bytesSent<sizeof(f)){
        perror("Failed to send complete frame in to_physical_layer\n");
        exit(1);
    }
    cout << "bytesSent=" << bytesSent << " and size of temp= " << sizeof(temp)<<endl;

    
}

/* Start the clock running and enable the timeout event. */
// Having a linked list here is simulating  start/stop timer functionality
void start_timer(seq_nr k){
    add_eventEntry(queueHead, k, (int)time(NULL), &queueHead);
}

/* Stop the clock and disable the timeout event. */
void stop_timer(seq_nr k){
    remove_bySeq(queueHead, &queueHead, k);
}

/* Allow the network layer to cause a network layer ready event. */
void enable_network_layer(void){
    //networkEnabled=0;
}

/* Forbid the network layer from causing a network layer ready event. */
void disable_network_layer(void){
    //networkEnabled=0;
}

/* Applies byte stuffing on *input* and puts the result in *output*. The
 function also returns the size of *output*, i.e. the stuffed buffer. */
int byteStuff(char *input, char *output){
     int ind = 0; //keeps track of the next position in output.
     
     for(int i = 0; i <= strlen(input); i++){
         if(input[i] == '\x10'){ //if the input is DLE
             output[ind++] = input[i];
             output[ind++] = input[i];
         }
         else
             output[ind++] = input[i];
     }
     
     return ind - 1; //size
 }

/* Puts two checksum bytes for *input* in *result*. *size* is the size of
 *input*. The function returns 1 on success. If the size of input is less
 than 2, the checksum cannot be computed; thus, the function returns -1.
 */
int checksum(const char* input, int size, char result[CHECK_SUM_LENGTH+1]){
    if(size < 2)
        return -1;
    
    result[0] = input[0];
    result[1] = input[1];
    
    for(int i = 2; i < size; i++){
        result[i%2] ^= input[i];
    }
    result[2]='\0';
    
    return 1;
}

int splitFrame(char *stuffedPacket, char rawFrames[MAX_FRAME_SPLIT][PAYLOAD_SIZE])
{
    int len = (int)strlen(stuffedPacket);
    int i=0;
    for(i=0;i<ceil(((float)len/PAYLOAD_SIZE));i++){
        int offset=min(PAYLOAD_SIZE,(int)strlen(stuffedPacket+i*PAYLOAD_SIZE));
        strncpy(rawFrames[i],stuffedPacket+(i*PAYLOAD_SIZE),offset);
    }
    return i;
}




