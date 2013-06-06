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
	STATE_WINDOW_FULL,
	STATE_EOF
} STATE;

Connection server;

void rcopy_runloop(char *server_host, uint16_t server_port, char *remote_file_name, char *output_file_name, uint32_t buf_size, uint32_t window_size);
STATE filename(char *fname, int32_t buf_size, int32_t window_size);
STATE recv_data(Window *window);
STATE window_full(Window *window, int output_fd);
STATE recv_eof(Window *window, int output_fd);
void send_ack(int seq_num);
void send_srej(int seq_num);

void rcopy_runloop(char *server_host, uint16_t server_port, char *remote_file_name, char *output_file_name, uint32_t buf_size, uint32_t window_size)
{
	int output_fd = -1;
	int select_count = 0;
	int state = STATE_FILENAME;
	Window *window = NULL;

	while (state != STATE_DONE) {
		switch (state) {

			case STATE_FILENAME:
				if (connectToServer(server_host, server_port, &server)) {
					perror("ERROR: unable to connect to host");
					exit(1);
				}
				state = filename(remote_file_name, buf_size, window_size);
				if (state == STATE_FILENAME) {
					close(server.socket_num);
				}
				select_count++;
				if (select_count >= MAX_RETRIES) {
					fprintf(stderr, "Ten failed filename tries: unable to reach server\n");
					exit(1);
				}
				break;

			case STATE_FILE_OK:
				select_count = 0;
				if((output_fd = open(output_file_name, O_WRONLY|O_CREAT|O_TRUNC, 0600)) < 0 ) {
			    	perror("Open local_file");
			    	exit(1);
			    }
			    window = newWindowWithSizeAndBuffer(window_size, buf_size);
			    state = STATE_RECV_DATA;
			    break;

			case STATE_RECV_DATA:
				state = recv_data(window);
				break;

			case STATE_WINDOW_FULL:
				state = window_full(window, output_fd);
				clearWindow(window);
				window->base_seq_num += window->window_size;
				window->buffer_size = 0;
				break;

			case STATE_EOF:
				if (select_count == 0) {
					window_full(window, output_fd);
				}
				select_count++;
				if (select_count >= MAX_RETRIES) {
					fprintf(stderr, "Ten failed EOF acks: file is ok but server doesn't know\n");
					state = STATE_DONE;
				}
				else {
					state = recv_eof(window, output_fd);
				}
				break;

			case STATE_DONE:
			default:
				break;
		}
	}

	if (window != NULL) {
		destroyWindow(window);
		window = NULL;
	}

	close(output_fd);
}

STATE filename(char *fname, int32_t buf_size, int32_t window_size)
{
	uint8_t buf[MAX_PACKET_LEN];
	uint8_t flag = 0;
	uint32_t seq_num = 0;
	int32_t fname_len = strlen(fname) + 1;
	int32_t recv_check = 0;

	buf_size = htonl(buf_size);
	memcpy(buf, &buf_size, 4);
	window_size = htonl(window_size);
	memcpy(&buf[4], &window_size, 4);
	memcpy(&buf[8], fname, fname_len);

	send_buf(buf, fname_len + 8, &server, FLAG_FILENAME_REQ, 0);

	if (select_call(server.socket_num, 1, 0, NOT_NULL) == 1) {
		recv_check = recv_buf(buf, MAX_PACKET_LEN, server.socket_num, &server, &flag, &seq_num);

		if (recv_check == CRC_ERROR) {
			return STATE_FILENAME;
		}

		switch (flag) {
			case FLAG_FILENAME_RESP_OK:
				return STATE_FILE_OK;
				break;
			case FLAG_FILENAME_RESP_BAD:
				printf("File (%s) not found on server\n", fname);
				exit(1);
				break;
			default:
				break;
		}
	}

	return STATE_FILENAME;
}

