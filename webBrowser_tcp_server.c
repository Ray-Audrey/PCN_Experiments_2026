#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>

void *handle(void *arg)
{
    int c = (intptr_t)arg, n, bytes = 0;
    char msg[100], line[500];
    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);
    n = recv(c, msg, sizeof(msg), 0);

    FILE *fp = fopen("index.html", "r");

    if (fp)
    {
        while (fgets(line, sizeof(line), fp))
        {
            int sent = send(c, line, strlen(line), 0);
            if (sent <= 0) break;
            bytes += sent;
        }
        fclose(fp);
    }
    else
        bytes = send(c, "File Not Found", 14, 0);

    clock_gettime(CLOCK_MONOTONIC, &end);

    double ms = (end.tv_sec - start.tv_sec) * 1000.0 +
                (end.tv_nsec - start.tv_nsec) / 1000000.0;

    printf("\n--- Server Metrics ---\n");
    printf("Request received: %s\n", n > 0 ? "Yes" : "No");
    printf("Response bytes sent: %d\n", bytes);
    printf("Processing time: %.2f ms\n", ms);
    printf("Throughput: %.2f bits/sec\n",
           ms > 0 ? bytes * 8.0 / (ms / 1000.0) : 0.0);

    close(c);
    return NULL;
}

int main()
{
    int s, c;
    struct sockaddr_in addr;
    pthread_t t;

    s = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(5000);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(s, (struct sockaddr *)&addr, sizeof(addr));
    listen(s, 5);

    printf("Web Server Running...\n");

    while (1)
    {
        c = accept(s, NULL, NULL);
        if (c >= 0)
        {
            pthread_create(&t, NULL, handle, (void *)(intptr_t)c);
            pthread_detach(t);
        }
    }
}