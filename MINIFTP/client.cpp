/* Team 2
   CS513
*/
#include "header.h" // shared functions/variables between server/client
#include "client.h" // relative only to client
#include "data_link.h"
#include "utilities.h"

// Both must be defined as extern in datalink.cpp
// so it knows to look here when linking occurs
int toDL[2];
int fromDL[2];
int signalFromDL[2];
int errorRate=-1;



string activeUser="";

// runs the program
int main(int argc, char **argv){
    checkCommandLine(argc,argv);
    int serverSock=connectToServer();
    int exit_status=0;
    srand((int)time(NULL));
    
    //while(!exit_status){
    if(1){
        // data link fork/pipes are created
        pipe(toDL);
        pipe(fromDL);
        pipe(signalFromDL);
        int pid=-1;
        if((pid=fork())==0){
#ifdef DEBUG
            cout << "Starting DL\n";
#endif
            //close(toDL[1]);
            //close(fromDL[0]);
            //close(signalFromDL[0]);
            protocol5(1,serverSock);
#ifdef DEBUG
            cout << "Closing DL\n";
#endif
            exit(0);
        }
        else if(pid>0){
            //close(toDL[0]);
            //close(fromDL[1]);
            //close(signalFromDL[1]);
            //cout << "DATALINK PID=" << pid << endl;
            //close(serverSock); // close communicating socket
            if(login()==1){
                exit_status=processCommands(toDL[1]); // if just logout->exit=0, if logout_exit->exit=1
                // collect?
            }
            cout << "CLIENT PROCESS DEAD\n";
            int status;
            wait(&status); // right syntax?
        }
        else{
            perror("OMG FORK");
            exit(1);
        }
    }
	return 0;
}

// Checks if user wishes to connected to server other than localhost
void checkCommandLine(int argc, char **argv){
    if(argc==1)
        serverAddress="127.0.0.1";
    else if(argc>1){
        if(strcmp(argv[1],"-a")==0 && argc>=3){
            serverAddress=string(argv[2]);
            if(argc>=5 && strcmp(argv[3],"-e")==0){
                errorRate=atoi(argv[4]);
            }
        }
        else if(strcmp(argv[1],"-e")==0 && argc>=3){
            errorRate=atoi(argv[2]);
        }
        else{
            fprintf(stderr,"Usage: ./client -a [serverAddress] -e [error]\n");
            exit(1);
        }
    }
    else{
        fprintf(stderr,"Usage: ./client [serverAddress] -r [error]\n");
        exit(1);
    }
    //cout << "Error rate=" << errorRate << endl;
}

// Setup socket to server or DIE
int connectToServer(){
    int status; // return status for getaddrinfo
	struct addrinfo hints;
	struct addrinfo *servinfo;  // for info stored by getaddrinfo()
	memset(&hints, 0, sizeof hints); // make sure data is initialized to 0
	hints.ai_family = AF_INET;     // we are using IPv4
	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets (not UDP)
    
	// retrieve information for connecting
	status = getaddrinfo(serverAddress.c_str(), PORT.c_str(), &hints, &servinfo);
	if(status!=0){
		fprintf(stderr, "Error with getaddrinfo: %s\n", gai_strerror(status));
		exit(1);
	}
    
	// create socket to connect through
	int sock=socket(servinfo->ai_family,servinfo->ai_socktype,servinfo->ai_protocol);
	if(sock<=0){
		fprintf(stderr,"Error creating socket\n");
		exit(1);
	}
    
	// connect to server
	if(connect(sock,servinfo->ai_addr,servinfo->ai_addrlen)==-1){
		close(sock);
		perror("Error connecting to server");
		exit(1);
	}
    
    return sock;
}

// Prompts user for username and password to relay to server
int login(){
    
    if(activeUser!=""){
        cout << "User " << activeUser << " already logged in\n";
        return 1;
    }
    
    string username;
    string password;
    cout <<"Enter username:";
    cin >>username;
    password=string((char*)getpass("Enter password-->")); // built-in function to hide terminal input
    
    vector <string> parameters;
    parameters.push_back(username);
    parameters.push_back(password);
    
#ifdef DEBUG
    //cout << "LOGIN USER/PASS=" << username << "/" << password << endl;
#endif
    
    vector<string> empty;

    sendMessage(MSG_LOGIN,parameters,toDL[1], fromDL[0], signalFromDL[0]); // send login message to server
    
    //string response=receiveResponse(sock);
    string response=messageFromDL(fromDL[0]);

    
    string cmd;
    vector<string> arguments;
    
    parseMessage(response.c_str(), cmd, arguments);
    
    //replace this with conversion function
    if(atoi(cmd.c_str())==MSG_OK){
        activeUser=username;
        return 1;
    }
    else
        return 0;
}

