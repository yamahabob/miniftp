//
//  event_queue.cpp
//  MINIFTP
//
//  Created by Curtis Taylor on 4/4/13.
//  Copyright (c) 2013 WPI. All rights reserved.
//
//  LDatabase.c
//  LocationServer
//
//  Created by Salman Saghafi on 1/30/13.
//  Copyright (c) 2013 WPI. All rights reserved.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "event_queue.h"


int add_eventEntry(eventEntry* head, seq_nr seqNum, int timestamp ,eventEntry** newHead){
    int output = 1; //determines whether the new entry will be added
    
    //create the new entry:
    eventEntry* newEntry = (eventEntry *)malloc(sizeof(eventEntry));
    newEntry->seqNum= seqNum;
    newEntry->timestamp=timestamp;
    newEntry->next = NULL;
    
    if(head == NULL) //if the list is empty
        *newHead = newEntry;
    else{ //the list is not empty
        eventEntry* ptr = head;
        eventEntry* prev = NULL; //previous entry
        
        while(ptr != NULL){ //traverse the list
            if(ptr->seqNum==newEntry->seqNum){
                output = 0; //duplicate entry
                cerr << "Duplicate sequence number found\n";
                break;
            }
            
            //move on:
            prev = ptr;
            ptr = ptr->next;
        }
        
        if(output == 1){ //if the entry is not duplicate
            //add newEntry:
            if(prev == NULL){ //to the beginning of the list
                *newHead = newEntry;
                newEntry->next = ptr;
            }
            else{ //in the middle of the list
                prev->next = newEntry;
                *newHead = head;
            }
        }
        else{ //Entry is duplicate. Free its memory.
            free (newEntry);
        }
    }
    
    return output;
}


int remove_byTime(eventEntry* head, eventEntry** newHead, int curTime){//removes head only, possibly make his remove all timestamp of 0
    int output = 0; //return value
    if(head == NULL){ //if the list is empty
        newHead = NULL;
        output = 0;
    }
    else{ //the list is not empty
        eventEntry* ptr = head;
        eventEntry* prev = NULL;
        
        while(ptr != NULL){ //traverse the list
#ifdef DEBUG
            cout << curTime << "-" << ptr->timestamp << ">" << FRAME_TIMEOUT << "=" << ((curTime-ptr->timestamp)>FRAME_TIMEOUT) <<endl;
#endif
            if((curTime-ptr->timestamp)>=FRAME_TIMEOUT){
#ifdef DEBUG
                cout << "Removing element from list\n";
#endif
                if(ptr==head){
                    //update head and remove
                    *newHead=ptr->next;
                    free(ptr);
                }
                else{
                    // remove and update previous pointer
                    prev->next=ptr->next;
                    free(ptr);
                }
            }
            prev=ptr;
            ptr=ptr->next;
        }
    }
    return output;
}

int remove_bySeq(eventEntry* head, eventEntry** newHead, seq_nr seqNum){
    int output = 0; //return value
    if(head == NULL){ //if the list is empty
        newHead = NULL;
        output = 0;
    }
    else{ //the list is not empty
        eventEntry* ptr = head;
        eventEntry* prev = NULL;
        while(ptr != NULL){ //traverse the list
            if(ptr->seqNum==seqNum){
                if(ptr==head){
                    //update head and remove
                    *newHead=ptr->next;
                    free(ptr);
                }
                else{
                    // remove and update previous pointer
                    prev->next=ptr->next;
                    free(ptr);
                }
                break;
            }
            prev=ptr;
            ptr=ptr->next;
        }
    }
    return output;
}













