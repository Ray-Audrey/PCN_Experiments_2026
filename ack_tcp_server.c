#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

int main()
{
    int s, c, n;
    char msg[100];
    struct sockaddr_in a = {0};

    s = socket(AF_INET, SOCK_STREAM, 0);
    a.sin_family = AF_INET;
    a.sin_port = htons(5000);
    a.sin_addr.s_addr = INADDR_ANY;

    bind(s, (struct sockaddr *)&a, sizeof(a));
    listen(s, 5);

    printf("Server Waiting...\n");
    c = accept(s, NULL, NULL);

    while ((n = recv(c, msg, 99, 0)) > 0)
    {
        msg[n] = '\0';
        printf("Received: %s", msg);
        send(c, "ACK\n", 4, 0);
    }

    close(c);
    close(s);
}