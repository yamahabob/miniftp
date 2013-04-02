#ifndef client_h
#define client_h
void checkCommandLine(int argc, char** argv); // sets serverAddress
string serverAddress;
int connectToServer();
int processCommands(int fd, int sock);
string receiveResponse(int sock);

int serverSocket;

// Commands
int login(int sock);
int put(vector<string> arguments, int sock);
int get(vector<string> arguments, int sock);

#endif