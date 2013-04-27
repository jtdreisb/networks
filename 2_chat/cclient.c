// chat
// CPE464 Program 2
// Jason Dreisbach
#include "cpe464.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>

#define MAX_SEND_RETRIES 10

struct ClientInfo {
	int socket;
	uint32_t sequenceNumber;
	char handle[MAX_HANDLE_LENGTH];
};
struct ClientInfo *gClient;

void cleanupClient();

uint8_t *makePacket(HeaderFlag flag, uint8_t *data, ssize_t dataLen)
{
	int packetLen = kChatHeaderSize + dataLen;
	uint8_t *packet = malloc(packetLen);
	if (packet == NULL) {
		perror("makePacket:malloc");
		exit(1);
	}

	struct ChatHeader *packetHeader = (struct ChatHeader *)packet;
	packetHeader->sequenceNumber = gClient->sequenceNumber++;
	packetHeader->flag = flag;
	packetHeader->checksum = 0;

	memcpy(packet + kChatHeaderSize, data, dataLen);

	// Put header fields into network order
	packetHeader->sequenceNumber = htonl(packetHeader->sequenceNumber);
	// Calculate the checksum and stuff it into the right part
	packetHeader->checksum = in_cksum((uint16_t *)packet, packetLen);

	return packet;
}

void sendWait(uint8_t *outPacket, ssize_t outPacketLen, uint8_t **inPacket, ssize_t *inPacketLen)
{
	int selectStatus, i;
	ssize_t numBytes;
	fd_set fdSet;
	static struct timeval timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	struct ChatHeader *sendHeader = (struct ChatHeader *)outPacket;

	if (inPacket != NULL)
		*inPacket = NULL;
	if (inPacketLen != NULL)
		*inPacketLen = 0;

	for (i = 0; i < MAX_SEND_RETRIES ; i++) {
		// send packet
		if((numBytes = sendErr(gClient->socket, outPacket, outPacketLen, 0)) < 0) {
			perror("sendWait:send");
			return;
		}

		// wait for a response
		FD_ZERO(&fdSet);
		FD_SET(gClient->socket, &fdSet);

		if ((selectStatus = select(gClient->socket+1, &fdSet, NULL, NULL, &timeout)) < 0) {
			perror("sendWait:select");
			exit(1);
		}
		else if (selectStatus > 0) {
			if (FD_ISSET(gClient->socket, &fdSet)) {

				struct ChatHeader *recvBuf = malloc(MAX_PACKET_SIZE);
				ssize_t numBytes;
				if ((numBytes = recv(gClient->socket, (uint8_t *)recvBuf, MAX_PACKET_SIZE, 0)) < 0) {
					perror("sendWait:recv");
					return;
				}
				// check for invalid checksum
				if (in_cksum((uint16_t *)recvBuf, numBytes) != 0)
					return;
				uint32_t sequenceNumber;
				switch(recvBuf->flag) {
					case FLAG_MSG_ACK:
						sequenceNumber = *(uint32_t *)((uint8_t *)recvBuf+kChatHeaderSize);
						break;
					default:
						sequenceNumber = recvBuf->sequenceNumber;
						break;
				}
				if (sequenceNumber == sendHeader->sequenceNumber) {
					if (inPacket != NULL) {
						*inPacket = (uint8_t *)recvBuf;
					}
					else {
						free(recvBuf);
					}
					if (inPacketLen != NULL) {
						*inPacketLen = numBytes;
					}
					return;
				}
			}
		}
	}
}

void sendMessageACK(uint32_t sequenceNumber, char *handle)
{
	uint8_t handleLen = strlen(handle);
	ssize_t payloadLen = sizeof(uint32_t) + handleLen + 1;
	uint8_t *payload = malloc(payloadLen);
	sequenceNumber = htonl(sequenceNumber);
	memcpy(payload, &sequenceNumber, sizeof(uint32_t));
	memcpy(payload+sizeof(uint32_t), &handleLen, sizeof(uint8_t));
	memcpy(payload+sizeof(uint32_t)+sizeof(uint8_t), handle, handleLen);

	ssize_t packetLen = kChatHeaderSize + payloadLen;
	struct ChatHeader *packet = (struct ChatHeader *)makePacket(FLAG_MSG_ACK, payload, payloadLen);

	ssize_t numBytes;
	if((numBytes = sendErr(gClient->socket, packet, packetLen, 0)) < 0) {
		perror("sendMessageACK:send");
		return;
	}

	free(payload);
	free(packet);
}

