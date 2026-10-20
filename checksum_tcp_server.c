
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

int main()
{
    int s, c, n, sum = 0, carry;
    unsigned char data[20], check;
    struct sockaddr_in addr;

    s = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(8081);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(s, (struct sockaddr *)&addr, sizeof(addr));
    listen(s, 1);

    printf("Checksum Server Waiting...\n");
    c = accept(s, NULL, NULL);

    recv(c, &n, sizeof(n), 0);
    recv(c, data, n, 0);
    recv(c, &check, 1, 0);

    printf("\nReceived Data: ");
    for (int i = 0; i < n; i++)
    {
        printf("%02X ", data[i]);
        sum += data[i];
        carry = sum > 255;

        if (carry)
            sum = (sum & 255) + 1;

        printf("\nSum = %02X, Carry = %d\n", sum, carry);
    }

    printf("Received Checksum: %02X\n", check);

    sum += check;
    if (sum > 255)
        sum = (sum & 255) + 1;

    printf("Verification Sum: %02X\n", sum);

    if (sum == 255)
        printf("No Error Detected\n");
    else
        printf("Error Detected\n");

    close(c);
    close(s);
    return 0;
}
