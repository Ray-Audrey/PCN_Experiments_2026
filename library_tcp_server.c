#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main()
{
    int s, c, n;

    char bid[50];
    char line[200];
    char result[200];

    char id[50];
    char title[100];
    char status[50];

    struct sockaddr_in a = {0};

    s = socket(AF_INET, SOCK_STREAM, 0);

    a.sin_family = AF_INET;
    a.sin_port = htons(5000);
    a.sin_addr.s_addr = INADDR_ANY;

    bind(s, (struct sockaddr *)&a, sizeof(a));

    listen(s, 1);

    printf("Library Server Running...\n");

    c = accept(s, NULL, NULL);

    n = recv(c, bid, sizeof(bid) - 1, 0);

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

    send(c, result, strlen(result), 0);

    printf("\n--- Server Metrics ---\n");
    printf("Bytes Sent: %lu\n", strlen(result));
    printf("Packets Processed: 1\n");

    close(c);
    close(s);

    return 0;
}