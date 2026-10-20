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
    char result[5000];

    client = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(5000);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    connect(client, (struct sockaddr *)&addr, sizeof(addr));

    clock_gettime(CLOCK_MONOTONIC, &start);

    send(client, "GET", 3, 0);

    n = recv(client, result, sizeof(result)-1, 0);

    clock_gettime(CLOCK_MONOTONIC, &end);

    if(n > 0)
    {
        result[n] = '\0';

        FILE *fp = fopen("downloaded.html", "w");

        if(fp != NULL)
        {
            fwrite(result, 1, n, fp);
            fclose(fp);
        }

        printf("Web Page Downloaded Successfully\n");

        double rtt =
            (end.tv_sec - start.tv_sec) * 1000.0 +
            (end.tv_nsec - start.tv_nsec) / 1000000.0;

        printf("RTT: %.2f ms\n", rtt);

        printf("Throughput: %.2f bits/sec\n",
               n * 8.0 / (rtt / 1000.0));
    }
    else
    {
        printf("Download Failed\n");
    }

    close(client);
    return 0;
}