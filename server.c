#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

struct thread_args {
    int csock;
};

/* Standard error function because I am lazy */
void frick(char *msg) {
    puts(msg);
    exit(1);
}

int accept_client(int ssock) {
    int csock;
    struct sockaddr_in caddr;
    unsigned int clength;

    clength = sizeof caddr;
    if((csock = accept(ssock, (struct sockaddr *)&caddr, &clength)) < 0) frick("Failed to accept a connection");
    printf("Handling client %s\n", inet_ntoa(caddr.sin_addr));
    return csock;
}

void handle_client(int csock) {
    char msg[512];
    for(;;) {
        int n_bytes = recv(csock, msg, 511, 0);
        msg[n_bytes] = '\0';
        puts(msg);
        /* Handler for snowdays eventually */
    }
}

void *thread_main(void *thrargs) {
    int csock;  // maybe fix this ambiguity too? ehh whatever
    pthread_detach(pthread_self()); // Ensures thr resources are reallocated on return
    csock = ((struct thread_args *)thrargs) -> csock;
    free(thrargs);
    handle_client(csock);
    return NULL;
}


int main(void) {
    int csock, ssock;
    struct sockaddr_in saddr;
    pthread_t tid;
    struct thread_args *thrargs;

    /* create listening socket */
    if((ssock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) frick("Failed to create socket");

    memset(&saddr, 0, sizeof saddr);
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    saddr.sin_port = htons(8081);

    if(bind(ssock, (struct sockaddr *)&saddr, sizeof saddr) < 0) frick("Failed to bind socket to port");
    if(listen(ssock, 5) < 0) frick("Failed to listen for incoming connections");

    /* begin fun multithreading */
    for(;;) {
        csock = accept_client(ssock);
        /* make memory space for client arg */
        if((thrargs = malloc(sizeof(struct thread_args))) == NULL) frick("Failed to create memory space for new client args");
        thrargs -> csock = csock;   // Maybe differentiate better between thrargs client socket and the client socket passed to it?
        /* make client thread */
        if(pthread_create(&tid, NULL, thread_main, (void *)thrargs) < 0) frick("Failed on pthread_create");
    }

    /* never reached */
    close(csock);
    close(ssock);
    exit(0);
}
