//
//Client.h
//MINIFTP
//CS513-TEAM2

#ifndef client_h
#define client_h

typedef struct timeval timestruct;

//Primary Author: Curtis
void checkCommandLine(int argc, char** argv); // sets serverAddress

string serverAddress;

//Primary Author: Manik
int connectToServer();

//Primary Author: Salman
int processCommands(int fd);

//Primary Author: Manik
string receiveResponse(int sock);

int serverSocket;

// Commands
//Primary Author: Manik
int login();
//Primary Author: Salman
int put(vector<string> arguments);
//Primary Author: Curtis
int get(vector<string> arguments);
//Primary Author: Manik
int remove(vector<string> arguments);
//Primary Author: Manik
int grant(vector<string>arguments);
//Primary Author: Manik
int timeDiff(timestruct *result, timestruct *x, timestruct *y);
//Primary Author: Manik
bool isValidArg(vector<string> arguments);
//Primary Author: Manik
void list(vector<string> arguments,string user);
//Primary Author: Manik
int checkNumArguments(string cmd, vector<string> arguments);
//Primary Author: Manik
int revoke(vector <string> arguments);

#endif
