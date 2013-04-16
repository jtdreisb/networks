// trace
// trace
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

void ethernet(u_char *packetData, int packetLength);
void arp(u_char *packetData, int packetLength);
void ip(u_char *packetData, int packetLength);