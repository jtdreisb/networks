// tcp
// trace

#include "trace.h"
#include "checksum.h"
#include <string.h>
#include <stdlib.h>

uint16_t tcp_checksum(struct TCPFrameHeader *tcp_header, struct IPFrameHeader *ip_header)
{
	uint8_t ipHeaderSize = ((ip_header->ip_version_and_header_length & 0x0F) * 4);
	uint16_t ipHeaderTotalLength = ntohs(ip_header->ip_total_length);
	uint16_t tcpFullHeaderLength = sizeof(struct TCPPsuedoHeader) + ipHeaderTotalLength - ipHeaderSize;
	uint16_t tcpLength = ipHeaderTotalLength - ipHeaderSize;

	struct TCPPsuedoHeader *psuedoHeader = malloc(tcpFullHeaderLength);

	psuedoHeader->tcp_source_address = ip_header->ip_source_address;
	psuedoHeader->tcp_destination_address = ip_header->ip_destination_address;
	psuedoHeader->tcp_zeroes = 0;
	psuedoHeader->tcp_protocol = IP_PROTO_TCP;
	psuedoHeader->tcp_length = htons(tcpLength);

	memcpy(((char *)psuedoHeader)+sizeof(struct TCPPsuedoHeader), tcp_header, tcpLength);
	uint16_t checksum = in_cksum((uint16_t *)psuedoHeader, tcpFullHeaderLength);
	free((void *)psuedoHeader);
	return checksum;
}

void tcp(uint8_t *packetData, int packetLength, struct IPFrameHeader *ipHeader)
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


	printf("\t\tChecksum: ");


	if(tcp_checksum(header, ipHeader)) {
		printf("Incorrect");
	}
	else {
		printf("Correct");
	}

	printf(" (0x%x)", ntohs(header->tcp_checksum));
	// free(psuedoHeader);

}