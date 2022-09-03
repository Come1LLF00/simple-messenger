#include <stdio.h>
#include <stdlib.h>

#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>

#include <string.h>

#include <errno.h>

// #include "proto/client.h"

struct client {
  struct hostent* server;
  uint16_t port_nr;
  char* nickname;
};


static struct client parse_args(int, char**);


int main(int argc, char *argv[]) {
  struct client me = parse_args(argc, argv);

  int sockfd, n;
  struct sockaddr_in serv_addr;

  char buffer[256];

  /* Create a socket point */
  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if (sockfd < 0) {
    perror("ERROR opening socket");
    exit(-errno);
  }

  bzero((char *)&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy(me.server->h_addr, (char *)&serv_addr.sin_addr.s_addr,
        (size_t)me.server->h_length);
  serv_addr.sin_port = htons(me.port_nr);

  /* Now connect to the server */
  if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("ERROR connecting");
    exit(-errno);
  }

  /* Now ask for a message from the user, this message
   * will be read by server
   */

  printf("Please enter the message: ");
  bzero(buffer, 256);
  if (fgets(buffer, 255, stdin) == NULL) {
    perror("ERROR reading from stdin");
    exit(1);
  }

  /* Send message to the server */
  n = write(sockfd, buffer, strlen(buffer));

  if (n < 0) {
    perror("ERROR writing to socket");
    exit(1);
  }

  /* Now read server response */
  bzero(buffer, 256);
  n = read(sockfd, buffer, 255);

  if (n < 0) {
    perror("ERROR reading from socket");
    exit(1);
  }

  printf("%s\n", buffer);
  return 0;
}


static struct client parse_args(int argc, char** argv) {
  if (argc < 4) {
    fprintf(stderr, "usage %s [hostname] [port] [nickname]\n", argv[0]);
    exit(EINVAL);
  }

  struct client me = {0, 0, 0};
  me.server = gethostbyname(argv[1]);

  if (me.server == NULL) {
    herror("ERROR");
    exit(h_errno);
  }

  me.port_nr = (uint16_t) strtoul(argv[2], NULL, 10);
  me.nickname = argv[3];

  return me;
}
