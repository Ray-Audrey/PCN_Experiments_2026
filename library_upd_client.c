#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main()
{
    int s, n;

    char bid[50];
    char result[200];

    struct sockaddr_in addr;

    struct timespec start, end;
    double rtt;

    s = socket(AF_INET, SOCK_DGRAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(5001);

    inet_pton(AF_INET,
              "127.0.0.1",
              &addr.sin_addr);

    printf("Enter Book ID: ");
    scanf("%s", bid);

    clock_gettime(CLOCK_MONOTONIC, &start);

    sendto(s, bid, strlen(bid), 0,
           (struct sockaddr *)&addr,
           sizeof(addr));

    n = recvfrom(s, result, sizeof(result) - 1,
                 0, NULL, NULL);

    clock_gettime(CLOCK_MONOTONIC, &end);

    result[n] = '\0';

    printf("\n%s\n", result);

    rtt =
        (end.tv_sec - start.tv_sec) * 1000.0 +
        (end.tv_nsec - start.tv_nsec) / 1000000.0;

    printf("\n--- Metrics ---\n");
    printf("RTT: %.2f ms\n", rtt);

    printf("Throughput: %.2f bits/sec\n",
           rtt > 0 ?
           strlen(result) * 8.0 / (rtt / 1000.0) : 0);

    printf("PDR: 100.00%%\n");

    close(s);

    return 0;
}