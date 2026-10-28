#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sys/time.h>

int main()
{
    int s, n, attempts = 0, received = 0, timeouts = 0;
    char roll[100], result[200];
    struct sockaddr_in addr;
    struct timeval tv = {2, 0};
    struct timespec start, end;

    s = socket(AF_INET, SOCK_DGRAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(5001);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    printf("Enter Roll Number: ");
    fgets(roll, sizeof(roll), stdin);
    roll[strcspn(roll, "\n")] = '\0';

    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < 3; i++)
    {
        attempts++;

        sendto(s, roll, strlen(roll), 0,
               (struct sockaddr *)&addr, sizeof(addr));

        n = recvfrom(s, result, sizeof(result) - 1, 0, NULL, NULL);

        if (n > 0)
        {
            clock_gettime(CLOCK_MONOTONIC, &end);
            result[n] = '\0';
            printf("\n%s\n", result);
            received = 1;
            break;
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            timeouts++;
            printf("Timeout! Retrying...\n");
        }
        else break;
    }

    printf("\n--- Performance Metrics ---\n");
    printf("Attempts: %d\n", attempts);
    printf("Timeouts: %d\n", timeouts);

    if (received)
    {
        double rtt = (end.tv_sec - start.tv_sec) * 1000.0 +
                     (end.tv_nsec - start.tv_nsec) / 1000000.0;

        printf("RTT: %.2f ms\n", rtt);
        printf("Throughput: %.2f bits/sec\n",
               rtt > 0 ? strlen(result) * 8.0 / (rtt / 1000.0) : 0.0);
    }
    else
        printf("No response received\n");

    printf("Successful response ratio: %.2f%% (%d/%d)\n",
           received * 100.0 / attempts, received, attempts);
    system("cat students.csv");

    close(s);
    return 0;
}