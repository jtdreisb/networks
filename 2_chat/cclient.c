// chat
// CPE464 Program 2
// Jason Dreisbach
#include "cpe464.h"
#include <arpa/inet.h>
#include <netdb.h>

#define MAX_HANDLE_LENGTH 255
#define MAX_MESSAGE_LENGTH 1000
#define MAX_PACKET_SIZE 2048

struct ClientInfo {
	int socket;
	uint32_t sequenceNumber;
	char handle[MAX_HANDLE_LENGTH];
};
struct ClientInfo *gClient;

void sendWait(uint8_t *outPacket, ssize_t outPacketLen, uint8_t **inPacket, ssize_t *inPacketLen)
{
	int selectStatus;
	ssize_t numBytes;
	fd_set fdSet;
	static struct timeval timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	struct ChatHeader *sendHeader = (struct ChatHeader *)outPacket;

	for (;;) {
		// send packet
		if((numBytes = send(gClient->socket, outPacket, outPacketLen, 0)) < 0) {
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
					perror("registerHandle:recv");
					return;
				}

				if (recvBuf->sequenceNumber == sendHeader->sequenceNumber) {
					gClient->sequenceNumber++;
					if (*inPacket != NULL) {
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

void registerHandle()
{
	struct ChatHeader packetHeader;
	packetHeader.sequenceNumber = 1;
	packetHeader.flag = FLAG_INIT_REQ;

	uint8_t handleLen = strlen(gClient->handle);
	uint8_t *data = malloc(handleLen + 1);
	data[0] = handleLen;
	memcpy(data+1, gClient->handle, handleLen);

	ssize_t outPacketLen = handleLen + 1 + kChatHeaderSize;
	uint8_t *outPacket = makePacket(packetHeader, data, handleLen + 1);

	ssize_t inPacketLen;
	uint8_t *inPacket;

	sendWait(outPacket, outPacketLen, &inPacket, &inPacketLen);

	struct ChatHeader *responseHeader = (struct ChatHeader *)inPacket;
	switch(responseHeader->flag) {
		case FLAG_INIT_ACK:
			printf("Handle Registered!\n");
			break;
		case FLAG_INIT_ERR:
			fprintf(stderr, "ERROR:Could not register handle with server\n");
			exit(1);
			break;
		default:
			break;
	}

	free(inPacket);
}

//************************************************************************************
//** Connection Setup
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
    strncpy(gClient->handle, argv[1], MAX_HANDLE_LENGTH);

    // TODO: argv[2] error

    gClient->socket = connectToServer(argv[3], atoi(argv[4]));

    registerHandle();

    exit(0);
}