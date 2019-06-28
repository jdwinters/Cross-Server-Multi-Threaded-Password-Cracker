#include "crack.h"
#include <algorithm>
#include <math.h>
#include <cmath>
#include <iostream>
#include <sstream>
#include <string.h>
#include <string>
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <strings.h>
#include <netinet/in.h>
#include <crypt.h>
#include <future>
#include <unistd.h>

using namespace std;

//Function Declarations
void send_TO(int &sockFD, Message &msgToSend, struct sockaddr_in &remote_ADDR);
void recv_FROM(int &sockFD, Message &msgToRecv);
void receiverServer();
void setUpReceiverNetwork(struct sockaddr_in &server_ADDR, struct ip_mreq &multicastREQUEST, int &sockFD);
void setUPTCPReceiverNetwork(Message &outGoingMessage, int & sockTCPFD, struct sockaddr_in &serv_ADDR, struct hostent* &serVER);
Message createMessage(char cruzid[MAX_CRUZID_LEN], char passwds[MAX_HASHES][HASH_LENGTH+1], unsigned int num_passwd, char hostname[MAX_HOSTNAME_LEN], unsigned int port );
Message createMessage(char cruzid[MAX_CRUZID_LEN], unsigned int num_passwds, char hostname[MAX_HOSTNAME_LEN], unsigned int port );
//Main Threads
int main(){
	
	receiverServer();
	
	return 0;
}
//Function Definitions
//Client & Server---------------------------------------------------------------------------------------------------------------------------------------------------------
void receiverServer(){
	struct sockaddr_in server_addr;
	struct ip_mreq multicatRequest;
	int sockfd;
	char cruzid[MAX_CRUZID_LEN] = "jswinter";

	setUpReceiverNetwork(server_addr, multicatRequest, sockfd);
	Message firstMessageReceived;
	recv_FROM(sockfd, firstMessageReceived);
	
	if(strcmp(cruzid, firstMessageReceived.cruzid) == 0)
		cout << "Got our cruzid!" <<endl;
	cout << firstMessageReceived.cruzid << endl;
	cout << firstMessageReceived.passwds[0] << endl;
	Message firstMessageSent = createMessage(firstMessageReceived.cruzid, firstMessageReceived.num_passwds, firstMessageReceived.hostname, firstMessageReceived.port);
	//Time to distribute
	unsigned int numberEachServer = 0;
	char currentHost[MAX_HOSTNAME_LEN];
	//gethostname(currentHost, MAX_HOSTNAME_LEN);
	//numberEachServer = firstMessageReceived.num_passwds / 4;
	//
	//
	//if(strcmp(currentHost, "thor") == 0){
	//	
	//}else if(strcmp(currentHost, "graculus") == 0){
	//	
	//}else if(strcmp(currentHost, "grolliffe") == 0){
	//	
	//}else if(strcmp(currentHost, "olaf") == 0){
	//	
	//}
	
	
	//vector<thread> threadPool;
	vector<future<void>> futuresVec;
	for(unsigned int k = 0; k < firstMessageReceived.num_passwds; k++){
		//crack(firstMessageReceived.passwds[k], firstMessageSent.passwds[k]);
		//threadPool.push_back(crack, &firstMessageReceived.passwds[k], &firstMessageSent.passwds[k]);
		futuresVec.push_back(async(crack, ref(firstMessageReceived.passwds[k]), ref(firstMessageSent.passwds[k])));
	}
	for(auto &e : futuresVec) {
		e.wait();
	}
 
	int sockTCPfd;
	struct hostent *server;
	struct sockaddr_in serv_addr;
	setUPTCPReceiverNetwork(firstMessageSent, sockTCPfd, serv_addr, server);
	
	firstMessageSent.num_passwds = htons(firstMessageSent.num_passwds);
	firstMessageSent.port = htons(firstMessageSent.port);
	send(sockTCPfd, (void *)&firstMessageSent, sizeof(firstMessageSent), 0);
	
	
	close(sockfd);
	close(sockTCPfd);
	
}
//Utility Functions-------------------------------------------------------------------------------------------------------------------------------------------------------
void setUpReceiverNetwork(struct sockaddr_in &server_ADDR, struct ip_mreq &multicastREQUEST, int &sockFD){
	sockFD = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockFD < 0){
		cout << "Client: Exited on socketFD" << endl;
		exit(-1);
	}
	
	bzero((char *) &server_ADDR, sizeof(server_ADDR));
	server_ADDR.sin_family = AF_INET;
	server_ADDR.sin_port = htons(get_multicast_port());
	
	if(bind(sockFD, (struct sockaddr *) &server_ADDR, sizeof(server_ADDR)) < 0){
		cout << "Client: Exited on binding" << endl;
		exit(-1);
	}
		
	
	multicastREQUEST.imr_multiaddr.s_addr = get_multicast_address();
	multicastREQUEST.imr_interface.s_addr = htonl(INADDR_ANY);
	if (setsockopt(sockFD, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *) &multicastREQUEST, sizeof(multicastREQUEST)) < 0){
		cout << "Client: Exited on setsocketopt" << endl;
		exit(-1);
	}
		
}
void setUPTCPReceiverNetwork(Message &outGoingMessage, int & sockTCPFD, struct sockaddr_in &serv_ADDR, struct hostent* &serVER){
	sockTCPFD = socket(AF_INET, SOCK_STREAM, 0);
	if(sockTCPFD < 0) exit(-1);
	serVER = gethostbyname(outGoingMessage.hostname);
	bzero((char *) &serv_ADDR, sizeof(serv_ADDR));
	serv_ADDR.sin_family = AF_INET;
	bcopy((char *)serVER->h_addr, (char *)&serv_ADDR.sin_addr.s_addr, serVER->h_length);
	serv_ADDR.sin_port = htons(get_unicast_port());
	if(connect(sockTCPFD, (struct sockaddr *) &serv_ADDR, sizeof(serv_ADDR)) < 0) exit(-1);
	
}
Message createMessage(char cruzid[MAX_CRUZID_LEN], char passwds[MAX_HASHES][HASH_LENGTH+1], unsigned int num_passwd, char hostname[MAX_HOSTNAME_LEN], unsigned int port ){
	Message creatingMessage;
	
	strcpy(creatingMessage.cruzid, cruzid);
	copy(&passwds[0][0], &passwds[0][0]+(num_passwd*(HASH_LENGTH+1)), &creatingMessage.passwds[0][0]);
	creatingMessage.num_passwds = num_passwd;
	strcpy(creatingMessage.hostname, hostname);
	creatingMessage.port = port;
	
	return creatingMessage;
}
Message createMessage(char cruzid[MAX_CRUZID_LEN], unsigned int num_passwds, char hostname[MAX_HOSTNAME_LEN], unsigned int port ){
	Message creatingMessage;
	
	strcpy(creatingMessage.cruzid, cruzid);
	creatingMessage.num_passwds = num_passwds;
	strcpy(creatingMessage.hostname, hostname);
	creatingMessage.port = port;
	
	return creatingMessage;
}
void send_TO(int &sockFD, Message &msgToSend, struct sockaddr_in &remote_ADDR){
	msgToSend.num_passwds = htons(msgToSend.num_passwds);
	msgToSend.port = htons(msgToSend.port);
	int sendN = sendto(sockFD, &msgToSend, sizeof(msgToSend), 0, (struct sockaddr *) &remote_ADDR, sizeof(remote_ADDR));
	if(sendN < 0) exit(-1);
	if(sendN == 0) close(sockFD);
	msgToSend.num_passwds = ntohs(msgToSend.num_passwds);
	msgToSend.port = ntohs(msgToSend.port);
	
}
void recv_FROM(int &sockFD, Message &msgToRecv){
	int recvN = recvfrom(sockFD, &msgToRecv, sizeof(msgToRecv), 0, NULL, 0);
	if(recvN < 0) exit(-1);
	if(recvN == 0) close(sockFD);
	msgToRecv.num_passwds = ntohs(msgToRecv.num_passwds);
	msgToRecv.port = ntohs(msgToRecv.port);
}


