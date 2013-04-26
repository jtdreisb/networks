// chat
// CPE464 Program 2
// Jason Dreisbach
#include "cpe464.h"
#include <arpa/inet.h>
#include <netdb.h>

#define MAX_HANDLE_LENGTH 255
#define MAX_MESSAGE_LENGTH 1000

struct ClientInfo {
	int socket;
	char handle[MAX_HANDLE_LENGTH];
};
struct ClientStatus *gClient;


void registerHandle()
{

	// makePacket
}

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
    serverAddr.sin_addr.s_addr = (struct in_addr *)hostEntry->h_addr;
    serverAddr.sin_port        = htons(serverPort);
	if (connect(sock, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
		perror("connectToServer:connect");
		exit(-5);
	}

    return sock;
}

int main(int argc, char *argv[])
{
    gClient = malloc(sizeof(struct ClientInfo));
    if (gClient == NULL) {
    	perror("main:malloc");
    	exit(-1);
    }

    if (argc != 5) {
       fprintf(stderr, "Usage: %s <handle> <error> <hostname> <port>]\n", argv[0]);
       exit(-2);
    }

    // Save handle
    strncpy(gClient->handle, argv[1], MAX_HANDLE_LENGTH);

    // TODO: argv[2] error

    gClient->socket = connectToServer(argv[3], atoi(argv[4]));

    registerHandle();


    printf("\n");    /* Print a final linefeed */

    close(sock);
    exit(0);
}