#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main()
{
    int s, c, n;
    char msg[100];
    struct sockaddr_in addr;

    s = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(5000);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(s, (struct sockaddr *)&addr, sizeof(addr));
    listen(s, 5);

    printf("TCP Server Waiting...\n");
    c = accept(s, NULL, NULL);

    while ((n = recv(c, msg, sizeof(msg)-1, 0)) > 0)
    {
        msg[n] = '\0';
        printf("Received: %s", msg);

        send(c, "ACK\n", 4, 0);
    }

    close(c);
    close(s);
    return 0;
}