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
    int s, n;
    char domain[100], result[500];

    struct sockaddr_in addr;
    struct timeval tv = {2,0};

    struct timespec start, end;

    s = socket(AF_INET, SOCK_DGRAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(5001);

    inet_pton(AF_INET,
              "127.0.0.1",
              &addr.sin_addr);

    setsockopt(s, SOL_SOCKET,
               SO_RCVTIMEO,
               &tv,
               sizeof(tv));

    printf("Enter Domain Name: ");
    fgets(domain, sizeof(domain), stdin);

    domain[strcspn(domain,"\n")] = '\0';

    clock_gettime(CLOCK_MONOTONIC,&start);

    sendto(s,
           domain,
           strlen(domain),
           0,
           (struct sockaddr *)&addr,
           sizeof(addr));

    n = recvfrom(s,
                 result,
                 sizeof(result)-1,
                 0,
                 NULL,
                 NULL);

    clock_gettime(CLOCK_MONOTONIC,&end);

    if(n > 0)
    {
        result[n] = '\0';

        printf("\n%s\n", result);

        double rtt =
            (end.tv_sec - start.tv_sec) * 1000.0 +
            (end.tv_nsec - start.tv_nsec) / 1000000.0;

        printf("\n--- DNS Metrics ---\n");
        printf("RTT: %.2f ms\n", rtt);

        printf("Throughput: %.2f bits/sec\n",
               strlen(result) * 8.0 /
               (rtt / 1000.0));
    }
    else
    {
        printf("DNS Query Timeout\n");
    }

    close(s);
}