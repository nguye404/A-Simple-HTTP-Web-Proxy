
#include <stdio.h>
#include <sys/types.h>  // size_t, ssize_t
#include <sys/socket.h> // socket funcs
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h>  // htons, inet_pton
#include <netdb.h>      // gai_strerror
#include <unistd.h>     // close
#include <errno.h>      // perror
#include <stdlib.h>
#include <cstring>
#include <string>
#include <pthread.h>
#include "multi_thread.h"

#define DEFAULT_METHOD "GET"
#define DEFAULT_PORT "80"
#define DEFUALT_VERSION "HTTP/1.0"
#define CONNECTION_CLOSE "Connection: close"
#define INTERNAL_ERROR "505 INTERNAL ERROR\n"

#define MAXPENDING 5

static int count;

static void
send_error(int& sock) {
  size_t bytes_sent = send(sock, (void*) &INTERNAL_ERROR, strlen(INTERNAL_ERROR), 0);
  if (bytes_sent != strlen(INTERNAL_ERROR))
    perror("Error with send");
}

static int
send_all(int& sock, char *buffer, int *len)
{
  int total = 0;
  int bytes_left = *len;
  int n;
  while(total < *len) {
    n = send(sock, buffer + total, bytes_left, 0);
    if (n == -1) {
      break;
    }
    total += n;
    bytes_left -= n;
  }
  *len = total;
  return (n == -1) ? -1 : 0;
}

static void
process(void* param) {
  char msg_recv[2000];
  char* bp = msg_recv;
  size_t bytes_left = sizeof(msg_recv) / sizeof(char);
  size_t bytes_recv;

  int* telnet_sock = (int*) param;

  while (bytes_left) {
    bytes_recv = recv(*telnet_sock, bp, bytes_left, 0);
    if (bytes_left <= 0) {
      send_error(*telnet_sock);
      return;
    }
    if (strcmp(bp, "\n"))
      bytes_left = 0;
    else
      bytes_left = bytes_left - bytes_recv;
    bp = bp + bytes_recv;
    }

  printf("Message:%lu\n%s", pthread_self(),msg_recv);

  strtok(msg_recv, "\n");

  printf("\n%i\n%s\n\n", count++, msg_recv);

  char* method = strtok(msg_recv, " ");
  if (!strcmp(method, "GET"))
    strcpy(method, "GET");

  char temp_url[20];
  char* url, * path, * temp_end_port, * temp_port;
  char port[6];
  if ((url = strtok(NULL, "w")) == NULL) {
    send_error(*telnet_sock);
    return;
  }
  url = strtok(NULL, "/ :");
  if ((path = strtok(NULL, ":")) == NULL) {
    send_error(*telnet_sock);
    return;
  }
  if ((temp_end_port = strtok(NULL, ":")) == NULL) {
    if ((temp_end_port = strtok(temp_end_port, " ")) == NULL)
      ;
  }
  if ((temp_port = strtok(path, ":")) == NULL) {
    send_error(*telnet_sock);
    return;
  }
  if (path[0] == 'H' || isdigit(path[0])) {
    if (atoi(path) > 1) {
      strcpy(port, path);
      temp_port = port;
    }
    strcpy(path, "/");
  }
  strcpy(temp_url, "w");
  strcat(temp_url, url);
  url = temp_url;

  if (temp_end_port != NULL && atoi(temp_end_port) > 1)
    strcpy(port, temp_end_port);
  else if (temp_port == nullptr)
    strcpy(port, DEFAULT_PORT);
  else if (!(atoi(temp_port)))
    strcpy(port, DEFAULT_PORT);

  struct addrinfo hints, *serv_info, *p;
  int result, web_server_sock = 0;

  memset(&hints, 0, sizeof(hints));
  hints.ai_flags = AI_CANONNAME; // returns server name
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM; // TCP

  if ((result = getaddrinfo(url, port, &hints, &serv_info)) != 0) {
    send_error(*telnet_sock);
    return;
  }

  for (p = serv_info; p != NULL; p = p->ai_next) {
    if ((web_server_sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      continue;
  }

    if (connect(web_server_sock, p->ai_addr, p->ai_addrlen) == -1) {
      continue;
    }

    break;
  }

  freeaddrinfo(serv_info);

  if (web_server_sock <= 0) {
    send_error(*telnet_sock);
    close(web_server_sock);
    return;
  }

  printf("Did it get here");

  char message[100];
  strcpy(message, "GET");
  strcat(message, " / ");
  strcat(message, DEFUALT_VERSION);
  strcat(message, "\r\n");
  strcat(message, "Host: ");
  strcat(message, "www.washington.com");
  strcat(message, "\r\n");
  strcat(message, CONNECTION_CLOSE);
  strcat(message, "\r\n\r\n");

  int message_length = sizeof(message);
  if (send_all(web_server_sock, message, &message_length) < 0) {
    send_error(*telnet_sock);
    close(web_server_sock);
    return;
  }

  printf("This got here");
  printf("This was sent: %s\n", message);

  char http_msg_recv[1000000];
  bp = http_msg_recv;
  bytes_left = (sizeof(http_msg_recv) / sizeof(char));
 
  while (bytes_left){
    bytes_recv = recv(web_server_sock, bp, bytes_left, 0);
    if (bytes_recv <= 0) {
      break;
    }
    bytes_left = bytes_left - bytes_recv;
    bp = bp + bytes_recv;
  }

  printf("This was received: %s\n\n\n\n", http_msg_recv);

  int http_msg_recv_length = sizeof(http_msg_recv);
  if (send_all(*telnet_sock, http_msg_recv, &http_msg_recv_length) < 0) {
    send_error(*telnet_sock);
    close(web_server_sock);
    return;
  }

  close(web_server_sock);
}

int main (int argc, const char* argv[]) {
   if (argc != 2) {
    errno = EINVAL;
    perror("Command line");
    exit(-1);
  }

   count = 0;

  int local_sock;
  struct sockaddr_in serv_addr;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(atoi(argv[1]));

  if ((local_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    exit(-1);

  if (bind(local_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
    close(local_sock);
    exit(-1);
  }

  if (listen(local_sock, MAXPENDING) < 0) {
    close(local_sock);
    exit(-1);
  }

  Event event;
  event.fn = process;
  event.param = (void*) &local_sock;
  run_thread(event);

  close(local_sock);
  return 0;
}
