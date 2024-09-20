/* httpd.c */

#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#define LISTENADDR "0.0.0.0"    // server listens to all interfaces   
/* 0.0.0.0 - listens to all interfaces, if you don't have a firewall for protection, use the localhost IP - 127.0.0.1 */

/* store the HTTP request */
struct sHttpRequest {
	char method[8];    // HTTP method (GET, POST, etc.)
	char url[128];    // Request URL
}; typedef struct sHttpRequest httpreq;

char *error;    // Global variable to store error messages

/* start the server | returns -1 on error, or it returns a socket fd */
int srv_init(int port) {
	int sock_fd;    // socket file descriptor 
	struct sockaddr_in server;    // structure for storing server information

	memset(&server, 0, sizeof(server));    // clean structure

	/* create socket and check for errors */
	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(sock_fd < 0) {
		error = "socket() error! Socket creation failed!";
		return -1;
	}

	/* server structure configuration */
	server.sin_family = AF_INET;    // address family (IPv4)
	server.sin_addr.s_addr = inet_addr(LISTENADDR);    // IP address (0.0.0.0)
	server.sin_port = htons(port);    // Port

	/* bind the newly created socket to the IP and check for errors */
	if(bind(sock_fd, (struct sockaddr *)&server, sizeof(server))) {
		error = "bind() error! Socket bind failed!";
		close(sock_fd);    // close the socket
		return -1;
	}

	/* server ready | listen for connections and check for erros */
	if(listen(sock_fd, 5)) {
		error = "listen() error! Listen failed!";
		close(sock_fd);    // close the socket
		return -1;
	}

	return sock_fd;    // return the socket descriptor
}

/* accept client connections | returns -1 on error, or returns the new client's socket fd */
int cli_accept(int srv_fd) {
	int sock_fd;    // client socket file descriptor
	socklen_t addrlen = 0;    // client structure size
	struct sockaddr_in client;    // structure for storing customer information

	memset(&client, 0, sizeof(client));    // clean structure

	/* accept connection (data packet) from client end check for errors */
	sock_fd = accept(srv_fd, (struct sockaddr *)&client, &addrlen);
	if(sock_fd < 0) {
		error = "accept() error! Server accept failed!";
		close(sock_fd);    // close the socket
		return -1;
}

	return sock_fd;    // return the client socket descriptor
}

/* parse HTTP request | returns NULL on error, or it returna a httpreq structure */
httpreq *parse_http(char *str) {
	char *pointer;    // pointer to traverse the string
	httpreq *request = malloc(sizeof(httpreq));    // allocate memory for the request
  
	if(!request) {
		error = "malloc() error! Allocate memory failed!";
		return NULL;
	}

	memset(request, 0, sizeof(httpreq));    // clean structure

	/* extract method from request */
	for(pointer = str; *pointer && *pointer != ' '; pointer++);
	if(*pointer == ' ') *pointer++ = 0;    // replace space with NULL
	else {
		error = "parse-http() error! NoSpace error!";
		free(request);    // free up memory
		return NULL;
	}
	/* strncpy(request->method, str, 7); */
	snprintf(request->method, sizeof(request->method), "%s", str);    // copy the method to the structure

	/* extract URL from request */
	str = pointer;
	for(pointer = str; *pointer && *pointer != ' '; pointer++);
	if(*pointer == ' ') *pointer = 0;    // replace space with NULL
	else {
		error = "parse_http() error! 2ndSpace error!";
		free(request);    // free up memory
		return NULL;
	}
	/* strncpy(request->url, str, 127); */
	snprintf(request->url, sizeof(request->url), "%s", str);    // copy the URL to the structure

	return request;    // returns the filled structure
}

/* read client data | return 0 on error, or return the data */
char *client_read(int client) {
	static char buffer[512];    // store read data

	memset(&buffer, 0, 512);    // clean buffer
				    
	/* read client ocket and check for errors */
	if(read(client, buffer, 511) < 0) {
		error = "read() error! Reading failed!";
		return 0;
	}

	return buffer;    // returns buffer read
}

/* manage client connection */
void client_connection(int server, int client) {
        httpreq *request;    // pointer to the request
        char buffer[512], *pointer;    // read data

	/* read client data and check for errors */
        pointer = client_read(client); 
        if(!pointer) {
                fprintf(stderr, "%s\n", error);
                close(client);    // close socket
                return;
        }

	/* analyze request and check for errors */
        request = parse_http(pointer);
        if(!request) {
                fprintf(stderr, "%s\n", error);
                close(client);    // close socket
                return;
        }

        printf("%s\n%s\n", request->method, request->url);    // display method and URL
        free(request);    // free up memory
        close(client);    // close socket

        return;
}

/* main function | returns -1 on error */
int main(int argc, char *argv[]) {
	int server_fd, client_fd;    /* server and client socket file descriptor */
	char *port;    /* port for connection */

	/* checks if port was passed as argument */
	if(argc < 2) {
		fprintf(stderr, "Usage: %s <listening port>\n", argv[0]);
		return -1;
	} else {
		port = argv[1];    // assign port
	}


	/* initialize server socket and check for errors */
	server_fd = srv_init(atoi(port));
	if(server_fd == -1) {
		fprintf(stderr, "%s\n", error);
		return -1;
	}

	printf("Listening on: %s:%s\n", LISTENADDR, port);    // inform that you ar listening

	/* infinite loop for accepting connections */
	for(;;) {
		/* accept client connection and check for errors */
		client_fd = cli_accept(server_fd);
		if(client_fd == -1) {
			fprintf(stderr, "%s\n", error);
			continue;    // continue on error
		}

		printf("Incoming connection!\n");    // report connection established
		/* create a child process to manage the connection */
		if(!fork()) client_connection(server_fd, client_fd);
	}
	
	return -1;
}
