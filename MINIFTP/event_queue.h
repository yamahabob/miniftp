//
//  event_queue.h
//  MINIFTP
//
//  Created by Curtis Taylor on 4/4/13.
//  Copyright (c) 2013 WPI. All rights reserved.
//

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


int add_eventEntry(eventEntry* head, seq_nr seqNum, int timestamp ,eventEntry** newHead);

int remove_byTime(eventEntry* head, eventEntry** newHead, int curTime);

int remove_bySeq(eventEntry* head, eventEntry** newHead, seq_nr seqNum);
void printQueue(eventEntry* head);


#endif /* defined(__MINIFTP__event_queue__) */
