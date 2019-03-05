#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <libgen.h>

#include <netdb.h>
#include <sys/socket.h>

#define BUF_SIZE	100

struct addrinfo *getAddrInfo(char* port)
{
	int r;
	struct addrinfo hints, *getaddrinfo_res;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_socktype = SOCK_STREAM;

	if((r = getaddrinfo(NULL, port, &hints, &getaddrinfo_res)))
	{
		fprintf(stderr, "getAddrInfo: %s\n", gai_strerror(r));
		return NULL;
	}

	return getaddrinfo_res;
}

int bindListener(struct addrinfo *info)
{
	if(info == NULL)
		return -1;

	int serverfd;
	for( ;info != NULL; info = info->ai_next)
	{
		if((serverfd = socket(info->ai_family, info->ai_socktype, info->ai_protocol)) < 0)
		{
			perror("bindListener socket error\n");
			continue;
		}

		int opt = 1;
		if(setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)) < 0)
		{
			perror("bindListener setsockopt error\n");
			return -1;
		}

		if(bind(serverfd, info->ai_addr, info->ai_addrlen) < 0)
		{
			close(serverfd);
			perror("bindListener bind error\n");
			continue;
		}

		freeaddrinfo(info);
		return serverfd;
	}

	freeaddrinfo(info);
	return -1;
}

void header(int handler, int status)
{
	int header[BUF_SIZE] = {0};
	if(status == 0)
		sprintf(header, "HTTP/1.0 200 OK\r\n\r\n");
	else if(status == 1)
		sprintf(header, "HTTP/1.0 403 Forbidden\r\n\r\n");
	else
		sprintf(header, "HTTP/1.0 404 Not Found\r\n\r\n");

	send(handler, header, strlen(header), 0);
}

void resolve(int handler)
{
	int size;
	int buf[BUF_SIZE];
	char* method;
	char* filename;

	recv(handler, buf, BUF_SIZE, 0);
//	fprintf(stderr, "buf: %s\n", buf);
	method = strtok(buf, " ");
	
	if(strcmp(method, "GET") != 0 && (strcmp(method, "PUT") != 0))
		return;

	filename = strtok(NULL, " ");
	if(filename[0] == '/')
		filename++;

	fprintf(stderr, "fileName: %s\n", filename);
	if(access(filename, F_OK) != 0)
	{
		header(handler, 2);
		return;
	}
	else if(access(filename, R_OK) != 0)
	{
		header(handler, 1);
		return;
	}
	else
	{
		header(handler, 0);
	}

	if(strcmp(method, "GET") == 0)
	{
		FILE *file = fopen(filename, "r");
		if(file == NULL)
			fprintf(stderr, "open failed \n");
		while(fgets(buf, BUF_SIZE, file))
		{
			send(handler, buf, strlen(buf), 0);
			memset(buf, 0, BUF_SIZE);
		}
	}
	else
	{
		char* bname;
		bname = basename(filename);
		FILE *file_write = fopen(bname, "w+");
		if(file_write == NULL)
			fprintf(stderr, "open failed \n");

		FILE *file_read = fopen(filename, "r");
		if(file_read == NULL)
			fprintf(stderr, "open failed \n");

		while(fgets(buf, BUF_SIZE, file_read))
		{
			fputs(buf, file_write);
			memset(buf, 0, BUF_SIZE);
		}

		fclose(file_write);
		fclose(file_read);
	}
}

int main(int argc, char** argv)
{
	if(argc != 2)
	{
		fprintf(stderr, "USAGE: ./myserver port");
		return 1;
	}

	int server = bindListener(getAddrInfo(argv[1]));
	if(server < 0)
	{
		fprintf(stderr, "bindListener failed at port %s\n", argv[1]);
		return 2;
	}
	
	if(listen(server, 10) < 0)
	{
		perror("listen failed");
		return 3;
	}

	//Accept incoming requests asynchronously
	int handler;
	socklen_t size;
	struct sockaddr_storage client;

	while(1)
	{
		size = sizeof(client);
		handler = accept(server, (struct sockaddr *)&client, &size);
		if(handler < 0)
		{
			perror("Error at main::accept\n");
			continue;
		}

		//handle asynchronously
		switch(fork())
		{
			case -1:
				fprintf(stderr, "fork failed\n");
				break;
			case 0:
				close(server);
				resolve(handler);
				close(handler);
				exit(0);
			default:
				close(handler);
		}
	}

	close(server);
	return 0;
}
