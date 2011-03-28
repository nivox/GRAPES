#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/file.h>
#include <string.h>
#include <stdint.h>
#include <semaphore.h>
#include <pthread.h>
#include <time.h>

#include "net_helper.h"

#define CLOUD_KEY_MAX_SIZE 255
#define CLOUD_VALUE_MAX_SIZE 2000

struct delegate_iface {
  void* (*cloud_helper_init)(struct nodeID *local, const char *config);
  int (*get_from_cloud)(void *context, char *key, uint8_t *header_ptr, int header_size);
  int (*put_on_cloud)(void *context, char *key, uint8_t *buffer_ptr, int buffer_size);
  struct nodeID* (*get_cloud_node)(void *context, uint8_t variant);
  time_t (*timestamp_cloud)(void *context);
  int (*is_cloud_node)(void *context, struct nodeID* node);
  int (*wait4cloud)(void *context, struct timeval *tout);
  int (*recv_from_cloud)(void *context, uint8_t *buffer_ptr, int buffer_size);
};

struct mem_offset {
  int start;
  int end;
};

struct file_cloud_context {
  const char *path;
  sem_t sem;
  time_t last_timestamp;
  uint8_t *out_buffer[10];
  struct mem_offset out_len[10];
  int out_cnt;
  int key_error;
  struct nodeID* cloud_node_base;
};


struct gpThreadContext{
  struct file_cloud_context *cloud;
  char *key;
  uint8_t *value;
  int value_len;
  uint8_t *header_ptr;
  int header_size;
};

struct entry {
  char key[CLOUD_KEY_MAX_SIZE];
  uint8_t value[CLOUD_VALUE_MAX_SIZE];
  int value_len;
  time_t timestamp;
};


void* getValueForKey(void *context)
{
  struct entry e;
  struct gpThreadContext *ctx;
  FILE *fd;
  uint8_t *buffer_ptr;
  struct mem_offset *len;
  ctx = (struct gpThreadContext *) context;

  fd = fopen(ctx->cloud->path, "r");
  if (!fd) pthread_exit(NULL);
  flock(fileno(fd), LOCK_SH);

  sem_wait(&ctx->cloud->sem);
  ctx->cloud->key_error = -1;
  sem_post(&ctx->cloud->sem);

  ctx->cloud->out_cnt++;
  ctx->cloud->out_buffer[ctx->cloud->out_cnt] = calloc(CLOUD_VALUE_MAX_SIZE, sizeof(uint8_t));
  len = malloc(sizeof(struct mem_offset));
  len->start = 0;
  len->end = ctx->header_size;

  buffer_ptr = ctx->cloud->out_buffer[ctx->cloud->out_cnt];
  memcpy(buffer_ptr, ctx->header_ptr, ctx->header_size);

  buffer_ptr += ctx->header_size;

  sem_wait(&ctx->cloud->sem);

  while (fread(&e, sizeof(e), 1, fd) != 0){
    if (strcmp(e.key, ctx->key) == 0){
      memcpy(buffer_ptr, e.value, e.value_len);
      len->end += e.value_len;
      ctx->cloud->last_timestamp = e.timestamp;
      ctx->cloud->out_len[ctx->cloud->out_cnt] = *len;
      ctx->cloud->key_error = 0;
      break;
    }
  }

  if (ctx->cloud->key_error == -1){
    ctx->cloud->key_error = 1;
  }
  sem_post(&ctx->cloud->sem);

  flock(fileno(fd), LOCK_UN);
  fclose(fd);

  if (ctx->key) free(ctx->key);
  if (ctx->value) free(ctx->value);
  free(ctx);

  pthread_exit(NULL);
}

int putValueForKey(struct gpThreadContext *ctx)
{
  struct entry e;
  FILE *fd;
  int found = 0;

  fd = fopen(ctx->cloud->path, "r+");
  if (!fd) fd = fopen(ctx->cloud->path, "w+");
  flock(fileno(fd), LOCK_EX);
  while (fread(&e, sizeof(e), 1, fd) != 0){
    if (strcmp(e.key, ctx->key) == 0){
      memcpy(e.value, ctx->value, ctx->value_len);
      e.value_len = ctx->value_len;
      time(&e.timestamp);
      fseek(fd, -sizeof(e), SEEK_CUR);
      fwrite(&e, sizeof(e), 1, fd);
      found = 1;
      break;
    }
  }
  if (!found){
    strcpy(e.key, ctx->key);
    memcpy(e.value, ctx->value, ctx->value_len);
    e.value_len = ctx->value_len;
    time(&e.timestamp);
    fwrite(&e, sizeof(e), 1, fd);
  }
  fflush(fd);
  flock(fileno(fd), LOCK_UN);
  fclose(fd);

  if (ctx->key) free(ctx->key);
  if (ctx->value) free(ctx->value);
  free(ctx);

  return 0;
}



