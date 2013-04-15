//
// utilities.h
// MINIFTP
// CS513-TEAM2

#ifndef utilities
#define utilities

#include <iostream>
#include <sstream>
#include <errno.h>
#include <sys/time.h>

//Primary Author: Salman
void parseMessage(const char* msg, string & command,
                  vector<string> & arguments);
//Primary Author: Salman
void parseUserMessage(char* msg, string & command,
                  vector<string> & arguments);

//Primary Author: Curtis
int sendData(int cmd, vector<string> parameters, int toDL, int fromDL, int signalFromDL);
//Primary Author: Curtis
int receiveData(vector<string> arguments, int fromDL);
//Primary Author: Cutis
void sendMessage(int cmd, vector<string> parameters, int toDL, int fromDL, int signalFromDL);

//Primary Author: Curtis
int to_data_link(packet *p, int toDL, int fromDL, int signalFromDL);

//Primary Author: Curtis
string messageFromDL(int fromDL);

#endif /* defined(__MINIFTP__utilities__) */
