#include <stdio.h>
#include <stdlib.h>

#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>

#include <string.h>

#include <errno.h>

#include <inttypes.h>

#include "proto/client.h"

struct client {
  struct hostent* server;
  uint16_t port_nr;
  uint32_t nickname_size;
  char* nickname;
};


static struct client parse_args(int, char**);


int main(int argc, char *argv[]) {
  struct client me = parse_args(argc, argv);


  /* Create a socket point */
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if (sockfd < 0) {
    perror("ERROR opening socket");
    exit(errno);
  }

  struct sockaddr_in serv_addr;
  bzero((char *)&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(me.port_nr);
  
  bcopy(me.server->h_addr, (char *)&serv_addr.sin_addr.s_addr,
        (size_t)me.server->h_length);

  /* Now connect to the server */
  if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("ERROR connecting");
    exit(errno);
  }

  /* Now ask for a message from the user, this message
   * will be read by server
   */

  char* message = NULL;
  size_t allocated_length = 0;
  printf("Please enter the message: ");
  if (getline(&message, &allocated_length, stdin) == -1) {
    free(message);
    perror("ERROR reading from stdin");
    exit(errno);
  }

  /* Send message to the server */
  size_t msg_length = strlen(message);
  char* payload = NULL;
  size_t payload_length = client_msg_serialize({me.nickname_size, me.nickname, (uint32_t) msg_length, message}, 0, &payload);
  printf("sent sizes: %" PRIu32 "; %zu\n", me.nickname_size, msg_length);
  printf("[%.*s]: %.*s\n",
    (int) me.nickname_size, me.nickname,
    (int) msg_length, message);

  int n = write(sockfd, payload, payload_length);
  free(payload);
  free(message);

  if (n < 0) {
    perror("ERROR writing to socket");
    exit(1);
  }

  /* Now read server response */
  char buffer[256];
  bzero(buffer, 256);
  n = read(sockfd, buffer, 255);

  struct server_msg response = server_msg_deserialize(buffer);

  if (n < 0) {
    perror("ERROR reading from socket");
    exit(errno);
  }

  printf("sizes: %" PRIu32 "; %" PRIu32 "; %" PRIu32 "\n", response.date_size, response.nickname_size, response.body_size);
  printf("{%.*s} [%.*s]: %.*s\n",
    (int) response.date_size, response.date,
    (int) response.nickname_size, response.nickname,
    (int) response.body_size, response.body);

  free(response.body);
  free(response.date);
  free(response.nickname);
  return 0;
}


static struct client parse_args(int argc, char** argv) {
  if (argc < 4) {
    fprintf(stderr, "usage %s [hostname] [port] [nickname]\n", argv[0]);
    exit(EINVAL);
  }

  struct client me = {0, 0, 0, 0};
  me.server = gethostbyname(argv[1]);

  if (me.server == NULL) {
    herror("ERROR");
    exit(h_errno);
  }

  me.port_nr = (uint16_t) strtoul(argv[2], NULL, 10);
  me.nickname = argv[3];
  me.nickname_size = strlen(argv[3]);
  return me;
}
