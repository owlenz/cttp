#ifndef _SERVER_H
#define _SERVER_H

#define BUF_SIZE 4096 * 10
#define IP_ADDR_LEN 16

#define DEFAULT_PORT 6969
#define DEFAULT_IP "192.168.1.107"

typedef int err;

typedef enum { PATH = 1, STRING = 2 } MessageType;
struct message {
  MessageType type;
  char content[BUF_SIZE];
};

typedef struct {
  char ip_addr[IP_ADDR_LEN];
  int port;
} config_t;

config_t *default_config();
config_t *read_file();
config_t *read_args(int, char **);
#endif
