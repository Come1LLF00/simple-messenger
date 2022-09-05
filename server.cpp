#include <stdio.h>
#include <stdlib.h>

#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <fcntl.h>

#include <errno.h>

#include <string.h>

#include <time.h>
#include <inttypes.h>

#include <pthread.h>

#include <set>

#include "proto/server.h"

#define MAX_PENDING_CONN 5


struct clients_context {
  std::set<int>* client_fds;
  int client;

  clients_context(std::set<int>* fds, int fd): client_fds(fds), client(fd) {}
};


static pthread_mutex_t context_lock;

static bool should_stop = false;

static void server_stop(int signum) {
  (void) signum;
  should_stop = true;
}

static uint16_t parse_args(int, char**);

static void* client_handler(void*);


int main(int argc, char *argv[]) {
  signal(SIGINT, server_stop);
  uint16_t port_nr = parse_args(argc, argv);


  /* First call to socket() function */
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (server_fd < 0) {
    perror("ERROR opening socket");
    exit(errno);
  }

  /* Initialize socket structure */
  struct sockaddr_in serv_addr;
  memset(&serv_addr, 0, sizeof(serv_addr)); // bzero((char *)&serv_addr, sizeof(serv_addr));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(port_nr);

  /* Now bind the host address using bind() call.*/
  if (bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("ERROR on binding");
    exit(errno);
  }

  /* Now start listening for the clients */
  listen(server_fd, MAX_PENDING_CONN);
  fcntl(server_fd, F_SETFL, O_NONBLOCK); 
  pthread_mutex_init(&context_lock, NULL);

  std::set<int> client_fds;
  std::set<pthread_t> clients;
  while (!should_stop) {
  
    /* Accept actual connection from the client */
    struct {struct sockaddr_in address; unsigned int length; int fd;} client;
    client.length = sizeof(client.address);
    client.fd = accept(server_fd, (struct sockaddr *)&(client.address), &(client.length));
    if (client.fd < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
      continue;

    if (client.fd < 0) {
      perror("ERROR on accept");
      break;
    }

    struct clients_context client_context = {&client_fds, client.fd};

    pthread_mutex_lock(&context_lock);
    client_fds.insert(client.fd);
    pthread_mutex_unlock(&context_lock);

    pthread_t thread;
    pthread_create(&thread, NULL, client_handler, &client_context);
    clients.insert(thread);

    // pthread_mutex_lock(&context_lock);
    // should_stop = should_stop || client_fds.size() == 0;
    // pthread_mutex_unlock(&context_lock);
  }

  // wait until all clients shutdown
  for (auto client : clients)
    pthread_join(client, NULL);

  pthread_mutex_destroy(&context_lock);
  close(server_fd);
  return 0;
}


static uint16_t parse_args(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "usage %s [port]\n", argv[0]);
    exit(EINVAL);
  }

  return (uint16_t) strtoul(argv[1], NULL, 10);
}


#define MAX_LENGTH 256

static void* client_handler(void* arg) {
  struct clients_context* context_p = (struct clients_context*) arg;
  struct clients_context context(context_p->client_fds, context_p->client);
  
  struct client_msg received_msg;
  
  time_t time_now; 
  struct tm* local_time_now;
  struct {char date[MAX_LENGTH]; uint32_t date_size;} now;

  struct server_msg response_msg;
  struct {char* text; size_t length;} payload;

  while (1) {
    /* If connection is established then start communicating */
    char buffer[MAX_LENGTH];
    memset(buffer, 0, MAX_LENGTH); // bzero(buffer, 256);
    ssize_t count = recv(context.client, buffer, MAX_LENGTH - 1, MSG_NOSIGNAL);

    // check if connection to client lost
    if (count == 0 && errno != EAGAIN) {
      fprintf(stderr, "[%d] lost connection\n", context.client);
      goto out;
    }

    // check if no error while reading
    if (count < 0) {
      perror("ERROR reading from socket");
      goto out;
    }

    // deserialize incoming message
    received_msg = client_msg_deserialize(buffer);
    
    // save the current time
    time_now = time(NULL);
    local_time_now = localtime(&time_now);

    strftime(now.date, MAX_LENGTH - 1, "%H:%M", local_time_now); // formatting the time buffer according to predefined format %H:%M
    now.date_size = (uint32_t) strlen(now.date); // save the real length of time representation
    
    // printf("sizes: %" PRIu32 "; %" PRIu32 "; %" PRIu32"\n", now_len, received_msg.nickname_size, received_msg.body_size);
    fprintf(stderr, "{%.*s} [%.*s]: %.*s", (int) now.date_size, now.date, (int) received_msg.nickname_size, received_msg.nickname, (int) received_msg.body_size, received_msg.body);

    /* write a response to the clients */

    // response package
    response_msg = {
      received_msg.nickname_size,
      received_msg.nickname,
      received_msg.body_size,
      received_msg.body,
      now.date_size,
      now.date
    };

    
    payload.length = server_msg_serialize(response_msg, &(payload.text));

    for (auto client_fd : *(context.client_fds)) {
      count = send(client_fd, payload.text, payload.length, MSG_NOSIGNAL);
      if (count < 0 && client_fd == context.client) {
        fprintf(stderr, "ERROR writing to socket[%d]", client_fd);
        goto response_out;
      }
    }
  }


response_out:
  // freeing allocated buffers
  free(payload.text);

  free(received_msg.nickname);
  free(received_msg.body);

out:
  // removing lost client socket fd
  pthread_mutex_lock(&context_lock);
  context.client_fds->erase(context.client);
  pthread_mutex_unlock(&context_lock);

  shutdown(context.client, SHUT_RDWR);
  close(context.client);
  return nullptr;
}
