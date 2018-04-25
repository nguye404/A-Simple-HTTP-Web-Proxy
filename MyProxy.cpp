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

#include <vector>
#include <sstream>
#include <algorithm>
#include <iostream>

#define DEFAULT_METHOD "GET"
#define DEFAULT_PORT "80"
#define DEFUALT_VERSION "HTTP/1.0"
#define CONNECTION_CLOSE "Connection: close"
#define INTERNAL_ERROR "500 INTERNAL ERROR\n"

#define MAXPENDING 5
const int BUFFER_LENGTH = 2048; 
const int SMALL_BUFFER_LENGTH = 1024;

using namespace std;

struct ThreadArgs 
{
	int clientSock;
	pthread_t tid;
};

static void
send_error(int sock) 
{
	size_t bytes_sent = send(sock, (void*) &INTERNAL_ERROR, strlen(INTERNAL_ERROR), 0);
	if (bytes_sent != strlen(INTERNAL_ERROR))
		perror("Error with send");
}


void send_chunk(int client_sock, const char* buffer, int buffer_length)
{
	int bytes_sent = 0;
	int bytes_just_sent;
	
	while (bytes_sent < buffer_length)
	{
		if ((bytes_just_sent = send(client_sock, (void *) (buffer + bytes_sent), buffer_length - bytes_sent, 0)) < 0)
		{
            perror ("Sending Error: ");
            //exit(-1);
		}
		bytes_sent += bytes_just_sent;
	}
}


void producer_consumer(int client_sock, int http_server_sock)
{
	int bytes_recv;
	char buffer[BUFFER_LENGTH];
	while((bytes_recv = recv(http_server_sock, buffer, BUFFER_LENGTH, 0)) > 0)
	{
		send_chunk(client_sock, buffer, bytes_recv);
		memset(buffer, 0, sizeof(buffer));
	}
}


void processClient(int telnet_sock)
{
	string command;
	string url;

	int bytesLeft = SMALL_BUFFER_LENGTH; // bytes to read
	char buffer[bytesLeft];    // initially empty
	char *bp = buffer;
		  
	while (command.find("\r\n\r\n") == string::npos && bytesLeft) 
	{
		int bytesRecv = recv(telnet_sock, bp, bytesLeft, 0);
		bytesLeft = bytesLeft - bytesRecv;
		bp = bp + bytesRecv;
		command = buffer;	
	}

	command.erase(std::remove(command.begin(), command.end(), '\n'), command.end());

	string buf; // Have a buffer string
	stringstream ss(command); // Insert the string into a stream
	vector<string> tokens; // Create vector to hold our words

	while (ss >> buf)
	{
		tokens.push_back(buf);
	}
	
	if  (tokens.size() < 3)
	{
		send_chunk(telnet_sock, INTERNAL_ERROR, strlen(INTERNAL_ERROR));
		return;
	}
	else if (tokens[0] != "GET")
	{
		send_chunk(telnet_sock, INTERNAL_ERROR, strlen(INTERNAL_ERROR));
		return;
	}
	else if (tokens[2] != "HTTP/1.0")
	{
		send_chunk(telnet_sock, INTERNAL_ERROR, strlen(INTERNAL_ERROR));
		return;
	}
	else
	{
		url = tokens[1];
		// string strReceive = GET(httpUrl);
		// sendString(clientSock, strReceive);
		size_t found = url.find_first_of(":");
		string host;
		string port;
		string path;

		try
		{
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
		}
		catch (const std::out_of_range& oor) 
		{
			send_chunk(telnet_sock, INTERNAL_ERROR, strlen(INTERNAL_ERROR));
			return;
		}
		
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
		
		printf("ctrsHost: %s\n", cstrHost);
		printf("cstrPort: %s\n", cstrPort);

		if ((result = getaddrinfo(cstrHost, cstrPort, &hints, &serv_info)) != 0) 
		{
			send_error(telnet_sock);
			return;
		}

		delete [] cstrHost;
		delete [] cstrPort;

		for (p = serv_info; p != NULL; p = p->ai_next) 
		{
			if ((web_server_sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) 
			{
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
			send_error(telnet_sock);
			close(web_server_sock);
			return;
		}

		string strHeader = "GET " + path + " HTTP/1.0\r\nHost: " + string(host) 
							+ "\r\nConnection: close\r\n\r\n";
		
		char *header  = new char[strHeader.length() + 1];
		strcpy(header, strHeader.c_str());

		if (send(web_server_sock, header, strlen(header), 0) < 0)
		{	delete [] header;
			send_error(telnet_sock);
			close(web_server_sock);
			return;
			
		}
		delete [] header;

		producer_consumer(telnet_sock, web_server_sock);
		printf("***********after sending the response****************\n");
		
		char end_of_head[SMALL_BUFFER_LENGTH] = "\r\n";
		write(telnet_sock, end_of_head, strlen(end_of_head));
		close(web_server_sock);
		
		return;
	}

	
}


void* processThread(void *args)
{
	//extract socket file descriptor from argument
	struct ThreadArgs *threadArgs = (struct ThreadArgs *) args;
	int clientSock = threadArgs -> clientSock;
	delete threadArgs;
	
	//communicate with client
	processClient(clientSock);
	
	cout << "done______________________*******************\n";
	

	//reclaim resources before finnishing
	pthread_detach(pthread_self());
  
  
	//Close the connection with the client.
	close (clientSock);
	return NULL;
}


int main (int argc, const char* argv[]) {
	if (argc != 2) {
    errno = EINVAL;
    perror("Command line");
    exit(-1);
  }

   //count = 0;

  int local_sock;
  struct sockaddr_in serv_addr;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(atoi(argv[1]));

	if ((local_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		exit(-1);

	if (bind(local_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) 
	{
		close(local_sock);
		exit(-1);
	}

	if (listen(local_sock, MAXPENDING) < 0) 
	{
		close(local_sock);
		exit(-1);
	}

	//Event event;
	//event.fn = process;
	//event.param = (void*) &local_sock;
	//run_thread(event);
	while (true)
	{
		//accept connection	
		struct sockaddr_in clientAddr;
		socklen_t addrLen = sizeof(clientAddr);
		int clientSock = accept(local_sock, (struct sockaddr *) &clientAddr, &addrLen);
		if (clientSock < 0)
			exit(-1);
	  
		struct ThreadArgs* threadArgs = new ThreadArgs;
		threadArgs -> clientSock = clientSock; //one thread at a time
		
		pthread_t threadID;
		int status = pthread_create(&threadID, NULL, processThread, (void *) threadArgs);
		
		if (status != 0)
			exit(-1);
	}

	

	close(local_sock);
	return 0;
}
