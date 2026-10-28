#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>

int main()
{
    int s, c, n;
    int sum = 0;

    unsigned char data[20], check;

    struct sockaddr_in a = {0};

    s = socket(AF_INET, SOCK_STREAM, 0);

    a.sin_family = AF_INET;
    a.sin_port = htons(8081);
    a.sin_addr.s_addr = INADDR_ANY;

    bind(s, (struct sockaddr *)&a, sizeof(a));

    listen(s, 1);

    printf("Checksum Server Running...\n");

    c = accept(s, NULL, NULL);

    recv(c, &n, sizeof(n), 0);
    recv(c, data, n, 0);
    recv(c, &check, 1, 0);

    printf("\nReceived Data:\n");

    for (int i = 0; i < n; i++)
    {
        printf("%02X\n", data[i]);

        sum += data[i];

        if (sum > 255)
            sum = (sum & 255) + 1;
    }

    printf("\nReceived Checksum: %02X\n", check);

    sum += check;

    if (sum > 255)
        sum = (sum & 255) + 1;

    printf("Verification Sum: %02X\n", sum);

    int bytes_received = sizeof(n) + n + sizeof(check);

    printf("\n--- Receiver Metrics ---\n");
    printf("Bytes Received: %d\n", bytes_received);
    printf("Packets Received: 1\n");

    if (sum == 255)
    {
        printf("Packet Status: Correct Packet\n");
        printf("Error Detection Rate: 0%%\n");
        printf("Result: No Error Detected\n");
    }
    else
    {
        printf("Packet Status: Corrupted Packet\n");
        printf("Error Detection Rate: 100%%\n");
        printf("Result: Error Detected\n");
    }
    char result[100];

    if(sum == 255)
        strcpy(result, "No Error Detected");
    else
        strcpy(result, "Error Detected");

    send(c, result, strlen(result), 0);

    close(c);
    close(s);

    return 0;
}