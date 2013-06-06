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
	STATE_WINDOW_NEXT,
	STATE_WINDOW_CLOSE,
	STATE_SEND_DATA,
	STATE_SEND_EOF,
	STATE_READ_ACKS,
	STATE_READ_EOF_ACK
} STATE;

void server_runloop();
void process_client(uint8_t *buf, int32_t recv_len, Connection *client, int nest_level);
STATE filename(Connection *client, uint8_t *buf, int32_t recv_len, int32_t *data_file, Window **window);
STATE send_next_data(Connection *client, Window *window);
STATE read_acks(Connection *client, Window *window, int nest_level);
STATE next_window(Window *window, int data_file);
STATE close_window(Connection *client, Window *window, int nest_level);
STATE send_eof(Connection *client, Window *window);

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
					process_client(buf, recv_len, &client, 0);
					exit(0);
				}
				while (waitpid(-1, &status, WNOHANG) > 0) {
				}
			}
		}
	}
}

void process_client(uint8_t *buf, int32_t recv_len, Connection *client, int nest_level)
{
	STATE state = STATE_START;

	int32_t data_file = 0;

	Window *window = NULL;

	if (nest_level < MAX_RETRIES) {
		while (state != STATE_DONE) {
			switch (state) {

				case STATE_START:
					state = STATE_FILENAME;

				case STATE_FILENAME:
					state = filename(client, buf, recv_len, &data_file, &window);
					break;

				case STATE_WINDOW_NEXT:
					state = next_window(window, data_file);
					break;

				case STATE_WINDOW_CLOSE:
					state = close_window(client, window, nest_level);
					break;

				case STATE_SEND_DATA:
					state = send_next_data(client, window);
					break;

				case STATE_SEND_EOF:
					state = send_eof(client, window);
					break;

				case STATE_READ_ACKS:
					state = read_acks(client, window, nest_level);
					break;

				case STATE_DONE:
				default:
					state = STATE_DONE;
					break;
			}
		}
	}
	if (window != NULL) {
		destroyWindow(window);
		window = NULL;
	}
}

