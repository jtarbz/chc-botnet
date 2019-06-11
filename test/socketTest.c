#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main() {
    struct sockaddr_in server;
    int socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_desc == -1) puts("Could not create socket");

    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons(8081);

    // Connect
    if(connect(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0) {
        puts("fuck");
        return 1;
    }
    puts("connected");

    //Send data
    if(send(socket_desc, "pee pee poo poo", 15, 0) < 0) {
        puts("fuck");
        return 1;
    }
    puts("sent data");

    return 0;
}