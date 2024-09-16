/* httpd.c */

#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#define LISTENADDR "127.0.0.1"    /* listening to localhost */   
/* 0.0.0.0 - listens to all interfaces, if you don't have a firewall for protection, use the localhost IP - 127.0.0.1 */

/* global variables */
char *error;

/* returns -1 on error, or it returns a socket fd */
int srv_init(int port) {
	int sock_fd;    /* socket file descriptor */
	struct sockaddr_in server;

	memset(&server, 0, sizeof(server));

	/* socket creation and verification */
	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(sock_fd < 0) {
		error = "socket() error! Socket creation failed!";
		return -1;
	}

	/* socket structure - assignconnection family, standard IP and Port */
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(LISTENADDR);
	server.sin_port = htons(port);

	/* binding newly created ocket to given IP and verification */
	if(bind(sock_fd, (struct sockaddr *)&server, sizeof(server))) {
		error = "bind() error! Socket bind failed!";
		/* close the socket */
		close(sock_fd);
		return -1;
	}

	/* server is ready to listen and verification */
	if(listen(sock_fd, 5)) {
		error = "listen() error! Listen failed!";
		/* close the socket */
		close(sock_fd);
		return -1;
	}

	return sock_fd;
}

/* returns -1 on error, or returns the new client's socket fd */
int cli_accept(int srv_fd) {
	int sock_fd;    /* socket file descriptor */
	socklen_t addrlen = 0;
	struct sockaddr_in client;

	memset(&client, 0, sizeof(client));

	/* accept the data packet from client and verification */
	sock_fd = accept(srv_fd, (struct sockaddr *)&client, &addrlen);
	if(sock_fd < 0) {
		error = "accept() error! Server accept failed!";
		/* close the socket */
		close(sock_fd);
		return -1;
	}

	return sock_fd;
}

void client_connection() {
	return;
}

/* main function */
int main(int argc, char *argv[]) {
	int server_fd, client_fd;    /* socket file descriptor */
	char *port;    /* port for connection */

	/* checks whether the user entered the connection port as an argument on the command line when running the program */
	if(argc < 2) {
		fprintf(stderr, "Usage: %s <listening port>\n", argv[0]);
		return -1;
	} else {
		port = argv[1];
	}


	/* socket initialization function and verification */
	server_fd = srv_init(atoi(port));
	if(server_fd == -1) {
		fprintf(stderr, "%s\n", error);
		return -1;
	}

	printf("Listening on: %s:%s\n", LISTENADDR, port);

	/* infinite loop - keeps the server 'alive' waiting for connections */
	for(;;) {
		/* function to establish/accept connection with the client and verification */
		client_fd = cli_accept(server_fd);
		if(client_fd == -1) {
			fprintf(stderr, "%s\n", error);
			continue;
		}

		printf("Incoming connection!\n");
		/* generates a fork allowing multiple connections/multiple processes running at the same time */
		if(!fork()) client_connection(server_fd, client_fd);
	}
	
	return -1;
}
