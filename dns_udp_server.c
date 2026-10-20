
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main()
{
    int s, n;
    char domain[100], result[100];

    char *names[] = {"google.com", "youtube.com", "ssn.edu.in",
                     "amazon.com", "wikipedia.org"};

    char *ips[] = {"142.250.183.14", "142.250.190.14",
                   "203.0.113.10", "205.251.242.103",
                   "208.80.154.224"};

    struct sockaddr_in server, client;
    socklen_t len = sizeof(client);

    s = socket(AF_INET, SOCK_DGRAM, 0);

    server.sin_family = AF_INET;
    server.sin_port = htons(5353);
    server.sin_addr.s_addr = INADDR_ANY;

    bind(s, (struct sockaddr *)&server, sizeof(server));

    printf("DNS Server Running...\n");

    while (1)
    {
        n = recvfrom(s, domain, sizeof(domain)-1, 0,
                     (struct sockaddr *)&client, &len);

        if (n <= 0)
            continue;

        domain[n] = '\0';

        if (strchr(domain, '.') == NULL)
            strcpy(result, "INVALID");
        else
        {
            strcpy(result, "NXDOMAIN");

            for (int i = 0; i < 5; i++)
            {
                if (strcasecmp(domain, names[i]) == 0)
                {
                    strcpy(result, ips[i]);
                    break;
                }
            }
        }

        sendto(s, result, strlen(result), 0,
               (struct sockaddr *)&client, len);
    }

    close(s);
    return 0;
}
