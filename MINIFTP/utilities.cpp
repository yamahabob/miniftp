//
//  utilities.cpp
//  MINIFTP
//
//  Created by Curtis Taylor on 3/19/13.
//  Copyright (c) 2013 WPI. All rights reserved.
//

#include "header.h"
#include "utilities.h"

//Parses the input message and breaks it down into its corresponding
//command and arguments. It returns the number of arguments.
void parseMessage(const char* msg, string & command,
                  vector<string> & arguments){
    char str[BUFFER_SIZE]; //work with a copy of the input message
    strcpy(str, msg);
    
    char *tmp=strtok (str,&DELIM); //get the first token, i.e. the command
    if(tmp)
        command=string(tmp);
    
    while (tmp != NULL){
        char* str = strtok (NULL, &DELIM); //fill in the next argument
        tmp = str;
        
        if(tmp == NULL) //end of the message
            break;
        
        //copy the value to the returning list of arguments:
        arguments.push_back(string(str));
    }
}

int sendData(int cmd, vector<string> parameters, int sock){
    ifstream fin;
    fin.open(parameters[0],ios::in);
    if(!fin.is_open()){
        cerr << "File failed to open " <<  parameters[0]  << " in send data: This should never happen!\n";
        exit(1);
    }
    
    //char msg_buffer[PACKET_SIZE];
    packet msg_buffer;
    //char temp_msg_buffer[PACKET_SIZE];
    packet temp_msg_buffer;
    int packetNum=0;
    while(!fin.eof()){
        memset(msg_buffer.data,'\0',PACKET_SIZE); // null ensures strlen works
        memset(temp_msg_buffer.data,'\0',PACKET_SIZE); // null ensures strlen works
        
        fin.read(temp_msg_buffer.data,PACKET_SIZE-2); // worst case, only 1 packet and it will
        // need both start and end DELIM
        int bytesRead=(int)fin.gcount(); // originally a long casted to int
        // returns the number of bytes read
        
        // if first packet of "message", needs start delim
        if(packetNum==0){
            memcpy(msg_buffer.data,&START_DELIM,sizeof(START_DELIM));
            bytesRead++; // to ensure correct # bytes sent
        }
        
        // regardless of which packet, copy bytes into packet
        memcpy(msg_buffer.data+strlen(msg_buffer.data), temp_msg_buffer.data, strlen(temp_msg_buffer.data));
        
        // lastly, if we are at end of the file, we are ending the message
        if(fin.eof()){
            memcpy(msg_buffer.data+strlen(msg_buffer.data),&END_DELIM,sizeof(END_DELIM));
            bytesRead++; // to ensure correct # bytes sent
            
        }
        
        // will be replaced with to_data_link()
        ssize_t bytesSent=send(sock,msg_buffer.data,bytesRead,0);
        if(bytesSent<bytesRead){
            perror("Failed to send complete message in sendServerData\n");
            exit(1);
        }
        packetNum++;
    }
    return 1;
}

int receiveData(vector<string> arguments, int sock){
    ofstream fout;
    fout.open(arguments[0]);
    if(!fout.is_open()){
        // LOG
        cerr << "Failed to open file\n";
        exit(1);
    }
    
    //Now that file is open, receive and write
    //char packet_buffer[PACKET_SIZE];
    packet packet_buffer;
    int packetNum=0;
    
    while(1){
        memset(packet_buffer.data,'\0',PACKET_SIZE);
        size_t bytesRec=recv(sock,packet_buffer.data,PACKET_SIZE,0);
        if((int)bytesRec < 0){
            perror("Failed to received file from client");
            cout<<"strerrno=" <<errno << endl;
            exit(1);
        }
        cout << packetNum+1 << ". \""<<packet_buffer.data << "\""<< endl;
        
        
        if(packetNum==0 && packet_buffer.data[bytesRec-1]!=END_DELIM){
            fout.write(packet_buffer.data+1, bytesRec-1);
        }else if(packetNum!=0 && packet_buffer.data[bytesRec-1]!=END_DELIM){
            fout.write(packet_buffer.data, bytesRec);
        } else if(packetNum==0 && packet_buffer.data[bytesRec-1]==END_DELIM){
            fout.write(packet_buffer.data+1, bytesRec-2);
            fout.close();
            return 1;
        } else if(packetNum!=0 && packet_buffer.data[bytesRec-1]==END_DELIM){
            fout.write(packet_buffer.data, bytesRec-1);
            fout.close();
            return 1;
        }
        packetNum++;
    }
    
    return 0;
}

// Sends a string to server - separated by DLE byte
void sendMessage(int cmd, vector<string> parameters, int toDL, int fromDL){
    string msg_buffer= START_DELIM + to_string(cmd);
    for(int i=0;i<parameters.size();i++){
        msg_buffer+= DELIM + parameters[i] + DELIM;
    }
    msg_buffer += END_DELIM;
    cout << "msg_buffer=\"" <<msg_buffer << "\"\n";

    
    unsigned long i = 0;
    
    //Break the message into packets and send each packet individually to the
    //data-link process.
    while(i < msg_buffer.length()){
        unsigned long len = msg_buffer.length() - i > PACKET_SIZE ?
        msg_buffer.length() - i :
        PACKET_SIZE;
        string data = msg_buffer.substr(i, len);
        packet pack;
        strcpy(pack.data, data.c_str());
        to_data_link(&pack, toDL, fromDL);
        i+=len;
    }
    //ssize_t bytesSent=send(sock,msg_buffer.c_str(),msg_buffer.length(),0);
    //if(bytesSent<msg_buffer.length()){
    //    perror("Failed to send complete message\n");
    //    exit(1);
    //}
}

//Sends a packet to the data-link layer
int to_data_link(packet *p, int toDL, int fromDL){
    //char buff[3];
    
    //We assume that it waits until a message comes from data-link:
#ifdef DEBUG
    cout <<"Checking if DL is OK" << endl;
#endif
    //read(fromDL, buff, 3);
#ifdef DEBUG
    //cout <<"DL returned -- " << buff << endl;
#endif
    
    
    //if(strcmp(buff, "OK")){
        //write(toDL, p->data, strlen(p->data));
    write(toDL, p, sizeof(packet));
    cout << "\nSENT TO DL\n";
    //}
    //else{
       // cerr << "data link is out of sync";
    //}
    return 1;
}

string messageFromDL(int fromDL){
#ifdef DEBUG
    cout <<"Starting messsageFromDL" << endl;
#endif
    string message;
    char buffer;
    int packetNum=0;
    int bytesRec=0;
    int bytesTotal=0;
    while(packetNum<1){
        while((bytesRec=read(fromDL,&buffer,1)>0)){
            bytesTotal+=bytesRec;
            if(packetNum==0 && buffer!=START_DELIM){
                message+=buffer;
                cerr << "Message doesn't start with START_DELIM. Message ==" << message <<endl;
                exit(1);
            }
            else if(buffer==END_DELIM){
                message+=buffer;
                break;
            }
            message+=buffer;
            packetNum++;
        }
        //cout << "bytesRec=" << bytesRec << " " << strerror(errno) <<endl;
    }
    
    if(errno!=0){
        cout << "errno set! -- " << errno << " with error:" << strerror(errno) << endl;
    }
    
#ifdef DEBUG
    cout <<"Message being returned from DL="<< message<< " -- Total rec bytes=" << bytesTotal << endl;
#endif
    return message;
}



























