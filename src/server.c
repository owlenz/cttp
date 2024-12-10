#include "../include/balls.h"
#include <aio.h>
#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <bits/types/struct_timeval.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#define MAX_CLIENTS 10

// my dumb ass didnt know that setting this as non-blocking
// will affect recv, makeing it non-blocking too xddMORS
int set_nonblocking(int fd) {
  int erm = fcntl(fd, F_GETFL, 0);
  if (erm == -1)
    return -1;
  return fcntl(fd, F_SETFL, erm | O_NONBLOCK);
}

// formant response
void response(char *response_buf, int code, const char *content_type) {
  ssize_t content_length = strlen(response_buf);
  char response_body[BUF_SIZE];
  strcpy(response_body, response_buf);
  snprintf(response_buf, BUF_SIZE,
           "HTTP/1.1 200 OK\r\n"
           "Content-Length: %zu\r\n"
           "Content-Type: %s\r\n"
           "Server: CTTP\r\n"
           "\r\n"
           "%s",
           content_length, content_type, response_body);
}

char *parse_file_type(const char *file) {
  char *dot = strrchr(file, '.');
  if (dot == NULL) {
    return "text/plain";
  }
  char file_type[dot - file];
  strncpy(file_type, dot + 1, dot - file);
  file_type[dot - file] = '\0';
  printf("file_type: %s\n", file_type);
  if (strcmp(file_type, "html") == 0) {
    return "text/html";
  } else if (strcmp(file_type, "txt") == 0) {
    return "text/plain";
  }
  return "text/bozo";
};

int set_socket_opts(int fd) {
  // For Handling address in use even after killing server
  // (SO_REUSEADDR)
  int opt = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
    perror("setsockopt failed");
    return -1;
  }

  struct linger so_linger;
  so_linger.l_onoff = 1;
  so_linger.l_linger = 30;

  if (setsockopt(fd, SOL_SOCKET, SO_LINGER, &so_linger, sizeof(so_linger)) ==
      -1) {
    perror("SERVER: setsockopt SO_LINGER failed");
    return -1;
  }
  return 0;
}

// Handling GET requests
void handle_get(int client_fd, const char *path) {
  char response_buf[BUF_SIZE] = "Hello, GET!\0";
  char *content_type = "text/plain\0";
  int code = 200;

  // if path specified
  if (strcmp(path, "/") != 0) {
    char path_good[strlen(path)];
    strncpy(path_good, path + 1, strlen(path) - 1);
    path_good[strlen(path) - 1] = '\0';
    int fd = open(path_good, O_RDONLY, S_IRWXU);
    if (fd == -1) {
      perror("SERVER: error opening file");
      printf("file name: %s\n", path_good);
      code = 404;
    }
    int err = read(fd, response_buf, BUF_SIZE);
    if (err == -1) {
      perror("SERVER: error reading file");
      code = 500;
    }
    content_type = parse_file_type(path_good);

  } else {
    // serve index.html
    int fd = open("index.html", O_RDONLY, S_IRWXU);
    if (fd == -1) {
      perror("SERVER: error opening index.html");
      code = 404;
    }
    int err = read(fd, response_buf, BUF_SIZE);
    if (err == -1) {
      perror("SERVER: error reading file");
      code = 500;
    }
    content_type = parse_file_type("index.html");
  }

  response(response_buf, code, content_type);

  send(client_fd, response_buf, strlen(response_buf), 0);
};

// Handling POST requests
void handle_post(int client_fd) {

  const char *body = "Hello, POST! Body:\nRIP BOZO";
  size_t content_length = strlen(body);
  char response[BUF_SIZE];
  snprintf(response, BUF_SIZE,
           "HTTP/1.1 200 OK\r\n"
           "Content-Length: %zu\r\n"
           "Content-Type: text/plain\r\n"
           "\r\n"
           "%s",
           content_length, body);
  send(client_fd, response, strlen(response), 0);
};

