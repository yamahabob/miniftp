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
#include <sstream>

eventEntry* queueHead=NULL;

int numFramesSent=0; // good
int numReTrans=0; // good
int numAckSent=0; // good
int numFramesRecCor=0; // good
int numAckRecCor=0; // good
int numFrameErr=0; // good
int numAckRecErr=0; // good
int numDupFrameRec=0; // good
int numBlockFull=0; // good
int lastLog=(int)time(NULL);

static bool between(seq_nr a, seq_nr b, seq_nr c){
    /* Return true if a <= b < c circularly; false otherwise. */
    if (((a <= b) && (b < c)) || ((c < a) && (a <= b)) || ((b < c) && (c < a)))
        return(true);
    else
        return(false);
}

static void send_data(seq_nr frame_nr, frame buffer[], int sock){
    numFramesSent++; // update frames sent!
    
    /* Construct and send a data frame. */
    frame s; /* scratch variable */
    memcpy(s.info, buffer[frame_nr].info,PAYLOAD_SIZE);
    s.kind=data;
    s.seq = frame_nr; /* insert sequence number into frame */
    s.remaining=buffer[frame_nr].remaining;
    s.ack=1337; // just to initialize it

    char result[CHECK_SUM_LENGTH];
    checksum(s.info, PAYLOAD_SIZE, result);
    memcpy(s.checkSum,result,2);
    to_physical_layer(&s,sock); /* transmit the frame */
    start_timer(frame_nr); /* start the timer running */
}

// type 0=server -- used for 
// type 1=client
void protocol5(int type,int sock){ // removed network_fd because it's now global
    seq_nr next_frame_to_send; /* MAX SEQ > 1; used for outbound stream */

    vector<frame> partialPacket; //stores fragmented frames of a packet
    
    seq_nr ack_expected; /* oldest frame as yet unacknowledged */
    seq_nr frame_expected; /* next frame expected on inbound stream */
    frame r; /* scratch variable */
    frame buffer[MAX_SEQ + 1]; /* buffers for the outbound stream */
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
   
    seq_nr lastSuccessful=-1;
    
    log(type); // create initial log
    
    while (true) {
        wait_for_event(&event,sock); /* four possibilities: see event type above */
        switch(event) {
            case network_layer_ready: /* the network layer has a packet to send */
            {
                /* Accept, save, and transmit a new frame. */
                packet p;
                from_network_layer(&p); /* fetch new packet */
                int numFramesAddedtoBuffer=split(&p, buffer, reserved, next_frame_to_send, nbuffered);
                for(int i=0;i<numFramesAddedtoBuffer;i++){
                    nbuffered = nbuffered + 1; /* expand the sender’s window */
                    send_data(next_frame_to_send, buffer, sock);/* transmit the frame */
                    inc(next_frame_to_send); /* advance sender’s upper window edge */
                }
                
                if (nbuffered < MAX_SEQ){
                    enable_network_layer(); // send enable token to network
                }
                else{
                    numBlockFull++; // update number of times blocked
                }

                break;
            }
            case frame_arrival: /* a data or control frame has arrived */
                from_physical_layer(&r,sock); /* get incoming frame from physical layer */
                if(!checksumFrame(r)){
                    numFrameErr++;
                    break;
                }
                else{
                    numFramesRecCor++;
                    if (r.seq == frame_expected && r.kind==data) {
                        /* Frames are accepted only in order. */
                        lastSuccessful=r.seq;
                        partialPacket.push_back(r);
                        if(r.remaining==0){
                            to_network_layer(partialPacket); /* pass packet to network layer */
                            partialPacket.clear();
                        }
                        
                        inc(frame_expected); /* advance lower edge of receiver’s window */
                        
                        // SEND ACK FOR THIS FRAME
                        sendAck(&r,sock);
                    }
                    else if(r.kind==data && between(lastSuccessful,r.ack,frame_expected)){
                        numDupFrameRec++;
                        sendAck(&r,sock);
                    }
                    else if(r.kind==data){
                        break;
                    }
                    else{ // means it's an ACK frame
                        /* Ack n implies n − 1, n − 2, etc. Check for this. */
                        if(!between(ack_expected, r.ack, next_frame_to_send))
                            numAckRecErr++;
                        
                        while (between(ack_expected, r.ack, next_frame_to_send)) {
                            numAckRecCor++;
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
                            }else{
                                if(nbuffered < MAX_SEQ){
                                    enable_network_layer();
                                }
                                else{
                                    numBlockFull++;
                                }
                            }
                        }
                        
                    }
                }
                break;
            case timeout: /* trouble; retransmit all outstanding frames */
                next_frame_to_send = ack_expected; /* start retransmitting here */
                for (i = 1; i <= nbuffered; i++) {
                    numReTrans++;
                    remove_bySeq(queueHead, &queueHead, queueHead->seqNum);
                    send_data(next_frame_to_send, buffer, sock);/* resend frame */
                    inc(next_frame_to_send); /* prepare to send the next one */
                }
                break;
            case dl_die:
                log(type);
                return;
        }
        if((int)(time(NULL)-lastLog)>2){
            lastLog=(int)(time(NULL));
            log(type);
        }
    }
}

