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

#include "header.h"
#include "server.h"
#include "data_link.h"
#include "utilities.h"


// Both must be defined as extern in datalink.cpp
// so it knows to look here when linking occurs
int toDL[2];
int fromDL[2];


int main(int argc, char **argv){

    int sock=serverSetup();
    
    
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
        }//cout <<"in while pid2="<<pid2<<endl;}
        
        cout <<"sitting on accept\n";
		client_sock = accept(sock, (struct sockaddr *)&client, &sin_size);
		if (client_sock == -1) {
			perror("error accept");
			exit(1);
		}
        
        int pid;
        if((pid=fork())==0){//child process
            // Two sets of pipes:
            // first pipe allows out ouput (on the FD created by pipe) to be DL layer's input
            // second pipe allows our input to be DL layer's output
            if(pipe(toDL))
            {
                fprintf (stderr, "Pipe failed.\n");
                return EXIT_FAILURE;
            }

            if(pipe(fromDL))
            {
                fprintf (stderr, "Pipe failed.\n");
                return EXIT_FAILURE;
            }

            
            int pidDL;
            if((pidDL=fork())==0){
#ifdef DEBUG
                cout << "DL layer started\n";
                //write(fromDL[1],"hello",5);
                //char buffer[6];
                //read(fromDL[0],buffer,5);
                //buffer[5]='\0';
                //cout << "read from pipe "<< buffer<<endl;
                //cout << "done reading from pipe\n";
#endif
                close(toDL[1]);
                close(fromDL[0]);
                //write(fromDL[1],"ShelloE",7);
                protocol5(client_sock); //?
                exit(0);
            }
            else if (pidDL>0){
#ifdef DEBUG
                cout << "Processing client started\n";
#endif
                close(toDL[0]);
                close(fromDL[1]);
                close(sock);
                //int returnStatus=processClient(client_sock); // shouldn't have a socket anymore because DL does all transfers
                int returnStatus=processClient();
                wait(&pidDL); // HOW DOES DL KNOW TO DIE?
                exit(returnStatus);
                
                //exit(processClient(client_sock));
            }
            else{
                perror("OMG FORK FAILED FOR DL");
                exit(1);
            }
            //cout << "shouldn't happen EVER\n";
        }
        else if(pid>0){ // parent
            // do nothing for now
            cout <<"Parent do nothing\n";
            close(client_sock);
        }
        else{ // fork failed
            perror("OMG FORK FAILED FOR CLIENT");
            exit(1);
        }
	}
}

int processClient(){
    int sock=0; // THIS NO LONGER EXISTS should be writing everything to pipe, which is global (toDL/fromDL)
    
    // "global" activeUser
    string activeUser;
    
    char msg[BUFFER_SIZE];
    memset(msg,0,BUFFER_SIZE);
    //size_t bytesRec=recv(sock,msg,BUFFER_SIZE,0);
//    msg[bytesRec]='\0';
//#ifdef DEBUG
//    cout <<"Received="<< msg<< " rec bytes=" << bytesRec << endl;
//#endif
//    if((int)bytesRec < 0){
//        perror("Receive from client");
//        cout<<"strerrno=" <<errno << endl;
//        exit(1);
//    }
    
    string messageReceived=messageFromDL(fromDL[0]);
    //exit(1);
    cout << "!!!Message received from DL in Server app--" << messageReceived << endl;
    //cout << "Pausing Server APP. if continued, things will break";
    
    // LOGIN
    string cmd="";
    vector<string> arguments;
    parseMessage(msg, cmd, arguments);
    
    
    
    
    // Verifying credentials against "shadow" file
    //if(verifyClient()){
    if(1){
        activeUser=""; // fill in
        
        //send "OK"
        //size_t bytesSent=send(sock,to_string(MSG_OK).c_str(),sizeof(MSG_OK),0);
        //if((int)bytesSent < sizeof(MSG_OK)){
        //    perror("Sending to client:");
        //    exit(1);
        //}
        
        vector<string> empty;
        sendMessage(MSG_OK,empty,toDL[1], fromDL[0]); // send login message to server
        
        
        while(1){ // until logout
            // receive next command from client
            string line;
            receiveCommand(line, sock);
                        
            string cmd;
            vector<string> arguments;
            parseMessage(line.c_str(), cmd, arguments);
            
           // logout
           if(strcasecmp(cmd.c_str(),"1")==0){
                activeUser="";
                break;
            }
            // list
            else if(strcasecmp(cmd.c_str(),"2")==0){
            }
            // put
            else if(strcasecmp(cmd.c_str(),"3")==0){
                size_t bytesSent=send(sock,to_string(MSG_OK).c_str(),sizeof(MSG_OK),0);
                if((int)bytesSent < sizeof(MSG_OK)){
                    perror("Sending to client:");
                    exit(1);
                }
                int retVal=receiveData(arguments, sock);
                if(retVal==0)
                    cout << "Failed to received file\n";
                else
                    cout << "File received successfully\n";
            }
            // get
            else if(strcasecmp(cmd.c_str(),"4")==0){
                // will need to verify file exists. for now assume it does
                size_t bytesSent=send(sock,to_string(MSG_OK).c_str(),sizeof(MSG_OK),0);
                if((int)bytesSent < sizeof(MSG_OK)){
                    perror("Sending to client:");
                    exit(1);
                }
                int retVal=sendData(MSG_DATA,arguments, sock);
                if(retVal==0)
                    cout << "Failed to send file\n";
                else
                    cout << "File sent successfully\n";
                
            }
            // remove
            else if(strcasecmp(cmd.c_str(),"5")==0){
            }
            // grant
            else if(strcasecmp(cmd.c_str(),"6")==0){
            }
            // revoke
            else if(strcasecmp(cmd.c_str(),"7")==0){
            }
        }
    }
    else{
        //send failed
        return 0;
    }
    
#ifdef DEBUG
    cout <<"Successful login\n";
#endif
    
    return 0;
}

void receiveCommand(string & line, int sock){
   /*
    //char msg[PACKET_SIZE];
    //packet msg;
    //memset(msg.data,0,PACKET_SIZE);
    //size_t bytesRec=recv(sock,msg.data,PACKET_SIZE,0);
    //msg.data[bytesRec]='\0';
    
    cout <<"Read to receive command from client\n";
    
    
#ifdef DEBUG
    cout <<"Received="<< msg.data<< " rec bytes=" << bytesRec << endl;
#endif
    if((int)bytesRec < 0){
        perror("Receive from client");
        cout<<"strerrno=" <<errno << endl;
        exit(1);
    }
    line=string(msg.data);
    */
    cout << "Wanting command from client!\n";
    line=messageFromDL(fromDL[0]);
    int temp;
    cin >> temp;
    cout << "First command from client=" << line << endl;
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

