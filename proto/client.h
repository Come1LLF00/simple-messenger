#ifndef _CLIENT_H_
#define _CLIENT_H_

#include <stdlib.h>
#include <string.h>

#include "common.h"

static struct server_msg server_msg_deserialize(int fd) {
  struct client_msg msg = client_msg_deserialize(fd);
  if (msg.nickname_size == 0 && msg.nickname == NULL && msg.body_size == 0 &&
      msg.body == NULL)
    return {0, 0, 0, 0, 0, 0};

  int count = 0;
  uint32_t date_size = 0;
  count = read_field(fd, &date_size, sizeof(uint32_t));
  if (count != sizeof(uint32_t)) {
    free(msg.body);
    free(msg.nickname);
    return {0, 0, 0, 0, 0, 0};
  }
  date_size = ntohl(date_size);

  char* date = (char*)malloc(date_size);
  count = read_field(fd, date, date_size);
  if ((uint32_t)count != date_size) {
    free(msg.body);
    free(msg.nickname);
    free(date);
    return {0, 0, 0, 0, 0, 0};
  }

#if 0
  printf("deserialized date: %" PRIu32 "; %s\n", date_size, date);
#endif

  return {msg.nickname_size, msg.nickname, msg.body_size,
          msg.body,          date_size,    date};
}

#endif /* _CLIENT_H_ */
