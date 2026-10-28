#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main()
{
    int s, n;

    char bid[50];
    char line[200];
    char result[200];

    char id[50];
    char title[100];
    char status[50];

    struct sockaddr_in addr, client;
    socklen_t len = sizeof(client);

    s = socket(AF_INET, SOCK_DGRAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(5001);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(s, (struct sockaddr *)&addr, sizeof(addr));

    printf("Library Server Running...\n");

    while (1)
    {
        n = recvfrom(s, bid, sizeof(bid) - 1, 0,
                     (struct sockaddr *)&client, &len);

        bid[n] = '\0';

        int found = 0;

        FILE *fp = fopen("books.csv", "r");

        while (fp && fgets(line, sizeof(line), fp))
        {
            sscanf(line,
                   "%49[^,],%99[^,],%49[^\n]",
                   id, title, status);

            if (strcmp(id, bid) == 0)
            {
                snprintf(result,
                         sizeof(result),
                         "Book ID: %s\nTitle: %s\nStatus: %s",
                         id, title, status);

                found = 1;
                break;
            }
        }

        if (fp)
            fclose(fp);

        if (!found)
            strcpy(result, "Book Not Found");

        sendto(s, result, strlen(result), 0,
               (struct sockaddr *)&client, len);
    }

    close(s);
    return 0;
}