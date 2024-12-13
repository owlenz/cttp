#include "../include/balls.h"
#include <ini.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SUCCESS 0
#define FAILURE 1

config_t *default_config() {
  config_t *config = malloc(sizeof(config_t));
  if (config == NULL) {
    perror("Error allocating memory");
    return NULL;
  }
  config->port = DEFAULT_PORT;
  strcpy(config->ip_addr, DEFAULT_IP);
  return config;
}

// returns 1 on failure and 0 on success
int parse_ip(const char *ip_addr) {
  if (strlen(ip_addr) >= IP_ADDR_LEN) {
    /*fprintf(stderr, "ip address too long :%s\n", ip_addr);*/
    return FAILURE;
  }

  char ip_copy[IP_ADDR_LEN];
  strncpy(ip_copy, ip_addr, sizeof(ip_copy) - 1);
  ip_copy[sizeof(ip_copy) - 1] = '\0';

  char *token = strtok(ip_copy, ".");
  int i = 0;
  while (token != NULL) {
    if (i >= 4) {
      /*fprintf(stderr, "invalid ip address:%s\n", ip_addr);*/
      return FAILURE;
    }
    int part = atoi(token);
    if (part < 0 || part > 255) {
      /*fprintf(stderr, "invalid ip address:%s\n", ip_addr);*/
      return FAILURE;
    }
    i++;
    token = strtok(NULL, ".");
  }
  if (i != 4) {
    /*fprintf(stderr, "Incomplete IP address: %s\n", ip_addr);*/
    return FAILURE;
  }
  return SUCCESS;
}

int parse_port(const char *value) {
  int port = atoi(value);
  if (port <= 0 || port > 65535) {
    return FAILURE;
  }
  return SUCCESS;
}

int handler(void *user, const char *section, const char *name,
            const char *value) {
  config_t *config = (config_t *)user;
  if (strcmp(name, "port") == 0) {
    if (parse_port(value)) {
      return FAILURE;
    }
    config->port = atoi(value);
  }
  if (strcmp(name, "ip") == 0) {
    int error = parse_ip(value);
    if (error) {
      return FAILURE;
    }
    strncpy(config->ip_addr, value, IP_ADDR_LEN - 1);
    config->ip_addr[IP_ADDR_LEN - 1] = '\0';
  }
  return SUCCESS;
}

config_t *read_file() {
  config_t *config = default_config();
  if (ini_parse("config.ini", handler, config) < 0) {
    printf("Can't load 'config.ini'\n");
    free(config);
    return NULL;
  }
  return config;
}

config_t *read_args(int argc, char **argv) {
  config_t *config = default_config();
  int opt;
  int x = 0;
  while ((opt = getopt(argc, argv, "i:p:")) != -1) {
    x = 1;
    switch (opt) {
    case 'i':
      if (parse_ip(optarg)) {
        fprintf(stderr, "Invalid ip address: %s, using default (%s)\n", optarg,
                DEFAULT_IP);
        break;
      }
      strcpy(config->ip_addr, optarg);
      printf("Option -a with value: %s\n", optarg);
      break;
    case 'p':
      if (parse_port(optarg)) {
        fprintf(stderr, "Invalid port value: %s, using default (%d)\n", optarg,
                DEFAULT_PORT);
        break;
      }
      config->port = atoi(optarg);
      printf("Option -b with value: %s\n", optarg);
      break;
    default:
      fprintf(stderr, "Usage: %s [-i ip_address] [-p port]\n", argv[0]);
      free(config);
      return NULL;
    }
  }
  if (x == 0) {
    free(config);
    return NULL;
  }
  return config;
}
