
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
    int s, c, n, error = 0;
    char frame[200], rem[10];
    struct sockaddr_in addr;

    s = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(8082);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(s, (struct sockaddr *)&addr, sizeof(addr));
    listen(s, 1);

    printf("CRC Server Waiting...\n");
    c = accept(s, NULL, NULL);

    n = recv(c, frame, sizeof(frame)-1, 0);
    if (n <= 0)
    {
        close(c);
        close(s);
        return 1;
    }
    frame[n] = '\0';

    printf("Received Frame: %s\n", frame);

    divide(frame, rem);
    printf("Verification Remainder: %s\n", rem);

    for (int i = 0; i < 4; i++)
        if (rem[i] != '0')
            error = 1;

    if (error)
        printf("Error Detected\n");
    else
        printf("No Error Detected\n");

    close(c);
    close(s);
    return 0;
}
