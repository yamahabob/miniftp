#ifndef server_h
#define server_h


int processClient();
int serverSetup();
void receiveCommand(string & line, int sock);
string getUserHash(string username);
void checkCommandLine(int argc, char **argv);


#endif

