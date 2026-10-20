#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main()
{
    int s, n, sent = 1, received = 0;
    char roll[100], result[500];
    struct sockaddr_in a;
    struct timespec start, end;

    s = socket(AF_INET, SOCK_STREAM, 0);

    a.sin_family = AF_INET;
    a.sin_port = htons(5000);
    inet_pton(AF_INET, "127.0.0.1", &a.sin_addr);

    if (connect(s, (struct sockaddr *)&a, sizeof(a)) < 0) {
        perror("Connection failed");
        close(s);
        return 1;
    }

    printf("Enter Roll Number: ");
    fgets(roll, sizeof(roll), stdin);

    clock_gettime(CLOCK_MONOTONIC, &start);
    send(s, roll, strlen(roll), 0);
    n = recv(s, result, sizeof(result) - 1, 0);
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
    printf("Approx. one-way latency: %.2f ms\n", rtt / 2);

    printf("Throughput: %.2f bits/sec\n",
           (received && rtt > 0) ? n * 8.0 / (rtt / 1000.0) : 0.0);

    printf("Successful response ratio: %.2f%%\n", received * 100.0);
    printf("Application-level PDR estimate: %.2f%%\n",
           received * 100.0);
    printf("Application-level failure ratio: %.2f%%\n",
           (sent - received) * 100.0);

    close(s);
    return 0;
}