#include "crack.h"
#include <algorithm>
#include <math.h>
#include <cmath>
#include <iostream>
#include <sstream>
#include <string.h>
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <strings.h>
#include <netinet/in.h>

using namespace std;

//Functions Declarations
void send_TO(int &sockFD, Message &msgToSend, struct sockaddr_in &remote_ADDR);
void recv_FROM(int &sockFD, Message &msgToRecv);
void senderClient();
void setUpSenderNetwork(struct sockaddr_in &multicast_ADDR, int &sockFD, int &ttL);
void setUpTCPSenderNetwork(int &sockTCPFD, struct sockaddr_in &server_ADDR, int &newsockTCPFD, struct sockaddr_in &client_ADDR, socklen_t &lEN);
Message createMessage(char cruzid[MAX_CRUZID_LEN], char passwds[MAX_HASHES][HASH_LENGTH+1], unsigned int num_passwd, char hostname[MAX_HOSTNAME_LEN], unsigned int port );
char hashArray[MAX_HASHES][HASH_LENGTH+1] = {"xxo0q4QVK0mOg", "00Pp9Oy0VWmn2", "zzOzL0bB0ocqo", "yyNhnfhEpDmTY", "5tQvIqEDV1qzw", "lqFucz6Kp.jPE"};
//Hash intended output: "cmps", "ucsc", "lab9", "CMPS", "UCSC", "LAB9"

int main(){
	
	senderClient();

	return 0;
}
//Function Definitions
void senderClient(){
	char cruzid[] = "jswinter";
	char hostNAME[] = "localhost";
	int thisPort = 7042;
	int numberOfPW = 1;
	Message firstSendMessage = createMessage( cruzid, hashArray, numberOfPW, hostNAME, thisPort);
	
	struct sockaddr_in multicast_Addr;
	int sockfd;
	int ttl= 1;
	
	setUpSenderNetwork(multicast_Addr, sockfd, ttl);
	
	send_TO(sockfd, firstSendMessage, multicast_Addr);
	
	int sockTCPfd;
	struct sockaddr_in server_addr;
	int newsockTCPfd;
	struct sockaddr_in client_addr;
	socklen_t len;
	Message firstReceiveMessage;

	setUpTCPSenderNetwork(sockTCPfd, server_addr, newsockTCPfd, client_addr, len);
	cout << "waiting for a response from the cracker." <<endl;
	
	recv(newsockTCPfd, (void *)&firstReceiveMessage, sizeof(firstReceiveMessage), 0);
	firstReceiveMessage.num_passwds = ntohs(firstReceiveMessage.num_passwds);
	firstReceiveMessage.port = ntohs(firstReceiveMessage.port);
	
	
	for(unsigned int k = 0; k < firstReceiveMessage.num_passwds; k++){
		cout << "Password[" << k << "]: " << firstReceiveMessage.passwds[k] << endl;
	}
	
	
	//recv_FROM(sockfd, firstReceiveMessage, multicast_Addr);
	close(sockfd);
	close(sockTCPfd);
	
	
}
//Utility Functions---------------------------------------------------
void setUpSenderNetwork(struct sockaddr_in &multicastADDR, int &sockFD, int &ttL){
	sockFD = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockFD < 0){
		cout << "Server: Exited on sockFD" <<endl;
		exit(-1);
	}
	if(setsockopt(sockFD, IPPROTO_IP, IP_MULTICAST_TTL, (void *) &ttL, sizeof(ttL)) < 0){
		cout << "Sender: exited on setsockopt" << endl;
		exit(-1);
	}
	memset(&multicastADDR, 0, sizeof(multicastADDR));
	multicastADDR.sin_family = AF_INET;
	multicastADDR.sin_addr.s_addr = get_multicast_address();
	multicastADDR.sin_port = htons(get_multicast_port());
	
}
void setUpTCPSenderNetwork(int &sockTCPFD, struct sockaddr_in &server_ADDR, int &newsockTCPFD, struct sockaddr_in &client_ADDR, socklen_t &lEN){
	sockTCPFD = socket(AF_INET, SOCK_STREAM, 0);
	if(sockTCPFD < 0) exit(-1);
	bzero((char *) &server_ADDR, sizeof(server_ADDR));
	server_ADDR.sin_family = AF_INET;
	server_ADDR.sin_addr.s_addr = INADDR_ANY;
	server_ADDR.sin_port = htons(get_unicast_port());
	if(bind(sockTCPFD, (struct sockaddr *) &server_ADDR, sizeof(server_ADDR)) <  0) exit(-1);
	
	
	listen(sockTCPFD, 1);
	
	lEN = sizeof(client_ADDR);
	newsockTCPFD = accept(sockTCPFD, (struct sockaddr *) &client_ADDR, &lEN);
	if(newsockTCPFD < 0) exit(-1);
	
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














