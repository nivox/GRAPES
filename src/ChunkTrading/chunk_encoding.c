/*
 *  Copyright (c) 2009 Luca Abeni.
 *  Copyright (c) 2009 Alessandro Russo.
 *
 *  This is free software;
 *  see lgpl-2.1.txt
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "chunk.h"
#include "trade_msg_la.h"
#include "int_coding.h"

int encodeChunk(const struct chunk *c, uint8_t *buff, int buff_len)
{
  uint32_t half_ts;

  if (buff_len < 20 + c->size + c->attributes_size) {
    /* Not enough space... */
    return -1;
  }

  int_cpy(buff, c->id);
  half_ts = c->timestamp >> 32;
  int_cpy(buff + 4, half_ts);
  half_ts = c->timestamp;
  int_cpy(buff + 8, half_ts);
  int_cpy(buff + 12, c->size);
  int_cpy(buff + 16, c->attributes_size);
  memcpy(buff + 20, c->data, c->size);
  if (c->attributes_size) {
    memcpy(buff + 20 + c->size, c->attributes, c->attributes_size);
  }

  return 20 + c->size + c->attributes_size;
}

int decodeChunk(struct chunk *c, const uint8_t *buff, int buff_len)
{
  if (buff_len < 20) {
    return -1;
  }
  c->id = int_rcpy(buff);
  c->timestamp = int_rcpy(buff + 4);
  c->timestamp = c->timestamp << 32;
  c->timestamp |= int_rcpy(buff + 8); 
  c->size = int_rcpy(buff + 12);
  c->attributes_size = int_rcpy(buff + 16);

  if (buff_len < c->size + 20) {
    return -2;
  }
  c->data = malloc(c->size);
  if (c->data == NULL) {
    return -3;
  }
  memcpy(c->data, buff + 20, c->size);

  if (c->attributes_size > 0) {
    if (buff_len < c->size + c->attributes_size) {
      return -4;
    }
    c->attributes = malloc(c->attributes_size);
    if (c->attributes == NULL) {
      return -5;
    }
    memcpy(c->attributes, buff + 20 + c->size, c->attributes_size);
  }

  return 20 + c->size + c->attributes_size;
}
