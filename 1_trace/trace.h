// trace
// trace
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdint.h>

void ethernet(uint8_t *packetData, int packetLength);
void arp(uint8_t *packetData, int packetLength);
void ip(uint8_t *packetData, int packetLength);
void icmp(uint8_t *packetData, int packetLength);