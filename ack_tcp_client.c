#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/time.h>

int main()
{
    int s, n, attempts = 0, ack = 0, timeouts = 0;
    char msg[] = "1|Hello\n", reply[100];
    struct sockaddr_in a = {0};
    struct timeval tv = {2, 0};
    struct timespec start, end;

    s = socket(AF_INET, SOCK_STREAM, 0);
    a.sin_family = AF_INET;
    a.sin_port = htons(5000);
    inet_pton(AF_INET, "127.0.0.1", &a.sin_addr);

    connect(s, (struct sockaddr *)&a, sizeof(a));
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < 3; i++)
    {
        attempts++;
        send(s, msg, strlen(msg), 0);
        n = recv(s, reply, 99, 0);

        if (n > 0 && strncmp(reply, "ACK", 3) == 0)
        {
            ack = 1;
            break;
        }

        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
        {
            timeouts++;
            printf("Timeout! Retrying...\n");
        }
        else if (n <= 0)
            break;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    double rtt = (end.tv_sec - start.tv_sec) * 1000.0 +
                 (end.tv_nsec - start.tv_nsec) / 1000000.0;

    printf("\nACK: %s\n", ack ? "Received" : "Not received");
    printf("Attempts: %d\nTimeouts: %d\n", attempts, timeouts);
    printf("RTT: %.2f ms\n", rtt);
    printf("Throughput: %.2f bits/sec\n",
           ack && rtt > 0 ? strlen(msg) * 8.0 / (rtt / 1000.0) : 0.0);
    printf("Successful delivery ratio: %.2f%%\n", ack * 100.0);
    printf("Attempt ACK ratio: %.2f%%\n", ack * 100.0 / attempts);

    close(s);
    return 0;
}