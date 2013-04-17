// ip
// trace

// #include <netinet/if_ether.h>
#include <arpa/inet.h>
#include "trace.h"
#include "checksum.h"

void ip(uint8_t *packetData, int packetLength)
{
	char *protocolName;
	uint16_t checksum;
	struct in_addr netAddr;
	uint8_t *nextFrame;
	int nextFrameLength;
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

	checksum = in_cksum((uint16_t *)packetData, sizeof(struct IPFrameHeader));
	// unsigned short in_cksum(unsigned short *addr,int len);
	header->ip_header_checksum = ntohs(header->ip_header_checksum);
	if (checksum == 0) {
		printf("Correct");
	}
	else {
		printf("Incorrect");
	}

	printf(" (0x%x)\n", header->ip_header_checksum);

	netAddr.s_addr = header->ip_source_address;
	printf("\t\tSender IP: %s\n", inet_ntoa(netAddr));

	netAddr.s_addr = header->ip_destination_address;
	printf("\t\tDest IP: %s\n", inet_ntoa(netAddr));

	// printf("%x, %x\n", checksum, header->ip_header_checksum);
	nextFrame = packetData + ((header->ip_version_and_header_length & 0x0F) * 4);
	nextFrameLength = packetLength - ((header->ip_version_and_header_length & 0x0F) * 4);
	switch (header->ip_protocol) {
		case IP_PROTO_ICMP:
			icmp(nextFrame, nextFrameLength);
		break;
		case IP_PROTO_TCP:
			tcp(nextFrame, nextFrameLength, header);
		break;
		case IP_PROTO_UDP:
			udp(nextFrame, nextFrameLength);
		break;
	}
}