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
  size_t length = 0;
  printf("Please enter the message: ");
  if (getline(&message, &length, stdin) == -1) {
    perror("ERROR reading from stdin");
    exit(errno);
  }

  /* Send message to the server */
  int n = write(sockfd, message, strlen(message));

  if (n < 0) {
    perror("ERROR writing to socket");
    exit(1);
  }

  /* Now read server response */
  bzero(message, 256);
  n = read(sockfd, message, 255);

  if (n < 0) {
    perror("ERROR reading from socket");
    exit(1);
  }

  printf("%s\n", message);
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
