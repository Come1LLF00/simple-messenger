#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <unistd.h>

#include <errno.h>

#include <string.h>

#include <inttypes.h>

#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/tcp.h>


int main(int argc, char** argv) {
  (void) argc; (void) argv;
  int server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);

  struct sockaddr_in serv_addr;
  memset(&serv_addr, 0,
         sizeof(serv_addr));  // bzero((char *)&serv_addr, sizeof(serv_addr));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(1234);

  if (bind(server_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("ERROR on binding");
    exit(errno);
  }

  listen(server_fd, 0);

  while (1) { sleep(10); }
  close(server_fd);

}
