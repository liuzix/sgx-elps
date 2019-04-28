// Client side C/C++ program to demonstrate Socket programming
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#define PORT 81

int main(int argc, char const *argv[]) {
    struct sockaddr_in address;
    int sock = 0, valread, valwrite;
    struct sockaddr_in serv_addr;
    char *hello = "Hello from client\n";
    char buffer[1024] = {0};

    errno = 0;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }
    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    int res = connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (res < 0) {
        printf("\nConnection Failed--[%d]\n", errno);
        return -1;
    }
    if ((res = send(sock, hello, strlen(hello), 0)) < 0) {
        printf("Send failed--[%d]\n", errno);
        return -1;
    }

// Alternative:
    if ((valwrite = write(sock, hello, strlen(hello))) < 0) {
        printf("Write failed--[%d]\n", errno);
        return -1;
    }

    printf("Hello message sent\n");
    if ((valread = read(sock, buffer, 1024)) < 0) {
        printf("Read failed--[%d]\n", errno);
        return -1;
    } else
        printf("Read %d words\n", valread);

    printf("%s\n", buffer);
   return 0;
}