STATE filename(Connection *client, uint8_t *buf, int32_t recv_len, int32_t *data_file, Window **window)
{
	char fname[MAX_FILE_NAME_LEN];
	uint32_t buf_size;
	uint32_t window_size;

	memcpy(&buf_size, buf, 4);
	buf_size = ntohl(buf_size);
	memcpy(&window_size, &buf[4], 4);
	window_size = ntohl(window_size);

	if ((client->socket_num = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("filename:socket");
		exit(1);
	}

	if ((recv_len - 8) > MAX_FILE_NAME_LEN) {
		send_buf(NULL, 0, client, FLAG_FILENAME_RESP_BAD, 0);
		return STATE_DONE;
	}

	memcpy(fname, &buf[8], recv_len - 8);

	if (((*data_file) = open(fname, O_RDONLY)) < 0) {
		send_buf(NULL, 0, client, FLAG_FILENAME_RESP_BAD, 0);
		free(*window);
		*window = NULL;
		return STATE_DONE;
	}

	send_buf(NULL, 0, client, FLAG_FILENAME_RESP_OK, 0);

	if (window != NULL) {
		*window = newWindowWithSizeAndBuffer(window_size, buf_size);
	}

	return STATE_WINDOW_NEXT;
}

STATE send_next_data(Connection *client, Window *window)
{
	STATE state_to_return = STATE_READ_ACKS;

	uint32_t window_buffer_offset = window->window_index * window->block_size;
	uint32_t packet_len = window->block_size;

	int32_t data_left = window->data_len - window_buffer_offset;
	if (data_left <= window->block_size) {
		packet_len = data_left;
		state_to_return = STATE_WINDOW_CLOSE;
	}
	// printf("SEND %u\n", window->base_seq_num + window->window_index); // !!!
	send_buf(window->buffer + window_buffer_offset, packet_len, client, FLAG_DATA, window->base_seq_num + window->window_index);

	window->window_index++;

	return state_to_return;
}

void send_data(Connection *client, Window *window, uint32_t seq_num)
{
	printf("RESEND %u\n", seq_num); // !!!

	uint32_t window_index = (seq_num-1) % window->window_size;

	uint32_t window_buffer_offset = window_index * window->block_size;

	uint32_t packet_len = window->block_size;

	int32_t data_left = window->buffer_size - window_buffer_offset;

	if (data_left < window->block_size) {
		packet_len = data_left;
	}
	send_buf(window->buffer + window_buffer_offset, packet_len, client, FLAG_DATA_RESENT, seq_num);
}

STATE read_acks(Connection *client, Window *window, int nest_level)
{
	uint32_t recv_len = 0;
	uint8_t buf[MAX_PACKET_LEN];
	uint8_t flag = 0;
	uint32_t seq_num = 0;
	uint32_t window_index;
	int i = 0;

	while(select_call(client->socket_num, 0, 0, NOT_NULL)) {
		recv_len = recv_buf(buf, MAX_PACKET_LEN, client->socket_num, client, &flag, &seq_num);

		if (recv_len == CRC_ERROR) {
			return STATE_SEND_DATA;
		}

		if ((seq_num >= window->base_seq_num) && (seq_num < (window->base_seq_num + window->window_size))) {
			window_index = (seq_num-1) % window->window_size;
			switch (flag) {
				case FLAG_FILENAME_REQ:
					process_client(buf, recv_len, client, nest_level + 1);
					return STATE_DONE;
					break;

				case FLAG_ACK:
					printf("-ACK %u\n", seq_num);
					for (i=0; i < window_index; i++) {
						window->registry[i] = 1;
					}
					break;

				case FLAG_SREJ:
					printf("-SREJ %u\n", seq_num);
					send_data(client, window, seq_num);
					break;

				default:
					fprintf(stderr, "ERROR: non-ACK flag (%u) in wait_on_ack\n", flag);
					break;
			}
		}
	}

	return STATE_SEND_DATA;
}

STATE next_window(Window *window, int data_file)
{
	int32_t len_read = 0;
	printf("Loading window base: %u length: %u size: %u\n", window->base_seq_num, window->block_size, window->window_size); // !!!
	len_read = read(data_file, window->buffer, window->buffer_size);
	clearWindow(window);
	window->window_index = 0;

	switch (len_read) {
		case -1:
			perror("send_data: read");
			exit(1);
			break;

		case 0:
			return STATE_SEND_EOF;
			break;

		default:
			window->data_len = len_read;
			break;
	}
	return STATE_SEND_DATA;
}

STATE close_window(Connection *client, Window *window, int nest_level)
{
	uint32_t recv_len = 0;
	uint8_t buf[MAX_PACKET_LEN];
	uint8_t flag = 0;
	uint32_t seq_num = 0;
	int send_count = 0;
	uint32_t window_index = 0;
	int i;

	for (;;) {

		if (windowIsFull(window)) {
			printf("!!! Window Full next_base: %u\n", nextOpenSequenceNumber(window));
			window->base_seq_num = nextOpenSequenceNumber(window);
			return STATE_WINDOW_NEXT;
		}

		if(select_call(client->socket_num, 1, 0, NOT_NULL)) {
			recv_len = recv_buf(buf, MAX_PACKET_LEN, client->socket_num, client, &flag, &seq_num);

			if (recv_len == CRC_ERROR) {
				continue;
			}

			if ((seq_num >= window->base_seq_num) && (seq_num < (window->base_seq_num + window->window_size))) {

				window_index = (seq_num-1) % window->window_size;

				switch (flag) {
					case FLAG_ACK:
						send_count = 0;
						for (i= 0; i <= window_index; i++) {
							window->registry[i] = 1;
						}
						printf("ACK %u\n", window_index);
						break;

					case FLAG_SREJ:
						send_count = 0;
						printf("SREJ %u\n", window_index);
						send_data(client, window, seq_num);
						break;

					default:
						fprintf(stderr, "ERROR: non-ACK flag in wait_on_ack\n");
						break;
				}
			}
		}
		else {
			send_count++;
			if (send_count > MAX_RETRIES) {
				fprintf(stderr, "ERROR: No Response from client in 10 seconds");
				exit(1);
			}
			printf("Timeout!\n"); // !!!
			send_data(client, window, window->base_seq_num);
		}
	}


	return STATE_DONE;
}

STATE send_eof(Connection *client, Window *window)
{
	uint32_t recv_len = 0;
	uint8_t buf[MAX_PACKET_LEN];
	uint8_t flag = 0;
	uint32_t seq_num = 0;
	int select_count = 0;

	for (;;) {
		// printf("Send EOF: base: %u registry: %u\n", window->base_seq_num, window->registry[0]);
		send_buf(NULL, 0, client, FLAG_END_OF_FILE, window->base_seq_num);

		select_count++;

		while(select_call(client->socket_num, 1, 0, NOT_NULL)) {
			recv_len = recv_buf(buf, MAX_PACKET_LEN, client->socket_num, client, &flag, &seq_num);

			if (recv_len == CRC_ERROR) {
				continue;
			}

			switch (flag) {
				case FLAG_SREJ:
				case FLAG_ACK:
					if (seq_num == window->base_seq_num) {
						return STATE_DONE;
					}
					break;
				default:
					fprintf(stderr, "ERROR: non-ACK flag (%u) in wait_on_ack\n", flag);
					break;
			}
		}

		if (select_count >= MAX_RETRIES) {
			fprintf(stderr, "ERROR: 10 failed EOF tries. Bailing\n");
			exit(1);
		}
	}
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