void wait_for_event(event_type *event, int sock){ // dl_die!!
    fd_set bvfdRead;
    *event=timeout;

    FD_ZERO(&bvfdRead);
    FD_SET(sock, &bvfdRead);      /* socket from receiver */
    FD_SET(toDL[0], &bvfdRead);   /* pipe from network layer */
    FD_SET(killDL[0], &bvfdRead); /* pipe from app layer to die*/
    
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
        }
    }
    
    if(timeToWait->tv_sec<=0){
        free(timeToWait);
        timeToWait=NULL;
    }
    
    int maxVal=max(toDL[0], sock);
    maxVal=max(maxVal,killDL[0]);
    if(select(maxVal+1, &bvfdRead, NULL, NULL, timeToWait)) {
        /* see what fd's have activity */
        if (FD_ISSET(sock, &bvfdRead)) {
            //cout << "Frame has arrived at the Physical layer\n";
            *event=frame_arrival;
            if(!(timeToWait)){
                free(timeToWait);
                timeToWait=NULL;
            }
            return;
        }
        if (FD_ISSET(toDL[0], &bvfdRead)) {
            //cout << "Network layer ready returning from wait for event\n";
            *event=network_layer_ready;
        }
        if (FD_ISSET(killDL[0], &bvfdRead)) {
            //cout << "Network layer ready returning from wait for event\n";
            *event=dl_die;
        }
    }
    else{
        *event=timeout;
        if((timeToWait!=NULL)){
            free(timeToWait);
            timeToWait=NULL;
        }
        return;
    }
    
}

/* Fetch a packet from the network layer for transmission on the channel */
void from_network_layer(packet *p){ /* fetch new packet */
    packet temp;
    int bytesRec=(int)read(toDL[0],&temp,PACKET_SIZE);
    memcpy(p,&temp,bytesRec);
}

/* Deliver information from an inbound frame to the network layer. */
void to_network_layer(vector<frame> framesToNL){
    packet p;
    deStuff(framesToNL,&p);
    int bytesSent=(int)write(fromDL[1],&p,sizeof(packet));
    if(bytesSent<sizeof(packet)){
        perror("Failed to send complete packet to network layer");
        exit(1);
    }
    
}
/* Go get an inbound frame from the physical layer and copy it to r. */

void from_physical_layer(frame *f, int sock){
    frame temp;
    ssize_t bytesRec=recv(sock,&temp,FRAME_SIZE,0);
    if(bytesRec < 0){
        perror("Failed to recv frame in from_physical_layer\n");
        exit(2);
    }
    f->kind=temp.kind;
    f->seq=temp.seq;
    f->ack=temp.ack;
    f->remaining=temp.remaining;
    f->checkSum[0]=temp.checkSum[0];
    f->checkSum[1]=temp.checkSum[1];
    memcpy(f->info, temp.info, PAYLOAD_SIZE);
}

/* Pass the frame to the physical layer for transmission. */
void to_physical_layer(frame *f, int sock){
    bzzzzzzzuppp(f);
   
    frame temp;
    temp.kind=f->kind;
    temp.seq=f->seq;
    temp.ack=f->ack;
    temp.remaining=f->remaining;
    temp.checkSum[0]=f->checkSum[0];
    temp.checkSum[1]=f->checkSum[1];
    memcpy(temp.info, f->info, PAYLOAD_SIZE);

    ssize_t bytesSent=send(sock,&temp,FRAME_SIZE,0);
    if(bytesSent < (ssize_t)FRAME_SIZE){
        perror("Failed to send complete frame in to_physical_layer");
        exit(3);
    }

    
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
    write(signalFromDL[1],&status,1);
}

/* Forbid the network layer from causing a network layer ready event. */
// Lack of token sent to NL means disabled
//void disable_network_layer(void){
//    //0==disabled
//    char status='0';
//    write(signalFromDL[1],&status,1);
//}

/* Applies byte stuffinfg on *input* and puts the result in *output*. The
 function also returns the size of *output*, i.e. the stuffed buffer. */
int byteStuff(char *input, char *output){
    int ind = 0; //keeps track of the next position in output.
    for(int i = 0; i < PACKET_SIZE; i++){
        if(input[i] == DL_DELIM){
            output[ind++] = input[i];
            output[ind++] = input[i];
        }
        else{
            output[ind++] = input[i];
        }
    }
    return ind; //size
}

