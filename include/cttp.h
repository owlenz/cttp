#ifndef _SERVER_H
#define _SERVER_H

#include <stdint.h>

#define BUF_SIZE 4096 * 10
#define IP_ADDR_LEN 16

#define DEFAULT_PORT 6969
#define DEFAULT_IP "192.168.1.106"

#ifdef DEBUG
#define ASSERT(expr)                                                           \
  if (!(expr)) {                                                               \
    fprintf(stderr, "Assertion failed: %s, file %s, line %d\n", #expr,         \
            __FILE__, __LINE__);                                               \
    abort();                                                                   \
  }

#define DEBUG_PRINT(fmt, ...)                                                  \
  fprintf(stdout, "[DEBUG], %s:%d:%s(): " fmt "\n", __FILE__, __LINE__,          \
          __func__, ##__VA_ARGS__)
#else
#define ASSERT(expr)

#define DEBUG_PRINT(fmt, ...)
#endif

typedef int err;

typedef enum { PATH = 1, STRING = 2 } MessageType;
struct message {
  MessageType type;
  char content[BUF_SIZE];
};

typedef struct response {
  uint32_t buffer_len;
  unsigned char buffer[BUF_SIZE];
  char *content_type;
	int code;
} response_t;

typedef struct {
  char ip_addr[IP_ADDR_LEN];
  int port;
} config_t;

config_t *default_config();
config_t *read_file();
config_t *read_args(int, char **);
#endif
