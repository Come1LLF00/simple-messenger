#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdint.h>
#include <stdio.h>

#include <inttypes.h>

#if 0
#define bytes_to_uint32_t(bytes)                                            \
  {                                                                         \
    (uint32_t)((bytes)[0]), (uint32_t)((bytes)[1]), (uint32_t)((bytes)[2]), \
        (uint32_t)((bytes)[3])                                              \
  }
#endif

#define shift_to_date(msg, data)                                 \
  (data) + sizeof((msg).nickname_size) + ((msg).nickname_size) + \
      sizeof((msg).body_size) + ((msg).body_size)

struct client_msg {
  uint32_t nickname_size;
  char* nickname;
  uint32_t body_size;
  char* body;
};

struct server_msg {
  uint32_t nickname_size;
  char* nickname;
  uint32_t body_size;
  char* body;
  uint32_t date_size;
  char* date;
};

static ssize_t write_field(int fd, void* field, size_t size) {
  size_t count = 0;
  while (size - count > 0) {
    ssize_t result = send(fd, (char*)field + count, size - count, MSG_NOSIGNAL);
    if (result <= 0) return -1;

    count += result;
  }
  return count;
}

static ssize_t client_msg_serialize(int fd, struct client_msg msg) {
  ssize_t total_bytes = 0;
  uint32_t net_nick_size = htonl(msg.nickname_size);
  ssize_t result = write_field(fd, &net_nick_size, sizeof(uint32_t));
  if (result != sizeof(uint32_t)) return -1;

  total_bytes += result;

  result = write_field(fd, msg.nickname, msg.nickname_size);
  if (result != msg.nickname_size) return -1;

  total_bytes += result;

  uint32_t net_body_size = htonl(msg.body_size);
  result = write_field(fd, &net_body_size, sizeof(u_int32_t));
  if (result != sizeof(uint32_t)) return -1;

  total_bytes += result;

  result = write_field(fd, msg.body, msg.body_size);
  if (result != msg.body_size) return -1;

  total_bytes += result;
  return total_bytes;
}

static ssize_t read_field(int fd, void* field, size_t size) {
  ssize_t count = recv(fd, field, size, MSG_NOSIGNAL | MSG_WAITALL);
  if (count == 0 && errno != EAGAIN)
    return -1;
  else
    return count;
}

static struct client_msg client_msg_deserialize(int fd) {
  ssize_t count = 0;
  uint32_t nickname_size = 0;
  count = read_field(fd, &nickname_size, sizeof(uint32_t));
  if (count != sizeof(uint32_t)) return {0, 0, 0, 0};
  nickname_size = ntohl(nickname_size);

#if 0
  printf("nickname length: %x\n", nickname_size);
#endif

  char* nickname = (char*)malloc(nickname_size);
  count = read_field(fd, nickname, nickname_size);
  if (count != nickname_size) {
    free(nickname);
    return {0, 0, 0, 0};
  }
#if 0
  printf("Got message from: %.*s\n", nickname_size, datp);
#endif

  uint32_t body_size = 0;
  count = read_field(fd, &body_size, sizeof(uint32_t));
  if (count != sizeof(uint32_t)) {
    free(nickname);
    return {0, 0, 0, 0};
  }
  body_size = ntohl(body_size);

#if 0
  printf("deserialized sizes: %" PRIu32 "; %" PRIu32 "\n", nickname_size, body_size);
#endif

  char* body = (char*)malloc(body_size);
  count = read_field(fd, body, body_size);
  if (count != body_size) {
    free(body);
    free(nickname);
    return {0, 0, 0, 0};
  }
  // memcpy(body, datp, body_size); // there is problem

  return {nickname_size, nickname, body_size, body};
}

#endif /* _COMMON_H_ */
