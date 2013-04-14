/* Team 2
   CS513
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <openssl/sha.h>
#include <sstream>

#include "header.h"
#include "server.h"
#include "data_link.h"
#include "utilities.h"


// Both must be defined as extern in datalink.cpp
// so it knows to look here when linking occurs
int toDL[2];
int fromDL[2];
int signalFromDL[2];
int killDL[2];
int errorRate=-1;
string activeUser;

int main(int argc, char **argv){
    checkCommandLine(argc, argv);
    int sock=serverSetup();
    srand((int)time(NULL));
    
    
	printf("Now accepting connections\n");

	int client_sock;
	struct sockaddr_storage client;
	socklen_t sin_size;


	while(1){ // really only once
		sin_size = sizeof client;
        // collect returning children here
        struct rusage rusage;
        int status;

        int pid2;
        while((pid2=wait4(-1,&status,WNOHANG,&rusage)>0)){
            //cout << "reaped childID=" <<pid2 <<endl;
        }
        
		client_sock = accept(sock, (struct sockaddr *)&client, &sin_size);
		if (client_sock == -1) {
			perror("error accept");
			exit(1);
		}
        
        cout << "Accepted new client" <<endl;
        
        int pid;
        if((pid=fork())==0){//child process
            // Two sets of pipes:
            // first pipe allows out ouput (on the FD created by pipe) to be DL layer's input
            // second pipe allows our input to be DL layer's output
            if(pipe(toDL)){
                fprintf (stderr, "Pipe failed.\n");
                return EXIT_FAILURE;
            }
            if(pipe(fromDL)){
                fprintf (stderr, "Pipe failed.\n");
                return EXIT_FAILURE;
            }
            if(pipe(signalFromDL)){
                fprintf (stderr, "Pipe failed.\n");
                return EXIT_FAILURE;
            }
            if(pipe(killDL)){
                fprintf (stderr, "Pipe failed.\n");
                return EXIT_FAILURE;
            }

            
            int pidDL;
            if((pidDL=fork())==0){
                close(toDL[1]);
                close(fromDL[0]);
                close(signalFromDL[0]);
                close(killDL[1]);
                protocol5(0,client_sock); //?
                cout << "DATA LINK LAYER DEAD\n";
                exit(0);
            }
            else if (pidDL>0){
#ifdef DEBUG
                //cout << "Processing client started\n";
#endif
                close(toDL[0]);
                close(fromDL[1]);
                close(signalFromDL[1]);
                close(killDL[0]);
                close(sock);
                close(client_sock);
                int returnStatus=processClient();
                cout << "CLIENT APP ON SERVER DEAD... KILLING DL\n";
                char kill='1';
                write(killDL[1],&kill,1);
                wait(&pidDL);
                exit(returnStatus);
            }
            else{
                perror("OMG FORK FAILED FOR DL");
                exit(1);
            }
            //cout << "shouldn't happen EVER\n";
        }
        else if(pid>0){ // parent
            //cout <<"Parent do nothing\n";
        }
        else{ // fork failed
            perror("OMG FORK FAILED FOR CLIENT");
            exit(1);
        }
	}
}

int processClient(){
    vector<string> empty;
    string messageReceived=messageFromDL(fromDL[0]); // receive login FIRST
        
    // Break up login message
    string cmd="";
    vector<string> arguments;
    parseMessage(messageReceived.c_str(), cmd, arguments);
    
    // if they didn't send login, error
    if(strcasecmp(cmd.c_str(),"0")!=0 || arguments.size()!=2){
        sendMessage(MSG_ERROR,empty,toDL[1], fromDL[0], signalFromDL[0]); // make sure client knows to get message
        return 1;
    }
    
    // Verifying credentials against "shadow" file
    if(verifyClient(arguments[0], arguments[1])){
        activeUser=arguments[0]; // fill in
        
        vector<string> empty;
        sendMessage(MSG_OK,empty,toDL[1], fromDL[0], signalFromDL[0]); // send login message to server
        
        while(1){ // until logout
            // receive next command from client
            string line;
            receiveCommand(line);
            string cmd;
            vector<string> arguments;
            parseMessage(line.c_str(), cmd, arguments);
            
           // logout
           if(atoi(cmd.c_str()) == MSG_LOGOUT){
                activeUser="";
                break;
            }
            // list
            else if(atoi(cmd.c_str()) == MSG_LIST){
                //cout<<"listing files for user " << activeUser << endl;
                listfiles(arguments,activeUser);
            }
            // put
            else if(atoi(cmd.c_str()) == MSG_PUT){
                /*
                 echo y | rm test.c // returns line below
                 rm: test.c: Operation not permitted
                 chflags nouchg test.c
                 echo y | rm test.c // has not return
                 */
                bool receive = true; //receive the file
                string filename=activeUser+"/"+arguments[0];
                if(access(filename.c_str(), F_OK) != -1){ //file exists?
                    //file exists
                    sendMessage(MSG_OVERWRITE,empty,toDL[1], fromDL[0], signalFromDL[0]);
                    string confirmationMsg = messageFromDL(fromDL[0]);
                    parseMessage(confirmationMsg.c_str(), cmd, arguments);
                    
                    if(atoi(cmd.c_str()) == MSG_NO) //The user does not want to overwrite
                        receive = false; //so don't receive the file
                }
                else{
                    sendMessage(MSG_OK,empty,toDL[1], fromDL[0], signalFromDL[0]); //tell the client it is ok to send the file
                }
                
                if(receive){
                    arguments[0]=filename;
                    int retVal=receiveData(arguments,fromDL[0]);
                    
                    if(retVal==0)
                        cout << "Failed to received file\n";
                    else
                        cout << "File received successfully\n";
                }
            }
            // get
            else if(atoi(cmd.c_str()) == MSG_GET){
                string filename=activeUser+"/"+arguments[0];
                arguments[0]=filename;
                cout << "file to GET=" << filename <<endl;
                if(access(filename.c_str(), F_OK) != -1){ //file exists?
                    sendMessage(MSG_OK,empty,toDL[1], fromDL[0], signalFromDL[0]);
                    int retVal=sendData(MSG_PUT, arguments, toDL[1], fromDL[0], signalFromDL[0]);
                    if(retVal==0)
                        cout << "Failed to send file\n";
                    else
                        cout << "File sent successfully\n";
                }
                else{
                    //tell the client that the file does not exist.
                    sendMessage(MSG_NO_EXIST,empty,toDL[1], fromDL[0], signalFromDL[0]);
                }
                
            }
            // remove
            else if(atoi(cmd.c_str()) == MSG_REMOVE){
                if(access(arguments[0].c_str(), F_OK) != -1){ //file exists?
                    remove(arguments[0].c_str());
                    sendMessage(MSG_OK,empty,toDL[1], fromDL[0], signalFromDL[0]);
                }
                else{
                    //tell the client that the file does not exist.
                    sendMessage(MSG_NO_EXIST,empty,toDL[1], fromDL[0], signalFromDL[0]);
                }
            }
            // grant
            else if(atoi(cmd.c_str()) == MSG_GRANT){
                grant(arguments);
            }
            // revoke
            else if(atoi(cmd.c_str()) == MSG_REVOKE){
                 revoke(arguments);
            }
            else{
                cout << "Invalid command from " << activeUser <<endl;
                sendMessage(MSG_ERROR,empty,toDL[1], fromDL[0], signalFromDL[0]); // not valid command
            }
        }
    }
    else{
        // login failed
        cout << "Login failed for " << arguments[0] <<endl;
        sendMessage(MSG_ERROR,empty,toDL[1], fromDL[0], signalFromDL[0]); // make sure client knows to get message
        string waitForResponse=messageFromDL(fromDL[0]);
        return 1;
    }    
    return 0;
}

