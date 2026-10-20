#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main()
{
    int s, n;
    char result[5000];
    struct sockaddr_in addr;
    struct timespec start, end;

    s = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(5000);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    connect(s, (struct sockaddr *)&addr, sizeof(addr));

    clock_gettime(CLOCK_MONOTONIC, &start);
    send(s, "GET", 3, 0);
    n = recv(s, result, sizeof(result) - 1, 0);
    clock_gettime(CLOCK_MONOTONIC, &end);

    if (n > 0)
    {
        result[n] = '\0';

        FILE *fp = fopen("downloaded.html", "w");
        if (fp)
        {
            fwrite(result, 1, n, fp);
            fclose(fp);
        }

        double rtt = (end.tv_sec - start.tv_sec) * 1000.0 +
                     (end.tv_nsec - start.tv_nsec) / 1000000.0;

        printf("Web Page Downloaded Successfully\n");
        printf("RTT: %.2f ms\n", rtt);
        printf("Throughput: %.2f bits/sec\n",
               rtt > 0 ? n * 8.0 / (rtt / 1000.0) : 0.0);
    }
    else
        printf("Download Failed\n");

    close(s);
    return 0;
}