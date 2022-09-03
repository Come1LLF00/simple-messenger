#ifndef _SERVER_H_
#define _SERVER_H_

#include "client.h"


#define shift_to_date(msg, data) (data) + sizeof((msg).nickname_size) + ((msg).nickname_size) + sizeof((msg).body_size) + ((msg).body_size)


struct server_msg {
  uint32_t nickname_size;
  char* nickname;
  uint32_t body_size;
  char* body;
  uint32_t date_size;
  char* date;
};


static char* server_msg_serialize(struct server_msg msg) {
  struct client_msg temp = {
    msg.nickname_size,
    msg.nickname,
    msg.body_size,
    msg.body
  };

  size_t date_length = sizeof(msg.date_size) + msg.date_size;
  char* data = client_msg_serialize(temp, date_length);
  char* datp = shift_to_date(msg, data);
  memcpy(datp, &msg.date_size, sizeof(msg.date_size));
  datp += sizeof(msg.date_size);
  memcpy(datp, msg.date, msg.date_size);
  return data;
}


static struct server_msg server_msg_deserialize(char* data) {
  struct client_msg msg = client_msg_deserialize(data);

  char* datp = shift_to_date(msg, data);
  uint32_t date_size = uint32_t_deserialize(datp);
  datp += sizeof(date_size);

  char* date = (char*) malloc(date_size);
  snprintf(date, date_size, "%s", datp);

  return {msg.nickname_size, msg.nickname, msg.body_size, msg.body, date_size, date};
}

#endif /* _SERVER_H_ */
