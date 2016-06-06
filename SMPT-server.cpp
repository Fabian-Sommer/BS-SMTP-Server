#include <stdio.h>
#include <iostream>
#include <winsock2.h>
#include <string.h>
#include <string>
#include <unordered_set>
#pragma comment(lib,"ws2_32.lib") //Winsock Library
using namespace std;
int main(int argc, char *argv[]) {
	WSADATA wsa;

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		printf("Failed. Error Code : %d", WSAGetLastError());
		return 1;
	}


	SOCKET s;
	if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
		printf("Could not create socket : %d", WSAGetLastError());
	}
	printf("Socket created.\n");


	//Prepare the sockaddr_in structure
	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(8888);

	//Bind
	if (bind(s, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR) {
		printf("Bind failed with error code : %d", WSAGetLastError());
	}

	puts("Bind done\n");

	//set up SMTP specifics
	unordered_set<string> users;
	users.insert("testuser");

	//Listen
	listen(s, 3);

	//Accept and incoming connection
	puts("Waiting for incoming connections...");

	SOCKET new_socket;
	struct sockaddr_in client;
	int c = sizeof(struct sockaddr_in);
	while ((new_socket = accept(s, (struct sockaddr *)&client, &c)) != INVALID_SOCKET) {
		//for multiple connections at once this needs to start a new thread
		puts("Connection accepted");
		char* message;

		message = "220 service ready";
		send(new_socket, message, strlen(message), 0);

		//Receive client name
		char reply[2000];
		int recv_size;
		if ((recv_size = recv(new_socket, reply, 2000, 0)) == SOCKET_ERROR) {
			puts("recv failed\n");
		}
		//TODO: check for correct message

		message = "250 OK"; //TODO: maybe all messages need \r\n at the end
		send(new_socket, message, strlen(message), 0);

		//Receive mail sender
		bool senderOk = false;
		string sender;
		while(!senderOk) {
			if ((recv_size = recv(new_socket, reply, 2000, 0)) == SOCKET_ERROR) {
				puts("recv failed\n");
			} else {
				//test for integrity
				if (reply[0] == 'M' && reply[1] == 'A' && reply[2] == 'I' && reply[3] == 'L' && reply[4] == ' ' && reply[5] == 'F' && reply[6] == 'R' && reply[7] == 'O' && reply[8] == 'M' && reply[9] == ':') {
					sender = string(reply + 10);
					cout << "Mail being sent by " << sender << endl;
					senderOk = true;
					message = "250 OK";
					send(new_socket, message, strlen(message), 0);
				} else {
					message = "500 Command not recognized";
					send(new_socket, message, strlen(message), 0);
				}
			}
		}

		//Receive recipients
		bool recipientsDone = false;
		vector<string> recipients;
		while (!recipientsDone) {
			if ((recv_size = recv(new_socket, reply, 2000, 0)) == SOCKET_ERROR) {
				puts("recv failed\n");
			} else {
				if (reply[0] == 'R' && reply[1] == 'C' && reply[2] == 'P' && reply[3] == 'T' && reply[4] == ' ' && reply[5] == 'T' && reply[6] == 'T' && reply[7] == ':') {
					string recip(reply + 8);
					cout << "Recipient: " << recip << endl;
					if (users.find(recip) != users.end()) {
						// User is known
						recipients.push_back(string(reply + 8));
						message = "250 OK";
						send(new_socket, message, strlen(message), 0);
					} else {
						message = "551 Unknown recipient";
						send(new_socket, message, strlen(message), 0);
					}
				} else if (reply[0] == 'D' && reply[1] == 'A' && reply[2] == 'T' && reply[3] == 'A') {
					recipientsDone = true;
					message = "354 start mail input";
					send(new_socket, message, strlen(message), 0);
				} else {
					message = "500 Command not recognized";
					send(new_socket, message, strlen(message), 0);
				}
			}
		}

		//TODO (maybe): check for \r\n . \r\n at end of mail, otherwise wait for more content
		if ((recv_size = recv(new_socket, reply, 2000, 0)) == SOCKET_ERROR) {
			puts("recv failed\n");
		} else {
			cout << "Mail: " << string(reply) << endl;
			message = "250 OK";
			send(new_socket, message, strlen(message), 0);
		}
		//TODO: receive QUIT from client
		//Reply to the client
		message = "221 closing channel";
		send(new_socket, message, strlen(message), 0);
	}

	if (new_socket == INVALID_SOCKET) {
		printf("accept failed with error code : %d", WSAGetLastError());
		return 1;
	}


	closesocket(s);
	WSACleanup();
	return 0;
}