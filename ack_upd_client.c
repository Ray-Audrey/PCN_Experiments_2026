#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/time.h>

int main()
{
    int s, n, attempts = 0, ack = 0, timeouts = 0;
    char msg[] = "1|Hello", reply[100];
    struct sockaddr_in addr;
    struct timeval tv = {2, 0};
    socklen_t len = sizeof(addr);
    struct timespec start, end;

    s = socket(AF_INET, SOCK_DGRAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(5001);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < 3; i++)
    {
        attempts++;

        sendto(s, msg, strlen(msg), 0,
               (struct sockaddr *)&addr, len);

        n = recvfrom(s, reply, sizeof(reply)-1, 0,
                     (struct sockaddr *)&addr, &len);

        if (n > 0 && strncmp(reply, "ACK", 3) == 0)
        {
            ack = 1;
            break;
        }
        else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
        {
            timeouts++;
            printf("Timeout! Retransmitting...\n");
        }
        else if (n < 0)
            break;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    double rtt = (end.tv_sec - start.tv_sec) * 1000.0 +
                 (end.tv_nsec - start.tv_nsec) / 1000000.0;

    printf("ACK: %s\n", ack ? "Received" : "Not received");
    printf("Attempts: %d\n", attempts);
    printf("Timeouts: %d\n", timeouts);
    printf("RTT: %.2f ms\n", rtt);

    if (rtt > 0)
        printf("Throughput: %.2f bits/sec\n",
               ack * strlen(msg) * 8.0 / (rtt / 1000.0));

    printf("Successful delivery ratio: %.2f%%\n", ack * 100.0);
    printf("Attempt ACK ratio: %.2f%%\n",
           ack * 100.0 / attempts);

    close(s);
    return 0;
}