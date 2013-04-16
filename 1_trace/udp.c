// tcp
// trace

#include "trace.h"
#include "checksum.h"

typedef enum {
	UDP_FIN = 1 << 0,
	UDP_SYN = 1 << 1,
	UDP_RST = 1 << 2,
	UDP_PSH = 1 << 3,
	UDP_ACK = 1 << 4,
	UDP_URG = 1 << 5,
	UDP_ECE = 1 << 6,
	UDP_CWR = 1 << 7
} UDPFlags;

struct UDPFrameHeader
{
	uint16_t udp_source_port;
	uint16_t udp_destination_port;
	uint16_t udp_length;
	uint16_t udp_checksum;

} __attribute__((packed));


// psuedoheader
// ===========
// source
// destination
// 0 6 length
// TCP header
// data

void udp(uint8_t *packetData, int packetLength)
{
	struct UDPFrameHeader *header = (struct UDPFrameHeader *)packetData;
	printf("\n\tUDP Header\n");

	printf("\t\tSource Port:  ");
	switch(ntohs(header->udp_source_port)) {
		case PORT_TELNET:
			printf("Telnet\n");
			break;
		case PORT_FTP:
			printf("FTP");
			break;
		case PORT_HTTP:
			printf("HTTP");
			break;
		default:
			printf("%d", ntohs(header->udp_source_port));
			break;
	}
	printf("\n");

	printf("\t\tDest Port:  ");
	switch(ntohs(header->udp_destination_port)) {
		case PORT_TELNET:
			printf("Telnet\n");
			break;
		case PORT_FTP:
			printf("FTP");
			break;
		case PORT_HTTP:
			printf("HTTP");
			break;
		default:
			printf("%d", ntohs(header->udp_destination_port));
			break;
	}
	// TODO: calculate checksum
	// printf("\t\tChecksum: (0x%x)", ntohs(header->udp_checksum));
}