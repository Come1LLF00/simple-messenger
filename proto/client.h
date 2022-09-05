#ifndef _CLIENT_H_
#define _CLIENT_H_

#include <stdlib.h>
#include <string.h>

#include "common.h"


static struct server_msg server_msg_deserialize(char* data) {
  struct client_msg msg = client_msg_deserialize(data);

  char* datp = shift_to_date(msg, data);
  uint32_t date_size = uint32_t_deserialize(datp);
  datp += sizeof(date_size);

  char* date = (char*) malloc(date_size);
  memcpy(date, datp, date_size);

  return {msg.nickname_size, msg.nickname, msg.body_size, msg.body, date_size, date};
}

#endif /* _CLIENT_H_ */