void receiveCommand(string & line){
    line=messageFromDL(fromDL[0]);
}

int serverSetup(){
    struct addrinfo info, *res;
	int sock;
	// first, load up address structs with getaddrinfo():
	memset(&info, 0, sizeof info); // inialize to 0
	info.ai_family = AF_INET;  // use IPv4
	info.ai_socktype = SOCK_STREAM; // TCP
	info.ai_flags = AI_PASSIVE;     // fill in my IP for me
	int status=getaddrinfo(NULL, PORT.c_str(), &info, &res); // get info for me (server) and store in res
	if(status!=0){
		fprintf(stderr, "Error with getaddrinfo: %s\n", gai_strerror(status));
		exit(1);
	}
    
	// make a socket for accepting clients
	sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    int option = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
	if(sock<=0){
		fprintf(stderr,"Error creating socket\n");
		exit(1);
	}
    
	// bind it to the port we passed in to getaddrinfo():
    if(bind(sock, (const struct sockaddr*)res->ai_addr, res->ai_addrlen)==-1){
		close(sock);
		perror("Bind failed");
		exit(1);
	}
     
	if(listen(sock, 5) == -1){
		perror("Listen failed");
		exit(1);
	}
    return sock;
}

string getUserHash(string username){
	ifstream passwd("shadow", ios::in);
	if(!passwd.is_open()){ cout << "Error: can't open shadow file\n"; exit(1);}
    
	string line;
	while(passwd>>line){
		string delimiters = ":";
		size_t current=0;
		size_t next =-1;
		next=line.find_first_of(delimiters,current);
		string temp=line.substr(current,next-current);
		if(temp==username)
			return line.substr(next-current+1,line.length());
        
	}
	return "";
}

// Checks if user wishes to connected to server other than localhost
void checkCommandLine(int argc, char **argv){
    if(argc>1){
        if(strcmp(argv[1],"-e")==0 && argc>=3){
            errorRate=atoi(argv[2]);
        }
        else{
            fprintf(stderr,"Usage: ./server -e [error]\n");
            exit(1);
        }
    }
}

string exec(char* cmd) {
	FILE* pipe = popen(cmd, "r");
	if (!pipe) return "ERROR";
	char buffer[128];
	string result = "";
	while(!feof(pipe)) {
		if(fgets(buffer, 128, pipe) != NULL)
			result += buffer;
	}
	pclose(pipe);
	return result;
}

