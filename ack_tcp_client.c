#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main()
{
    int s, n;
    int acks = 0, attempts = 0, timeouts = 0;

    char msg[10], reply[100];

    struct sockaddr_in addr;
    struct timespec start, end;

    s = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(5000);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    connect(s, (struct sockaddr *)&addr, sizeof(addr));

    clock_gettime(CLOCK_MONOTONIC, &start);

    for(int i = 1; i <= 5; i++)
    {
        sprintf(msg, "%d", i);

        attempts++;

        send(s, msg, strlen(msg), 0);

        n = recv(s, reply, sizeof(reply)-1, 0);

        if(n > 0)
        {
            reply[n] = '\0';

            if(i == 3 || i == 4)
            {
                printf("Timeout! Retransmitting Packet %d\n", i);

                timeouts++;
                attempts++;

                send(s, msg, strlen(msg), 0);

                recv(s, reply, sizeof(reply)-1, 0);
            }

            printf("Packet %d ACK Received\n", i);

            acks++;
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    double rtt =
        (end.tv_sec - start.tv_sec) * 1000.0 +
        (end.tv_nsec - start.tv_nsec) / 1000000.0;

    printf("\nPackets Sent: 5\n");
    printf("ACKs Received: %d\n", acks);
    printf("Attempts: %d\n", attempts);
    printf("Timeouts: %d\n", timeouts);

    printf("RTT: %.2f ms\n", rtt);

    printf("Throughput: %.2f bits/sec\n",
           (acks * 8.0) / (rtt / 1000.0));

    printf("Successful Delivery Ratio: %.2f%%\n",
           (acks * 100.0) / 5);

    printf("Attempt ACK Ratio: %.2f%%\n",
           (acks * 100.0) / attempts);

    close(s);

    return 0;
}