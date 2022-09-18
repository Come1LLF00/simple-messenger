#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include <errno.h>

#include <string.h>

#include <inttypes.h>

#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/tcp.h>


int main(int argc, char** argv) {
  (void) argc; (void) argv;
  int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);

  if (fd < 0) {
    perror("ERROR opening socket");
    exit(errno);
  }

  struct sockaddr_in serv_addr;
  memset(&serv_addr, 0, sizeof(serv_addr));  // bzero((char *)&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(1234);
  serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

  if (connect(fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("ERROR connecting");
    exit(errno);
  }


  printf("%d: connected\n", getpid());
  while (1) { sleep(10); }

  close(fd);
}
