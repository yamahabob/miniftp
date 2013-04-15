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
int put(string cmd, vector<string> arguments);
int get(string cmd, vector<string> arguments);
int removeFile(vector<string> filename);

bool isLink(char *filename);
bool is_shared(char*file);

//Types of access to a file: read, write or no access at all
typedef enum {read_access,write_access,no_access} access_types;

int writeAccess(char *activeUser, char *owner, char *fileName, access_types type);
int removeAccess(char *activeUser, char *owner, char *fileName);
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


void addInSharedDB(string fname, string uname);
bool isduplicate( string fname, string uname);
vector<string> returnUserList(string fname);
bool is_shared(string fname);
void removeSharedfiles(string fileName);
void removeAfterRevoke(string user , string revokedfile);
void returnSharedfileList(string & thing);




#endif

