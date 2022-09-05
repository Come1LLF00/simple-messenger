#include <stdio.h>
#include <stdlib.h>

#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>

#include <string.h>

#include <time.h>
#include <inttypes.h>


#include "proto/server.h"


int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  int sockfd, newsockfd;
  uint16_t portno;
  unsigned int clilen;
  char buffer[256];
  struct sockaddr_in serv_addr, cli_addr;

  /* First call to socket() function */
  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if (sockfd < 0) {
    perror("ERROR opening socket");
    exit(1);
  }

  /* Initialize socket structure */
  // bzero((char *)&serv_addr, sizeof(serv_addr));
  memset(&serv_addr, 0, sizeof(serv_addr));
  portno = 5001;

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  /* Now bind the host address using bind() call.*/
  if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("ERROR on binding");
    exit(1);
  }

  /* Now start listening for the clients, here process will
   * go in sleep mode and will wait for the incoming connection
   */

  listen(sockfd, 5);
  clilen = sizeof(cli_addr);

  /* Accept actual connection from the client */
  newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

  if (newsockfd < 0) {
    perror("ERROR on accept");
    exit(1);
  }

  /* If connection is established then start communicating */
  // bzero(buffer, 256);
  memset(buffer, 0, 256);
  ssize_t n = recv(newsockfd, buffer, 255, MSG_NOSIGNAL);
  printf("n = %zd\n", n);

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
  n = send(newsockfd, payload, length, MSG_NOSIGNAL);

  free(message.body);
  free(message.nickname);
  
  if (n < 0) {
    perror("ERROR writing to socket");
    exit(1);
  }

  shutdown(newsockfd, SHUT_RDWR);

  close(newsockfd);

  close(sockfd);
  return 0;
}
