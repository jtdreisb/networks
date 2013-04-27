// chat
// CPE464 Program 2
// Jason Dreisbach
#include "cpe464.h"

#define MAX_PENDING_CLIENTS 1024
#define MAX_PACKET_SIZE 2048
#define DEFAULT_MAX_SOCKET 3

struct ClientNode {
	int clientSocket;
	char *clientName;
	struct ClientNode *nextClient;
	// Used when handling incoming packets
	struct ChatHeader *packetData;
	ssize_t packetLength;
};

struct ClientList {
	struct ClientNode *firstClient;
	struct ClientNode *lastClient;
	int maxSocket;
};

struct ClientList _clientList;
struct ClientList *clientList = &_clientList;


//************************************************************************************
//** Client List Utility Functions
//************************************************************************************

struct ClientNode *clientNamed(char *name)
{
	struct ClientNode *node = clientList->firstClient;
	while (node != NULL) {
		if (node->clientName && (strcmp(name, node->clientName) == 0)) {
			break;
		}
		node = node->nextClient;
	}
	return node;
}

void printClient(struct ClientNode *node)
{
	printf("Node:\n"
		"\tName: %s\n"
		"\tSocket: %d\n",
		node->clientName,
		node->clientSocket);
}

void printClientList()
{
	printf("ClientList:\n================\n");
	int clientCount = 0;
	struct ClientNode *client = clientList->firstClient;
	while (client != NULL) {
		printf("%d ", ++clientCount);
		printClient(client);
		client = client->nextClient;
	}
	printf("%d active Clients\n", clientCount);
	printf("Max Socket: %d\n", clientList->maxSocket);
}

