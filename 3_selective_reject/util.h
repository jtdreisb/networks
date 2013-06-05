// rcopy
// CPE464 Program 3
// Jason Dreisbach

#ifndef _UTIL_H_
#define _UTIL_H_

#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <unistd.h>

#define START_SEQ_NUM 1
#define MAX_PACKET_LEN 2048

typedef enum {
	FLAG_DATA = 1,
	FLAG_DATA_RESENT = 2,
	FLAG_ACK = 3,
	FLAG_SREJ = 4,
	FLAG_FILENAME_REQ = 6,
	FLAG_FILENAME_RESP_OK = 7,
	FLAG_FILENAME_RESP_BAD,
	FLAG_END_OF_FILE,
	CRC_ERROR = -1
} FLAG;

enum SELECT
{
	SET_NULL,
	NOT_NULL
};

typedef struct connection Connection;

struct connection
{
  int32_t socket_num;
  struct sockaddr_in remote;
  uint32_t len;
};

int32_t startServer();
int32_t connectToServer(char * hostname, uint16_t port_num, Connection * connection);
int32_t select_call(int32_t socket_num, int32_t seconds, int32_t microseconds, int32_t set_null);
int32_t recv_buf(uint8_t *buf, uint32_t len, int32_t recv_sock, Connection *conn, uint8_t *flag, uint32_t *seq_num);
int32_t send_buf(uint8_t *buf, uint32_t len, Connection *conn, uint8_t flag, uint32_t seq_num, uint8_t *packet);

#endif