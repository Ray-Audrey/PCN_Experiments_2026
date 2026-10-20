
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

void *handle_client(void *arg)
{
    int client = (intptr_t)arg;
    char msg[100], line[500];
    int bytes = 0;
    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);

    int n = recv(client, msg, sizeof(msg), 0);

    FILE *fp = fopen("./index.html", "r");

    if (fp != NULL)
    {
        while (fgets(line, sizeof(line), fp))
        {
            int sent = send(client, line, strlen(line), 0);
            if (sent > 0)
                bytes += sent;
            else
                break;
        }
        fclose(fp);
    }
    else
    {
        bytes = send(client, "File Not Found", 14, 0);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    double time_ms = (end.tv_sec - start.tv_sec) * 1000.0 +
                     (end.tv_nsec - start.tv_nsec) / 1000000.0;

    printf("\n--- Client Metrics ---\n");
    printf("Request received: %s\n", n > 0 ? "Yes" : "No");
    printf("Response bytes sent: %d\n", bytes);
    printf("Processing time: %.2f ms\n", time_ms);

    if (time_ms > 0)
        printf("Throughput: %.2f bits/sec\n",
               bytes * 8.0 / (time_ms / 1000.0));

    close(client);
    return NULL;
}

int main()
{
    int server, client;
    struct sockaddr_in addr;
    pthread_t tid;

    server = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(5000);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server, (struct sockaddr *)&addr, sizeof(addr));
    listen(server, 5);

    printf("Web Server Running...\n");

    while (1)
    {
        client = accept(server, NULL, NULL);

        if (client >= 0)
        {
            pthread_create(&tid, NULL, handle_client,
                           (void *)(intptr_t)client);
            pthread_detach(tid);
        }
    }
}
