#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>

int main()
{
    int s, n, sent = 0, received = 0;
    char msg[1024], reply[1100];
    struct sockaddr_in a = {0};
    struct timespec start, end;
    double total_rtt = 0;
    int bytes = 0;

    s = socket(AF_INET, SOCK_STREAM, 0);

    a.sin_family = AF_INET;
    a.sin_port = htons(5000);
    inet_pton(AF_INET, "127.0.0.1", &a.sin_addr);

    if (connect(s, (struct sockaddr *)&a, sizeof(a)) < 0)
    {
        perror("Connection failed");
        close(s);
        return 1;
    }

    printf("Connected to TCP Chat Server\n");

    while (1)
    {
        printf("You: ");
        if (!fgets(msg, sizeof(msg), stdin))
            break;

        sent++;

        clock_gettime(CLOCK_MONOTONIC, &start);
        send(s, msg, strlen(msg), 0);

        if (strcmp(msg, "exit\n") == 0 || strcmp(msg, "exit") == 0)
            break;

        n = recv(s, reply, sizeof(reply) - 1, 0);
        clock_gettime(CLOCK_MONOTONIC, &end);

        if (n <= 0)
            break;

        reply[n] = '\0';
        printf("%s", reply);

        received++;
        bytes += n;

        total_rtt += (end.tv_sec - start.tv_sec) * 1000.0 +
                     (end.tv_nsec - start.tv_nsec) / 1000000.0;
    }

    printf("\n--- TCP Metrics ---\n");
    printf("Messages Sent: %d\n", sent);
    printf("Replies Received: %d\n", received);
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