#include <stdio.h>
#include <stdlib.h>

#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>

#include <string.h>

#include <errno.h>

#include <inttypes.h>

#include <signal.h>  // handle interruption

#include <pthread.h>

#include "proto/client.h"

struct client {
  int sockfd;
  struct hostent* server;
  uint16_t port_nr;
  uint32_t nickname_size;
  char* nickname;
};

static void client_stop(int signum) {
  (void)signum;
  exit(0);
}

static int should_input_stop = false;

static struct client parse_args(int, char**);

static void* send_msg_routine(void*);

int main(int argc, char* argv[]) {
  signal(SIGINT, client_stop);

  struct client me = parse_args(argc, argv);

  /* Create a socket point */
  me.sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if (me.sockfd < 0) {
    perror("ERROR opening socket");
    exit(errno);
  }

  struct sockaddr_in serv_addr;
  memset(&serv_addr, 0,
         sizeof(serv_addr));  // bzero((char *)&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(me.port_nr);

  // bcopy(me.server->h_addr, (char *)&serv_addr.sin_addr.s_addr,
  //       (size_t)me.server->h_length);
  memcpy(&serv_addr.sin_addr.s_addr, me.server->h_addr,
         (size_t)me.server->h_length);

  /* Now connect to the server */
  if (connect(me.sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("ERROR connecting");
    exit(errno);
  }

  pthread_t client_input;
  pthread_create(&client_input, NULL, send_msg_routine, &me);

  int return_code = 0;
  while (1) {
    /* Now read server response */
    char buffer[256];
    memset(buffer, 0, 256);  // bzero(buffer, 256);
    ssize_t n = recv(me.sockfd, buffer, 255, MSG_NOSIGNAL);

    if (n == 0 && errno != EAGAIN) {
      fprintf(stderr, "Connection lost\n");
      should_input_stop = true;
      return_code = 1;
      break;
    }

    if (n < 0) {
      perror("ERROR reading from socket");
      should_input_stop = true;
      return_code = errno;
      break;
    }

    struct server_msg response = server_msg_deserialize(buffer);
#if 0
    printf("sizes: %" PRIu32 "; %" PRIu32 "; %" PRIu32 "\n", response.date_size, response.nickname_size, response.body_size);
#endif
    printf("{%.*s} [%.*s]: %.*s", (int)response.date_size, response.date,
           (int)response.nickname_size, response.nickname,
           (int)response.body_size, response.body);

    free(response.body);
    free(response.date);
    free(response.nickname);
  }

  pthread_join(client_input, NULL);

  shutdown(me.sockfd, SHUT_RDWR);
  close(me.sockfd);

  return return_code;
}

static struct client parse_args(int argc, char** argv) {
  if (argc < 4) {
    fprintf(stderr, "usage %s [hostname] [port] [nickname]\n", argv[0]);
    exit(EINVAL);
  }

  struct client me = {0, 0, 0, 0, 0};
  me.server = gethostbyname(argv[1]);

  if (me.server == NULL) {
    herror("ERROR");
    exit(h_errno);
  }

  me.port_nr = (uint16_t)strtoul(argv[2], NULL, 10);
  me.nickname = argv[3];
  me.nickname_size = strlen(argv[3]);
  return me;
}

static void* send_msg_routine(void* arg) {
  struct client* me_p = (struct client*)arg;
  while (!should_input_stop) {
    char key = getchar();
    char newline = getchar();

    if (key == 'm' && newline == '\n') {
      /* Now ask for a message from the user, this message
       * will be read by server
       */
      char* message = NULL;
      size_t allocated_length = 0;
      printf("> ");
      if (getline(&message, &allocated_length, stdin) == -1) {
        free(message);
        perror("ERROR reading from stdin");
        return (void*)(uintptr_t)errno;
      }

      /* Send message to the server */
      size_t msg_length = strlen(message);
      char* payload = NULL;
      size_t payload_length = client_msg_serialize(
          {me_p->nickname_size, me_p->nickname, (uint32_t)msg_length, message},
          0, &payload);
#if 0
      printf("sent sizes: %" PRIu32 "; %zu\n", me_p->nickname_size, msg_length);
      printf("[%.*s]: %.*s",
        (int) me_p->nickname_size, me_p->nickname,
        (int) msg_length, message);
#endif

      ssize_t n = send(me_p->sockfd, payload, payload_length, MSG_NOSIGNAL);
      free(payload);
      free(message);

      if (n < 0) {
        perror("ERROR writing to socket");
        return (void*)(uintptr_t)1;
      }
    }
  }

  return nullptr;
}
