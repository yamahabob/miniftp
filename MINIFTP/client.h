#ifndef client_h
#define client_h

typedef struct timeval timestruct;

void checkCommandLine(int argc, char** argv); // sets serverAddress
string serverAddress;
int connectToServer();
int processCommands(int fd);
string receiveResponse(int sock);

int serverSocket;

// Commands
int login();
int put(vector<string> arguments);
int get(vector<string> arguments);
int remove(vector<string> arguments);
int grant(vector<string>arguments);
int timeDiff(timestruct *result, timestruct *x, timestruct *y);
bool isValidArg(vector<string> arguments);
void list(vector<string> arguments,string user);
int checkNumArguments(string cmd, vector<string> arguments);
int revoke(vector <string> arguments);

#endif
