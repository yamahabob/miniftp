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
int killDL[2];
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
        pipe(killDL);
        int pid=-1;
        if((pid=fork())==0){
#ifdef DEBUG
            cout << "Starting DL\n";
#endif
            close(toDL[1]);
            close(fromDL[0]);
            close(signalFromDL[0]);
            close(killDL[1]);
            protocol5(1,serverSock);
#ifdef DEBUG
            cout << "Closing DL\n";
#endif
            exit(0);
        }
        else if(pid>0){
            close(toDL[0]);
            close(fromDL[1]);
            close(signalFromDL[1]);
            close(killDL[0]);
            //cout << "DATALINK PID=" << pid << endl;
            close(serverSock); // close communicating socket
            while(exit_status!=1){
                if(login()==1){
                    exit_status=processCommands(toDL[1]);// if just logout->exit=0, if logout_exit->exit=1
                    // collect?
                }
            }
            // KILL DATA LINK HERE!
            char kill='1';
            write(killDL[1],&kill,1);
            cout << "CLIENT APP ON CLIENT DEAD... KILLING DL\n";
            int status;
            waitpid(pid,&status,0); // right syntax?
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
        fprintf(stderr,"Usage: ./client -a [serverAddress] -e [error]\n");
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
    
    vector<string> empty;

    sendMessage(MSG_LOGIN,parameters,toDL[1], fromDL[0], signalFromDL[0]); // send login message to server
    
    string response=messageFromDL(fromDL[0]);

    
    string cmd;
    vector<string> arguments;
    
    parseMessage(response.c_str(), cmd, arguments);
    
    if(atoi(cmd.c_str())==MSG_OK){
        activeUser=username;
        return 1;
    }
    else{
        // MEANS MSG_ERROR RETURNED -- bad credentials
        sendMessage(MSG_OK,empty,toDL[1], fromDL[0], signalFromDL[0]);
        cout << "Invalid credentials\n";
        return 0;
    }
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
        
        timestruct before;
        timestruct after;
        timestruct result;
        
        parseUserMessage(line, cmd, arguments);
        int correctNum=checkNumArguments(cmd, arguments);
        
        if(correctNum!=1){
            cout << "Command " << line << " INVALID -- Try again" << endl;
            continue;
        }
        
        if(strcasecmp(cmd.c_str(),"login")==0){
            login(); // ignoring return value of login (1)
        }
        else if(strcasecmp(cmd.c_str(),"logout")==0){
            sendMessage(MSG_LOGOUT,arguments,toDL[1], fromDL[0], signalFromDL[0]); // send login message to server
            sleep(1); // 1 second for data link to send message
            exitstatus=1;
            activeUser="";
            break;
        }
        else if(strcasecmp(cmd.c_str(),"list")==0){
            gettimeofday(&before, NULL);
            if (arguments.size() == 0){
                list(arguments,activeUser);
            }
            else if (isValidArg(arguments))
            {
                list(arguments,activeUser);
            }
            else
            {
                cout << "Invalid arguments for list..."<<endl;
                continue;
            }
            gettimeofday(&after, NULL);

        }
        else if(strcasecmp(cmd.c_str(),"put")==0){
            // START TIMER
            gettimeofday(&before, NULL);
            put(arguments);
            gettimeofday(&after, NULL);
            // END TIMER
        }
        else if(strcasecmp(cmd.c_str(),"get")==0){
            // START TIMER
            gettimeofday(&before, NULL);
            get(arguments);
            // END TIMER
            gettimeofday(&after, NULL);
        }
        else if(strcasecmp(cmd.c_str(),"remove")==0){
            // START TIMER
            gettimeofday(&before, NULL);
            remove(arguments);
            // END TIMER
            gettimeofday(&after, NULL);
        }
        else if(strcasecmp(cmd.c_str(),"grant")==0){
            // START TIMER
            gettimeofday(&before, NULL);
            grant(arguments);
            //grant()
            // END TIMER
            gettimeofday(&after, NULL);
        }
        else if(strcasecmp(cmd.c_str(),"revoke")==0){
            // START TIMER
            gettimeofday(&before, NULL);
            revoke(arguments);
            // END TIMER
            gettimeofday(&after, NULL);
        }else{
            cout << "!Command " << line << " INVALID -- Try again¡" << endl;
        }
        
        timeDiff(&result,&after,&before);
        cout << "Command took " << result.tv_sec << " second(s) and " << ((result.tv_usec)/1000) << " millisecond(s) to process\n";
    }
    
    if(exitstatus==1)
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
        cout << "Sending data to server...\n";
        //send file
        sendData(MSG_PUT, arguments, toDL[1], fromDL[0], signalFromDL[0]);
    }
    else if(atoi(cmd.c_str())==MSG_OVERWRITE){
        char ans;
        do{
        cout << "Filename exists -- Overwrite? (y/n) -->";
        cin >>ans;
        } while(ans!='y' && ans!='n' && ans!='Y' && ans!='N');
        
        if(ans=='y' || ans =='Y'){
            sendMessage(MSG_YES, arguments, toDL[1], fromDL[0], signalFromDL[0]);
            sendData(MSG_PUT, arguments, toDL[1], fromDL[0], signalFromDL[0]);
        }
        else{
            sendMessage(MSG_NO, arguments, toDL[1], fromDL[0], signalFromDL[0]);
            return 0;
        }

    }
    else if(atoi(cmd.c_str())==MSG_IN_USE){
        cout << "File in use -- Cannot overwrite. Try again later...\n";
        return 0;
    }
    else{
        cout << "Unknown error code returned\n";
        return 0;
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
        cout << "Receiving file from server....\n";
        receiveData(arguments,fromDL[0]);
        return 1;
    }
    else if(atoi(cmd.c_str())==MSG_NO_EXIST){
        // discover error, print message
        cout << "File does not exist on the server...\n";
        return 0; // for now, assuming we aren't doing overrides
    }
    else{
        cout << "Unknown error code returned\n";
        return 0;
    }
}

