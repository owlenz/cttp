#include "balls.h"
#include <arpa/inet.h>
#include <bits/types/struct_timeval.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#define MAX_SECS 10

int read_file(int client_fd, char *file_name) {
  sleep(1);
  struct message msg = {.type = PATH};
  strcpy(msg.content, file_name);

  if (send(client_fd, &msg, sizeof(msg), MSG_CONFIRM) == -1) {
    perror("error file name");
    return -1;
  } else {
    printf("Client: file name sent\n");
  }

  int n;
  while ((n = recv(client_fd, &msg, sizeof(msg), 0)) > 0) {
    printf("CLIENT: content recieved:\n%s\n", (char *)msg.content);
  }
  if (n < 0) {
    perror("recv error");
    return -1;
  } else if (n == 0) {
    printf("Server closed the connection.\n");
  }
  return 0;
}

int main(int argc, char **argv) {
  int client_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (client_fd == -1) {
    perror("cannot open client socket");
    return 1;
  }
  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(6969);
  if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) != 1) {
    perror("error converting ip str to byte");
    return 1;
  }

  // Establishing Connection
  for (int i = 0; i < MAX_SECS; i++) {
    if (connect(client_fd, (const struct sockaddr *)&server_addr,
                sizeof(server_addr)) == -1) {
      perror("error connecting with server");
      sleep(1);
    } else {
      printf("Client: connection established\n");
      break;
    }
    return 1;
  }
  // receiving time of day
  struct timeval tv;
  gettimeofday(&tv, NULL);
  char buf[BUF_SIZE];
  snprintf(buf, BUF_SIZE, "gettimeofday: tv_sec:%ld , tv_usec:%ld", tv.tv_sec,
           tv.tv_usec);
  struct message *msg = malloc(sizeof(struct message));
  msg->type = STRING;
  strcpy(msg->content, buf);

  if (send(client_fd, msg, sizeof(struct message), MSG_CONFIRM) == -1) {
    perror("error sending time of day");
    return 1;
  } else {
    printf("Client: time of day sent\n");
  }

  if (argc > 1)
    read_file(client_fd, argv[1]);
}
