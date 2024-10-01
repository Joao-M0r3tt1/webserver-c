/* httpd.c */

#include <sys/socket.h>		// definitions for creating sockets
#include <stdio.h>		// functions for standard input and output
#include <stdlib.h>		// general utility functions
#include <arpa/inet.h>		// definitions for handling IP addresses
#include <unistd.h>		// POSIX functions, such as close()
#include <string.h>		// functions for string manipulation
#include <netinet/in.h>		// definitions for network protocols
#include <fcntl.h>		// file control definitions

#define LISTENADDR "0.0.0.0"	// server listens on all available interfaces   
/* 0.0.0.0 - listens to all interfaces, if you don't have a firewall for protection, use the localhost IP - 127.0.0.1 */

/* structures */
/* represent an HTTP request */
struct sHttpRequest {
	char method[8];		// HTTP method (GET, POST, etc.)
	char url[128];		// Request URL
}; typedef struct sHttpRequest httpreq;

/* store information about files */
struct sFile {
	char file_name[64];	// file name
	char *file_contents;	// file content
	int size;		// file size
}; typedef struct sFile File;  

/* global variables */
char *error;	// store error messages

/* start the server | returns -1 on error, or it returns a socket fd */
int srv_init(int port) {
	int sock_fd;	// socket file descriptor 
	struct sockaddr_in server;	// structure for storing server information

	memset(&server, 0, sizeof(server));	// clean structure

	/* create socket and check for errors */
	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(sock_fd < 0) {
		error = "socket() error! Socket creation failed!";
		return -1;
	}

	/* server structure configuration */
	server.sin_family = AF_INET;	// address family (IPv4)
	server.sin_addr.s_addr = inet_addr(LISTENADDR);    // IP address (0.0.0.0)
	server.sin_port = htons(port);    // port listened to by the server

	/* associate the socket with the configured interface and port and check for errors */
	if(bind(sock_fd, (struct sockaddr *)&server, sizeof(server))) {
		error = "bind() error! Socket bind failed!";
		close(sock_fd);    // close the socket
		return -1;
	}

	/* listen for connections and check for erros */
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

/* parse HTTP request | returns NULL on error, or it return a httpreq structure */
httpreq *parse_http(char *str) {
	char *pointer;    // pointer to traverse the string
	httpreq *request = malloc(sizeof(httpreq));    // allocate memory for the request
  
	/* check if memory allocation failed */
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
	snprintf(request->url, sizeof(request->url), "%s", str);    // copy the URL to the structure

	return request;    // returns the filled structure
}

/* read client data | return 0 on error, or return the data */
char *client_read(int client) {
	static char buffer[512];    // store read data

	memset(&buffer, 0, 512);    // clean buffer
				    
	/* read client socket and check for errors */
	if(read(client, buffer, 511) < 0) {
		error = "read() error! Reading failed!";
		return 0;
	}

	return buffer;    // returns buffer read
}

/* HTTP response to the client based on the request | no return */
void http_response(int client, char *content_type, char *data) {
	char buffer[512];    // response data
	int length = strlen(data); // response length

	memset(buffer, 0, 512);    // clean buffer

	/* format the response and place it in the buffer */
	snprintf(buffer, 511,
		"Content-Type: %s\n"
		"Content-Length: %d\n"
		"\n%s\n",
		content_type, length, data);    // response data

	length = strlen(buffer);    // final response length
	write(client, buffer, length);    // send the response to the client

	return;
}

/* HTTP response headers | no return */
void http_headers(int client, int code) {
	char buffer[512];    // response header data
	int length;    // response header length

	memset(buffer, 0, 512);    // clean buffer

	/* format HTTP headers */
	snprintf(buffer, 511,
		"HTTP/1.0 %d OK HTTP Status\n"
		"Server: httpd.c\n"
		"Cache-Control: no-store\n"
		"Content-Language: en\n"
		"X-Frame-Options: SAMEORIGIN\n",
		code);    // response header

	length = strlen(buffer);    // final length of headers
	write(client, buffer, length);    // send the headers to the client

	return;
}

/* reads a file from the file system | return NULL on error, or return a file decriptor/structure */
File *read_file(char *file_name) {
	char buffer[512], *pointer;    // buffer and pointer
	int file_descriptor, n, content_bytes = 0;    // file descriptor, number of bytes read and content bytes
	File *file;    // pointer to file structure
 
	/* open the file for reading and check for errors */
	file_descriptor = open(file_name, O_RDONLY);
	if(file_descriptor < 0) return NULL;

	/* allocate memory for the file structure and check for errors */
	file = malloc(sizeof(struct sFile));
	if(!file) {
		close(file_descriptor);    // close the file descriptor
		return NULL;
	}

	snprintf(file->file_name, sizeof(file->file_name), "%s", file_name);    // copy file name
	file->file_contents = malloc(512);    // allocates memory for the file contents

	/* reads the contents of the file in blocks of 512 Bytes */
	while(1) {
		memset(buffer, 0, 512);    // clean buffer
		n = read(file_descriptor, buffer, 512);    // reads up to 512 bytes from the file

		/* exit the loop if there are no more bytes to read */
		if(!n) break;
		/* check reading error */
		else if(content_bytes == -1) {
			close(file_descriptor);    // closes the file descriptor
			free(file->file_contents);    // frees allocated memory
			free(file);    // frees the file structure

			return NULL;
		}

		memcpy((file->file_contents) + content_bytes, buffer, n);    // copies the read bytes to the structure
		content_bytes += n;    // updates the total bytes read
		file->file_contents = realloc(file->file_contents, (512 + content_bytes));    // reallocates memory for file contents
	}

	file->size = content_bytes;    // stores the size of the content read
	close(file_descriptor);    // closes the file descriptor

	return file;    // returns the filled file structure
}

/* send requested files | return -1 on error, or return 0 everything ok */
int send_file(int client, char *content_type, File *file) {
	char buffer[512], *pointer;    // buffer and pointer
	int length, amount_bytes;    // number bytes

	/* check if the file can be read */
	if(!file) return -1;

	memset(buffer, 0, 512);    // clean buffer

	/* format the HTTP header */
	snprintf(buffer, 511, 
		"Content-Type: %s\n"
		"Content-Length: %d\n\n",
		content_type, file->size);    // stores content in buffer 

	length = strlen(buffer);    // get the length of the header
	write(client, buffer, length);    // sends the header to the client

	length = file->size;    // update length for file content
	pointer = file->file_contents;    // point to file contents
					  
	while(1) {
		/* sends up to 512 bytes or the remaining bytes of the file and checks for errors */
		amount_bytes = write(client, pointer,  (length < 512) ? length : 512);
		if(amount_bytes < 1) return -1;

		length -= amount_bytes;    // update remaining length
		if(length < 1) break;    // exit the loop if all bytes have been sent
		else pointer += amount_bytes;    // move the pointer forward 
	}

	return 0;    // success
}

/* manage client connection | no return */
void client_connection(int server, int client) {
        httpreq *request;    // pointer to the request
        char *pointer, *response, str[96];    // read data
	File *file;    // pointer to file

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

	/* check if the request is valid | static verification */
	/* TODO: improve security by checking for things like "../.." etc */

	/* GET request for images */
	if(!strcmp(request->method, "GET") && !strncmp(request->url, "/img/", 5)) {
		memset(str, 0, 96);    // clear the string
		snprintf(str, 95, ".%s", request->url);    // form the file path
		
		/* read the file and check if it was not found */
		file = read_file(str);
		if(!file) {
			response = "<html>File Not Found</html>";    // error message
			http_headers(client, 404);    // 404 error header
			http_response(client, "text/plain", response);    // response to the client

		/* if the file was found*/									  
		} else {
			http_headers(client, 200);    // 200 success header

			/* send the file and check for error */
			if(send_file(client, "image/png", file)) { 
				response = "<html>HTTP Server Error</html>";    // error message
				http_response(client, "text/plain", response);    // response to the client
			}
		}
	}
	/* GET request for the page */
	else if(!strcmp(request->method, "GET") && !strcmp(request->url, "/app/webpage")) {
		response = "<html><img src='/img/test.png' alt='image' /></html>";    // html response
		http_headers(client, 200);    // 200 everything okay
		http_response(client, "text/html", response);    // response to the client 

	/* for other requests */
	} else {
		response = "<html>File Not Found</html>";    // error message
		http_headers(client, 404);    // 404 error header
		http_response(client, "text/plain", response);    // response to the client
	}

        free(request);    // free up memory
        close(client);    // close socket

        return;
}

/* main function | returns -1 on error */
int main(int argc, char *argv[]) {
	int server_fd, client_fd;    // server and client socket file descriptor
	char *port;    // port for connection

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
		if(!fork()) {
			client_connection(server_fd, client_fd);    // calls the function to manage the client connection
			exit(0);    // terminates the child process
		}
		close(client_fd);    // parent process closes client socket
	}
	
	return -1;
}
