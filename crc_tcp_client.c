#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

int main()
{
    int s;

    char data[50];
    char divisor[20];
    char codeword[50];
    char remainder[20];
    char result[100];

    struct sockaddr_in a = {0};

    struct timespec start, end;
    double rtt;

    s = socket(AF_INET, SOCK_STREAM, 0);

    a.sin_family = AF_INET;
    a.sin_port = htons(8081);

    inet_pton(AF_INET,
              "127.0.0.1",
              &a.sin_addr);

    connect(s,
            (struct sockaddr *)&a,
            sizeof(a));

    printf("Enter Data: ");
    scanf("%s", data);

    printf("Enter Divisor: ");
    scanf("%s", divisor);

    strcpy(codeword, data);

    int dl = strlen(data);
    int pl = strlen(divisor);

    for (int i = 0; i < pl - 1; i++)
        strcat(codeword, "0");

    strcpy(remainder, codeword);
    for (int i = 0; i <= strlen(codeword) - pl; i++)
    {
        if (remainder[i] == '1')
        {
            for (int j = 0; j < pl; j++)
            {
                if (remainder[i + j] == divisor[j])
                    remainder[i + j] = '0';
                else
                    remainder[i + j] = '1';
            }
        }
    }

    strcpy(codeword, data);

    for (int i = dl; i < dl + pl - 1; i++)
    {
        char temp[2];
        temp[0] = remainder[i];
        temp[1] = '\0';
        strcat(codeword, temp);
    }

    printf("\nGenerated Codeword: %s\n",
           codeword);

    clock_gettime(CLOCK_MONOTONIC,
                  &start);

    send(s,
         codeword,
         strlen(codeword) + 1,
         0);

    send(s,
         divisor,
         strlen(divisor) + 1,
         0);

    int bytes_sent =
        strlen(codeword) +
        strlen(divisor);

    int r = recv(s,
                 result,
                 sizeof(result) - 1,
                 0);

    clock_gettime(CLOCK_MONOTONIC,
                  &end);

    if (r > 0)
    {
        result[r] = '\0';

        printf("\nServer Result: %s\n",
               result);
    }

    rtt =
        (end.tv_sec - start.tv_sec) * 1000.0 +
        (end.tv_nsec - start.tv_nsec) / 1000000.0;

    printf("\n--- Sender Metrics ---\n");
    printf("Bytes Sent: %d\n",
           bytes_sent);

    printf("Packets Sent: 1\n");

    printf("RTT: %.2f ms\n",
           rtt);

    printf("Throughput: %.2f bits/sec\n",
           rtt > 0 ?
           bytes_sent * 8.0 /
           (rtt / 1000.0) : 0);

    printf("PDR: %.2f%%\n",
           r > 0 ? 100.0 : 0.0);

    close(s);

    return 0;
}