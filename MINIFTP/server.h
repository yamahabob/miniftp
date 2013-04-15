//
// server.h
// MINIFTP
// CS513-TEAM2

#ifndef server_h
#define server_h

#include <dirent.h>
#include <sys/stat.h>

//Primary Author: Salman
int processClient();
//Primary Author: Manik
int serverSetup();
//Primary Author: Manik
void receiveCommand(string & line);
//Primary Author: Curtis
string getUserHash(string username);
//Primary Author: Curtis
void checkCommandLine(int argc, char **argv);
//Primary Author: Manik
int verifyClient(string username, string password);
//Primary Author: Curtis
string exec(char* cmd);
//Primary Author: Manik
string list(vector<string> arguments,string dir);
//Primary Author: Manik
void listfiles(vector<string> arguments, string user);
//Primary Author: Manik
int grant(vector<string>arguments);
//Primary Author: Manik
int revoke(vector<string>arguments);
//Primary Author: Salman
int put(string cmd, vector<string> arguments);
//Primary Author: Curtis
int get(string cmd, vector<string> arguments);
//Primary Author: Manik
int removeFile(vector<string> filename);
//Primary Author: Manik
bool isLink(char *filename);
//Primary Author: Manik
bool is_shared(char*file);

//Types of access to a file: read, write or no access at all
typedef enum {read_access,write_access,no_access} access_types;

//Primary Author: Salman
int writeAccess(char *activeUser, char *owner, char *fileName, access_types type);
//Primary Author: Salman
int removeAccess(char *activeUser, char *owner, char *fileName);
//Primary Author: Salman
access_types readAccess(char *owner, char *fileName);


//Access records in access.tmp file
struct access_record{
    char fileName[BUFFER_SIZE]; //the file being accessed
    char owner[BUFFER_SIZE]; //the owner of the file being accessed
    char user[BUFFER_SIZE]; //the user who is accessing the file
    access_types type; //access type
};

struct shared_file
{
	char filename[50];
	char sharedUser[50];
};

//Primary Author: Manik
void addInSharedDB(string fname, string uname);
//Primary Author: Manik
bool isduplicate( string fname, string uname);
//Primary Author: Manik
vector<string> returnUserList(string fname);
//Primary Author: Manik
bool is_shared(string fname);
//Primary Author: Manik
void removeSharedfiles(string fileName);
//Primary Author: Manik
void removeAfterRevoke(string user , string revokedfile);
//Primary Author: Manik
void returnSharedfileList(string & thing);




#endif

