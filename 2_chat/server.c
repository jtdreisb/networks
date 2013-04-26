// chat
// CPE464 Program 2
// Jason Dreisbach
#include "cpe464.h"
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define MAX_PENDING_CLIENTS 1024

struct ClientNode {
	int clientSocket;
	char *name;
	struct ClientNode *nextClient;
};

struct ClientList {
	struct ClientNode *firstClient;
	struct ClientNode *lastClient;
	int maxSocket;
};

struct ClientList _clientList;
struct ClientList *clientList = &_clientList;

struct ClientNode *clientNamed(char *name)
{
	// TODO: parse clientList and return the node matching "name"
	return NULL;
}

void acceptNewClient(int serverSocket)
{
	struct sockaddr_in newClientAddress;
	socklen_t sockLength = sizeof(newClientAddress);
	struct ClientNode *newClient = malloc(sizeof(struct ClientNode));
	if (newClient == NULL) {
		perror("acceptNewClient:malloc");
		exit(-6);
	}

	memset(newClient, 0, sizeof(struct ClientNode));

	if ((newClient->clientSocket = accept(serverSocket, (struct sockaddr *) &newClientAddress, &clntLen)) < 0) {
		perror("acceptNewClient:accept");
		free(newClient);
		return;
	}

	if ((clientList->firstClient == NULL) || (clientList->lastClient == NULL)) {
		clientList->firstClient = newClient;
		clientList->lastClient = newClient;
	}
	else {
		clientList->lastClient->nextClient = newClient;
		clientList->lastClient = newClient;
	}
	if (clientList->maxSocket < newClient->clientSocket) {
		clientList->maxSocket = newClient->clientSocket;
	}
}

void handleClient(struct ClientNode *client)
{
	printf("handleClient\n" );
}

int startServer(in_port_t *portNumber)
{
	int serverSocket;
	struct sockaddr_in local;
	socklen_t len = sizeof(local);

	if ((serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		perror("startServer:socket");
		return -1;
	}

	local.sin_family = AF_INET;
	local.sin_addr.s_addr = htonl(INADDR_ANY);
	local.sin_port = htons(*portNumber);

	if(bind(serverSocket, (struct sockaddr *) &local, sizeof(local)) < 0) {
		perror("startServer:bind");
		return -2;
	}

	if (getsockname(serverSocket, (struct sockaddr *)&local, &len) < 0) {
		perror("startServer:getsockname");
		return -3;
	}

	*portNumber = local.sin_port;
	return serverSocket;
}

int main(int argc, char const *argv[])
{
	int serverSocket = -1;
	in_port_t serverPort = 0;

	serverSocket = startServer(&serverPort);
	if (serverSocket < 0) {
		exit(serverSocket);
	}

	clientList->maxSocket = serverSocket;

	printf("Server is using port %d\n", ntohs(serverPort));

	if (listen(serverSocket, MAX_PENDING_CLIENTS) < 0) {
		perror("main:listen");
		exit (-4);
	}

	for(;;) {
		struct ClientNode *activeClient;
		fd_set fdSet;
		static struct timeval timeout;
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		FD_ZERO(&fdSet);

		FD_SET(serverSocket, &fdSet);

		activeClient = clientList->firstClient;
		while(activeClient != NULL) {
			FD_SET(activeClient->clientSocket, &fdSet);
			activeClient = activeClient->nextClient;
		}

		if (select(clientList->maxSocket+1, &fdSet, NULL, NULL, &timeout) < 0){
			perror("main:select");
			exit(-5);
		}
		else {
			if (FD_ISSET(serverSocket, &fdSet)) {
				acceptNewClient(serverSocket);
			}

			activeClient = clientList->firstClient;
			while(activeClient != NULL) {
				if(FD_ISSET(activeClient->clientSocket, &fdSet)) {
					handleClient(activeClient);
				}
				activeClient = activeClient->nextClient;
			}
		}
	}

	close(serverSocket);

	return 0;
}