void readPacketFromSocket(int sock)
{
	struct ChatHeader *recvBuf = malloc(MAX_PACKET_SIZE);
	ssize_t numBytes;
	if ((numBytes = recv(sock, (uint8_t *)recvBuf, MAX_PACKET_SIZE, 0)) < 0) {
		perror("readPacketFromSocket:recv");
		return;
	}
	else if (numBytes == 0) {
		fprintf(stderr, "ERROR: Server has closed the socket\n");
		exit(1);
	}

	char *index;
	char fromHandle[MAX_HANDLE_LENGTH];
	uint8_t fromHandleLen;

	switch (recvBuf->flag) {
		case FLAG_MSG_REQ:
			index = (char *)recvBuf + kChatHeaderSize + 1 + strlen(gClient->handle);
			fromHandleLen = *index;
			memcpy(fromHandle, index+1, fromHandleLen);
			fromHandle[fromHandleLen] = '\0';
			index += fromHandleLen + 1;
			printf("\n%s: %s", fromHandle, index);
			sendMessageACK(recvBuf->sequenceNumber, fromHandle);
			break;
		default:
			fprintf(stderr, "ERROR: Unknown packet type %d\n", recvBuf->flag);
			break;
	}

	free(recvBuf);
}


void sendMessage(char *toHandle, char *message)
{
	uint8_t toHandleLen = strlen(toHandle);
	uint8_t fromHandleLen = strlen(gClient->handle);
	ssize_t payloadLen = 3 + toHandleLen + fromHandleLen + strlen(message);
	uint8_t *payload = malloc(payloadLen);
	ssize_t offset = 0;

	memcpy(payload+offset, &toHandleLen, sizeof(toHandleLen));
	offset += sizeof(toHandleLen);
	memcpy(payload+offset, toHandle, toHandleLen);
	offset += toHandleLen;

	memcpy(payload+offset, &fromHandleLen, sizeof(fromHandleLen));
	offset += sizeof(fromHandleLen);
	memcpy(payload+offset, gClient->handle, fromHandleLen);
	offset += fromHandleLen;

	memcpy(payload+offset, message, strlen(message)+1);

	uint8_t *outPacket = makePacket(FLAG_MSG_REQ, payload, payloadLen);
	ssize_t outPacketLen = kChatHeaderSize + payloadLen;

	ssize_t responseLen;
	struct ChatHeader *responseHeader;

	sendWait(outPacket, outPacketLen, (uint8_t **)&responseHeader, &responseLen);

	char badHandle[MAX_HANDLE_LENGTH];
	uint8_t badHandleLen;
	if (responseHeader != NULL) {
		switch(responseHeader->flag) {
			case FLAG_MSG_ACK:
				fprintf(stderr, "ACK:\n");
				break;
			case FLAG_MSG_ERR:
				badHandleLen = *((uint8_t *)responseHeader + kChatHeaderSize);
				memcpy(badHandle, ((uint8_t *)responseHeader + kChatHeaderSize + 1), badHandleLen);
				badHandle[badHandleLen] = '\0';
				fprintf(stderr, "ERROR: No record of handle %s on server\n", badHandle);
				break;
			default:
				fprintf(stderr, "ERROR: Unexpected header flag (%d)\n", responseHeader->flag);
				exit(1);
				break;
		}

		free(responseHeader);
	}

	free(outPacket);
	free(payload);
}

