// utilities.cpp
// MINIFTP
// CS513-TEAM2

#include "header.h"
#include "utilities.h"

//Parses the input message and breaks it down into its corresponding
//command and arguments. It returns the number of arguments.
void parseMessage(const char* msg, string & command,
                  vector<string> & arguments){
    bool arg = false;
    string tmp;
    
    for(int i = 0; i < strlen(msg); i++){
        if(msg[i] == APP_DELIM){
            if(!arg){
                command = tmp;
                arg = true;
            }
            else{
                arguments.push_back(tmp);
            }
            tmp="";
        }
        else{
            tmp += msg[i];
        }
    }

}

void parseUserMessage(char* msg, string & command,
                  vector<string> & arguments){
    char *tmp=strtok(msg," "); //get the first token, i.e. the command
    if(tmp)
        command=string(tmp);
    
    while (tmp != NULL){
        char* str2 = strtok (NULL, " "); //fill in the next argument
        tmp = str2;
        
        if(tmp == NULL) //end of the message
            break;        
        //copy the value to the returning list of arguments:
        arguments.push_back(string(str2));
    }
}


int sendData(int cmd, vector<string> parameters, int toDL, int fromDL, int signalFromDL){
    ifstream fin;
    fin.open(parameters[0].c_str(),ios::in);
    if(!fin.is_open()){
        cerr << "File failed to open " <<  parameters[0]  << " in send data: This should never happen!\n";
        exit(1);
    }
    
    packet pack;
    while(!fin.eof()){
        memset(pack.data,'\0',PACKET_DATA_SIZE); // null ensures strlen works
        fin.read(pack.data,PACKET_DATA_SIZE-1);
        
        int bytesRead=(int)fin.gcount(); // originally a long casted to int
        pack.dataSize = bytesRead;
        
        pack.last = 0;
        
        if(fin.eof())
            pack.last = 1;
            
        to_data_link(&pack, toDL, fromDL, signalFromDL);
    }
    return 1;
}

int receiveData(vector<string> arguments, int fromDL){
    ofstream fout;
    fout.open(arguments[0].c_str());
    if(!fout.is_open()){
        cout << "Failed to open file\n";
        return 0;
    }
    
    packet pack;    
    do{
        memset(pack.data,'\0',PACKET_DATA_SIZE); // null ensures strlen works
        
        int bytesRec=(int)read(fromDL,&pack,sizeof(packet));

        if(bytesRec<0){
            perror("Receive data from DL failed");
        }
        pack.data[pack.dataSize]='\0';
        
        fout.write(pack.data,pack.dataSize);
    } while(!pack.last);
    
    fout.close();
    return 1;
}

// Sends a string to server - separated by DLE byte
// This function assumes that the size of a message is never greater than 184
//bytes, which fits into one packet.
void sendMessage(int cmd, vector<string> parameters, int toDL, int fromDL, int signalFromDL){
    ostringstream convert;
    convert << cmd;
    
    string msg_buffer=convert.str();
    for(int i=0;i<parameters.size();i++){
        msg_buffer+= APP_DELIM + parameters[i];
    }
    msg_buffer+= APP_DELIM;

    packet pack;
    memcpy(pack.data, msg_buffer.c_str(), PACKET_DATA_SIZE);
    pack.dataSize = (int)msg_buffer.length();
    pack.last = 1;
    to_data_link(&pack, toDL, fromDL, signalFromDL);
}

//Sends a packet to the data-link layer
int to_data_link(packet *p, int toDL, int fromDL, int sigFromDL){    
    //We assume that it waits until a message comes from data-link:
    char dlStatus;
    read(sigFromDL, &dlStatus, 1);
    write(toDL, p, sizeof(packet));
    return 1;
}

string messageFromDL(int fromDL){
    char message[PACKET_DATA_SIZE+1];
    memset(message,'\0',PACKET_DATA_SIZE+1);

    packet packetReceived;
    
    read(fromDL,&packetReceived,sizeof(packet));
    
    memcpy(message,packetReceived.data,packetReceived.dataSize);
    
    message[packetReceived.dataSize]='\0';
    
    return string(message);
}

