void deStuff(vector <frame> partialPackets, packet *p){
    char pack_buf[PACKET_SIZE];
    int ind = 0; //The next position in the packet data
    bool metDLE = false; //The previously visited character is DLE

    while(!partialPackets.empty()){
        frame temp = (frame)partialPackets.front(); //Read the next stuffed frame
        char thing[PAYLOAD_SIZE+1];
        memcpy(thing,temp.info,PAYLOAD_SIZE);
        thing[PAYLOAD_SIZE]='\0';
        
        for(int i = 0; i < PAYLOAD_SIZE; i++){
            if(ind >= PACKET_SIZE)
                break;
            if(temp.info[i] == DL_DELIM){ //The char is DLE
                if(metDLE){
                    pack_buf[ind++] = temp.info[i];
                    metDLE=false;
                }
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
    return 1;
}

int fragment(char *stuffedPacket, frame rawFrames[MAX_FRAME_SPLIT], int size)
{
    int len = size;
    int i=0;
    for(i=0;i<ceil(((float)len/PAYLOAD_SIZE));i++){
        memcpy(rawFrames[i].info,stuffedPacket+(i*PAYLOAD_SIZE),PAYLOAD_SIZE);
    }
    return i;
}

void bzzzzzzzuppp(frame *f){
    int randNum=rand()%101;
    if(0<=randNum && randNum<=errorRate){
        //cout << "GOT ZAPPED with number=" << randNum <<endl;
        if(f->kind==ack)
            cout << "ACK FRAME " << f->ack <<" ZAPPED with number=" << randNum <<endl;
        else if(f->kind==data)
            cout << "DATA FRAME " << f->seq <<" ZAPPED with number=" << randNum <<endl;
        f->checkSum[0]=!f->checkSum[1];
    }
    else{
        if(f->kind==ack)
            cout << "ACK FRAME " << f->ack <<" *NOT* ZAPPED with number=" << randNum <<endl;
        else if(f->kind==data)
            cout << "DATA FRAME " << f->seq <<" *NOT* ZAPPED with number=" << randNum <<endl;
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
    
    int numFramesCreated=fragment(output,rawFrames,size);
    int framesAddedtoBuffer=0;
    
    for(int i=0;i<numFramesCreated;i++){
        if(nbuffered<MAX_SEQ){
            memcpy(buffer[next_frame_to_send].info,rawFrames[i].info,PAYLOAD_SIZE);
            buffer[next_frame_to_send++].remaining=numFramesCreated-i-1;
            nbuffered++;
            framesAddedtoBuffer++;
        }
        else{
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
    numAckSent++;
    to_physical_layer(&ackFrame, sock);
}

void log(int type){
    ostringstream convert;
    convert << getpid();
    
    //string msg_buffer=to_string(cmd);
    string msg_buffer=convert.str();
    if(type==0) {// server
        ofstream fout;
        string filename="logs/serverLog_"+convert.str()+".txt";
        fout.open(filename.c_str());
        if(!fout.is_open()){
            cerr << "Failed to open file\n";
        }
        else{
            fout << "SERVER LOG -- TIMESTAMP " << time(NULL) <<endl <<endl;
            fout << "1. Total number of data frames sent - " << numFramesSent <<endl;
            fout << "2. Total number of retransmissions sent - " << numReTrans <<endl;
            fout << "3. Total number of acknowledgments sent - " << numAckSent <<endl;
            fout << "4. Total number of data frames received correctly - " << numFramesRecCor <<endl;
            fout << "5. Total number of acknowledgments received correctly - " << numAckRecCor <<endl;
            fout << "6. Total number of data frames received with errors - " << numFrameErr <<endl;
            fout << "7. Total number of acknowledgements received with errors - " << numAckRecErr <<endl;
            fout << "8. Total number of duplicate frames received - " << numDupFrameRec <<endl;
            fout << "9. Total number of times the data link layer blocks due to a full window. - " << numBlockFull <<endl;
        }
        fout.close();
        
    }
    else if(type==1){
        ofstream fout;
        string filename="logs/clientLog_"+convert.str()+".txt";
        fout.open(filename.c_str());
        if(!fout.is_open()){
            cerr << "Failed to open file\n";
        }
        else{
            fout << "CLIENT LOG -- TIMESTAMP " << time(NULL) <<endl <<endl;
            fout << "1. Total number of data frames sent - " << numFramesSent <<endl;
            fout << "2. Total number of retransmissions sent - " << numReTrans <<endl;
            fout << "3. Total number of acknowledgments sent - " << numAckSent <<endl;
            fout << "4. Total number of data frames received correctly - " << numFramesRecCor <<endl;
            fout << "5. Total number of acknowledgments received correctly - " << numAckRecCor <<endl;
            fout << "6. Total number of data frames received with errors - " << numFrameErr <<endl;
            fout << "7. Total number of acknowledgements received with errors - " << numAckRecErr <<endl;
            fout << "8. Total number of duplicate frames received - " << numDupFrameRec <<endl;
            fout << "9. Total number of times the data link layer blocks due to a full window. - " << numBlockFull <<endl;
        }
        fout.close();
    }
    
}


