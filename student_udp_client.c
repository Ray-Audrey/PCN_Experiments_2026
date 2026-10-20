#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>

int main()
{
    int client, n, attempts = 0, received = 0;
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    struct timeval timeout = {2, 0};
    struct timespec start, end;
    char roll[100], result[200];

    client = socket(AF_INET, SOCK_DGRAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(5001);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    setsockopt(client, SOL_SOCKET, SO_RCVTIMEO,
               &timeout, sizeof(timeout));

    printf("Enter Roll Number: ");
    fgets(roll, sizeof(roll), stdin);

    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < 3; i++)
    {
        attempts++;

        sendto(client, roll, strlen(roll), 0,
               (struct sockaddr *)&addr, sizeof(addr));

        n = recvfrom(client, result, sizeof(result)-1, 0,
                     (struct sockaddr *)&addr, &len);

        if (n > 0)
        {
            clock_gettime(CLOCK_MONOTONIC, &end);
            result[n] = '\0';
            printf("\n%s\n", result);
            received = 1;
            break;
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK)
            printf("Timeout! Retrying...\n");
        else
        {
            perror("recvfrom");
            break;
        }
    }

    if (received)
    {
        double rtt = (end.tv_sec - start.tv_sec) * 1000.0 +
                     (end.tv_nsec - start.tv_nsec) / 1000000.0;

        printf("RTT: %.2f ms\n", rtt);
        printf("Throughput: %.2f bits/sec\n",
               strlen(result) * 8.0 / (rtt / 1000.0));
    }
    else
        printf("No response received\n");

    printf("Successful response ratio: %.2f%% (%d/%d)\n",
           received * 100.0 / attempts, received, attempts);

    close(client);
    return 0;
}