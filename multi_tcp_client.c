#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main()
{
    int s, n;
    char msg[1024], reply[1100];
    struct sockaddr_in a = {0}, client;
    socklen_t len;

    s = socket(AF_INET, SOCK_DGRAM, 0);

    a.sin_family = AF_INET;
    a.sin_port = htons(5001);
    a.sin_addr.s_addr = INADDR_ANY;

    bind(s, (struct sockaddr *)&a, sizeof(a));

    printf("UDP Chat Server Running on port 5001...\n");

    while (1)
    {
        len = sizeof(client);
        n = recvfrom(s, msg, sizeof(msg) - 1, 0,
                     (struct sockaddr *)&client, &len);

        if (n <= 0)
            continue;

        msg[n] = '\0';

        if (strcmp(msg, "exit") == 0)
            continue;

        printf("Client %s:%d: %s\n",
               inet_ntoa(client.sin_addr),
               ntohs(client.sin_port), msg);

        snprintf(reply, sizeof(reply), "Server reply: %s", msg);

        sendto(s, reply, strlen(reply), 0,
               (struct sockaddr *)&client, len);
    }

    close(s);
    return 0;
}