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
#define MAX_FILE_NAME_LEN 255
#define MAX_RETRIES 10

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

typedef struct
{
  int32_t socket_num;
  struct sockaddr_in remote;
  uint32_t len;
} Connection;

typedef struct
{
	uint8_t *registry;
	uint8_t *buffer;
	uint32_t window_size;
	uint32_t block_size;
	uint32_t buffer_size;
	int32_t data_len;
	uint32_t base_seq_num;
	uint32_t window_index;
} Window;

Window *newWindowWithSizeAndBuffer(uint32_t window_size, uint32_t buffer_size);
void clearWindow(Window *window);
int32_t windowIsFull(Window *window);
uint32_t maxSequenceNumber(Window *window);
uint32_t nextOpenSequenceNumber(Window *window);
void destroyWindow(Window *window);

int32_t startServer();
int32_t connectToServer(char * hostname, uint16_t port_num, Connection * connection);
int32_t select_call(int32_t socket_num, int32_t seconds, int32_t microseconds, int32_t set_null);
int32_t recv_buf(uint8_t *buf, uint32_t len, int32_t recv_sock, Connection *conn, uint8_t *flag, uint32_t *seq_num);
int32_t send_buf(uint8_t *buf, uint32_t len, Connection *conn, uint8_t flag, uint32_t seq_num);

#endif