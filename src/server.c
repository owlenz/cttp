#include "cttp.h"
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

// format response
void response(response_t *response_object) {
  unsigned char response_body[BUF_SIZE];
  memcpy(response_body, response_object->buffer, BUF_SIZE);
  printf("xdd %s \n", response_body);
  int header_length = snprintf(response_object->buffer, BUF_SIZE,
                               "HTTP/1.1 %d OK\r\n"
                               "Content-Length: %d\r\n"
                               "Content-Type: %s\r\n"
                               "Server: cttp\r\n"
                               "\r\n",
                               response_object->code, response_object->buffer_len, response_object->content_type);
  response_object->buffer_len += header_length;

  for (int i = header_length; i < response_object->buffer_len; i++) {
    response_object->buffer[i] = response_body[i - header_length];
  }
	DEBUG_PRINT("%s", response_object->buffer);
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
  } else if (strcmp(file_type, "webp") == 0) {
    return "image/webp";
  } else if (strcmp(file_type, "css") == 0) {
    return "text/css";
  } else if (strcmp(file_type, "js") == 0) {
    return "text/javascript";
  }
  return "text/bozo";
};

int read_file_res(response_t *response_object, char *file_name) {
	FILE *fptr = fopen(file_name, "rb");
	if (fptr == NULL) {
		perror("SERVER: error opening index.html");
		return 404;
	}
	// get file size
	fseek(fptr, 0, SEEK_END);
	long file_size = ftell(fptr);
	response_object->buffer_len = file_size;
	fseek(fptr, 0, SEEK_SET);

	size_t bytesRead =
			fread(response_object->buffer, sizeof(unsigned char), file_size, fptr);
	if (bytesRead != file_size) {
		perror("SERVER: error reading file");
		return 500;
	}
	response_object->content_type = parse_file_type(file_name);
	return 200;
}


// Handling GET requests
void handle_get(int client_fd, const char *path) {
	response_t *response_object = malloc(sizeof(response_t));
  strcpy(response_object->buffer, "Hello, GET!");
	response_object->buffer_len = 11;
  response_object->content_type = "text/plain\0";
  response_object->code = 200;

  // if path specified
  if (strcmp(path, "/") != 0) {
    char path_good[strlen(path)];
    strncpy(path_good, path + 1, strlen(path) - 1);
    path_good[strlen(path) - 1] = '\0';

		response_object->code = read_file_res(response_object, path_good);
  } else {
    // serve index.html
		response_object->code = read_file_res(response_object, "index.html");
  }

  response(response_object);

	DEBUG_PRINT("%s", response_object->buffer);
  send(client_fd, response_object->buffer, response_object->buffer_len, 0);
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
    // get path (second word)
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

int main(int argc, char **argv) {
  // setting up socket
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == -1) {
    perror("error opening socket");
    return 1;
  }

  config_t *config;
  // handling config
  config = read_args(argc, argv);
  config = config == NULL ? read_file() : config;
  config = config == NULL ? default_config() : config;

  // setting up address
  struct in_addr in_addr;
  inet_pton(AF_INET, config->ip_addr, &in_addr);

  set_socket_opts(server_fd);

  struct sockaddr_in *sockaddr = malloc(sizeof(struct sockaddr_in));
  memset(sockaddr, 0, sizeof(struct sockaddr_in));
  sockaddr->sin_family = AF_INET;
  // now it listens on any address
  /*sockaddr->sin_addr = in_addr;*/
  sockaddr->sin_addr.s_addr = INADDR_ANY;
  sockaddr->sin_port = htons(config->port);

  // binding socket with address
  if (bind(server_fd, (struct sockaddr *)sockaddr, sizeof(*sockaddr)) == -1) {
    perror("bind failed");
    close(server_fd);
    return 1;
  }

  if (listen(server_fd, MAX_CLIENTS) == -1) {
    perror("listen failed");
    return 1;
  }

  if (set_nonblocking(server_fd) == -1) {
    perror("Failed to set non-blocking");
    return 1;
  }

  printf("Server is listening on port %d\n", config->port);
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
      struct sockaddr_in sockaddr_client;
      int addrlen = sizeof(sockaddr_client);
      int client_fd = accept(fds[0].fd, (struct sockaddr *)&sockaddr_client,
                             (socklen_t *)&addrlen);
      if (client_fd == -1) {
        perror("error with client fd");
        return 1;
      } else {
        char *client_ip = inet_ntoa(sockaddr_client.sin_addr);
        printf("Connection established with client IP: %s\n", client_ip);
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
