#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>

int main()
{
    int s, n;
    char domain[100], line[200], result[500];
    char d[100], ip[100];

    int hits = 0, misses = 0;

    struct sockaddr_in addr, client;
    socklen_t len = sizeof(client);

    s = socket(AF_INET, SOCK_DGRAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(5001);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(s, (struct sockaddr *)&addr, sizeof(addr));

    printf("DNS Server Running...\n");

    while (1)
    {
        n = recvfrom(s, domain, sizeof(domain)-1, 0,
                     (struct sockaddr *)&client, &len);

        if (n <= 0)
            continue;

        domain[n] = '\0';

        time_t now = time(NULL);

        int found = 0;

        FILE *fp = fopen("dns.csv", "r");

        if (fp)
        {
            while (fgets(line, sizeof(line), fp))
            {
                if (sscanf(line, "%99[^,],%99[^\n]",
                           d, ip) == 2)
                {
                    if (strcmp(domain, d) == 0)
                    {
                        hits++;

                        snprintf(result, sizeof(result),
                                 "Timestamp: %sCache HIT\nDomain: %s\nIP: %s",
                                 ctime(&now), d, ip);

                        found = 1;
                        break;
                    }
                }
            }

            fclose(fp);
        }

        if (!found)
        {
            misses++;

            snprintf(result, sizeof(result),
                     "Timestamp: %sCache MISS\nDomain: %s\nIP Not Found",
                     ctime(&now), domain);
        }

        sendto(s, result, strlen(result), 0,
               (struct sockaddr *)&client, len);

        printf("\nHits: %d  Misses: %d\n",
               hits, misses);
    }
}