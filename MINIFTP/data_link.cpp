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

static void send_data(seq_nr frame_nr, frame buffer[], int sock){
    /* Construct and send a data frame. */
    frame s; /* scratch variable */
    //s.info = buffer[frame_nr]; /* insert packet into frame */
    
    //strcpy(s.info,buffer[frame_nr].info);
    memcpy(s.info, buffer[frame_nr].info,PAYLOAD_SIZE);
    
    //cout << "frame info being send=" << s.info << endl;
    s.kind=data;
    s.seq = frame_nr; /* insert sequence number into frame */
    s.remaining=buffer[frame_nr].remaining;
    s.ack=1337;

    char result[CHECK_SUM_LENGTH];
    checksum(s.info, PAYLOAD_SIZE, result);
    memcpy(s.checkSum,result,2);
    cout << "SENDING r0=" << (int)(result[0]) << " r1=" <<  (int)result[1]<<endl;

    //s.ack = (frame_expected + MAX_SEQ) % (MAX_SEQ + 1); /* piggyback ack */
    //s.ack = (frame_expected + MAX_SEQ) % (MAX_SEQ + 1); /* piggyback ack */
    to_physical_layer(&s,sock); /* transmit the frame */
    start_timer(frame_nr); /* start the timer running */
}

void protocol5(int sock){ // removed network_fd because it's now global
    seq_nr next_frame_to_send; /* MAX SEQ > 1; used for outbound stream */
    //pipe
    
    vector<frame> partialPacket; //stores fragmented frames of a packet
    
    seq_nr ack_expected; /* oldest frame as yet unacknowledged */
    seq_nr frame_expected; /* next frame expected on inbound stream */
    frame r; /* scratch variable */
    //packet buffer[MAX_SEQ + 1]; /* buffers for the outbound stream */
    frame buffer[MAX_SEQ + 1]; /* buffers for the outbound stream */
    //frame reserved[MAX_FRAME_SPLIT - 1]; /* buffers for the outbound stream */
    vector<frame> reserved;
    
    seq_nr nbuffered; /* number of output buffers currently in use */
    //seq_nr nbufferedReserve;
    seq_nr i; /* used to index into the buffer array */
    event_type event;
    enable_network_layer(); /* allow network layer ready events */
    
    ack_expected = 0; /* next ack expected inbound */
    next_frame_to_send = 0; /* next frame going out */
    frame_expected = 0; /* number of frame expected inbound */
    nbuffered = 0; /* initially no packets are buffered */
    //nbufferedReserve=0;
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
            {
                /* Accept, save, and transmit a new frame. */
                //from_network_layer(&buffer[next_frame_to_send]); /* fetch new packet */
                packet p;
                from_network_layer(&p); /* fetch new packet */
                int numFramesAddedtoBuffer=split(&p, buffer, reserved, next_frame_to_send, nbuffered);
                cout << "NUMBER OF FRAMES CREATED=" << numFramesAddedtoBuffer << endl;
                for(int i=0;i<numFramesAddedtoBuffer;i++){
                    nbuffered = nbuffered + 1; /* expand the sender’s window */
                    send_data(next_frame_to_send, buffer, sock);/* transmit the frame */
                    inc(next_frame_to_send); /* advance sender’s upper window edge */
                }
                //int temp;
                //cin >> temp;
                
                break;
            }
            case frame_arrival: /* a data or control frame has arrived */
                from_physical_layer(&r,sock); /* get incoming frame from physical layer */
                // move to from_phy?
                if(!checksumFrame(r)){
                    cout << "CHECK SUM ERROR\n";
                    break;
                }
                //cout << "r.seq=" << r.seq << " r.kind=" << r.kind << endl;
                if (r.seq == frame_expected && r.kind==data) {
                    /* Frames are accepted only in order. */
                    
                    //do stuff
                    partialPacket.push_back(r);
                    //cout << "r.remaining=" << r.remaining << endl;
                    if(r.remaining==0){
                        to_network_layer(partialPacket); /* pass packet to network layer */
                        partialPacket.clear();
                    }
                    
                    inc(frame_expected); /* advance lower edge of receiver’s window */
                    
                    // SEND ACK FOR THIS FRAME
                    cout << "SENDING ACK FRAME1\n";
                    sendAck(&r,sock);
                    //int temp;
                    //cin >>temp;

                }
                else if(r.kind==data){
                    // SEND ACK FOR LAST SUCCESSFUL FRAME
                    cout << "SENDING ACK FRAME2\n";
                    sendAck(&r,sock);
                }
                else{ // means it's an ACK frame
                    /* Ack n implies n − 1, n − 2, etc. Check for this. */
                    //cout << "CHECKING between(" << ack_expected << "," << r.ack << "," << next_frame_to_send <<")\n";
                    while (between(ack_expected, r.ack, next_frame_to_send)) {
                        /* Handle piggybacked ack. */
                        //cout << "stopping timer for "<< ack_expected << endl;
                        nbuffered = nbuffered - 1; /* one frame fewer buffered */
                        stop_timer(ack_expected); /* frame arrived intact; stop timer */
                        inc(ack_expected); /* contract sender’s window */
                        
                        /* If items in reserved, move to buffer and send */
                        if(!reserved.empty()){
                            frame temp=(frame)reserved.front();
                            memcpy(&buffer[next_frame_to_send],&temp,FRAME_SIZE);
                            reserved.erase(reserved.begin());
                            nbuffered = nbuffered + 1; /* expand the sender’s window */
                            send_data(next_frame_to_send, buffer, sock);
                            inc(next_frame_to_send); /* advance sender’s upper window edge */
                        }
                    }
                }
//                int temp;
//                cin>>temp;
                break;
            case cksum_err: break; /* just ignore bad frames */
            case timeout: /* trouble; retransmit all outstanding frames */
                next_frame_to_send = ack_expected; /* start retransmitting here */
                //cout << "Retransmitting " << next_frame_to_send << endl;
                for (i = 1; i <= nbuffered; i++) {
                    send_data(next_frame_to_send, buffer, sock);/* resend frame */
                    inc(next_frame_to_send); /* prepare to send the next one */
                }
                break;
            case dl_die: exit(0);
        }
        
        if (nbuffered < MAX_SEQ){ cout << "SENDING token to Network Layer\n" << endl;
            enable_network_layer();
            cout << "SENT token to Network Layer\n" << endl;
        }
