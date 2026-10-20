#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <strings.h>
#include <errno.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>

int main()
{
    int s, n, queries = 0, cache_count = 0;
    int hits = 0, timeouts = 0;
    char domain[100], result[100];
    char names[5][100], ips[5][100];

    struct sockaddr_in a = {0};
    struct timeval tv = {2, 0};
    struct timespec start, end;

    s = socket(AF_INET, SOCK_DGRAM, 0);
    a.sin_family = AF_INET;
    a.sin_port = htons(5353);
    inet_pton(AF_INET, "127.0.0.1", &a.sin_addr);
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    while (1)
    {
        printf("\nEnter domain (exit to stop): ");
        scanf("%99s", domain);
        if (!strcmp(domain, "exit")) break;

        queries++;
        int found = 0;

        for (int i = 0; i < cache_count; i++)
            if (!strcasecmp(domain, names[i]))
            {
                strcpy(result, ips[i]);
                found = 1;
                hits++;
                break;
            }

        if (found)
            printf("Cache Hit\n");
        else
        {
            printf("Cache Miss\n");
            clock_gettime(CLOCK_MONOTONIC, &start);

            sendto(s, domain, strlen(domain), 0,
                   (struct sockaddr *)&a, sizeof(a));
            n = recvfrom(s, result, 99, 0, NULL, NULL);

            clock_gettime(CLOCK_MONOTONIC, &end);

            if (n < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    timeouts++;
                printf("Timeout or receive error\n");
                continue;
            }

            result[n] = '\0';

            double rtt = (end.tv_sec - start.tv_sec) * 1000.0 +
                         (end.tv_nsec - start.tv_nsec) / 1000000.0;

            printf("RTT: %.2f ms\n", rtt);
            printf("Throughput: %.2f bits/sec\n",
                   rtt > 0 ? n * 8.0 / (rtt / 1000.0) : 0.0);

            if (strcmp(result, "NXDOMAIN") &&
                strcmp(result, "INVALID") && cache_count < 5)
            {
                strcpy(names[cache_count], domain);
                strcpy(ips[cache_count], result);
                cache_count++;
            }
        }

        printf("Result: %s\n", result);
    }

    printf("\n--- Metrics ---\n");
    printf("Queries: %d\nCache Hits: %d\n", queries, hits);
    printf("Timeouts: %d\n", timeouts);
    printf("Cache Hit Ratio: %.2f%%\n",
           queries ? hits * 100.0 / queries : 0.0);

    close(s);
    return 0;
}