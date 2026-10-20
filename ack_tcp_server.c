#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdint.h>

void *handle_client(void *arg)
{
    int client = (intptr_t)arg;
    int n;
    char msg[100];

    while ((n = recv(client, msg, 99, 0)) > 0)
    {
        msg[n] = '\0';

        printf("Received: %s", msg);

        send(client, "ACK\n", 4, 0);
    }

    close(client);
    return NULL;
}

int main()
{
    int server, client;
    pthread_t tid;
    struct sockaddr_in addr = {0};

    server = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(5000);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server, (struct sockaddr *)&addr, sizeof(addr));
    listen(server, 5);

    printf("Server Waiting...\n");

    while (1)
    {
        client = accept(server, NULL, NULL);

        if (client >= 0)
        {
            pthread_create(&tid, NULL,
                           handle_client,
                           (void *)(intptr_t)client);

            pthread_detach(tid);
        }
    }

    close(server);
    return 0;
}