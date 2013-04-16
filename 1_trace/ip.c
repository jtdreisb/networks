// ip
// trace

#include <netinet/if_ether.h>
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
	u_char ip_version_and_header_len;
	u_char ip_dscp_and_ecn;
	u_short ip_total_len;
	u_short ip_identifier;
	u_short ip_flags_and_fragment_offset;
	u_char ip_time_to_live;
	u_char ip_protocol;
	u_short ip_header_checksum;
	u_char ip_source_address[IPv4_ADDR_LENGTH];
	u_char ip_destination_address[IPv4_ADDR_LENGTH];

} __attribute__((packed));

void ip(u_char *packetData, int packetLength)
{
	char *protocolName;
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

	// unsigned short in_cksum(unsigned short *addr,int len);
	header->ip_header_checksum = ntohs(header->ip_header_checksum);


}