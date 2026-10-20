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
    int s, n, attempts = 0, ack = 0, timeouts = 0;
    char msg[] = "1|Hello\n", reply[100];
    struct sockaddr_in addr;
    struct timeval tv = {2, 0};
    struct timespec start, end;

    s = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(5000);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    connect(s, (struct sockaddr *)&addr, sizeof(addr));
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < 3; i++)
    {
        attempts++;
        send(s, msg, strlen(msg), 0);

        n = recv(s, reply, sizeof(reply) - 1, 0);

        if (n > 0)
        {
            reply[n] = '\0';
            if (strncmp(reply, "ACK", 3) == 0)
            {
                ack = 1;
                break;
            }
        }
        else if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            timeouts++;
            printf("Timeout! Retransmitting...\n");
        }
        else
            break;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    double rtt = (end.tv_sec - start.tv_sec) * 1000.0 +
                 (end.tv_nsec - start.tv_nsec) / 1000000.0;

    printf("\n--- Performance Metrics ---\n");
    printf("ACK: %s\n", ack ? "Received" : "Not received");
    printf("Attempts: %d\n", attempts);
    printf("Timeouts: %d\n", timeouts);
    printf("RTT: %.2f ms\n", rtt);

    if (ack && rtt > 0)
        printf("Throughput: %.2f bits/sec\n",
               strlen(msg) * 8.0 / (rtt / 1000.0));
    else
        printf("Throughput: 0 bits/sec\n");

    printf("Successful delivery ratio: %.2f%%\n", ack * 100.0);
    printf("Attempt ACK ratio: %.2f%%\n",
           ack * 100.0 / attempts);

    close(s);
    return 0;
}