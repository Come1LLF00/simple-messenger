#include <stdio.h>
#include <stdlib.h>

#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <getopt.h>

#include <errno.h>

#include <string.h>

#include <time.h>
#include <inttypes.h>


#include "proto/server.h"


static uint16_t parse_args(int, char**);


int main(int argc, char *argv[]) {
  uint16_t port_nr = parse_args(argc, argv);


  /* First call to socket() function */
  int masterfd = socket(AF_INET, SOCK_STREAM, 0);

  if (masterfd < 0) {
    perror("ERROR opening socket");
    exit(1);
  }

  /* Initialize socket structure */
  // bzero((char *)&serv_addr, sizeof(serv_addr));
  struct sockaddr_in serv_addr;
  memset(&serv_addr, 0, sizeof(serv_addr));
  port_nr = 5001;

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(port_nr);

  /* Now bind the host address using bind() call.*/
  if (bind(masterfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("ERROR on binding");
    exit(1);
  }

  /* Now start listening for the clients, here process will
   * go in sleep mode and will wait for the incoming connection
   */
  listen(masterfd, 5);
  struct sockaddr_in cli_addr;
  unsigned int clilen = sizeof(cli_addr);

  /* Accept actual connection from the client */
  int slavefd = accept(masterfd, (struct sockaddr *)&cli_addr, &clilen);

  if (slavefd < 0) {
    perror("ERROR on accept");
    exit(1);
  }

  /* If connection is established then start communicating */
  // bzero(buffer, 256);
  char buffer[256];
  memset(buffer, 0, 256);
  ssize_t n = recv(slavefd, buffer, 255, MSG_NOSIGNAL);

  if (n == 0 && errno != EAGAIN) {
    fprintf(stderr, "Connection lost\n");
    exit(1);
  }

  if (n < 0) {
    perror("ERROR reading from socket");
    exit(1);
  }

  struct client_msg message = client_msg_deserialize(buffer);
  time_t now = time(NULL);
  struct tm* local_now = localtime(&now);

  char now_line[256];
  strftime(now_line, 255, "%H:%M", local_now);
  const uint32_t now_len = (uint32_t) strlen(now_line);
  printf("sizes: %" PRIu32 "; %" PRIu32 "; %" PRIu32"\n", now_len, message.nickname_size, message.body_size);
  printf("{%.*s} [%.*s]: %.*s", (int) now_len, now_line, (int) message.nickname_size, message.nickname, (int) message.body_size, message.body);

  /* Write a response to the client */
  char* body = (char*) "I got your message";
  struct server_msg response = {
    message.nickname_size,
    message.nickname,
    (uint32_t) strlen(body),
    body,
    now_len,
    now_line
  };

  
  char* payload = NULL;
  size_t length = server_msg_serialize(response, &payload);
  n = send(slavefd, payload, length, MSG_NOSIGNAL);

  free(message.body);
  free(message.nickname);
  
  if (n < 0) {
    perror("ERROR writing to socket");
    exit(1);
  }

  shutdown(slavefd, SHUT_RDWR);

  close(slavefd);

  close(masterfd);
  return 0;
}


static uint16_t parse_args(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "usage %s [port]\n", argv[0]);
    exit(EINVAL);
  }

  return (uint16_t) strtoul(argv[1], NULL, 10);
}
