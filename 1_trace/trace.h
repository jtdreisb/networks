// trace
// trace
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdint.h>

typedef enum {
	PORT_HTTP = 80,
	PORT_TELNET = 23,
	PORT_FTP = 20,
	// TCP only
	PORT_POP3 = 110,
	PORT_SMTP = 25
} SPECIAL_PORT_NAMES;

void ethernet(uint8_t *packetData, int packetLength);
void arp(uint8_t *packetData, int packetLength);
void ip(uint8_t *packetData, int packetLength);
void icmp(uint8_t *packetData, int packetLength);
void tcp(uint8_t *packetData, int packetLength);
void udp(uint8_t *packetData, int packetLength);