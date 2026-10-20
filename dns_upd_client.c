#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>

int main()
{
    int s, n, queries = 0, hits = 0, timeouts = 0;
    char domain[100], result[100];
    char names[5][100], ips[5][100];
    struct sockaddr_in server;
    struct timeval tv = {2, 0};
    struct timespec start, end;

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

        queries++;
        int found = 0;

        for (int i = 0; i < hits; i++)
        {
            if (strcasecmp(domain, names[i]) == 0)
            {
                strcpy(result, ips[i]);
                found = 1;
                break;
            }
        }

        if (found)
            printf("Cache Hit\n");
        else
        {
            printf("Cache Miss\n");
            clock_gettime(CLOCK_MONOTONIC, &start);

            sendto(s, domain, strlen(domain), 0,
                   (struct sockaddr *)&server, sizeof(server));

            n = recvfrom(s, result, sizeof(result) - 1, 0, NULL, NULL);
            clock_gettime(CLOCK_MONOTONIC, &end);

            if (n < 0)
            {
                printf("Timeout!\n");
                timeouts++;
                continue;
            }

            result[n] = '\0';

            double rtt = (end.tv_sec - start.tv_sec) * 1000.0 +
                         (end.tv_nsec - start.tv_nsec) / 1000000.0;

            printf("RTT: %.2f ms\n", rtt);

            if (rtt > 0)
                printf("Throughput: %.2f bits/sec\n",
                       n * 8.0 / (rtt / 1000.0));

            if (strcmp(result, "NXDOMAIN") != 0 &&
                strcmp(result, "INVALID") != 0 && hits < 5)
            {
                strcpy(names[hits], domain);
                strcpy(ips[hits], result);
                hits++;
            }
        }

        printf("Result: %s\n", result);
    }

    printf("\n--- Metrics ---\n");
    printf("Queries: %d\n", queries);
    printf("Cache Entries: %d\n", hits);
    printf("Timeouts: %d\n", timeouts);
    printf("Cache Hit Ratio: %.2f%%\n",
           queries ? (queries - (hits + timeouts)) * 100.0 / queries : 0.0);

    close(s);
    return 0;
}