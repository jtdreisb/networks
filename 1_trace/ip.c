// ip
// trace

// #include <netinet/if_ether.h>
#include <arpa/inet.h>
#include "trace.h"
#include "checksum.h"

#define IPv4_ADDR_LENGTH 4

typedef enum {
	IP_PROTO_ICMP = 0x01,
	IP_PROTO_TCP = 0x06,
	IP_PROTO_UDP = 0x11
} IPProtocol;

struct IPFrameHeader
{
	uint8_t ip_version_and_header_len;
	uint8_t ip_dscp_and_ecn;
	uint16_t ip_total_len;
	uint16_t ip_identifier;
	uint16_t ip_flags_and_fragment_offset;
	uint8_t ip_time_to_live;
	uint8_t ip_protocol;
	uint16_t ip_header_checksum;
	in_addr_t ip_source_address;
	in_addr_t ip_destination_address;

} __attribute__((packed));

void ip(uint8_t *packetData, int packetLength)
{
	char *protocolName;
	uint16_t checksum;
	struct in_addr netAddr;
	uint8_t *nextFrame;
	int nextFrameLen;
	struct IPFrameHeader *header = (struct IPFrameHeader *)packetData;

	printf("\tIP Header\n");

	printf("\t\tTOS: 0x%x\n", header->ip_dscp_and_ecn);

	printf("\t\tTTL: %d\n", header->ip_time_to_live);


	switch (header->ip_protocol) {
		case IP_PROTO_ICMP:
			protocolName = "ICMP";
		break;
		case IP_PROTO_TCP:
			protocolName = "TCP";
		break;
		case IP_PROTO_UDP:
			protocolName = "UDP";
		break;
		default:
			protocolName = "Unknown";
		break;
	}
	printf("\t\tProtocol: %s\n", protocolName);

	printf("\t\tChecksum: ");

	checksum = in_cksum((uint16_t *)packetData, header->ip_total_len);
	// unsigned short in_cksum(unsigned short *addr,int len);
	header->ip_header_checksum = ntohs(header->ip_header_checksum);
	if (checksum == 0) {
		printf("Correct");
	} else {
		printf("Incorrect");
	}

	printf(" (0x%x)\n", header->ip_header_checksum);

	netAddr.s_addr = header->ip_source_address;
	printf("\t\tSender IP: %s\n", inet_ntoa(netAddr));

	netAddr.s_addr = header->ip_destination_address;
	printf("\t\tDest IP: %s\n", inet_ntoa(netAddr));

	// printf("%x, %x\n", checksum, header->ip_header_checksum);

	nextFrame = packetData + sizeof(struct IPFrameHeader);
	nextFrameLen = packetLength - sizeof(struct IPFrameHeader);
	switch (header->ip_protocol) {
		case IP_PROTO_ICMP:
			icmp(nextFrame, nextFrameLen);
		break;
		case IP_PROTO_TCP:
			protocolName = "TCP";
		break;
		case IP_PROTO_UDP:
			protocolName = "UDP";
		break;
		default:
			protocolName = "Unknown";
		break;
	}


}