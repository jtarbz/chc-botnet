#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <signal.h>

#define PORT 8081
#define MAX_CLIENTS 256
#define RECV_SIZE 1024

bool interrupt = false;	// this guy lets the main loop know when it's time to shut down ***gracefully***

/* custom signal handler for graceful shutdown purposes */
void handle_signal(int sig_type) {
	if(sig_type == SIGINT || sig_type == SIGTERM) {	// SIGPIPE should just be ignored, we don't care if some idiot disconnected
		printf("beginning clean server shutdown due to interrupt ...\n");
		interrupt = true;
	}

	return;
}

/* accept a client and add it to the epoll interest list  */
int accept_client(int server_socket, int epoll_fd) {
	int client_socket;
	unsigned int client_length;
	struct sockaddr_in client_addr;
	struct epoll_event client_event;

	/* here's the actual accepting part */
	if((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_length)) < 0) {
		fprintf(stderr, "failed to accept a client\n");
		return -1;
	}

	/* add client socket to epoll interest list */
	client_event.events = EPOLLIN;
	client_event.data.fd = client_socket;
	if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &client_event) < 0) {
		fprintf(stderr, "failed to add client to interest list");
		close(client_socket);
		return -1;
	}

	return client_socket;
}

/* receive snow day message from client and then handle it */
int handle_client(int client_socket) {
	unsigned char recv_buffer[RECV_SIZE];
	unsigned int recvd_msg_size = 0;

	/* receive only one message up to (RECV_SIZE - 1) bytes in size */
	do {
		if(interrupt == true) {	// implement graceful shutdown
			close(client_socket);
			return 0;
		}

		if((recvd_msg_size += recv(client_socket, recv_buffer + recvd_msg_size, RECV_SIZE - 1, 0)) == 0) {
			fprintf(stderr, "client socket has disconnected\n");
			return -1;
		}
	} while(recvd_msg_size < 0);

	recv_buffer[recvd_msg_size] = '\0';     // always end your sentence ;)
	printf("%s", recv_buffer);      // simple action for now, will change

	/* handler for snow days goes here */
	
	return 0;	
}

int main(void) {
	int server_socket, client_registry[MAX_CLIENTS], epoll_fd, number_fds, tmp_fd;
	struct sockaddr_in server_addr;
	struct epoll_event listener_event;
	struct epoll_event ready_sockets[MAX_CLIENTS + 1];	// leave room for the listener

	memset(client_registry, 0, sizeof(int) * MAX_CLIENTS);	// zero out registry to ensure things work smoothly in main loop

	/* set custom handler for SIGINT */
	struct sigaction handler;
	handler.sa_handler = handle_signal;
	if(sigfillset(&handler.sa_mask) < 0) {
		fprintf(stderr, "failed to set signal masks\n");
		exit(1);
	}

	handler.sa_flags = 0;	// no sa_flags
	if(sigaction(SIGINT, &handler, 0) < 0) {
		fprintf(stderr, "failed to set new handler for SIGINT\n");
		exit(1);
	}
	
	/* set custom handler for SIGTERM; uses same handler as SIGINT */
	if(sigaction(SIGTERM, &handler, 0) < 0) {
		fprintf(stderr, "failed to set new handler for SIGTERM\n");
		exit(1);
	}

	/* set custom handler for SIGPIPE; uses same handler as SIGINT but gets ignored */
	if(sigaction(SIGPIPE, &handler, 0) < 0) {
		fprintf(stderr, "failed to set new handler for SIGPIPE\n");
		exit(1);
	}

	/* spawn socket and prepare it for binding */
	if((server_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		fprintf(stderr, "failed to create server socket\n");
		exit(1);
	}

	if(setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {	// be vanquished, foul error!
		fprintf(stderr, "failed to set reusable option on listening socket\n");
		exit(1);
	}

	memset(&server_addr, 0, sizeof(struct sockaddr_in));	// zero out to prevent ~~BAD VOODOO~~
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(PORT);

	/* bind and listen */
	if(bind(server_socket, (struct sockaddr *)&server_addr, sizeof server_addr) < 0) {
		fprintf(stderr, "failed to bind socket to port\n");
		perror("guru meditation");
		exit(1);
	}

	if(listen(server_socket, 64) < 0) {
		fprintf(stderr, "server socket failed to listen\n");
		exit(1);
	}

	/* epoll setup */
	if((epoll_fd = epoll_create(MAX_CLIENTS + 1)) < 0) {	// size argument to epoll_create() is ignored, but required
		fprintf(stderr, "failed to create epoll file descriptor\n");
		close(server_socket);
		exit(1);
	}

	/* add listener to epoll interest list */
	listener_event.events = EPOLLIN;
	listener_event.data.fd = server_socket;
	if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &listener_event) < 0) {
		fprintf(stderr, "failed to add listener to interest list\n");
		close(epoll_fd);
		close(server_socket);
		exit(1);
	}

	/* main loop; blocks on epoll_wait() and then either accepts a new client or handles client data for "ready" sockets */
	while(interrupt != true) {
		/* block until an arbitrary amount of sockets are "ready" */
		if((number_fds = epoll_wait(epoll_fd, ready_sockets, MAX_CLIENTS + 1, -1)) < 0) {
			fprintf(stderr, "failed on epoll_wait() for some reason, probably due to SIGINT\n");
			continue;
		}

		/* loop through "ready" sockets */
		for(int i = 0; i < number_fds; ++i) {
			if((tmp_fd = ready_sockets[i].data.fd) == server_socket) {	// a new client is trying to connect
				if((tmp_fd = accept_client(server_socket, epoll_fd)) < 0) {
					fprintf(stderr, "failed to accept client socket\n");
					exit(1);
				}

				/* add new client to registry */
				for(int i = 0; i < MAX_CLIENTS; ++i) {
					if(client_registry[i] == 0) {	// this isn't ideal, but works for our purposes
						client_registry[i] = tmp_fd;
						break;
					}

					if(i == MAX_CLIENTS - 1) {	// execute if array is full
						send(tmp_fd, "too many clients! try again later ...\n", 48, 0);
						close(tmp_fd);
					}
				}
			}

			else {	// a client has sent data
				if(handle_client(tmp_fd) < 0) {
					/* remove client from registry */
					for(int i = 0; i < MAX_CLIENTS; ++i) {
						if(client_registry[i] == tmp_fd) {
							close(tmp_fd);
							client_registry[i] = 0;
							break;
						}
					}
				}
			}
		}
	}

	/* execute upon interrupt or epoll_wait() failure */
	for(int i = 0; i < MAX_CLIENTS; ++i) if(client_registry[i] != 0) close(client_registry[i]);	// clean up registered clients
	close(epoll_fd);
	close(server_socket);
	exit(0);
}
