#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

void divide(char data[], char rem[])
{
    char temp[200], gen[] = "10011";
    int n = strlen(data);

    strcpy(temp, data);

    for (int i = 0; i <= n - 5; i++)
        if (temp[i] == '1')
            for (int j = 0; j < 5; j++)
                temp[i+j] = (temp[i+j] == gen[j]) ? '0' : '1';

    strncpy(rem, temp + n - 4, 4);
    rem[4] = '\0';
}

int main()
{
    int s, error, pos, n;
    char data[100], temp[200], rem[10];
    struct sockaddr_in a = {0};

    s = socket(AF_INET, SOCK_STREAM, 0);
    a.sin_family = AF_INET;
    a.sin_port = htons(8082);
    inet_pton(AF_INET, "127.0.0.1", &a.sin_addr);

    connect(s, (struct sockaddr *)&a, sizeof(a));

    printf("Enter binary data: ");
    scanf("%99s", data);

    for (int i = 0; data[i]; i++)
        if (data[i] != '0' && data[i] != '1')
        {
            printf("Invalid binary data\n");
            close(s);
            return 1;
        }

    if (strlen(data) < 5)
    {
        printf("Enter at least 5 bits\n");
        close(s);
        return 1;
    }

    strcpy(temp, data);
    strcat(temp, "0000");
    divide(temp, rem);
    strcat(data, rem);

    printf("CRC Remainder: %s\n", rem);
    printf("Frame with CRC: %s\n", data);

    printf("Introduce error? (1=Yes, 0=No): ");
    scanf("%d", &error);

    n = strlen(data);

    if (error == 1)
    {
        printf("Bit position to flip (1-%d): ", n);
        scanf("%d", &pos);

        if (pos < 1 || pos > n)
        {
            printf("Invalid position\n");
            close(s);
            return 1;
        }

        data[pos-1] = (data[pos-1] == '0') ? '1' : '0';
        printf("Modified Frame: %s\n", data);
    }

    send(s, data, strlen(data), 0);
    printf("Frame sent through TCP\n");

    close(s);
    return 0;
}