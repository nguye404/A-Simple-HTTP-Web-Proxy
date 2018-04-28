//CPSC 4510, Spring 2018, Seattle University
//Homework Assignment #3
//Team Members: Jason Esparza, Hang Nguyen, Scott Little

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
#include <semaphore.h>

#define DEFAULT_METHOD "GET"
#define DEFAULT_PORT "80"
#define DEFUALT_VERSION "HTTP/1.0"
#define CONNECTION_CLOSE "Connection: close"
#define INTERNAL_ERROR "500 INTERNAL ERROR\n"
#define BACKLOG 30
const int BUFFER_LENGTH = 2048;
const int MAX_MESSAGE = 200000;
sem_t maxConcurrent;
int MAX_CONCURRENT_USERS = 30;

using namespace std;

struct ThreadArgs
{
	int clientSock;
};


int send_chunk(int client_sock, const char* buffer, int buffer_length)
{
	int bytes_sent = 0;
	int bytes_just_sent;

	while (bytes_sent < buffer_length)
	{
		if ((bytes_just_sent = send(client_sock, (void *) (buffer + bytes_sent), buffer_length - bytes_sent, MSG_NOSIGNAL)) < 0)
		{
			return bytes_just_sent;
		}
		bytes_sent += bytes_just_sent;
	}
	return 0;
}


// It receives the chunk of the response from http server and sends that 
// chunk to the client right away.  It doesn't wait until the whole response
// is receive to send to the client
void producer_consumer(int client_sock, int http_server_sock)
{
	int bytes_recv;
	char* buffer = (char*)calloc(BUFFER_LENGTH, 1);
	
	while((bytes_recv = recv(http_server_sock, buffer, BUFFER_LENGTH, 0)) > 0)
	{
		// send the chunk just received from the http server to the client
		send_chunk(client_sock, buffer, bytes_recv);
	}
	free(buffer);
}