void doMessage(char *buffer)
{
	ssize_t bufferLen = strlen(buffer);
	char *index = buffer;

	while(isspace(*index))
		index++;
	if ((index-buffer) >= bufferLen) {
		fprintf(stderr, "ERROR: usage: \%m <handle> [message]\n");
		return;
	}
	char *handle = index;

	while(!isspace(*index))
		index++;
	if ((index-buffer) >= bufferLen) {
		fprintf(stderr, "ERROR: usage: \%m <handle> [message]\n");
		return;
	}


	ssize_t handleLen = index - handle;
	if (handleLen > MAX_HANDLE_LENGTH) {
		fprintf(stderr, "ERROR: Handle exceeds maximum allowed length\n");
		return;
	}
	*index = '\0';
	index++;

	while(isspace(*index))
		index++;

	char *message;
	ssize_t messageLength;
	if ((index-buffer) >= bufferLen) {
		message = "";
		messageLength = 0;
	}
	else {
		message = index;
		messageLength = strlen(message);
	}

	if (messageLength > MAX_MESSAGE_LENGTH) {
		fprintf(stderr, "ERROR: Message exceeds maximum allowed length\n");
		return;
	}
	sendMessage(handle, message);
}

void printUser(uint32_t index)
{
	ssize_t outPacketLen = kChatHeaderSize + sizeof(uint32_t);
	index = htonl(index);
	uint8_t *outPacket = makePacket(FLAG_HNDL_REQ, (uint8_t *)&index, sizeof(uint32_t));
	ssize_t responseLen;
	struct ChatHeader *responseHeader;

	sendWait(outPacket, outPacketLen, (uint8_t **)&responseHeader, &responseLen);

	if (responseHeader != NULL) {
		switch(responseHeader->flag) {
			case FLAG_HNDL_RESP:
				break;
			default:
				fprintf(stderr, "ERROR:printUser: Unexpected header flag (%d)\n", responseHeader->flag);
				exit(1);
				break;
		}

		char *packetData = (char *)responseHeader;
		packetData += kChatHeaderSize;

		char handle[MAX_HANDLE_LENGTH];
		ssize_t handleLen = *packetData;

		memcpy(handle, packetData+1, handleLen);
		handle[handleLen] = '\0';

		printf("%s\n", handle);

		free(responseHeader);
	}

	free(outPacket);
}

void listUsers()
{
	ssize_t outPacketLen = kChatHeaderSize;
	uint8_t *outPacket = makePacket(FLAG_LIST_REQ, NULL, 0);
	ssize_t responseLen;
	struct ChatHeader *responseHeader;

	sendWait(outPacket, outPacketLen, (uint8_t **)&responseHeader, &responseLen);

	if (responseHeader != NULL) {
		switch(responseHeader->flag) {
			case FLAG_LIST_RESP:
				break;
			default:
				fprintf(stderr, "ERROR:listUsers: Unexpected header flag (%d)\n", responseHeader->flag);
				exit(1);
				break;
		}
		uint8_t *packetData = (uint8_t *)responseHeader;
		uint32_t clientCount = *(uint32_t *)(packetData+kChatHeaderSize);
		clientCount = ntohl(clientCount);

		uint32_t i;
		for (i = 0; i < clientCount; i++) {
			printf("%d: ", i);
			printUser(i);
		}

		free(responseHeader);
	}
	free(outPacket);
}


void handleUserInput()
{
	char inputBuf[MAX_PACKET_SIZE];
	if (fgets(inputBuf, MAX_PACKET_SIZE, stdin) == NULL) {
		return;
	}

	if (!strncmp(inputBuf, "\%M", 2) || !strncmp(inputBuf, "\%m", 2)) {
		doMessage(inputBuf+2);
	}
	else if (!strncmp(inputBuf, "\%L", 2) || !strncmp(inputBuf, "\%l", 2)) {
		listUsers();
	}
	else if (!strncmp(inputBuf, "\%E", 2) || !strncmp(inputBuf, "\%e", 2)) {
		cleanupClient();
		exit(0);
	}
}

