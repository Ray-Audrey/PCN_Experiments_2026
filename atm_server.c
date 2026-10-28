#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main()
{
    int s, c, n;
    char req[100], result[200];
    char acc[20], name[50], bal[20];
    char id[20], line[200];

    struct sockaddr_in a = {0};

    s = socket(AF_INET, SOCK_STREAM, 0);

    a.sin_family = AF_INET;
    a.sin_port = htons(5000);
    a.sin_addr.s_addr = INADDR_ANY;

    bind(s, (struct sockaddr *)&a, sizeof(a));

    listen(s, 1);

    printf("ATM Server Running...\n");

    c = accept(s, NULL, NULL);

    n = recv(c, req, sizeof(req) - 1, 0);

    req[n] = '\0';

    int found = 0;

    FILE *fp = fopen("atm.csv", "r");

    while (fp && fgets(line, sizeof(line), fp))
    {
        sscanf(line,
               "%19[^,],%49[^,],%19[^\n]",
               id, name, bal);

        if (strcmp(id, req) == 0)
        {
            snprintf(result,
                     sizeof(result),
                     "Account Number : %s\nName : %s\nBalance : %s",
                     id, name, bal);

            found = 1;
            break;
        }
    }

    if (fp)
        fclose(fp);

    if (!found)
        strcpy(result, "Account Not Found");

    send(c, result, strlen(result), 0);

    printf("\n--- Server Metrics ---\n");
    printf("Bytes Sent: %lu\n", strlen(result));
    printf("Packets Processed: 1\n");

    close(c);
    close(s);

    return 0;
}