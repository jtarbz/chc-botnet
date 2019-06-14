#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

void frick(char *msg) { // Standard error function because I am lazy
    perror(msg);
    exit(1);
}

int main() {
    char *echo_string = (char *)malloc(64);
    strncpy(echo_string, "This is an echo string", 22);

    struct sockaddr_in server;
    int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sock == -1) puts("Could not create socket");

    memset(&server, 0, sizeof(server)); // Zero out addressing to avoid ~~~bad voodoo~~~
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_port = htons(8081);

    // Connect
    if(connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) frick("frick");
    puts("connected");

    //Send data
    if(send(sock, echo_string, strlen(echo_string), 0) < 0) frick("frick");
    puts("sent data");

    close(sock);
    exit(0);
}