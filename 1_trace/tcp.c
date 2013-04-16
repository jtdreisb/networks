// tcp
// trace

#include "trace.h"
#include "checksum.h"


typedef enum {
	TCP_FIN = 1 << 0,
	TCP_SYN = 1 << 1,
	TCP_RST = 1 << 2,
	TCP_PSH = 1 << 3,
	TCP_ACK = 1 << 4,
	TCP_URG = 1 << 5,
	TCP_ECE = 1 << 6,
	TCP_CWR = 1 << 7
} TCPFlags;

struct TCPFrameHeader
{
	uint16_t tcp_source_port;
	uint16_t tcp_destination_port;
	uint32_t tcp_sequence_number;
	uint32_t tcp_acknowledgement_number;
	uint8_t tcp_data_offset;
	uint8_t tcp_flags;
	uint16_t tcp_window_size;
	uint16_t tcp_checksum;
	uint16_t tcp_urgent_pointer;
} __attribute__((packed));


// psuedoheader
// ===========
// source
// destination
// 0 6 length
// TCP header
// data

void tcp(uint8_t *packetData, int packetLength)
{
	struct TCPFrameHeader *header = (struct TCPFrameHeader *)packetData;
	printf("\n\tTCP Header\n");

	printf("\t\tSource Port:  ");
	switch(ntohs(header->tcp_source_port)) {
		case PORT_TELNET:
			printf("Telnet\n");
			break;
		case PORT_FTP:
			printf("FTP");
			break;
		case PORT_HTTP:
			printf("HTTP");
			break;
		case PORT_POP3:
			printf("POP3");
			break;
		case PORT_SMTP:
			printf("SMTP");
			break;
		default:
			printf("%d", ntohs(header->tcp_source_port));
			break;
	}
	printf("\n");

	printf("\t\tDest Port:  ");
	switch(ntohs(header->tcp_destination_port)) {
		case PORT_TELNET:
			printf("Telnet\n");
			break;
		case PORT_FTP:
			printf("FTP");
			break;
		case PORT_HTTP:
			printf("HTTP");
			break;
		case PORT_POP3:
			printf("POP3");
			break;
		case PORT_SMTP:
			printf("SMTP");
			break;
		default:
			printf("%d", ntohs(header->tcp_destination_port));
			break;
	}
	printf("\n");

	printf("\t\tSequence Number: %u\n", ntohl(header->tcp_sequence_number));
	printf("\t\tACK Number: %u\n", ntohl(header->tcp_acknowledgement_number));
	printf("\t\tSYN Flag: %s\n", (header->tcp_flags & TCP_SYN) ? "Yes" : "No");
	printf("\t\tRST Flag: %s\n", (header->tcp_flags & TCP_RST) ? "Yes" : "No");
	printf("\t\tFIN Flag: %s\n", (header->tcp_flags & TCP_FIN) ? "Yes" : "No");

	printf("\t\tWindow Size: %u\n", ntohs(header->tcp_window_size));

	// TODO: calculate checksum
	printf("\t\tChecksum: (0x%x)", ntohs(header->tcp_checksum));
}