int verifyClient(string username, string password){
    string existingHash=getUserHash(username);
    if(existingHash==""){
        return 0;
    }
    
    // FIXXXXXXXXXXXXXXX
    //string command="echo " + password + ">out_"+convert.str() +" && cat out_" + convert.str() + " | sha256sum | cut -d \" \" -f 1";
    ostringstream convert;
    convert << getpid();
    string command="echo " + password + ">out_"+convert.str() +" && cat out_" + convert.str() + " | shasum -a 256 | cut -d \" \" -f 1";
    string providedHash=exec((char*)command.c_str());
    //command="rm out_"+convert.str();
    system(command.c_str());
    providedHash[providedHash.length()-1]='\0';
    if(strcmp(providedHash.c_str(),existingHash.c_str())==0){
        return 1;
    }
    
    return 0;
}

void listfiles(vector<string> arguments, string user)
{
    string command;
    string listOfFiles;
    string listofFiles;
    ofstream fout;
    vector<string> filename;
    string f = "stemp"+user+".txt";
    string removefile = "rm "+f;
    fout.open(f.c_str());
    
    if(arguments.size()==0)
    {
        command= "ls "+user;
        listOfFiles=exec((char*)command.c_str());
        cout << "listoffiles for ls -->" << listOfFiles <<endl;
    }
    else if(arguments[0].compare("-d") == 0)
    {
        command= "du -sk ./"+user +"/*";
        listOfFiles=exec((char*)command.c_str());
    }
    else if(arguments[0].compare("-o") == 0)
    {
        cout << "list -o";
        listOfFiles= list(arguments,user);
    }
    else if (arguments.size() == 2 || arguments[0].compare("-t") == 0)
    {
        cout << "inside -t";
        listOfFiles= list(arguments,user);
    }
    
    
    fout.write(listOfFiles.c_str(),listOfFiles.size());
    fout.close();
    
    filename.push_back(f.c_str());
    sendData(MSG_PUT, filename, toDL[1], fromDL[0], signalFromDL[0]);
    system(removefile.c_str());
}

string list(vector<string> arguments,string dir)
{
    string listoffiles;
    DIR *dirp = opendir( dir.c_str() );
    if ( dirp != NULL){
        struct dirent *dp = 0;
        if(arguments.size() ==2){
            arguments[1].insert(0,".");
            while ( (dp = readdir( dirp )) != 0 ){
                struct stat file;
                stat( dp->d_name, &file);    
                if(strstr(dp->d_name,arguments[1].c_str()) >  0){
                    listoffiles += dp->d_name;
                    listoffiles += "\n";
                }
                listoffiles += "\0";
            }
        }
        else{
            while ( (dp = readdir( dirp )) != 0 ){
                if(dp->d_type == DT_LNK){
                    listoffiles += dp->d_name;
                    listoffiles += "\n";
                }
            }
            listoffiles[listoffiles.size()] = '\0';
        }
    }
    return listoffiles;
}

int grant(vector<string>arguments){
    
    string userReceivingFile=arguments[0];
    string originalFile=arguments[1];
    string filename=activeUser+"/"+arguments[1];
    
    
    vector <string> empty;
    
    if(getUserHash(userReceivingFile)==""){
        // user doesn't exist
        sendMessage(MSG_USER_NO_EXIST,empty,toDL[1], fromDL[0], signalFromDL[0]); 
        return 0;
    }
    else if(access(filename.c_str(), F_OK) == -1){
        // file doesn't exist
        sendMessage(MSG_NO_EXIST,empty,toDL[1], fromDL[0], signalFromDL[0]); 
        return 0;

    }
    

    string command="cd " + userReceivingFile + " && ln -s ../" + filename + " shared:" +activeUser + ":" + originalFile;
    cout << "command=" << command <<endl;
    string ret=exec((char*)command.c_str());
    cout << "ret=" << ret <<endl;
    
    sendMessage(MSG_OK,empty,toDL[1], fromDL[0], signalFromDL[0]);
    
    return 1;
}

int revoke(vector<string>arguments){
    
    string userReceivingFile=arguments[0];
    string filename=activeUser+"/"+arguments[1];
    string originalFile=arguments[1];
    string fileToRemove=userReceivingFile + "/shared:" +activeUser + ":" + originalFile;
    cout << "to remove="<< fileToRemove <<endl;
    
    vector <string> empty;
    
    if(getUserHash(userReceivingFile)==""){
        // user doesn't exist
        sendMessage(MSG_USER_NO_EXIST,empty,toDL[1], fromDL[0], signalFromDL[0]); 
        return 0;
    }
    else if(access(fileToRemove.c_str(), F_OK) == -1){
        // file doesn't exist
        sendMessage(MSG_NO_EXIST,empty,toDL[1], fromDL[0], signalFromDL[0]); 
        return 0;
        
    }
    
    string command="rm " + fileToRemove;
    cout << command << endl;
    string ret=exec((char*)command.c_str());
    cout << "ret=" << ret <<endl;
    
    sendMessage(MSG_OK,empty,toDL[1], fromDL[0], signalFromDL[0]); // make sure client knows to get message
    
    return 1;
}
    



