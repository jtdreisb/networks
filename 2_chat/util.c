// chat
// CPE464 Program 2
// Jason Dreisbach
#include "cpe464.h"

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

uint32_t clientCount()
{
	uint32_t numClients;
	struct ClientNode *index = clientList->firstClient;
	for (numClients = 0; index != NULL; index = index->nextClient) {
		if (index->clientName != NULL)
			numClients++;
	}
	numClients = htonl(numClients);
	return numClients;
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


struct ClientNode *newClient()
{
	struct ClientNode *newClient = malloc(sizeof(struct ClientNode));
	memset(newClient, 0, sizeof(struct ClientNode));
	if (newClient == NULL) {
		perror("acceptNewClient:malloc");
		exit(-6);
	}
	if ((clientList->firstClient == NULL) || (clientList->lastClient == NULL)) {
		clientList->firstClient = newClient;
		clientList->lastClient = newClient;
	}
	else {
		clientList->lastClient->nextClient = newClient;
		clientList->lastClient = newClient;
	}
	return newClient;
}

void removeClient(struct ClientNode *client)
{
	struct ClientNode *node = clientList->firstClient;
	if (node == client) {
		clientList->firstClient = node->nextClient;
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

