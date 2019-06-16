#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

// Standard error function because I am lazy
void frick(char *msg) {
    puts(msg);
    exit(1);
}

void handle_client(int csock) {
    char msg[512];
    recv(csock, msg, 512, 0);
    puts(msg);
}

int main(void) {
    int csock, ssock;
    struct sockaddr_in caddr, saddr;
    unsigned int clength;
    /* socket stuff */
    if((ssock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) frick("Failed to create socket");

    memset(&saddr, 0, sizeof saddr);
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    saddr.sin_port = htons(8081);

    if(bind(ssock, (struct sockaddr *)&saddr, sizeof saddr) < 0) frick("Failed to bind socket to port");
    if(listen(ssock, 5) < 0) frick("Failed to listen for incoming connections");

    clength = sizeof caddr;
    if((csock = accept(ssock, (struct sockaddr *)&caddr, &clength)) < 0) frick("Failed to accept a connection");
    printf("Handling client %s\n", inet_ntoa(caddr.sin_addr));
    handle_client(csock);   // Not multithreaded because I am lazy and this is a minimal working example

    close(csock);
    close(ssock);
    exit(0);
}