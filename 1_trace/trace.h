// trace
// trace
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdint.h>
#ifdef __APPLE__
#include <netinet/if_ether.h>
#else
#include <netinet/ether.h>
#endif
#include <arpa/inet.h>

typedef enum {
	PORT_HTTP = 80,
	PORT_TELNET = 23,
	PORT_FTP = 20,
	// TCP only
	PORT_POP3 = 110,
	PORT_SMTP = 25
} SPECIAL_PORT_NAMES;

struct EthernetFrameHeader
{
	uint8_t destinationMAC[6];
	uint8_t sourceMAC[6];
	uint16_t etherType;
} __attribute__((packed));

void ethernet(uint8_t *packetData, int packetLength);


struct ARPFrameHeader
{
	uint16_t	arp_hardware_format;
	uint16_t	arp_protocol_format;
	uint8_t	arp_hardware_length;
	uint8_t	arp_protocol_length;
	uint16_t	arp_opcode;
} __attribute__((packed));

void arp(uint8_t *packetData, int packetLength);

typedef enum {
	IP_PROTO_ICMP = 0x01,
	IP_PROTO_TCP = 0x06,
	IP_PROTO_UDP = 0x11
} IPProtocol;

struct IPFrameHeader
{
	uint8_t ip_version_and_header_length;
	uint8_t ip_dscp_and_ecn;
	uint16_t ip_total_length;
	uint16_t ip_identifier;
	uint16_t ip_flags_and_fragment_offset;
	uint8_t ip_time_to_live;
	uint8_t ip_protocol;
	uint16_t ip_header_checksum;
	in_addr_t ip_source_address;
	in_addr_t ip_destination_address;

} __attribute__((packed));

void ip(uint8_t *packetData, int packetLength);

struct ICMPFrameHeader
{
	uint8_t icmp_type;
	uint8_t icmp_code;
	uint16_t icmp_checksum;

} __attribute__((packed));

void icmp(uint8_t *packetData, int packetLength);

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

struct TCPPsuedoHeader {
	uint32_t tcp_source_address;
	uint32_t tcp_destination_address;
	uint8_t tcp_zeroes;
	uint8_t tcp_protocol;
	uint16_t tcp_length;
} __attribute__((packed));

void tcp(uint8_t *packetData, int packetLength, struct IPFrameHeader *ipHeader);

struct UDPFrameHeader
{
	uint16_t udp_source_port;
	uint16_t udp_destination_port;
	uint16_t udp_length;
	uint16_t udp_checksum;

} __attribute__((packed));

void udp(uint8_t *packetData, int packetLength);