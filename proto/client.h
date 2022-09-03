#ifndef _CLIENT_H_
#define _CLIENT_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


#define bytes_to_uint32_t(bytes) {(uint32_t)(bytes)[0], (uint32_t)(bytes)[1], (uint32_t)(bytes)[2], (uint32_t)(bytes)[3]}


struct client_msg {
  uint32_t nickname_size;
  char* nickname;
  uint32_t body_size;
  char* body;
};


static char* client_msg_serialize(struct client_msg msg, size_t extra_space) {
  size_t length = sizeof(msg.nickname_size) + msg.nickname_size + sizeof(msg.body_size) + msg.body_size + extra_space;
  char* data = (char*) malloc(length);
  char* datp = data;
  
  memcpy(datp, &msg.nickname_size, sizeof(msg.nickname_size));
  datp += sizeof(msg.nickname_size);
  memcpy(datp, msg.nickname, msg.nickname_size);
  datp += msg.nickname_size;
  memcpy(datp, &msg.body_size, sizeof(msg.body_size));
  datp += sizeof(msg.body_size);
  memcpy(datp, msg.body, msg.body_size);
  return data;
}


static uint32_t uint32_t_deserialize(char* data) {
  uint32_t bytes[4] = bytes_to_uint32_t(data);
  return (bytes[0] << 24) + (bytes[1] << 16) + (bytes[2] << 8) + bytes[3];
}

static struct client_msg client_msg_deserialize(char* data) {
  char* datp = data;
  uint32_t nickname_size = uint32_t_deserialize(datp);
  datp += sizeof(nickname_size);

  char* nickname = (char*) malloc(nickname_size);
  snprintf(nickname, nickname_size, "%s", datp);
  datp += nickname_size;

  uint32_t body_size = uint32_t_deserialize(datp);
  datp += sizeof(body_size);

  char* body = (char*) malloc(body_size);
  snprintf(body, body_size, "%s", datp);
  
  return {nickname_size, nickname, body_size, body};
}

#endif /* _CLIENT_H_ */
