//
//  utilities.h
//  MINIFTP
//
//  Created by Curtis Taylor on 3/19/13.
//  Copyright (c) 2013 WPI. All rights reserved.
//

#ifndef utilities
#define utilities

#include <iostream>


void parseMessage(const char* msg, string & command,
                  vector<string> & arguments);

int sendData(int cmd, vector<string> parameters, int sock);
int receiveData(vector<string> arguments, int sock);
void sendMessage(int cmd, vector<string> parameters, int toDL, int fromDL);
    
int to_data_link(packet *p, int toDL, int fromDL);

#endif /* defined(__MINIFTP__utilities__) */