//************************************************************************************
//** Client Addition and Removal
//************************************************************************************
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

	if ((newClient->clientSocket = accept(serverSocket, (struct sockaddr *) &newClientAddress, &sockLength)) < 0) {
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

void removeClient(struct ClientNode *client)
{
	struct ClientNode *node = clientList->firstClient;
	if (node == client) {
		clientList->firstClient = NULL;
		node = NULL;
	}
	else {
		while (node != NULL) {
			if (node->nextClient == client) {
				node->nextClient = client->nextClient;
				break;
			}
			node = node->nextClient;
		}
	}
	if (clientList->lastClient == client) {
		clientList->lastClient = node;
	}
	if (client->clientSocket == clientList->maxSocket) {
		if (clientList->firstClient == NULL) {
			clientList->maxSocket = DEFAULT_MAX_SOCKET;
		}
	}
	close(client->clientSocket);
	if (client->clientName != NULL)
		free(client->clientName);
	free(client);
}

//************************************************************************************
//** Request Handling
//************************************************************************************

void registerHandle(struct ClientNode *client);
void forwardMessage(struct ClientNode *client);
void respondToClient(struct ClientNode *client, HeaderFlag flag, uint8_t *data, ssize_t dataLen);

void handleClient(struct ClientNode *client)
{
	ssize_t bytesRcvd;
	uint8_t *buf = malloc(MAX_PACKET_SIZE);
	if (buf == NULL) {
		perror("handleClient:malloc");
		return;
	}

	if ((bytesRcvd = recv(client->clientSocket, buf, MAX_PACKET_SIZE-1, 0)) < 0) {
		perror("handleClient:recv");
		free(buf);
		return;
	}
	// Check for a shutdown
	if (bytesRcvd == 0) {
		removeClient(client);
	}
	// Parse the buffer
	else {
		client->packetData = (struct ChatHeader *)buf;
		client->packetLength = bytesRcvd;
		switch (client->packetData->flag) {
			case FLAG_INIT_REQ:
				printf("Registering Handle!\n");
				registerHandle(client);
				break;
			case FLAG_MSG_REQ:

			//forward
				break;
			case FLAG_MSG_ACK:
			// forward
				break;
			case FLAG_EXIT_REQ:
			// intercept
				break;
			case FLAG_LIST_REQ:
				break;
			case FLAG_HNDL_REQ:
				break;
			case FLAG_INIT_ACK:
			case FLAG_INIT_ERR:
			case FLAG_MSG_ERR:
			case FLAG_EXIT_ACK:
			case FLAG_LIST_RESP:
			case FLAG_HNDL_RESP:
			default:
				break;
		}
		client->packetData = NULL;
		client->packetLength = 0;
	}
	free(buf);
}

void registerHandle(struct ClientNode *client)
{
	// HEADER[kChatHeaderSize] handleLen[1] handle[handleLen]

	uint8_t *buf = (uint8_t *)client->packetData;
	uint8_t handleLen = *(buf + kChatHeaderSize);
	// Make sure we have received a buffer big enough to
	// contain the handle
	if (client->packetLength < (kChatHeaderSize + handleLen + 1)) {
		printf("Packet not long enough\n");
		respondToClient(client, FLAG_INIT_ERR, NULL, 0);
		removeClient(client);
		return;
	}

	char *clientHandle = malloc(handleLen+1);
	if (clientHandle == NULL) {
		perror("registerHandle:malloc");
		exit(1);
	}

	memcpy(clientHandle, buf + kChatHeaderSize + 1, handleLen);
	clientHandle[handleLen] = '\0';

	// Make sure this handle isn't already registered
	if (clientNamed(clientHandle) && (clientNamed(clientHandle) != client)) {
		printf("client already in list\n");
		respondToClient(client, FLAG_INIT_ERR, NULL, 0);
		removeClient(client);
		return;
	}

	client->clientName = clientHandle;
	printf("registering handle for");
	printClient(client);
	respondToClient(client, FLAG_INIT_ACK, NULL, 0);
}

void forwardMessage(struct ClientNode *client)
{

}

void respondToClient(struct ClientNode *client, HeaderFlag flag, uint8_t *data, ssize_t dataLen)
{
	struct ChatHeader responseHeader;
	responseHeader.sequenceNumber = client->packetData->sequenceNumber;
	responseHeader.flag = flag;
	uint8_t *packet = makePacket(responseHeader, data, dataLen);
	ssize_t packetLength = kChatHeaderSize + dataLen;

	ssize_t bytesSent;
	if ((bytesSent = send(client->clientSocket, packet, packetLength, 0)) < 0) {
		perror("respondToClient:send");
	}
	free(packet);
}

//************************************************************************************
//** Select() Utility Functions
//************************************************************************************
void setActiveClientsForSelect(fd_set *fdSet)
{
	struct ClientNode *activeClient;
	activeClient = clientList->firstClient;
	while(activeClient != NULL) {
		FD_SET(activeClient->clientSocket, fdSet);
		activeClient = activeClient->nextClient;
	}
}

void checkActiveClientsAfterSelect(fd_set *fdSet)
{
	struct ClientNode *activeClient = clientList->firstClient;
	while(activeClient != NULL) {
		if(FD_ISSET(activeClient->clientSocket, fdSet)) {
			handleClient(activeClient);
		}
		activeClient = activeClient->nextClient;
	}
}

//************************************************************************************
//** Server Setup
//************************************************************************************
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

	if (listen(serverSocket, MAX_PENDING_CLIENTS) < 0) {
		perror("startServer:listen");
		return -4;
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

	clientList->maxSocket = DEFAULT_MAX_SOCKET;

	printf("Server is using port %d\n", ntohs(serverPort));

	for(;;) {
		fd_set fdSet;
		static struct timeval timeout;
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		FD_ZERO(&fdSet);

		FD_SET(serverSocket, &fdSet);

		setActiveClientsForSelect(&fdSet);

		if (select(clientList->maxSocket+1, &fdSet, NULL, NULL, &timeout) < 0){
			perror("main:select");
			exit(-5);
		}
		else {
			if (FD_ISSET(serverSocket, &fdSet)) {
				acceptNewClient(serverSocket);
			}
			checkActiveClientsAfterSelect(&fdSet);
		}
	}

	close(serverSocket);
	return 0;
}