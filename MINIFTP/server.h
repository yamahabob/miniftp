#ifndef server_h
#define server_h


int processClient();
int serverSetup();
void receiveCommand(string & line);
string getUserHash(string username);
void checkCommandLine(int argc, char **argv);
int verifyClient(string username, string password);
string exec(char* cmd);

#endif

