//
//  event_queue.h
//  MINIFTP
//  CS513-TEAM2

#ifndef __MINIFTP__event_queue__
#define __MINIFTP__event_queue__

#include <stdio.h>
#include <time.h>
#include "header.h"

typedef struct eventEntry{
    seq_nr seqNum;
    int timestamp;
    struct eventEntry* next;
} eventEntry;


//Primary Author: Salman
int add_eventEntry(eventEntry* head, seq_nr seqNum, int timestamp ,eventEntry** newHead);

//Primary Author: Salman
int remove_byTime(eventEntry* head, eventEntry** newHead, int curTime);

//Primary Author: Salman
int remove_bySeq(eventEntry* head, eventEntry** newHead, seq_nr seqNum);

//Primary Author: Salman
void printQueue(eventEntry* head);


#endif /* defined(__MINIFTP__event_queue__) */
