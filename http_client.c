#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <netdb.h>
#include <sys/socket.h>

#define BUF_SIZE	100

struct addinfo* getHostInfo(char* host, char* port)
{
	int r;
	struct addrinfo hints, *getaddinfo_res;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if(r = getaddrinfo(host, port, &hints, &getaddinfo_res))
	{
		fprintf(stderr, "getHostInfo %s\n", gai_strerror(r));
		return NULL;
	}

	return getaddinfo_res;
}

int eastablishConnection(struct addrinfo *info)
{
	if(info == NULL)
		return -1;

	int clientfd;
	for( ;info != NULL; info = info->ai_next)
	{
		if((clientfd = socket(info->ai_family, info->ai_socktype, info->ai_protocol)) < 0)
		{
			perror("Error in eastablishing connection socket\n");
			continue;
		}

		if(connect(clientfd, info->ai_addr, info->ai_addrlen) < 0)
		{
			close(clientfd);
			perror("Error in eastablishing connection::connect\n");
			continue;
		}

		freeaddrinfo(info);
		return clientfd;
	}

	freeaddrinfo(info);
	return -1;
}

void GET(int clientfd, char* host, char* path)
{
	int req[1000] = {0};
	sprintf(req, "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", path, host);
	send(clientfd, req, strlen(req), 0);
}

void PUT(int clientfd, char* host, char* path)
{
	int req[1000] = {0};
	sprintf(req, "PUT %s HTTP/1.1\r\nHost: %s\r\n\r\n", path, host);
	send(clientfd, req, strlen(req), 0);
}

int main(int argc, char** argv)
{
	int clientfd;
	char buf[BUF_SIZE];

	if(argc != 5)
	{
		fprintf(stderr, "USAGE: ./myclient <hostname> <port> <command> <filename>");
		return 1;
	}

	clientfd = eastablishConnection(getHostInfo(argv[1], argv[2]));
	if(clientfd == -1)
	{
		fprintf(stderr, "failed to connect to: %s %s %s\n", argv[1], argv[2], argv[3]);
		return 3;
	}

	if(strcmp(argv[3], "GET") == 0)
		GET(clientfd, argv[1], argv[4]);
	else
		PUT(clientfd, argv[1], argv[4]);

	while(recv(clientfd, buf, BUF_SIZE, 0) > 0)
	{
		fputs(buf, stdout);
		memset(buf, 0, BUF_SIZE);
	}

	close(clientfd);
	return 0;
}
