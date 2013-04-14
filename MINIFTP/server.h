#ifndef server_h
#define server_h

#include <dirent.h>
#include <sys/stat.h>

int processClient();
int serverSetup();
void receiveCommand(string & line);
string getUserHash(string username);
void checkCommandLine(int argc, char **argv);
int verifyClient(string username, string password);
string exec(char* cmd);
string list(vector<string> arguments,string dir);
void listfiles(vector<string> arguments, string user);
int grant(vector<string>arguments);
int revoke(vector<string>arguments);

#endif