int processCommands(int dl_fd){
    int exitstatus=0;
    cin.ignore(MAX_MESSAGE_LEN, '\n');
    while(1){
        //string line;
        cout << "#>";
        char line[MAX_MESSAGE_LEN];
        memset(line,'\0',MAX_MESSAGE_LEN);
        cin.getline(line, MAX_MESSAGE_LEN);
        
        string cmd;
        vector<string> arguments;
        
        parseUserMessage(line, cmd, arguments);
        
        //cout << "Command=" << cmd;
        //cout << "Argument of 0" << arguments[0] << endl;
        
        if(strcasecmp(cmd.c_str(),"login")==0){
            login(); // ignoring return value of login (1)
        }
        else if(strcasecmp(cmd.c_str(),"logout")==0){
            activeUser="";
            break;
        }
        else if(strcasecmp(cmd.c_str(),"list")==0){
        }
        else if(strcasecmp(cmd.c_str(),"put")==0){
            int retVal=put(arguments);
            if(retVal==0)
                cout << "Failed to transmit file\n";
            else
                cout << "File transmitted successfully\n";
            
        }
        else if(strcasecmp(cmd.c_str(),"get")==0){
            int retVal=get(arguments);
            if(retVal==0)
                cout << "Failed to receive file\n";
            else
                cout << "File received successfully\n";
        }
        else if(strcasecmp(cmd.c_str(),"remove")==0){
        }
        else if(strcasecmp(cmd.c_str(),"grant")==0){
        }
        else if(strcasecmp(cmd.c_str(),"revoke")==0){
        }
        else if(strcasecmp(cmd.c_str(),"exit")==0){
            activeUser="";
            exitstatus=1;
            break;
        }else{
            cout << "Command " << line << " INVALID" << endl;
            cout << "enter val:";
            int temp;
            cin >>temp;
        }
    }
    
    if(exit)
        return 1;
    else
        return 0;
}

int checkNumArguments(int cmd, vector<string> arguments){
    // switch statement to check number of arguments based on cmd
    return 1;
}

int put(vector<string> arguments){
    ifstream fin;
    fin.open(arguments[0].c_str(),ios::in);
    if(!fin.is_open()){
        cout << "File " << arguments[0] << " failed to open" << endl;
        return 0;
    }
    sendMessage(MSG_PUT, arguments, toDL[1], fromDL[0], signalFromDL[0]);

    string response=messageFromDL(fromDL[0]);

    string cmd;
    vector<string> args;
    parseMessage(response.c_str(), cmd, args);
    
    if(atoi(cmd.c_str())==MSG_OK){
        cout << "Received OK. Sending data....\n";
        //send file
        sendData(MSG_PUT, arguments, toDL[1], fromDL[0], signalFromDL[0]);
    }
    else{
        // discover error, print message
        return 0; // for now, assuming we aren't doing overrides
    }
    return 1;
}

int get(vector<string> arguments){
    ofstream fout;
    fout.open(arguments[0].c_str(),ios::out);
    if(!fout.is_open()){
        cout << "File " << arguments[0] << " failed to open" << endl;
        return 0;
    }
    
    sendMessage(MSG_GET, arguments, toDL[1], fromDL[0], signalFromDL[0]);
    string response=messageFromDL(fromDL[0]);
    
    string cmd;
    vector<string> args;
    parseMessage(response.c_str(), cmd, args);
    
    if(atoi(cmd.c_str())==MSG_OK){
        cout << "Received OK. Sending data....\n";
        receiveData(arguments,fromDL[0]);
        //send file
    }
    else{
        // discover error, print message
        cout << "Get didn't get the OK!\n";
        return 0; // for now, assuming we aren't doing overrides
    }
    return 1;
}


