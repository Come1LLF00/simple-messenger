#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdint.h>
#include <stdio.h>

#include <inttypes.h>

#define bytes_to_uint32_t(bytes)                                      \
  {                                                                   \
    (uint32_t)(bytes)[0], (uint32_t)(bytes)[1], (uint32_t)(bytes)[2], \
        (uint32_t)(bytes)[3]                                          \
  }
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

static size_t client_msg_serialize(struct client_msg msg, size_t extra_space,
                                   char** datpp) {
  size_t length = sizeof(msg.nickname_size) + msg.nickname_size +
                  sizeof(msg.body_size) + msg.body_size + extra_space;
  *datpp = (char*)malloc(length);
  char* datp = *datpp;

  uint32_t net_nickname_size = htonl(msg.nickname_size);
  memcpy(datp, &net_nickname_size, sizeof(net_nickname_size));
  datp += sizeof(msg.nickname_size);

  memcpy(datp, msg.nickname, msg.nickname_size);
  datp += msg.nickname_size;

  uint32_t net_body_size = htonl(msg.body_size);
  memcpy(datp, &net_body_size, sizeof(net_body_size));
  datp += sizeof(msg.body_size);

  memcpy(datp, msg.body, msg.body_size);
  return length;
}

static uint32_t uint32_t_deserialize(char* data) {
  uint32_t bytes[4] = bytes_to_uint32_t(data);
  return (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
}

static struct client_msg client_msg_deserialize(char* data) {
  char* datp = data;
  uint32_t nickname_size = uint32_t_deserialize(datp);
  datp += sizeof(nickname_size);

  char* nickname = (char*)malloc(nickname_size);
#if 0
  printf("Got message from: %.*s\n", nickname_size, datp);
#endif

  memcpy(nickname, datp, nickname_size);
  datp += nickname_size;

  uint32_t body_size = uint32_t_deserialize(datp);
  datp += sizeof(body_size);
#if 0
  printf("deserialized sizes: %" PRIu32 "; %" PRIu32 "\n", nickname_size, body_size);
#endif
  char* body = (char*)malloc(body_size);
  memcpy(body, datp, body_size);

  return {nickname_size, nickname, body_size, body};
}

#endif /* _COMMON_H_ */
