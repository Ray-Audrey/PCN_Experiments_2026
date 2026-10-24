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
    int s, n, sent = 0, received = 0, timeouts = 0, bytes = 0;
    char msg[1024], reply[1100];
    struct sockaddr_in a = {0};
    struct timeval tv = {2, 0};
    struct timespec start, end;
    double total_rtt = 0;

    s = socket(AF_INET, SOCK_DGRAM, 0);

    a.sin_family = AF_INET;
    a.sin_port = htons(5001);
    inet_pton(AF_INET, "127.0.0.1", &a.sin_addr);

    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    printf("Connected to UDP Chat Server\n");

    while (1)
    {
        printf("You: ");
        if (!fgets(msg, sizeof(msg), stdin))
            break;

        msg[strcspn(msg, "\r\n")] = '\0';
        sent++;

        clock_gettime(CLOCK_MONOTONIC, &start);

        sendto(s, msg, strlen(msg), 0,
               (struct sockaddr *)&a, sizeof(a));

        if (strcmp(msg, "exit") == 0)
            break;

        n = recvfrom(s, reply, sizeof(reply) - 1, 0, NULL, NULL);
        clock_gettime(CLOCK_MONOTONIC, &end);

        if (n < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                timeouts++;
                printf("Timeout: No reply received\n");
                continue;
            }
            break;
        }

        reply[n] = '\0';
        printf("%s\n", reply);

        received++;
        bytes += n;

        total_rtt += (end.tv_sec - start.tv_sec) * 1000.0 +
                     (end.tv_nsec - start.tv_nsec) / 1000000.0;
    }

    printf("\n--- UDP Metrics ---\n");
    printf("Messages Sent: %d\n", sent);
    printf("Replies Received: %d\n", received);
    printf("Timeouts: %d\n", timeouts);
    printf("Average RTT: %.2f ms\n",
           received ? total_rtt / received : 0);
    printf("Throughput: %.2f bits/sec\n",
           total_rtt > 0 ? bytes * 8.0 / (total_rtt / 1000.0) : 0);
    printf("Response Ratio: %.2f%%\n",
           sent ? received * 100.0 / sent : 0);
    printf("Application-level PDR Estimate: %.2f%%\n",
           sent ? received * 100.0 / sent : 0);

    close(s);
    return 0;
}