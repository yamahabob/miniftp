#ifndef server_h
#define server_h



int processClient(int sock);
int serverSetup();
void receiveCommand(string & line, int sock);


#endif

