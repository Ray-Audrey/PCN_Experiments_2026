#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main()
{
    int s, n;

    char cid[50];
    char line[200];
    char result[200];

    char id[50], name[100], product[100];

    struct sockaddr_in addr, client;
    socklen_t len = sizeof(client);

    s = socket(AF_INET, SOCK_DGRAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(5001);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(s, (struct sockaddr *)&addr, sizeof(addr));

    printf("Customer Product Server Running...\n");

    while (1)
    {
        n = recvfrom(s, cid, sizeof(cid)-1, 0,
                     (struct sockaddr *)&client, &len);

        cid[n] = '\0';

        int found = 0;

        FILE *fp = fopen("customer_product.csv", "r");

        while (fp && fgets(line, sizeof(line), fp))
        {
            sscanf(line,
                   "%49[^,],%99[^,],%99[^\n]",
                   id, name, product);

            if (strcmp(id, cid) == 0)
            {
                snprintf(result, sizeof(result),
                         "Customer ID: %s\nName: %s\nProduct: %s",
                         id, name, product);

                found = 1;
                break;
            }
        }

        if (fp)
            fclose(fp);

        if (!found)
            strcpy(result, "Customer Not Found");

        sendto(s, result, strlen(result), 0,
               (struct sockaddr *)&client, len);
    }

    close(s);
    return 0;
}