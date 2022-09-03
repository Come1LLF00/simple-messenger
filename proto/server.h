#ifndef _SERVER_H_
#define _SERVER_H_

#include "common.h"


static size_t server_msg_serialize(struct server_msg msg, char** datpp) {
  struct client_msg temp = {
    msg.nickname_size,
    msg.nickname,
    msg.body_size,
    msg.body
  };

  size_t date_length = sizeof(msg.date_size) + msg.date_size;
  size_t total_length = client_msg_serialize(temp, date_length, datpp);
  char* datp = shift_to_date(msg, *datpp);

  uint32_t net_date_size = htonl(msg.date_size);
  memcpy(datp, &net_date_size, sizeof(net_date_size));
  datp += sizeof(msg.date_size);
  
  memcpy(datp, msg.date, msg.date_size);
  return total_length;
}

#endif /* _SERVER_H_ */