void* processThread(void *args)
{
	struct ThreadArgs *threadArgs = (struct ThreadArgs *) args;
	int telnet_sock = threadArgs -> clientSock;
	delete threadArgs;

	string command = "";
    string url = "";

	int bytesLeft = MAX_MESSAGE;
	char* buffer = (char*)calloc(bytesLeft, 1);

	while (command.find("\r\n\r\n") == string::npos && bytesLeft)
	{
		int bytesRecv = recv(telnet_sock, buffer, bytesLeft, 0);
		bytesLeft = bytesLeft - bytesRecv;
		command = buffer;
	}
	free(buffer);

	command.erase(std::remove(command.begin(), command.end(), '\n'), command.end());
	
	string buf;	// Have a buffer string
	stringstream ss(command);	// Insert the string into a stream
	vector<string> tokens;	// Create vector to hold our words

	while (ss >> buf)
	{
		tokens.push_back(buf);
	}
	
	// token size less than 3 means that there are only 2 combination between
	// 'GET', host and 'HTTP/1.0' or the request is invalid
	if  (tokens.size() < 3)
	{
		send_chunk(telnet_sock, INTERNAL_ERROR, strlen(INTERNAL_ERROR));
		close (telnet_sock);
		sem_post(&maxConcurrent);
		return NULL;	
	}
	else if (tokens[0] != "GET")
	{
		send_chunk(telnet_sock, INTERNAL_ERROR, strlen(INTERNAL_ERROR));
		close (telnet_sock);
		sem_post(&maxConcurrent);
		return NULL;
	}
	else if (tokens[2] != "HTTP/1.0")
	{
		send_chunk(telnet_sock, INTERNAL_ERROR, strlen(INTERNAL_ERROR));
		close (telnet_sock);
		sem_post(&maxConcurrent);
		return NULL;
	}
	else
	{
		url = tokens[1];
		size_t found = url.find_first_of(":");
		string host;
		string port;
		string path;

		try
		{
			string protocol = url.substr(0,found);
			if (protocol != "http")
			{
				send_chunk(telnet_sock, INTERNAL_ERROR, strlen(INTERNAL_ERROR));
				close (telnet_sock);
				sem_post(&maxConcurrent);
				return NULL;
			}
			string url_new = url.substr(found + 3);
			size_t found1 = url_new.find_first_of(":");
			int convertFound1 = static_cast<int>(found1);
			
			if (convertFound1 == -1)
			found1 = url_new.find_first_of("/");
			host = url_new.substr(0,found1);
			size_t found2 = url_new.find_first_of("/");

			if (convertFound1 == -1)
			{
				port = "80";
			}
			else
			{
				port = url_new.substr(found1 + 1, found2 - found1 - 1);
			}
			path = url_new.substr(found2);
		}
		catch (const std::out_of_range& oor)
		{
			send_chunk(telnet_sock, INTERNAL_ERROR, strlen(INTERNAL_ERROR));
			close (telnet_sock);
			sem_post(&maxConcurrent);
		}

		char *cstrHost = new char[host.length() + 1];
		strcpy(cstrHost, host.c_str());
		char *cstrPort = new char[port.length() + 1];
		strcpy(cstrPort, port.c_str());

		struct addrinfo hints, *serv_info, *p;
		int result, web_server_sock = 0;

		memset(&hints, 0, sizeof(hints));
		hints.ai_flags = AI_CANONNAME;
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;

		if ((result = getaddrinfo(cstrHost, cstrPort, &hints, &serv_info)) != 0)
		{
		  send_chunk(telnet_sock, INTERNAL_ERROR, strlen(INTERNAL_ERROR));
		  close (telnet_sock);
		  sem_post(&maxConcurrent);
		  return NULL;
		}

		delete [] cstrHost;
		delete [] cstrPort;

		// loop through all the results and bind to the first 
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
		freeaddrinfo(serv_info);	// No longer need list of addresses

		if (web_server_sock <= 0)
		{
			send_chunk(telnet_sock, INTERNAL_ERROR, strlen(INTERNAL_ERROR));
			close(web_server_sock);
			close (telnet_sock);
			sem_post(&maxConcurrent);
			return NULL;
		}

		string strHeader = "GET " + path + " HTTP/1.0\r\nHost: " + string(host)
				  + "\r\nConnection: close\r\n\r\n";

		char *header  = new char[strHeader.length() + 1];
		strcpy(header, strHeader.c_str());

		if (send_chunk(web_server_sock, header, strlen(header)) < 0)
		{	delete [] header;
			send_chunk(telnet_sock, INTERNAL_ERROR, strlen(INTERNAL_ERROR));
			close(web_server_sock);
			close (telnet_sock);
			sem_post(&maxConcurrent);
			return NULL;
		}
		delete [] header;

		producer_consumer(telnet_sock, web_server_sock);
		close(web_server_sock);
	}
	sem_post(&maxConcurrent);
	pthread_detach(pthread_self());
	close (telnet_sock);
	return NULL;
}


int main (int argc, const char* argv[]) 
{
	if (argc != 2) 
	{
		errno = EINVAL;
		perror("Command line");
		exit(-1);
	}

    int local_sock;
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

	if ((local_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
		close(local_sock);
		exit(-1);
	}

	if (bind(local_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
	{
		close(local_sock);
		exit(-1);
	}

	if (listen(local_sock, BACKLOG) < 0)
	{
		close(local_sock);
	}

	sem_init(&maxConcurrent, 0, MAX_CONCURRENT_USERS);
	while (true)
	{
		sem_wait(&maxConcurrent);
		struct sockaddr_in clientAddr;
		socklen_t addrLen = sizeof(clientAddr);
		int clientSock = accept(local_sock, (struct sockaddr *) &clientAddr, &addrLen);

		if (clientSock < 0)
		{
			exit(-1);
		}
		else
		{
			struct ThreadArgs* threadArgs = new ThreadArgs;
			threadArgs -> clientSock = clientSock;
			pthread_t threadID;
			pthread_create(&threadID, NULL, processThread, (void *) threadArgs);
		}
	}
	sem_close(&maxConcurrent);
	return 0;
}
