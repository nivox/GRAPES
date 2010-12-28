#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "chunk.h"
#include "config.h"
#include "chunkiser.h"
#include "dechunkiser_iface.h"

extern struct dechunkiser_iface out_avf;
extern struct dechunkiser_iface out_raw;

struct output_stream {
  struct dechunkiser_ctx *c;
  struct dechunkiser_iface *out;
};

struct output_stream *out_stream_init(const char *fname, const char *config)
{
  struct tag *cfg_tags;
  struct output_stream *res;

  res = malloc(sizeof(struct output_stream));
  if (res == NULL) {
    return NULL;
  }

#ifdef AVF
  res->out = &out_avf;
#else
  res->out = &out_raw;
#endif
  cfg_tags = config_parse(config);
  if (cfg_tags) {
    const char *type;

    type = config_value_str(cfg_tags, "dechunkiser");
    if (type && !strcmp(type, "raw")) {
      res->out = &out_raw;
    }
  }
  free(cfg_tags);

  res->c = res->out->open(fname, config);
  if (res->c == NULL) {
    free(res);

    return NULL;
  }

  return res;
}

void out_stream_close(struct output_stream *s)
{
  s->out->close(s->c);
  free(s);
}

void chunk_write(struct output_stream *o, const struct chunk *c)
{
  o->out->write(o->c, c->id, c->data, c->size);
}
