// chat
// CPE464 Program 2
// Jason Dreisbach
#include "cpe464.h"

#define MAX_PENDING_CLIENTS 1024
#define MAX_PACKET_SIZE 2048

void acceptNewClient(int serverSocket)
{
	struct sockaddr_in newClientAddress;
	socklen_t sockLength = sizeof(newClientAddress);
	struct ClientNode *client =  newClient();

	if ((client->clientSocket = accept(serverSocket, (struct sockaddr *) &newClientAddress, &sockLength)) < 0) {
		perror("acceptNewClient:accept");
		free(client);
		return;
	}

	if (clientList->maxSocket < client->clientSocket) {
		clientList->maxSocket = client->clientSocket;
	}
}

//************************************************************************************
//** Request Handling
//************************************************************************************
uint8_t *makePacket(struct ChatHeader header, uint8_t *data, ssize_t dataLen);
void registerHandle(struct ClientNode *client);
void clientExit(struct ClientNode *client);
void forwardMessage(struct ClientNode *client);
void getClientCount(struct ClientNode *client);
void clientNameRequest(struct ClientNode *client);
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
		client->packetLength = bytesRcvd;
		client->packetData = (struct ChatHeader *)buf;
		// make sure the checksum is correct before parsing the packets
		if (in_cksum((uint16_t *)client->packetData, client->packetLength) == 0) {
			// Put the packet header fields in host order
			switch (client->packetData->flag) {
				case FLAG_INIT_REQ:
					registerHandle(client);
					break;
				case FLAG_MSG_REQ:
				case FLAG_MSG_ACK:
					forwardMessage(client);
					break;
				case FLAG_EXIT_REQ:
					clientExit(client);
					break;
				case FLAG_LIST_REQ:
					getClientCount(client);
					break;
				case FLAG_HNDL_REQ:
					clientNameRequest(client);
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
		}
		client->packetData = NULL;
		client->packetLength = 0;
	}
	free(buf);
}

void getClientCount(struct ClientNode *client)
{
	uint32_t numClients = clientCount();
	respondToClient(client, FLAG_LIST_RESP, (uint8_t *)&numClients, sizeof(uint32_t));
}

void clientNameRequest(struct ClientNode *client)
{
	uint8_t *index = (uint8_t *)client->packetData;
	index += kChatHeaderSize;
	uint32_t clientIndex = *(uint32_t *)index;
	clientIndex = ntohl(clientIndex);
	struct ClientNode *node = clientList->firstClient;
	int i;
	for (i = 0;; node = node->nextClient) {
		if (node && node->clientName != NULL) {
			if (i == clientIndex) {
				break;
			}
			i++;
		}
	}

	uint8_t *payload = malloc(1 + strlen(node->clientName));
	payload[0] = strlen(node->clientName);
	memcpy(payload+1, node->clientName, strlen(node->clientName));

	respondToClient(client, FLAG_HNDL_RESP, payload, 1 + strlen(node->clientName));
	free(payload);
}

void registerHandle(struct ClientNode *client)
{
	// HEADER[kChatHeaderSize] handleLen[1] handle[handleLen]

	uint8_t *buf = (uint8_t *)client->packetData;
	uint8_t handleLen = *(buf + kChatHeaderSize);
	// Make sure we have received a buffer big enough to
	// contain the handle
	if (client->packetLength < (kChatHeaderSize + handleLen + 1)) {
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
		respondToClient(client, FLAG_INIT_ERR, NULL, 0);
		removeClient(client);
		return;
	}

	client->clientName = clientHandle;
	respondToClient(client, FLAG_INIT_ACK, NULL, 0);
}

void clientExit(struct ClientNode *client)
{
	respondToClient(client, FLAG_EXIT_ACK, NULL, 0);
	removeClient(client);
}

void forwardMessage(struct ClientNode *client)
{
	char handle[MAX_HANDLE_LENGTH];
	uint8_t *index = (uint8_t *) client->packetData;
	switch (client->packetData->flag) {
		case FLAG_MSG_REQ:
			index += kChatHeaderSize;
			break;
		case FLAG_MSG_ACK:
			index += kChatHeaderSize + sizeof(uint32_t);
			break;
		default:
			fprintf(stderr, "ERROR: Unknown forwarding packet scheme %d\n", client->packetData->flag);
			return;
			break;
	}

	uint8_t handleSize = *index;
	memcpy(handle, index+1, handleSize);
	handle[handleSize] = '\0';

	struct ClientNode *destClient = clientNamed(handle);
 	if (destClient == NULL) {
 		respondToClient(client, FLAG_MSG_ERR, index, handleSize+1);
 		return;
 	}

	ssize_t bytesSent;
	if ((bytesSent = sendErr(destClient->clientSocket, client->packetData, client->packetLength, 0)) < 0) {
		perror("forwardMessage:send");
	}
}

void respondToClient(struct ClientNode *client, HeaderFlag flag, uint8_t *data, ssize_t dataLen)
{
	struct ChatHeader responseHeader;
	responseHeader.sequenceNumber = ntohl(client->packetData->sequenceNumber);
	responseHeader.checksum = ntohs(client->packetData->checksum);
	responseHeader.flag = flag;
	uint8_t *packet = makePacket(responseHeader, data, dataLen);
	ssize_t packetLength = kChatHeaderSize + dataLen;

	ssize_t bytesSent;
	if ((bytesSent = sendErr(client->clientSocket, packet, packetLength, 0)) < 0) {
		perror("respondToClient:send");
	}
	free(packet);
}

uint8_t *makePacket(struct ChatHeader header, uint8_t *data, ssize_t dataLen)
{
	int packetLen = kChatHeaderSize + dataLen;
	uint8_t *packet = malloc(packetLen);
	if (packet == NULL) {
		perror("makePacket:malloc");
		exit(1);
	}

	memcpy(packet, &header, kChatHeaderSize);

	struct ChatHeader *packetHeader = (struct ChatHeader *)packet;
	packetHeader->checksum = 0;
	memcpy(packet + kChatHeaderSize, data, dataLen);
	// Put header fields into network order
	packetHeader->sequenceNumber = htonl(packetHeader->sequenceNumber);
	// Calculate the checksum and stuff it into the right part
	packetHeader->checksum = in_cksum((uint16_t *)packet, packetLen);
	return packet;
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

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <error>\n", argv[0]);
		exit(1);
	}

	serverSocket = startServer(&serverPort);
	if (serverSocket < 0) {
		exit(serverSocket);
	}

	double errorRate;
    sscanf(argv[1], "%lf", &errorRate);

	sendErr_init(errorRate,
		DROP_ON,
		FLIP_ON,
		DEBUG_OFF,
		RSEED_OFF);

	clientList->maxSocket = DEFAULT_MAX_SOCKET;

	printf("Server is using port %d\n", ntohs(serverPort));

	for(;;) {
		fd_set fdSet;
		// static struct timeval timeout;
		// timeout.tv_sec = 1;
		// timeout.tv_usec = 0;
		FD_ZERO(&fdSet);

		FD_SET(serverSocket, &fdSet);

		setActiveClientsForSelect(&fdSet);

		if (select(clientList->maxSocket+1, &fdSet, NULL, NULL, NULL) < 0){
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