// Handling PUT requests
void handle_put(int client_fd) {

  const char *body = "Hello, POST! Body:\nRIP BOZO";
  size_t content_length = strlen(body);
  char response[BUF_SIZE];
  snprintf(response, BUF_SIZE,
           "HTTP/1.1 200 OK\r\n"
           "Content-Length: %zu\r\n"
           "Content-Type: text/plain\r\n"
           "\r\n"
           "%s",
           content_length, body);
  send(client_fd, response, strlen(response), 0);
};

void handle_wrong(int client_fd) {

  char response[BUF_SIZE];
  snprintf(response, BUF_SIZE,
           "HTTP/1.1 405 Method Not Allowed\r\n"
           "Content-Length: 0\r\n"
           "\r\n");
  send(client_fd, response, strlen(response), 0);
}

void handle_req(int *client_fd) {
  // Recieve Data
  char buf[BUF_SIZE];
  int bytes_read = recv(*client_fd, buf, BUF_SIZE - 1, 0);

  // error handling
  if (bytes_read == -1) {
    perror("SERVER: error recieving");
  } else if (bytes_read == 0) {
    perror("SERVER: client disconnected");
    close(*client_fd);
    *client_fd = -1;
  }
  buf[bytes_read] = '\0';
  printf("buf: %s\n", buf);

  // Handle The Request
  if (strncmp(buf, "GET", 3) == 0) {
    char *path = strtok(buf, " ");
    path = strtok(NULL, " ");
    handle_get(*client_fd, path);
  } else if (strncmp(buf, "POST", 4) == 0) {
    handle_post(*client_fd);
  } else if (strncmp(buf, "PUT", 3) == 0) {
    handle_put(*client_fd);
  } else {
  }
  close(*client_fd);
  *client_fd = -1;
}

int main() {
  // setting up socket
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == -1) {
    perror("error opening socket");
    return 1;
  }

  // setting up address
  struct in_addr in_addr;
  inet_pton(AF_INET, "127.0.0.1", &in_addr);

  set_socket_opts(server_fd);

  struct sockaddr_in *sockaddr = malloc(sizeof(struct sockaddr_in));
  memset(sockaddr, 0, sizeof(struct sockaddr_in));
  sockaddr->sin_family = AF_INET;
  sockaddr->sin_addr = in_addr;
  sockaddr->sin_port = htons(6969);

  // binding socket with address
  if (bind(server_fd, (struct sockaddr *)sockaddr, sizeof(*sockaddr)) == -1) {
    perror("bind failed");
    close(server_fd);
    return 1;
  }

  if (listen(server_fd, 10) == -1) {
    perror("listen failed");
    return 1;
  }

  if (set_nonblocking(server_fd) == -1) {
    perror("Failed to set non-blocking");
    return 1;
  }

  printf("Server is listening on http://127.0.0.1:6969\n");
  struct pollfd fds[MAX_CLIENTS + 1];
  fds[0].fd = server_fd;
  fds[0].events = POLLIN;

  for (int i = 1; i < MAX_CLIENTS + 1; i++) {
    fds[i].fd = -1;
    fds[i].events = POLLIN;
  }
  while (1) {

    // Polling
    int poll_result = poll(fds, MAX_CLIENTS + 1, 1000);

    if (poll_result == -1)
      perror("poll error");

    if (fds[0].revents & POLLIN) {
      int client_fd = accept(fds[0].fd, NULL, NULL);
      if (client_fd == -1) {
        perror("error with client fd");
        return 1;
      } else {
        printf("Server: got a connection\n");
      }
      int full = 1;
      for (int i = 1; i < MAX_CLIENTS + 1; i++) {
        if (fds[i].fd == -1) {
          fds[i].fd = client_fd;
          fds[i].events = POLLIN;
          full = 0;
          break;
        }
      }
      if (full) {
        printf("Max client reached");
        close(client_fd);
      }
    }
    for (int i = 1; i < MAX_CLIENTS + 1; i++) {
      if (fds[i].revents & POLLIN && fds[i].fd != -1) {
        handle_req(&fds[i].fd);
      }
    }
  }

  close(server_fd);
}
