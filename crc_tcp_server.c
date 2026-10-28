#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main()
{
    int s, c;
    char data[50], divisor[20], remainder[20];
    char result[100];

    struct sockaddr_in a = {0};

    s = socket(AF_INET, SOCK_STREAM, 0);

    a.sin_family = AF_INET;
    a.sin_port = htons(8081);
    a.sin_addr.s_addr = INADDR_ANY;

    bind(s, (struct sockaddr *)&a, sizeof(a));

    listen(s, 1);

    printf("CRC Server Running...\n");

    c = accept(s, NULL, NULL);

    recv(c, data, sizeof(data), 0);
    recv(c, divisor, sizeof(divisor), 0);

    strcpy(remainder, data);

    int dl = strlen(data);
    int pl = strlen(divisor);

    for (int i = 0; i <= dl - pl; i++)
    {
        if (remainder[i] == '1')
        {
            for (int j = 0; j < pl; j++)
            {
                remainder[i + j] =
                    (remainder[i + j] == divisor[j]) ? '0' : '1';
            }
        }
    }

    int error = 0;

    for (int i = dl - pl + 1; i < dl; i++)
    {
        if (remainder[i] != '0')
        {
            error = 1;
            break;
        }
    }

    printf("\nReceived Codeword : %s\n", data);
    printf("Divisor           : %s\n", divisor);

    if (error)
    {
        printf("Result            : Error Detected\n");
        strcpy(result, "Error Detected");
    }
    else
    {
        printf("Result            : No Error Detected\n");
        strcpy(result, "No Error Detected");
    }

    printf("\n--- Receiver Metrics ---\n");
    printf("Bytes Received: %lu\n",
           strlen(data) + strlen(divisor));
    printf("Packets Received: 1\n");

    send(c, result, strlen(result), 0);

    close(c);
    close(s);

    return 0;
}