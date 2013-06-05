// server
// CPE464 Program 3
// Jason Dreisbach

#include "cpe464.h"
#include "util.h"

typedef enum
{
	STATE_START,
	STATE_DONE,
	STATE_FILENAME,
	STATE_SEND_DATA,
	STATE_WAIT_ON_ACK,
	STATE_TIMEOUT_ON_ACK
} STATE;

void server_runloop();
void process_client(int32_t serv_sock, uint8_t *buf, int32_t recv_len, Connection *client);
STATE filename(Connection *client, uint8_t *buf, int32_t recv_len, int32_t *data_file, uint32_t *buf_size);
STATE send_data(Connection *client, uint8_t *packet, int32_t *packet_len, int32_t data_file, uint32_t buf_size, uint32_t *seq_num);
STATE wait_on_ack(Connection *client);
STATE timeout_on_ack(Connection *client, uint8_t *packet, uint32_t packet_len);

void server_runloop()
{
	pid_t pid = 0;
	int status;

	uint8_t buf[MAX_PACKET_LEN];
	uint8_t flag;
	uint32_t recv_len;
	uint32_t seq_num;
	Connection client;
	int serv_sock = startServer();

	for (;;) {
		if (select_call(serv_sock, 1, 0, NOT_NULL) == 1) {
			recv_len = recv_buf(buf, MAX_PACKET_LEN, serv_sock, &client, &flag, &seq_num);
			if (recv_len != CRC_ERROR) {
				if ((pid = fork()) < 0) {
					perror("mainServerRunLoop:fork");
					exit(1);
				}
				// We are child
				if (pid == 0) {
					process_client(serv_sock, buf, recv_len, &client);
					exit(0);
				}
				while (waitpid(-1, &status, WNOHANG) > 0) {
					printf("Processed wait\n");
				}
			}
		}

	}
}

void process_client(int32_t serv_sock, uint8_t *buf, int32_t recv_len, Connection *client)
{
	STATE state = STATE_START;
	int32_t data_file = 0;
	int32_t packet_len = 0;
	uint8_t packet[MAX_PACKET_LEN];
	uint32_t buf_size = 0;
	uint32_t seq_num = START_SEQ_NUM;

	while (state != STATE_DONE) {

		switch (state) {
			case STATE_START:
				state = STATE_FILENAME;

			case STATE_FILENAME:
				seq_num = START_SEQ_NUM;
				state = filename(client, buf, recv_len, &data_file, &buf_size);
				break;

			case STATE_SEND_DATA:
				state = send_data(client, packet, &packet_len, data_file, buf_size, &seq_num);
				break;

			case STATE_WAIT_ON_ACK:
				state = wait_on_ack(client);
				break;

			case STATE_TIMEOUT_ON_ACK:
				state = timeout_on_ack(client, packet, packet_len);
				break;

			case STATE_DONE:
			default:
			state = STATE_DONE;
			break;

		}
	}
}

STATE filename(Connection *client, uint8_t *buf, int32_t recv_len, int32_t *data_file, uint32_t *buf_size)
{
	uint8_t response[1];
	char fname[MAX_PACKET_LEN];

	memcpy(buf_size, buf, 4);
	memcpy(fname, &buf[4], recv_len - 4);

	if ((client->socket_num = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("filename:socket");
		exit(1);
	}

	if (((*data_file) = open(fname, O_RDONLY)) < 0) {
		send_buf(response, 0, client, FLAG_FILENAME_RESP_BAD, 0, buf);
		return STATE_DONE;
	}

	send_buf(response, 0, client, FLAG_FILENAME_RESP_OK, 0, buf);
	return STATE_SEND_DATA;
}

STATE send_data(Connection *client, uint8_t *packet, int32_t *packet_len, int32_t data_file, uint32_t buf_size, uint32_t *seq_num)
{
	uint8_t buf[MAX_PACKET_LEN];
	int32_t len_read = 0;

	len_read = read(data_file, buf, buf_size);

	switch (len_read) {

		case -1:
			perror("send_data: read");
			break;

		case 0:
			(*packet_len) = send_buf(buf, 1, client, FLAG_END_OF_FILE, *seq_num, packet);
			printf("File transfer complete.\n");
			break;

		default:
			(*packet_len) = send_buf(buf, len_read, client, FLAG_DATA, *seq_num, packet);
			(*seq_num)++;
			return STATE_WAIT_ON_ACK;

	}
	return STATE_DONE;
}

STATE wait_on_ack(Connection *client)
{
	static int32_t send_count = 0;
	uint32_t crc_check = 0;
	uint8_t buf[MAX_PACKET_LEN];
	int32_t len = MAX_PACKET_LEN;
	uint8_t flag = 0;
	uint32_t seq_num = 0;

	send_count++;
	if (send_count > 10) {
		fprintf(stderr, "Ten failed attempts, terminating client\n");
		return STATE_DONE;
	}

	if (select_call(client->socket_num, 1, 0, NOT_NULL) != 1) {
		return STATE_TIMEOUT_ON_ACK;
	}

	crc_check = recv_buf(buf, len, client->socket_num, client, &flag, &seq_num);

	if (crc_check == CRC_ERROR) {
		return STATE_WAIT_ON_ACK;
	}

	if (flag != FLAG_ACK) {
		fprintf(stderr, "ERROR: non-ACK flag in wait_on_ack\n");
		exit(1);
	}

	send_count = 0;

	return STATE_SEND_DATA;
}

STATE timeout_on_ack(Connection *client, uint8_t *packet, uint32_t packet_len)
{

	return STATE_DONE;
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
       fprintf(stderr, "usage: %s <error_rate>\n" , argv[0]);
       exit(-2);
    }

    sendErr_init(atof(argv[1]),
		DROP_ON,
		FLIP_ON,
		DEBUG_OFF,
		RSEED_OFF);


	server_runloop();

	return 0;
}
