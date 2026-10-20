#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <stdint.h>

void *handle_client(void *arg)
{
    int client = (intptr_t)arg;
    char msg[100], line[500];

    recv(client, msg, sizeof(msg), 0);

    FILE *fp = fopen("./index.html", "r");

    if(fp != NULL)
    {
        while(fgets(line, sizeof(line), fp))
            send(client, line, strlen(line), 0);

        fclose(fp);
    }
    else
    {
        send(client, "File Not Found", 14, 0);
    }

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

    while(1)
    {
        client = accept(server, NULL, NULL);

        if(client >= 0)
        {
            pthread_create(&tid, NULL,
                           handle_client,
                           (void *)(intptr_t)client);

            pthread_detach(tid);
        }
    }
}