/* Team 2
   CS513
*/
#include "header.h" // shared functions/variables between server/client
#include "client.h" // relative only to client
#include "data_link.h"

string activeUser="";

// runs the program
int main(int argc, char **argv){
    
    checkCommandLine(argc,argv);
    int serverSock=connectToServer();
    int exit_status=0;
    while(!exit_status){
        if(login(serverSock)==1){
            // data link fork/pipes are created
            int fd[2];
            pipe(fd);
            int pid;
            if((pid=fork())==0){
                close(fd[0]);
                protocol5(fd[1],serverSock);
                exit(0);
            }
            else if(pid>0){
                close(fd[1]);
                exit_status=processCommands(fd[0],serverSock); // if just logout->exit=0, if logout_exit->exit=1
                // collect?
                wait(&pid); // right syntax?
            }
            else{
                perror("OMG FORK");
                exit(1);
            }
        }
        
    }
	close(serverSock); // close communicating socket
	return 0;
}

// Checks if user wishes to connected to server other than localhost
void checkCommandLine(int argc, char **argv){
    if(argc==1)
        serverAddress="127.0.0.1";
    else if(argc==2){
        serverAddress=string(argv[1]);
    }
    else{
        fprintf(stderr,"Usage: ./client [serverAddress]\n");
    }
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
int login(int sock){
    
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
    
    sendMessage(MSG_LOGIN,parameters,sock); // send login message to server

    
#ifdef DEBUG
    cout << "LOGIN USER/PASS=" << username << "/" << password << endl;
#endif
    
    string response=receiveResponse(sock);
    
    string cmd;
    vector<string> arguments;
    
    parseMessage(response.c_str(), cmd, arguments);
    
    
    cout << "RESPONSE=" << cmd << endl;
    
    //replace this with conversion function
    if(atoi(cmd.c_str())==MSG_OK)
        return 1;
    else
        return 0;
}

int processCommands(int dl_fd, int sock){
    int exit=0;
    while(1){
        //string line;
        cout << "#>";
        char line[MAX_MESSAGE_LEN];
        memset(line,'\0',MAX_MESSAGE_LEN);
        cin.ignore(MAX_MESSAGE_LEN, '\n');
        cin.getline(line, MAX_MESSAGE_LEN);
        
        cout << "line=" << line << endl;
        string cmd;
        vector<string> arguments;
        parseMessage(line, cmd, arguments);
        
        cout << "Command=" << cmd;
        cout << "Argument of 0" << arguments[0] << endl;
        
        if(strcasecmp(cmd.c_str(),"login")==0){
            login(sock); // ignoring return value of login (1)
        }
        else if(strcasecmp(cmd.c_str(),"logout")==0){
            activeUser="";
            break;
        }
        else if(strcasecmp(cmd.c_str(),"list")==0){
        }
        else if(strcasecmp(cmd.c_str(),"put")==0){
            
            int retVal=put(arguments, sock);
            if(retVal==0)
                cout << "Failed to transmit file\n";
            else
                cout << "File transmitted successfully\n";
            
        }
        else if(strcasecmp(cmd.c_str(),"get")==0){
            int retVal=get(arguments, sock);
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
            exit=1;
            break;
        }
    }
    
    if(exit)
        return 1;
    else
        return 0;
}

string receiveResponse(int sock){
    char msg_buffer[BUFFER_SIZE];
    ssize_t bytesRec=recv(sock,&msg_buffer,100,0);
    msg_buffer[bytesRec]='\0';
    if(bytesRec < 1){
        perror("Failed to receive a message from server\n");
        exit(1);
    }
    if(bytesRec>1)
        return string(msg_buffer);
    else
        return "";
}

int checkNumArguments(int cmd, vector<string> arguments){
    // switch statement to check number of arguments based on cmd
    return 1;
}

int put(vector<string> arguments, int sock){
    ifstream fin;
    fin.open(arguments[0],ios::in);
    if(!fin.is_open()){
        cout << "File " << arguments[0] << " failed to open" << endl;
        return 0;
    }
    
    sendMessage(MSG_PUT, arguments, sock);
    string response=receiveResponse(sock);
    
    string cmd;
    vector<string> args;
    parseMessage(response.c_str(), cmd, args);
    
    if(atoi(cmd.c_str())==MSG_OK){
        cout << "Received OK. Sending data....\n";
        sendData(MSG_DATA, arguments, sock);
        //send file
    }
    else{
        // discover error, print message
        return 0; // for now, assuming we aren't doing overrides
    }
    return 1;
}

int get(vector<string> arguments, int sock){
    ofstream fout;
    fout.open(arguments[0],ios::out);
    if(!fout.is_open()){
        cout << "File " << arguments[0] << " failed to open" << endl;
        return 0;
    }
    
    sendMessage(MSG_GET, arguments, sock);
    string response=receiveResponse(sock);
    
    string cmd;
    vector<string> args;
    parseMessage(response.c_str(), cmd, args);
    
    if(atoi(cmd.c_str())==MSG_OK){
        cout << "Received OK. Sending data....\n";
        receiveData(arguments, sock);
        //send file
    }
    else{
        // discover error, print message
        cout << "Get didn't get the OK!\n";
        return 0; // for now, assuming we aren't doing overrides
    }
    return 1;
}


