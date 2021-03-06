// rcopy
// CPE464 Program 3
// Jason Dreisbach

/* UDP code   - written by Hugh Smith (any bugs were written by
   someone else.)*/

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include "util.h"
#include <math.h>
#include "cpe464.h"

Window *newWindowWithSizeAndBuffer(uint32_t window_size, uint32_t buffer_size)
{
  Window *window = malloc(sizeof(Window));
  window->base_seq_num = START_SEQ_NUM;
  window->window_size = window_size;
  window->block_size = buffer_size;
  window->buffer_size = window_size * buffer_size;
  window->registry = malloc(window_size);
  window->buffer = malloc(window_size * buffer_size);
  window->data_len = window->block_size * window->window_size;
  clearWindow(window);
  return window;
}

void clearWindow(Window *window)
{
  window->window_index = 0;
  memset(window->registry, 0, window->window_size);
}

int32_t windowIsFull(Window *window)
{
  int32_t window_size = (int32_t) ceil((1.0 * window->data_len) / window->block_size);
  int i;
  for (i = 0; i < window_size; i++) {
    if(window->registry[i] == 0) {
      return 0; // Not full: found an empty spot
    }
  }

  return 1; // Full: not empty spots filled
}

uint32_t maxSequenceNumber(Window *window)
{
  uint32_t max_window_index = 0;
  int i = 0;
  for (i = 0; i < window->window_size; i++) {
    if(window->registry[i] == 1) {
      max_window_index = i;
    }
  }
  return window->base_seq_num + max_window_index;
}

uint32_t nextOpenSequenceNumber(Window *window)
{
  int i;
  for (i = 0; i < window->window_size; i++) {
    if(window->registry[i] == 0) {
      break;
    }
  }
  return window->base_seq_num + i;
}

void destroyWindow(Window *window)
{
  if (window != NULL) {
    if (window->buffer != NULL)
      free(window->buffer);
    if(window->registry != NULL)
      free(window->registry);
    free(window);
  }
}

int32_t startServer()
{
  int sk = 0;     // socket descriptor
  struct sockaddr_in  local ;   // socket address for us
  uint32_t len = sizeof(local) ;  // length of local address

  if ((sk = socket(AF_INET,SOCK_DGRAM,0)) < 0) {
    perror("socket");
    exit(-1);
  }

  local.sin_family = AF_INET ;
  local.sin_addr.s_addr = INADDR_ANY ;
  local.sin_port = htons(0);

  if (bind(sk,(struct sockaddr *)&local,sizeof(local)) < 0)
    {
      perror("udp_server, bind");
      exit(-1);
    }

  // get the port name and print32_t it out
  getsockname(sk,(struct sockaddr *)&local,  &len) ;
  printf("Using Port #: %d\n", ntohs(local.sin_port));

  return(sk);
}

int32_t connectToServer(char * hostname, uint16_t port_num, Connection * connection)
{
  /* returns pointer to a sockaddr_in that it created or NULL if host not found */
  /* also passes back the socket number in sk */


  struct hostent * hp = NULL ;       // address of remote host

  connection->socket_num = 0;
  connection->len = sizeof(struct sockaddr_in);

    // create the socket
  if ((connection->socket_num = socket(AF_INET,SOCK_DGRAM,0)) < 0)
    {
      perror("udp_client_setup, socket");
      exit(-1);

    }

  // designate the addressing family
  connection->remote.sin_family = AF_INET ;

  // get the address of the remote host and store
  hp = gethostbyname(hostname);

  if (hp == NULL)
    {
      printf("Host not found: %s\n", hostname);
      return -1;
    }

  memcpy(&(connection->remote.sin_addr),hp->h_addr,hp->h_length) ;

  // get the port used on the remote side and store
  connection->remote.sin_port = htons(port_num);

  return 0;

}

int32_t select_call(int32_t socket_num, int32_t seconds, int32_t microseconds, int32_t set_null)
{
  /* this function is written to only look at one socket  */

  fd_set fdvar;
  struct timeval * timeout = NULL;

  if (set_null == NOT_NULL) {
    timeout = (struct timeval *) malloc(sizeof(struct timeval));
    timeout->tv_sec = seconds;  // set timeout to 1 second
    timeout->tv_usec = microseconds; // set timeout (in micro-second)
  }

  FD_ZERO(&fdvar); 	  // reset variables
  FD_SET(socket_num, &fdvar);	  //

  if (select(socket_num+1,(fd_set *) &fdvar, (fd_set *) 0, (fd_set *) 0, timeout) < 0) {
    perror("select");
    exit(-1);
  }

  if (FD_ISSET(socket_num, &fdvar)) {
    return 1;
  }
  return 0;
}

int32_t recv_buf(uint8_t *buf, uint32_t len, int32_t recv_sock, Connection *conn, uint8_t *flag, uint32_t *seq_num)
{
  char data_buf[MAX_PACKET_LEN];
  int32_t recv_len = 0;
  uint32_t remote_len = sizeof(struct sockaddr_in);


  if ((recv_len = recvfrom(recv_sock, data_buf, len, 0, (struct sockaddr *) &(conn->remote), &remote_len)) < 0) {
    perror("recv_buf:recvfrom");
    exit(1);
  }

   memcpy(seq_num, data_buf, 4);
  *seq_num = ntohl(*seq_num);

  *flag = data_buf[6];

  if (recv_len > 7) {
    memcpy(buf, &data_buf[7], recv_len-8);
  }


  uint16_t checksum = 0;
  memcpy(&checksum, &data_buf[4], 2);


  // printf("recv_buf: len: %u seq: %u flag: %u sum:%u\n", recv_len, *seq_num, *flag, checksum) ;

  if (in_cksum((unsigned short *)data_buf, recv_len) != 0) {
    return CRC_ERROR;
  }

  conn->len = remote_len;
  return (recv_len - 8);
}


int32_t send_buf(uint8_t *buf, uint32_t len, Connection *conn, uint8_t flag, uint32_t seq_num)
{
  uint8_t packet[MAX_PACKET_LEN];
  int32_t send_len = 0;
  uint16_t checksum = 0;

  if (len > 0) {
    memcpy(&packet[7], buf, len);
  }

  seq_num = htonl(seq_num);
  memcpy(packet, &seq_num, 4);
  packet[6] = flag;

  memset(&packet[4], 0, 2);
  checksum = in_cksum((unsigned short *)packet, len+8);
  memcpy(&packet[4], &checksum, 2);

  if ((send_len = sendtoErr(conn->socket_num, packet, len+8, 0, (struct sockaddr *)&conn->remote, conn->len)) < 0) {
    perror("send_buf:sendto");
    exit(1);
  }

  // printf("send_buf: len: %u seq: %u flag: %u sum:%u\n", send_len, ntohl(seq_num), flag, checksum);

  return send_len;
}
