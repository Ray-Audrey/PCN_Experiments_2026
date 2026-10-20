#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main()
{
    int client, n, sent = 1, received = 0;
    struct sockaddr_in addr;
    struct timespec start, end;
    char roll[100], result[500];

    client = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(5000);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(client, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Connection failed");
        close(client);
        return 1;
    }

    printf("Enter Roll Number: ");
    fgets(roll, sizeof(roll), stdin);

    clock_gettime(CLOCK_MONOTONIC, &start);

    if (send(client, roll, strlen(roll), 0) > 0) {
        n = recv(client, result, sizeof(result) - 1, 0);
    } else {
        n = -1;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    double rtt = (end.tv_sec - start.tv_sec) * 1000.0 +
                 (end.tv_nsec - start.tv_nsec) / 1000000.0;

    if (n > 0) {
        result[n] = '\0';
        printf("\n%s\n", result);
        received = 1;
    } else {
        printf("No response received\n");
    }

    printf("\n--- Performance Metrics ---\n");
    printf("RTT: %.2f ms\n", rtt);
    printf("Approx. one-way latency (RTT/2): %.2f ms\n", rtt / 2.0);

    if (rtt > 0 && received)
        printf("Throughput: %.2f bits/sec\n",
               n * 8.0 / (rtt / 1000.0));
    else
        printf("Throughput: 0 bits/sec\n");

    printf("Successful response ratio: %.2f%%\n",
           received * 100.0 / sent);

    printf("Application-level PDR estimate: %.2f%%\n",
           received * 100.0 / sent);

    printf("Application-level failure ratio: %.2f%%\n",
           (sent - received) * 100.0 / sent);

    close(client);
    return 0;
}