STATE recv_data(Window *window)
{
	uint32_t seq_num = 0;
	uint8_t flag = 0;
	int32_t data_len = 0;
	uint8_t data_buf[MAX_PACKET_LEN];

	uint32_t window_index;
	uint32_t buffer_offset;
	uint32_t next_seq_num;
	uint32_t max_seq_num;

	static uint32_t expected_seq_number = 0;

	if (select_call(server.socket_num, 10, 0, NOT_NULL) == 0) {
		fprintf(stderr, "Shutting down: No response from server for 10 seconds.\n");
		exit(1);
	}

	data_len = recv_buf(data_buf, MAX_PACKET_LEN, server.socket_num, &server, &flag, &seq_num);

	if (data_len == CRC_ERROR) {
		return STATE_RECV_DATA;
	}

	switch (flag) {
		case FLAG_DATA:
		case FLAG_DATA_RESENT:

			max_seq_num = maxSequenceNumber(window);
			next_seq_num = nextOpenSequenceNumber(window);

			// printf("PACKET: seq_num: %u max: %u next: %u exp: %u\n", seq_num, max_seq_num, next_seq_num, expected_seq_number);

			if (seq_num < next_seq_num) {
				if (seq_num < window->base_seq_num) {
					send_ack(window->base_seq_num-1);
				}
				else {
					if (windowIsFull(window)) {
						send_ack(max_seq_num);
					}
					else {
						send_srej(next_seq_num);
					}
				}
				break;
			}

			// See if the seq_num falls within this windows range
			if ((seq_num >= window->base_seq_num) && (seq_num < (window->base_seq_num + window->window_size))) {
				window_index = (seq_num-1) % window->window_size;
				// Save data if it isn't already in there
				if (window->registry[window_index] == 0) {

					buffer_offset = window_index * window->block_size;
					memcpy(&window->buffer[buffer_offset], data_buf, data_len);

					// Mark the window as received
					window->registry[window_index] = 1;

					max_seq_num = maxSequenceNumber(window);
					next_seq_num = nextOpenSequenceNumber(window);

					if (seq_num == max_seq_num) {
						window->buffer_size = buffer_offset + data_len;
					}
				}
			}


			if (seq_num > next_seq_num) {
				// the previous sequence number
				window_index = ((seq_num-1) % window->window_size);
				if (window->registry[window_index-1] == 0) {
					send_srej(window->base_seq_num+window_index-1);
				}
			}
			else {
				send_ack(next_seq_num-1);
			}

			expected_seq_number = next_seq_num;

			if (windowIsFull(window)) {
				// printf("Window Full\n");
				return STATE_WINDOW_FULL;
			}

			break;

		case FLAG_END_OF_FILE:
			// printf("GOT EOF: seq_num: %u\n", seq_num);// !!!
			return STATE_EOF;
			break;

		default:
			break;

	}

	return STATE_RECV_DATA;
}

STATE window_full(Window *window, int output_fd)
{
	// uint32_t window_count = nextOpenSequenceNumber(window) - window->base_seq_num;
	// printf("Writing %d windows %u bytes\n", window_count, window->buffer_size); // !!!
	write(output_fd, window->buffer, window->buffer_size);
	return STATE_RECV_DATA;
}

STATE recv_eof(Window *window, int output_fd)
{
	uint8_t flag = 0;
	int32_t data_len = 0;
	uint8_t data_buf[MAX_PACKET_LEN];
	uint32_t seq_num = nextOpenSequenceNumber(window);

	send_ack(seq_num);
	// Drain the packet queue
	while (select_call(server.socket_num, 1, 0, NOT_NULL) == 1) {
		data_len = recv_buf(data_buf, MAX_PACKET_LEN, server.socket_num, &server, &flag, &seq_num);
		return STATE_EOF;
	}

	printf("File Transfer Complete\n");
	return STATE_DONE;
}

void send_ack(int seq_num)
{
	// printf("SEND ACK: %u\n", seq_num);
	send_buf(NULL, 0, &server, FLAG_ACK, seq_num);
}

void send_srej(int seq_num)
{
	// printf("SEND SREJ: %u\n", seq_num);
	send_buf(NULL, 0, &server, FLAG_SREJ, seq_num);
}

int main(int argc, char *argv[])
{
	char *remote_file_name;
	char *local_file_name;
	uint32_t buf_size;
	uint32_t window_size;

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
    local_file_name = argv[2];

    buf_size = atoi(argv[3]);

    sendErr_init(atof(argv[4]),
		DROP_ON,
		FLIP_ON,
		DEBUG_OFF,
		RSEED_OFF);

    window_size = atoi(argv[5]);

    rcopy_runloop(argv[6], atoi(argv[7]), remote_file_name, local_file_name, buf_size, window_size);

	return 0;
}
