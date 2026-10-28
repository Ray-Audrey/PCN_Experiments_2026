#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <string.h>

int main()
{
    int s, n, sum = 0;
    unsigned int x;
    unsigned char data[20], check;

    char result[100];

    struct sockaddr_in a = {0};

    struct timespec start, end;
    double rtt;

    s = socket(AF_INET, SOCK_STREAM, 0);

    a.sin_family = AF_INET;
    a.sin_port = htons(8081);
    inet_pton(AF_INET, "127.0.0.1", &a.sin_addr);

    if (connect(s, (struct sockaddr *)&a, sizeof(a)) < 0)
    {
        perror("Connection Failed");
        return 1;
    }

    printf("Number of words (1-20): ");
    scanf("%d", &n);

    if (n < 1 || n > 20)
        return 1;

    for (int i = 0; i < n; i++)
    {
        printf("Enter word %d (hex): ", i + 1);
        scanf("%x", &x);

        data[i] = x;

        sum += data[i];

        if (sum > 255)
            sum = (sum & 255) + 1;
    }

    check = ~sum;

    printf("\nGenerated Checksum: %02X\n", check);

    clock_gettime(CLOCK_MONOTONIC, &start);

    send(s, &n, sizeof(n), 0);
    send(s, data, n, 0);
    send(s, &check, 1, 0);

    int bytes_sent = sizeof(n) + n + sizeof(check);

    int r = recv(s, result, sizeof(result) - 1, 0);

    clock_gettime(CLOCK_MONOTONIC, &end);

    if (r > 0)
    {
        result[r] = '\0';
        printf("\nServer Result: %s\n", result);
    }

    rtt = (end.tv_sec - start.tv_sec) * 1000.0 +
          (end.tv_nsec - start.tv_nsec) / 1000000.0;

    printf("\n--- Sender Metrics ---\n");
    printf("Bytes Sent: %d\n", bytes_sent);
    printf("Packets Sent: 1\n");
    printf("Checksum Generated: %02X\n", check);

    printf("RTT: %.2f ms\n", rtt);

    printf("Throughput: %.2f bits/sec\n",
           rtt > 0 ?
           bytes_sent * 8.0 / (rtt / 1000.0) : 0);

    printf("PDR: %.2f%%\n",
           r > 0 ? 100.0 : 0.0);

    close(s);

    return 0;
}