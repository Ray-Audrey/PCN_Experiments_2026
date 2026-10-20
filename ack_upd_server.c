#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main()
{
    int s, n;
    char msg[100];
    struct sockaddr_in addr, client;
    socklen_t len = sizeof(client);

    s = socket(AF_INET, SOCK_DGRAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(5001);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(s, (struct sockaddr *)&addr, sizeof(addr));
    printf("UDP Server Waiting...\n");

    while (1)
    {
        n = recvfrom(s, msg, sizeof(msg)-1, 0,
                     (struct sockaddr *)&client, &len);

        if (n > 0)
        {
            msg[n] = '\0';
            printf("Received: %s", msg);

            sendto(s, "ACK", 3, 0,
                   (struct sockaddr *)&client, len);
        }
    }

    close(s);
    return 0;
}