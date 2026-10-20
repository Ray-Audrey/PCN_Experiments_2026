#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

int main()
{
    int s, c, n, sum = 0, carry;
    unsigned char data[20], check;
    struct sockaddr_in a = {0};

    s = socket(AF_INET, SOCK_STREAM, 0);
    a.sin_family = AF_INET;
    a.sin_port = htons(8081);
    a.sin_addr.s_addr = INADDR_ANY;

    bind(s, (struct sockaddr *)&a, sizeof(a));
    listen(s, 1);

    printf("Checksum Server Waiting...\n");
    c = accept(s, NULL, NULL);

    recv(c, &n, sizeof(n), 0);
    recv(c, data, n, 0);
    recv(c, &check, 1, 0);

    for (int i = 0; i < n; i++)
    {
        sum += data[i];
        carry = sum > 255;
        if (carry) sum = (sum & 255) + 1;

        printf("Word: %02X, Sum: %02X, Carry: %d\n",
               data[i], sum, carry);
    }

    printf("Checksum: %02X\n", check);

    sum += check;
    if (sum > 255) sum = (sum & 255) + 1;

    printf("Verification Sum: %02X\n", sum);
    printf("%s\n", sum == 255 ? "No Error Detected" : "Error Detected");

    close(c);
    close(s);
    return 0;
}