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

#include <vector>
#include <sstream>

#define DEFAULT_METHOD "GET"
#define DEFAULT_PORT "80"
#define DEFUALT_VERSION "HTTP/1.0"
#define CONNECTION_CLOSE "Connection: close"
#define INTERNAL_ERROR "500 INTERNAL ERROR\n"

#define MAXPENDING 5

using namespace std;

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

static void process(void* param) 
{
  char msg_recv[2000];
  char* bp = msg_recv;
  size_t bytes_left = sizeof(msg_recv) / sizeof(char);
  size_t bytes_recv;

  int* telnet_sock = (int*) param;

	while (bytes_left) 
	{
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

		strtok(msg_recv, "\n");
	  
		string command = string(msg_recv);
		string buf; // Have a buffer string
		stringstream ss(command); // Insert the string into a stream
		vector<string> tokens; // Create vector to hold our words
	
		while (ss >> buf)
		{
			tokens.push_back(buf);
		}
		
		// if (tokens[0] != "GET")
		// {
			// sendString(clientSock, errorMsg);
		// }
		// else if (tokens[2] != "HTTP/1.0")
		// {
				// sendString(clientSock, errorMsg);
		// }
		// else
		// {
			string url = tokens[1];
			// string strReceive = GET(httpUrl);
			// sendString(clientSock, strReceive);
		// }
	
		size_t found = url.find_first_of(":");
		string host;
		string port;
		string path;
	
		//try
		//{
			string protocol = url.substr(0,found); 

			//url_new is the url excluding the http part
			string url_new = url.substr(found + 3); 
			size_t found1 = url_new.find_first_of(":");
			int convertFound1 = static_cast<int>(found1);
			if (convertFound1 == -1)
				found1 = url_new.find_first_of("/");
			host = url_new.substr(0,found1);

			// get the port number and path
			size_t found2 = url_new.find_first_of("/");

			if (convertFound1 == -1)
				port = "80";
			else
				port = url_new.substr(found1 + 1, found2 - found1 - 1);
			path = url_new.substr(found2);
		//}
		//catch (const std::out_of_range& oor) 
		//{
			//return errorMsg;
		//}
	
		char *cstrHost = new char[host.length() + 1];
		strcpy(cstrHost, host.c_str());
		char *cstrPort = new char[port.length() + 1];
		strcpy(cstrPort, port.c_str());
	

		struct addrinfo hints, *serv_info, *p;
		int result, web_server_sock = 0;

		memset(&hints, 0, sizeof(hints));
		hints.ai_flags = AI_CANONNAME; // returns server name
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM; // TCP

		if ((result = getaddrinfo(cstrHost, cstrPort, &hints, &serv_info)) != 0) 
		{
			send_error(*telnet_sock);
			return;
		}
  
		delete [] cstrHost;
		delete [] cstrPort;

		for (p = serv_info; p != NULL; p = p->ai_next) 
		{
			if ((web_server_sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
				continue;
		}

		if (connect(web_server_sock, p->ai_addr, p->ai_addrlen) == -1) 
		{
		  continue;
		}

		break;
	}

	freeaddrinfo(serv_info);

	if (web_server_sock <= 0) 
	{
		send_error(*telnet_sock);
		close(web_server_sock);
		return;
	}

	string strHeader = "GET " + path + " HTTP/1.0\r\nHost: " + string(host) 
						+ "\r\nConnection: close\r\n\r\n";
    
	char *header  = new char[strHeader.length() + 1];
	strcpy(header, strHeader.c_str());

	if (send(web_server_sock, header, strlen(header), 0) < 0)
	{	delete [] header;
		send_error(*telnet_sock);
		close(web_server_sock);
		return;
		
	}
	delete [] header;

	char http_msg_recv[1000000];
	bp = http_msg_recv;
	bytes_left = (sizeof(http_msg_recv) / sizeof(char));

	while (bytes_left)
	{
		bytes_recv = recv(web_server_sock, bp, bytes_left, 0);
		if (bytes_recv <= 0) 
		{
			break;
		}
		bytes_left = bytes_left - bytes_recv;
		bp = bp + bytes_recv;
	}

	printf("\n%s\n", http_msg_recv);

	int http_msg_recv_length = sizeof(http_msg_recv);
	if (send_all(*telnet_sock, http_msg_recv, &http_msg_recv_length) < 0) 
	{
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