//        else{ cout << "network layer disabled" << endl;
//            if(netEnabled){
//                disable_network_layer();
//                netEnabled=false;
//            }
//        }
        cout << "NUMBUFFERED=" << nbuffered << " BUFFER INDEX=" << next_frame_to_send <<endl;
    }
}

void wait_for_event(event_type *event, int sock){ // dl_die!!
    fd_set bvfdRead;
    *event=timeout;

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
        //cout << "Nothing in window\n";
#endif
    }
    
    cout << "Time to wait=" << timeToWait->tv_sec << "+" << timeToWait->tv_usec << endl;
    if(timeToWait->tv_sec<=0){
        free(timeToWait);
        timeToWait=NULL;
    }
    
    int maxVal=max(toDL[0], sock);
    if(select(maxVal+1, &bvfdRead, NULL, NULL, timeToWait)) {
        /* see what fd's have activity */
        if (FD_ISSET(sock, &bvfdRead)) {
            cout << "Frame has arrived at the Physical layer\n";
            *event=frame_arrival;
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
    }
    else{
        int curTime2=(int)time(NULL);
        *event=timeout;
        
        //remove all timeouts based on curTime
#ifdef DEBUG
        //cout << "removing by time\n";
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
//void from_network_layer(packet *p){
void from_network_layer(packet *p){ /* fetch new packet */
    packet temp;
    int bytesRec=(int)read(toDL[0],&temp,PACKET_SIZE);
    memcpy(p,&temp,bytesRec);

    //cout << "DL read from network layer -- " << temp.data << " with size=" << bytesRec << endl;
}

/* Deliver information from an inbound frame to the network layer. */
void to_network_layer(vector<frame> framesToNL){
    packet p;
    deStuff(framesToNL,&p);
    cout << "PACKET TO BE SENT TO THE NETWORK LAYER-->" << p.data << endl;
    int bytesSent=(int)write(fromDL[1],&p,sizeof(packet));
    if(bytesSent<sizeof(packet)){
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
    ssize_t bytesRec=recv(sock,&temp,FRAME_SIZE,0);
    if(bytesRec < 0){
        perror("Failed to recv frame in to_physical_layer\n");
        exit(1);
    }
    f->kind=temp.kind;
    f->seq=temp.seq;
    f->ack=temp.ack;
    f->remaining=temp.remaining;
    //memcpy(f->checkSum,temp.checkSum,2);
    f->checkSum[0]=temp.checkSum[0];
    f->checkSum[1]=temp.checkSum[1];
    //strcpy(f->info,temp.info);
    memcpy(f->info, temp.info, PAYLOAD_SIZE);
    //cout << "bytesRec=" << bytesRec<<endl;
    if(temp.kind==0)
        cout << "TEMP.SEQ BEING RECEIVED - DATA" << endl;
    else
        cout << "TEMP.SEQ BEING RECEIVED - ACK" << endl;
    cout << "TEMP.SEQ BEING RECEIVED " << temp.seq << endl;
    cout << "TEMP.ACK BEING RECEIVED " << temp.ack << endl;
    cout << "TEMP.REMAINING BEING RECEIVED " << temp.remaining << endl;
    cout << "TEMP.INFO BEING RECEIVED " << temp.info << endl;
    
    //cout << "frame received=" << f << endl;
    //cout << "In PHY : f0=" << (int)f->checkSum[0] << " f1=" << (int)f->checkSum[1] << endl;
    //cout << "In PHY : temp0=" << (int)temp.checkSum[0] << " temp1=" << (int)temp.checkSum[1] << endl;


#ifdef DEBUG
    cout << "Leaving from_physical\n";
#endif
    
}
/* Pass the frame to the physical layer for transmission. */

void to_physical_layer(frame *f, int sock){
    // ZAP
    bzzzzzzzuppp(f);
    // send via TCP
    // will be replaced with to_data_link() --> I dunno why this coment is here?
    frame temp;
    temp.kind=f->kind;
    temp.seq=f->seq;
    temp.ack=f->ack;
    temp.remaining=f->remaining;
    temp.checkSum[0]=f->checkSum[0];
    temp.checkSum[1]=f->checkSum[1];
    memcpy(temp.info, f->info, PAYLOAD_SIZE);//(temp.info,f->info);
    if(temp.kind==0)
        cout << "TEMP.SEQ BEING SENT - DATA" << endl;
    else
        cout << "TEMP.SEQ BEING SENT - ACK" << endl;
    cout << "TEMP.SEQ BEING SENT " << temp.seq << endl;
    cout << "TEMP.ACK BEING SENT " << temp.ack << endl;
    cout << "TEMP.REMAINING BEING SENT " << temp.remaining << endl;
    cout << "TEMP.INFO BEING SENT " << temp.info << endl;

    //cout << "sizeof(temp)=" << FRAME_SIZE << " length= " << strlen(temp.info) << endl;
    ssize_t bytesSent=send(sock,&temp,FRAME_SIZE,0);
    if(bytesSent < (ssize_t)FRAME_SIZE){
        perror("Failed to send complete frame in to_physical_layer\n");
        exit(1);
    }
    //cout << "bytesSent=" << bytesSent << " and size of temp= " << FRAME_SIZE<<endl;

    
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
    //1==enabled
    char status='1';
    cout << "EXACTLY before write\n";
    write(signalFromDL[1],&status,1);
    cout << "EXACTLY after write\n";

}

/* Forbid the network layer from causing a network layer ready event. */
void disable_network_layer(void){
    //0==disabled
    char status='0';
    write(signalFromDL[1],&status,1);
}

/* Applies byte stuffinfg on *input* and puts the result in *output*. The
 function also returns the size of *output*, i.e. the stuffed buffer. */
int byteStuff(char *input, char *output){
     int ind = 0; //keeps track of the next position in output.
     
     for(int i = 0; i < PACKET_DATA_SIZE; i++){
         //if(input[i] == '\x10'){ //if the input is DLE
         if(input[i] == DELIM){
             output[ind++] = input[i];
             output[ind++] = input[i];
         }
         else
             output[ind++] = input[i];
     }
     
     return ind; //size
 }

void deStuff(vector <frame> partialPackets, packet *p){
    char pack_buf[PACKET_SIZE];
    int ind = 0; //The next position in the packet data
    bool metDLE = false; //The previously visited character is DLE
    
    while(!partialPackets.empty()){
        frame temp = (frame)partialPackets.front(); //Read the next stuffed frame
        
        for(int i = 0; i < PAYLOAD_SIZE; i++){
            if(ind >= PACKET_DATA_SIZE)
                break;
            
            //if(temp.info[i] == '\x10'){ //The char is DLE
            if(temp.info[i] == DELIM){ //The char is DLE

                if(metDLE)
                    pack_buf[ind++] = temp.info[i];
                else
                    metDLE = true;
            }
            else{
                pack_buf[ind++] = temp.info[i];
                metDLE = false;
            }
        }
        
        partialPackets.erase(partialPackets.begin());
    }
    
    memcpy(p, pack_buf, PACKET_SIZE);
}

/* Puts two checksum bytes for *input* in *result*. *size* is the size of
 *input*. The function returns 1 on success. If the size of input is less
 than 2, the checksum cannot be computed; thus, the function returns -1.
 */
int checksum(const char* input, int size, char result[CHECK_SUM_LENGTH]){
    //cout << "Computing CS for " << input << endl;
    if(size < 2){
        result[0] = input[0];
        result[1] = input[0];
        return -1;
    }
    
    result[0] = input[0];
    result[1] = input[1];
    
    for(int i = 2; i < size; i++){
        result[i%2] ^= input[i];
    }
    //result[2]='\0';
    
    return 1;
}

int fragment(char *stuffedPacket, frame rawFrames[MAX_FRAME_SPLIT], int size)
{
    int len = size;
    int i=0;
    for(i=0;i<ceil(((float)len/PAYLOAD_SIZE));i++){
        //int offset=min(PAYLOAD_SIZE,(int)strlen(stuffedPacket+i*PAYLOAD_SIZE));
        memcpy(rawFrames[i].info,stuffedPacket+(i*PAYLOAD_SIZE),PAYLOAD_SIZE);
    }
    return i;
}

void bzzzzzzzuppp(frame *f){
    int randNum=rand()%101;
    if(0<=randNum && randNum<=errorRate){
        cout << "GOT ZAPPED with number=" << randNum <<endl;
        f->checkSum[0]=!f->checkSum[1];
    }
}

int checksumFrame(frame f){
    char result[2];
    checksum(f.info, PAYLOAD_SIZE, result);
    if(result[0]==f.checkSum[0] && result[1]==f.checkSum[1])
        return 1;
    else
        return 0;
}

// returns number of frames created
// always assumes reserve is empty, otherwise no packet would have been received from network layer
int split(packet *p, frame buffer[], vector<frame> & reserved, int next_frame_to_send, seq_nr nbuffered){
    char output[2*PACKET_SIZE];
    char pack_buf[PACKET_SIZE];
    
    frame rawFrames[MAX_FRAME_SPLIT];
    
    memset(output,'\0',2*PACKET_SIZE);
    
    memcpy(pack_buf, p, PACKET_SIZE);
    
    int size=byteStuff(pack_buf,output);
    
    cout << "Packet size AFTER stuffing=" << size << endl;
    int numFramesCreated=fragment(output,rawFrames,size);
    int framesAddedtoBuffer=0;
    //*nbufferedReserve=0;
    
    for(int i=0;i<numFramesCreated;i++){
        if(nbuffered<MAX_SEQ){
            memcpy(buffer[next_frame_to_send].info,rawFrames[i].info,PAYLOAD_SIZE);
            buffer[next_frame_to_send++].remaining=numFramesCreated-i-1;
            cout << "Next frame = " << next_frame_to_send-1 << " Assigning remaining to be " << buffer[next_frame_to_send-1].remaining << endl;
            nbuffered++;
            framesAddedtoBuffer++;
        }
        else{
            //memcpy(reserve[*nbufferedReserve].info,rawFrames[i].info,PAYLOAD_SIZE);
            //(*nbufferedReserve)++;
            rawFrames[i].remaining=numFramesCreated-i-1;
            reserved.push_back(rawFrames[i]);
        }
    }
    
    return framesAddedtoBuffer;
}

void sendAck(frame *r, int sock){
    frame ackFrame;
    memcpy(&ackFrame, r,FRAME_SIZE);
    ackFrame.kind=ack;
    ackFrame.ack=ackFrame.seq;
    
    char result[CHECK_SUM_LENGTH];
    checksum(ackFrame.info, PAYLOAD_SIZE, result);
    memcpy(ackFrame.checkSum,result,2);
    cout << "SENDING r0=" << (int)(result[0]) << " r1=" <<  (int)result[1]<<endl;
    cout << "SIZE of ack to be transmitted" << FRAME_SIZE << endl;
    to_physical_layer(&ackFrame, sock);
}