void mainEventLoop()
{
	int selectStatus;
	fd_set fdSet;

	for (;;) {
		printf("> ");
		fflush(stdout);

		for (;;) {
			FD_ZERO(&fdSet);
			FD_SET(gClient->socket, &fdSet);
			FD_SET(STDIN_FILENO, &fdSet);
			if ((selectStatus = select(gClient->socket+1, &fdSet, NULL, NULL, NULL)) < 0) {
				perror("mainEventLoop:select");
				exit(1);
			}
			else if (selectStatus > 0) {
				if (FD_ISSET(gClient->socket, &fdSet) || FD_ISSET(STDIN_FILENO, &fdSet)) {
					if (FD_ISSET(gClient->socket, &fdSet)) {
						readPacketFromSocket(gClient->socket);
					}

					if (FD_ISSET(STDIN_FILENO, &fdSet)) {
						handleUserInput();
					}
					break;
				}
			}
		}
	}
}

void registerHandle()
{
	uint8_t handleLen = strlen(gClient->handle);
	uint8_t *data = malloc(handleLen + 1);
	data[0] = handleLen;
	memcpy(data+1, gClient->handle, handleLen);

	ssize_t outPacketLen = handleLen + 1 + kChatHeaderSize;
	uint8_t *outPacket = makePacket(FLAG_INIT_REQ, data, handleLen + 1);

	ssize_t responseLen;
	struct ChatHeader *responseHeader;

	sendWait(outPacket, outPacketLen, (uint8_t **)&responseHeader, &responseLen);

	if (responseHeader != NULL) {
		switch(responseHeader->flag) {
			case FLAG_INIT_ACK:
				break;
			case FLAG_INIT_ERR:
				fprintf(stderr, "ERROR: Could not register handle with server\n");
				exit(1);
				break;
			default:
				fprintf(stderr, "ERROR: Unexpected header flag (%d)\n", responseHeader->flag);
				exit(1);
				break;
		}

		free(responseHeader);
	}
	free(outPacket);
}

//************************************************************************************
//** Connection Setup & teardown
//************************************************************************************

int connectToServer(char *serverHostName, uint16_t serverPort)
{
	int sock;
	struct sockaddr_in serverAddr;

	struct hostent *hostEntry = gethostbyname(serverHostName);
	if (hostEntry == NULL) {
		herror("ipFromHost:gethostbyname");
		exit(-3);
	}

    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    	perror("connectToServer:socket");
    	exit(-4);
    }

	memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family      = AF_INET;
    serverAddr.sin_port        = htons(serverPort);
    memcpy(&(serverAddr.sin_addr.s_addr), hostEntry->h_addr, hostEntry->h_length);
	if (connect(sock, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
		perror("connectToServer:connect");
		exit(-5);
	}

    return sock;
}

void cleanupClient()
{
	uint8_t *cleanupHeader = makePacket(FLAG_EXIT_REQ, NULL, 0);
	struct ChatHeader *cleanupResponse;
	ssize_t cleanupResponseLen;
	sendWait(cleanupHeader, kChatHeaderSize, (uint8_t **)&cleanupResponse, &cleanupResponseLen);

	if (cleanupResponse != NULL) {
		if(cleanupResponse->flag != FLAG_EXIT_ACK) {
			fprintf(stderr, "ERROR: Invalid Acknowledgment of Shutdown from server\n");
		}
		free(cleanupResponse);
	}

	free(cleanupHeader);

	close(gClient->socket);
    free(gClient);
    gClient = NULL;
}

int main(int argc, char *argv[])
{
    gClient = malloc(sizeof(struct ClientInfo));
    if (gClient == NULL) {
    	perror("main:malloc");
    	exit(-1);
    }

    if (argc != 5) {
       fprintf(stderr, "Usage: %s <handle> <error> <hostname> <port>\n", argv[0]);
       exit(-2);
    }


    // Save handle
    if (strlen(argv[1]) >  MAX_HANDLE_LENGTH) {
    	fprintf(stderr, "ERROR: Handle exceeds maximum allowed length\n");
    	exit(1);
    }
    strncpy(gClient->handle, argv[1], MAX_HANDLE_LENGTH);

    double errorRate;
    sscanf(argv[2], "%lf", &errorRate);

	sendErr_init(errorRate,
		DROP_ON,
		FLIP_ON,
		DEBUG_OFF,
		RSEED_OFF);

    gClient->socket = connectToServer(argv[3], atoi(argv[4]));

    registerHandle();

    mainEventLoop();

  	// Never Reached

    exit(0);
}