int remove(vector<string> arguments){    
    sendMessage(MSG_REMOVE, arguments, toDL[1], fromDL[0], signalFromDL[0]);
    string response=messageFromDL(fromDL[0]);
    string cmd;
    vector<string> args;
    parseMessage(response.c_str(), cmd, args);
    
    if(atoi(cmd.c_str())==MSG_OK){
        cout << "File removed from server...\n";
        return 1;
    }
    else if(atoi(cmd.c_str())==MSG_NO_EXIST){
        // discover error, print message
        cout << "File does not exist on the server...\n";
        return 0; 
    }
    else if(atoi(cmd.c_str())==MSG_IN_USE){
        cout << "File in use -- Cannot remove. Try again later...\n";
        return 0; 
    }
    else{
        cout << "Unknown error code returned\n";
        return 0;
    }    
}

int grant(vector<string>arguments){
    sendMessage(MSG_GRANT, arguments, toDL[1], fromDL[0], signalFromDL[0]);
    string response=messageFromDL(fromDL[0]);
    string cmd;
    vector<string> args;
    parseMessage(response.c_str(), cmd, args);
    
    if(atoi(cmd.c_str())==MSG_OK){
        cout << "File shared...\n";
        return 1;
    }
    else if(atoi(cmd.c_str())==MSG_USER_NO_EXIST){
        // discover error, print message
        cout << "User does not exist...\n";
        return 0;
    }
    else if(atoi(cmd.c_str())==MSG_NO_EXIST){
        cout << "File for sharing does not exist...\n";
        return 0;
    }
    else{
        cout << "Unknown error code returned\n";
        return 0;
    }

    return 1;
}
int revoke(vector <string> arguments){
    sendMessage(MSG_REVOKE, arguments, toDL[1], fromDL[0], signalFromDL[0]);
    string response=messageFromDL(fromDL[0]);
    string cmd;
    vector<string> args;
    parseMessage(response.c_str(), cmd, args);
    
    if(atoi(cmd.c_str())==MSG_OK){
        cout << "Share access revoked...\n";
        return 1;
    }
    else if(atoi(cmd.c_str())==MSG_USER_NO_EXIST){
        // discover error, print message
        cout << "User does not exist...\n";
        return 0;
    }
    else if(atoi(cmd.c_str())==MSG_NO_EXIST){
        cout << "File not shared...\n";
        return 0;
    }
    else{
        cout << "Unknown error code returned\n";
        return 0;
    }
    
    return 1;
}

int timeDiff(timestruct *result, timestruct *x, timestruct *y)
{
    /* Perform the carry for the later subtraction by updating y. */
    if (x->tv_usec < y->tv_usec) {
        int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
        y->tv_usec -= 1000000 * nsec;
        y->tv_sec += nsec;
    }
    
    if (x->tv_usec - y->tv_usec > 1000000) {
        int nsec = (x->tv_usec - y->tv_usec) / 1000000;
        y->tv_usec += 1000000 * nsec;
        y->tv_sec -= nsec;
    }
    
    /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
    result->tv_sec = x->tv_sec - y->tv_sec;
    result->tv_usec = x->tv_usec - y->tv_usec;
    
    /* Return 1 if result is negative. */
    return x->tv_sec < y->tv_sec;
}

bool isValidArg(vector<string> arguments)
{
    if(arguments.size() == 1 && (arguments[0].compare("-d") == 0 || arguments[0].compare("-s")==0 || arguments[0].compare("-o")==0))
        return true;
    else if(arguments.size() == 2 && arguments[0].compare("-t")==0){
        return true;
    }

    return false;
}

void list(vector<string> arguments,string user)
{
    sendMessage(MSG_LIST, arguments, toDL[1], fromDL[0], signalFromDL[0]);
    vector<string> filename;
    string f = "ctemp"+activeUser+".txt";
    string removefile = "rm "+f;
    filename.push_back(f);
    char listfiles[200];
    int retVal=receiveData(filename,fromDL[0]);
    
    if(retVal==0){
        cout << "Error in receiving..\n";
    }
    else{
        ifstream fin;
        fin.open(f.c_str(),ios::in);
        
        if(!fin.is_open()){
			return;
        }
        while(!fin.eof())
        {
            memset(listfiles,'\0',200);
            fin.read(listfiles,199);
            cout << listfiles;
        }
        fin.close();
        system(removefile.c_str());
    }
    
}

int checkNumArguments(string cmd, vector<string> arguments){
    // switch statement to check number of arguments based on cmd
    int isvalid = 0;
    //cout << cmd <<endl;
    //cout << arguments.size() <<endl;
    if (arguments.size()==0)
    {
        if (strcasecmp(cmd.c_str(),"logout")==0 || strcasecmp(cmd.c_str(),"list")==0)
        {
            isvalid =1;
        }
    }
    else if (arguments.size()==1)
    {
        //cout << "arg ---2 "<<endl ;
        if (strcasecmp(cmd.c_str(),"login")==0 || strcasecmp(cmd.c_str(),"put")==0||cmd.compare("get")==0 ||strcasecmp(cmd.c_str(),"remove")==0||cmd.compare("list")==0)
        {
            //cout << arguments[0] <<endl;
            //if (arguments[0].find(".") != string::npos )
            //{
                isvalid = 1;
            //}
            
        }
    }
    else if (arguments.size()==2)
    {
        if ( cmd.compare("grant")==0 || cmd.compare("revoke")==0 || cmd.compare("list")==0)
        {
            isvalid = 1;
        }
    }
	
    return isvalid;
}

