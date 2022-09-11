#ifndef _SERVER_H_
#define _SERVER_H_

#include "common.h"

static ssize_t server_msg_serialize(int fd, struct server_msg msg) {
  struct client_msg temp = {msg.nickname_size, msg.nickname, msg.body_size,
                            msg.body};
  ssize_t total_bytes = 0;
  ssize_t result = client_msg_serialize(fd, temp);
  if (result == -1) return -1;

  total_bytes += result;

  uint32_t net_date_size = htonl(msg.date_size);
  result = write_field(fd, &net_date_size, sizeof(uint32_t));
  if (result != sizeof(uint32_t)) return -1;

  total_bytes += result;

  result = write_field(fd, msg.date, msg.date_size);
  if (result != msg.date_size) return -1;

  total_bytes += result;
  return total_bytes;
}

#endif /* _SERVER_H_ */
