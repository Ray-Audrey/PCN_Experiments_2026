
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/time.h>

int main()
{
    int s, n, count = 0;
    char domain[100], result[100];
    char cache_name[5][100], cache_ip[5][100];

    struct sockaddr_in server;
    struct timeval tv = {2, 0};
    socklen_t len = sizeof(server);

    s = socket(AF_INET, SOCK_DGRAM, 0);

    server.sin_family = AF_INET;
    server.sin_port = htons(5353);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    while (1)
    {
        printf("\nEnter domain (exit to stop): ");
        scanf("%99s", domain);

        if (strcmp(domain, "exit") == 0)
            break;

        int found = 0;

        for (int i = 0; i < count; i++)
        {
            if (strcasecmp(domain, cache_name[i]) == 0)
            {
                strcpy(result, cache_ip[i]);
                found = 1;
                printf("Cache Hit\n");
                break;
            }
        }

        if (!found)
        {
            printf("Cache Miss\n");

            sendto(s, domain, strlen(domain), 0,
                   (struct sockaddr *)&server, len);

            n = recvfrom(s, result, sizeof(result)-1, 0, NULL, NULL);

            if (n < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    printf("Timeout!\n");
                else
                    perror("recvfrom");
                continue;
            }

            result[n] = '\0';

            if (strcmp(result, "NXDOMAIN") != 0 &&
                strcmp(result, "INVALID") != 0 && count < 5)
            {
                strcpy(cache_name[count], domain);
                strcpy(cache_ip[count], result);
                count++;
            }
        }

        if (strcmp(result, "NXDOMAIN") == 0)
            printf("NXDOMAIN: Domain not found\n");
        else if (strcmp(result, "INVALID") == 0)
            printf("Invalid domain format\n");
        else
            printf("IP Address: %s\n", result);
    }

    close(s);
    return 0;
}