static void* file_cloud_helper_init(struct nodeID *local, const char *config)
{
  struct file_cloud_context *ctx;
  char *conf = strdup(config);
  char *opt;
  char *key;
  const char *path = NULL;


  while((opt = strsep(&conf, ",")) != NULL) {
    key = strsep(&opt, "=");
    if (!key) continue;
    if (strcmp(key, "file_cloud_path") == 0){
      if (opt) path = opt;
    }
  }

  if (!path) path = "cloud_dump";

  ctx = malloc(sizeof(struct file_cloud_context));

  sem_init(&ctx->sem, 0, 1);
  ctx->path = path;
  ctx->out_cnt = -1;
  ctx->cloud_node_base = create_node("0.0.0.0", 0);
  return ctx;
}

static int file_cloud_get_from_cloud(void *context, char *key, uint8_t *header_ptr, int header_size)
{
  struct file_cloud_context *ctx;
  int err;
  struct gpThreadContext *tc;
  pthread_t *thread = malloc(sizeof(pthread_t));


  ctx = (struct file_cloud_context *)context;
  tc = malloc(sizeof(struct gpThreadContext));
  tc->cloud = ctx;
  tc->key = strdup(key);
  tc->value = NULL;
  tc->header_ptr = header_ptr;
  tc->header_size = header_size;

  err = pthread_create(thread, NULL, getValueForKey, (void *)tc);
  if (err) return 1;
  return 0;
}

static int file_cloud_put_on_cloud(void *context, char *key, uint8_t *buffer_ptr, int buffer_size)
{
  struct file_cloud_context *ctx;
  struct gpThreadContext *tc;

  ctx = (struct file_cloud_context *)context;
  tc = malloc(sizeof(struct gpThreadContext));
  tc->cloud = ctx;
  tc->key = strdup(key);
  tc->value = calloc(buffer_size, sizeof(uint8_t));
  memcpy(tc->value, buffer_ptr, buffer_size);
  tc->value_len = buffer_size;

  return putValueForKey(tc);
}

struct nodeID* file_cloud_get_cloud_node(void *context, uint8_t variant)
{
  struct file_cloud_context *ctx;
  ctx = (struct file_cloud_context *)context;
  return create_node("0.0.0.0", variant);
}

time_t file_cloud_timestamp_cloud(void *context)
{
  struct file_cloud_context *ctx;
  ctx = (struct file_cloud_context *)context;

  return ctx->last_timestamp;
}

int file_cloud_is_cloud_node(void *context, struct nodeID* node)
{
  struct file_cloud_context *ctx;
  struct nodeID *candidate_node;
  int result = -1;
  ctx = (struct file_cloud_context *)context;
  candidate_node = create_node(node_ip(node), 0);

  result = nodeid_equal(ctx->cloud_node_base, candidate_node);
  nodeid_free(candidate_node);
  return result;
}

static int file_cloud_wait4cloud(void *context, struct timeval *tout)
{
  struct file_cloud_context *ctx;
  long timeout = tout->tv_sec * 1000 + tout->tv_usec;

  ctx = (struct file_cloud_context *)context;
  if (ctx->key_error == 1) return -1;
  if (ctx->out_cnt > 0) return 1;
  if (ctx->out_cnt == 0 && ctx->key_error >= 0) return 1;

  while (timeout > 0){
    usleep(100);
    timeout -= 100;
    if (ctx->key_error == 1) return -1;
    if (ctx->out_cnt > 0) return 1;
    if (ctx->out_cnt == 0 && ctx->key_error >= 0) return 1;
  }
  return 0;
}

static int file_cloud_recv_from_cloud(void *context, uint8_t *buffer_ptr, int buffer_size)
{
  struct file_cloud_context *ctx;
  uint8_t *source_ptr;
  int start, end, len;
  ctx = (struct file_cloud_context *)context;

  if (ctx->out_cnt < 0) return 0;

  sem_wait(&ctx->sem);

  source_ptr = ctx->out_buffer[ctx->out_cnt];
  start = ctx->out_len[ctx->out_cnt].start;
  end = ctx->out_len[ctx->out_cnt].end;
  len = end - start;
  source_ptr = source_ptr + start;

  if (buffer_size < len){
    memcpy(buffer_ptr,  source_ptr, buffer_size);
    start += buffer_size;
    ctx->out_len[ctx->out_cnt].start = start;
    sem_post(&ctx->sem);
    return buffer_size;
  } else {
    memcpy(buffer_ptr,  source_ptr, len);
    free(ctx->out_buffer[ctx->out_cnt]);
    ctx->out_cnt--;
    sem_post(&ctx->sem);
    return len;
  }
}

struct delegate_iface delegate_impl = {
  .cloud_helper_init = &file_cloud_helper_init,
  .get_from_cloud = &file_cloud_get_from_cloud,
  .put_on_cloud = &file_cloud_put_on_cloud,
  .get_cloud_node = &file_cloud_get_cloud_node,
  .timestamp_cloud = &file_cloud_timestamp_cloud,
  .is_cloud_node = &file_cloud_is_cloud_node,
  .wait4cloud = file_cloud_wait4cloud,
  .recv_from_cloud = file_cloud_recv_from_cloud
};