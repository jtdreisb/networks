// rcopy
// CPE464 Program 3
// Jason Dreisbach
#include "cpe464.h"
#include "util.h"

#define MAX_FILE_NAME_LEN 255

typedef enum
{
	STATE_START,
	STATE_DONE,
	STATE_FILENAME,
	STATE_FILE_OK,
	STATE_RECV_DATA,
	STATE_WAIT_ON_ACK
} STATE;

Connection server;

STATE filename(char *fname, int32_t buf_size, int32_t window_size);
STATE recv_data(int output_fd);

STATE filename(char *fname, int32_t buf_size, int32_t window_size)
{
	uint8_t packet[MAX_PACKET_LEN];
	uint8_t buf[MAX_PACKET_LEN];
	uint8_t flag = 0;
	uint32_t seq_num = 0;
	int32_t fname_len = strlen(fname) + 1;
	int32_t recv_check = 0;

	memcpy(buf, &buf_size, 4);
	memcpy(&buf[4], fname, fname_len);

	send_buf(buf, fname_len + 4, &server, FLAG_FILENAME_REQ, 0, packet);

	if (select_call(server.socket_num, 1, 0, NOT_NULL) == 1) {
		recv_check = recv_buf(packet, MAX_PACKET_LEN, server.socket_num, &server, &flag, &seq_num);

		if (recv_check == CRC_ERROR) {
			return STATE_FILENAME;
		}

		if (flag == FLAG_FILENAME_RESP_BAD) {
			printf("File (%s) not found on server\n", fname);
			exit(1);
		}
		return STATE_FILE_OK;
	}

	return STATE_FILENAME;
}

STATE recv_data(int output_fd)
{
	uint32_t seq_num = 0;
	uint8_t flag = 0;
	int32_t data_len = 0;
	uint8_t data_buf[MAX_PACKET_LEN];
	uint8_t packet[MAX_PACKET_LEN];
	static int32_t expected_seq_num = START_SEQ_NUM;

	if (select_call(server.socket_num, 10, 0, NOT_NULL) == 0) {
		printf("Shutting down: No response from server for 10 seconds.\n");
		exit(1);
	}

	data_len = recv_buf(data_buf, MAX_PACKET_LEN, server.socket_num, &server, &flag, &seq_num);

	if (data_len == CRC_ERROR) {
		// TODO: SREJ it
		return STATE_RECV_DATA;
	}

	send_buf(packet, 1, &server, FLAG_ACK, 0, packet);

	if (flag == FLAG_END_OF_FILE) {
		printf("File Transfer Complete\n");
		return STATE_DONE;
	}

	if (seq_num == expected_seq_num) {
		expected_seq_num++;
		write(output_fd, &data_buf, data_len);
	}

	return STATE_RECV_DATA;
}

int main(int argc, char *argv[])
{
	char *remote_file_name;
	int output_fd = -1;
	uint32_t buf_size;
	uint32_t window_size;

	int select_count = 0;


	if (argc != 8) {
       fprintf(stderr, "usage: %s <remote_file_name> <local_file_name> <buf_size> <error_rate> <window_size> <remote-machine> <remote-port>\n" , argv[0]);
       exit(-2);
    }

    // Save remote file name
    if (strlen(argv[1]) >  MAX_FILE_NAME_LEN) {
    	fprintf(stderr, "ERROR: Remote filename exceeds maximum allowed length\n");
    	exit(1);
    }
    remote_file_name = argv[1];

    // Save local file name
    if (strlen(argv[2]) >  MAX_FILE_NAME_LEN) {
    	fprintf(stderr, "ERROR: Local filename exceeds maximum allowed length\n");
    	exit(1);
    }

    buf_size = atoi(argv[3]);

    sendErr_init(atof(argv[4]),
		DROP_ON,
		FLIP_ON,
		DEBUG_OFF,
		RSEED_OFF);

    window_size = atoi(argv[5]);

	select_count = 0; //TODO: REMOVE
	int state = STATE_FILENAME;

	while (state != STATE_DONE) {
		switch (state) {
			case STATE_FILENAME:
				if (connectToServer(argv[6], atoi(argv[7]), &server)) {
					perror("ERROR: unable to connect to host");
					exit(1);
				}
				state = filename(remote_file_name, buf_size, window_size);
				if (state == STATE_FILENAME) {
					close(server.socket_num);
				}
				select_count++;
				if (select_count > 9) {
					fprintf(stderr, "Ten retries unable to reach server\n");
					exit(1);
				}
				break;

			case STATE_FILE_OK:
				select_count = 0;
				if((output_fd = open(argv[2], O_WRONLY|O_CREAT|O_TRUNC, 0600)) < 0 ) {
			    	perror("main:open");
			    	exit(1);
			    }
			    state = STATE_RECV_DATA;
			    break;

			case STATE_RECV_DATA:
				state = recv_data(output_fd);
				break;

			case STATE_DONE:
			default:
				break;
		}
	}


	close(output_fd);

	return 0;
}
