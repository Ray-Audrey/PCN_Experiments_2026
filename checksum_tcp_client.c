
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

int main()
{
    int s, n, sum = 0, carry;
    unsigned int x;
    unsigned char data[20], check;
    struct sockaddr_in server;

    s = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_family = AF_INET;
    server.sin_port = htons(8081);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(s, (struct sockaddr *)&server, sizeof(server));

    printf("Number of 8-bit words (1-20): ");
    scanf("%d", &n);

    if (n < 1 || n > 20)
    {
        printf("Invalid number of words\n");
        close(s);
        return 1;
    }

    for (int i = 0; i < n; i++)
    {
        printf("Enter word %d in hexadecimal: ", i + 1);
        scanf("%x", &x);

        if (x > 255)
        {
            printf("Word must be between 00 and FF\n");
            close(s);
            return 1;
        }

        data[i] = x;
        sum += data[i];
        carry = sum > 255;

        if (carry)
            sum = (sum & 255) + 1;

        printf("Sum = %02X, Carry = %d\n", sum, carry);
    }

    check = ~sum;
    printf("Checksum: %02X\n", check);

    send(s, &n, sizeof(n), 0);
    send(s, data, n, 0);
    send(s, &check, 1, 0);

    printf("Data and checksum sent.\n");

    close(s);
    return 0;
}
