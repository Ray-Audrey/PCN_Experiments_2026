#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

int main()
{
    int s, n, sum = 0, carry;
    unsigned int x;
    unsigned char data[20], check;
    struct sockaddr_in a = {0};

    s = socket(AF_INET, SOCK_STREAM, 0);
    a.sin_family = AF_INET;
    a.sin_port = htons(8081);
    a.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(s, (struct sockaddr *)&a, sizeof(a));

    printf("Number of words (1-20): ");
    scanf("%d", &n);

    if (n < 1 || n > 20) return 1;

    for (int i = 0; i < n; i++)
    {
        printf("Enter word %d in hex: ", i + 1);
        scanf("%x", &x);

        if (x > 255) return 1;

        data[i] = x;
        sum += data[i];
        carry = sum > 255;
        if (carry) sum = (sum & 255) + 1;

        printf("Sum: %02X, Carry: %d\n", sum, carry);
    }

    check = ~sum;
    printf("Checksum: %02X\n", check);

    send(s, &n, sizeof(n), 0);
    send(s, data, n, 0);
    send(s, &check, 1, 0);

    printf("Data sent successfully.\n");

    close(s);
    return 0;
}