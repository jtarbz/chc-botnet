#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PINGINT 2
bool interrupt = false;
unsigned int thr_cnt = 0;   // Thread count

/* Thread arguments structure; allows for future functionality expansion */
struct thread_args {
    int csock;
};

/* Standard error function because I am lazy */
void frick(char *msg) {
    puts(msg);
    exit(1);
}

void handle_interrupt(int sig_type) {
    printf("Beginning server shutdown ...\n");
    interrupt = true;
}

/* Modularized client connection function */
int accept_client(int ssock) {
    int csock;
    struct sockaddr_in caddr;
    unsigned int clength;

    clength = sizeof caddr;
    if((csock = accept(ssock, (struct sockaddr *)&caddr, &clength)) < 0) frick("Failed to accept a connection");
    printf("Handling client %s\n", inet_ntoa(caddr.sin_addr));
    return csock;
}

/* Handle data incoming from the client; this runs in an infinite loop per-thread */
void handle_client(int csock) {
    char msg[512];
    for(;;) {
        /* Check if client is still alive after ping period */
        if(recv(csock, msg, 511, MSG_PEEK | MSG_DONTWAIT) == 0) {
            close(csock);
            printf("A client disconnected!\n");   // I don't feel like passing the client address through like three functions right now
            break;
        }
        /* Check if SIGINT has been received */
        if(interrupt == true) {
            close(csock);
            break;
        }

        /* Receive client message */
        int n_bytes = recv(csock, msg, 511, 0);
        msg[n_bytes] = '\0';
        puts(msg);
        /* Handler for snowdays eventually */
        sleep(PINGINT);   // Wait on ping interval
    }
}

void *thread_main(void *thrargs) {
    int csock;  // maybe fix this ambiguity too? ehh whatever
    pthread_detach(pthread_self()); // Ensures thr resources are reallocated on return
    csock = ((struct thread_args *)thrargs) -> csock;
    free(thrargs);
    handle_client(csock);
    thr_cnt--;
    return NULL;
}

int main(void) {
    int csock, ssock;
    struct sockaddr_in saddr;
    pthread_t tid;
    struct thread_args *thrargs;

    /* Set custom handler for SIGINT */
    struct sigaction handler;
    handler.sa_handler = handle_interrupt;
    if(sigfillset(&handler.sa_mask) < 0) frick("Failed to set signal masks");
    handler.sa_flags = 0;   // No sa_flags
    if(sigaction(SIGINT, &handler, 0) < 0) frick("Failed to set the new handler for SIGINT");

    /* create listening socket */
    if((ssock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) frick("Failed to create socket");

    /* Set addressing information */
    memset(&saddr, 0, sizeof saddr);    // Prevents ~~~BAD VOODOO~~~
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    saddr.sin_port = htons(8081);

    /* Bind! And listen! */
    if(bind(ssock, (struct sockaddr *)&saddr, sizeof saddr) < 0) frick("Failed to bind socket to port");
    if(listen(ssock, 5) < 0) frick("Failed to listen for incoming connections");

    /* Begin fun multithreading loop*/
    for(;;) {
        if(interrupt == false) {
            csock = accept_client(ssock);
            /* Make memory space for client arg */
            if((thrargs = malloc(sizeof(struct thread_args))) == NULL) frick("Failed to create memory space for new client args");
            thrargs -> csock = csock;   // Maybe differentiate better between thrargs client socket and the client socket passed to it?
            /* Make client thread */
            if(pthread_create(&tid, NULL, thread_main, (void *)thrargs) < 0) frick("Failed on pthread_create");
            thr_cnt++;
        }
        else if (thr_cnt == 0) break;
    }

    close(csock);
    close(ssock);
    free(thrargs);
    exit(0);
}
