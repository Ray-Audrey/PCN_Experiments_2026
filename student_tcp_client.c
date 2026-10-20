#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main()
{
    int client, n;
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
    send(client, roll, strlen(roll), 0);
    n = recv(client, result, sizeof(result) - 1, 0);
    clock_gettime(CLOCK_MONOTONIC, &end);

    if (n > 0) {
        result[n] = '\0';
        printf("\n%s\n", result);

        double rtt = (end.tv_sec - start.tv_sec) * 1000.0 +
                     (end.tv_nsec - start.tv_nsec) / 1000000.0;

        printf("RTT: %.2f ms\n", rtt);
        printf("Throughput: %.2f bits/sec\n",
               n * 8.0 / (rtt / 1000.0));
        printf("Successful responses: 100%% (1/1)\n");
    } else {
        printf("No response received\n");
        printf("Successful responses: 0%% (0/1)\n");
    }

    close(client);
    return 0;
}
