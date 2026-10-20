#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main()
{
    int s, n;
    char roll[100], line[300], result[200];
    char r[50], name[100], dept[100], sem[50], cgpa[50];
    struct sockaddr_in addr, client;
    socklen_t len = sizeof(client);

    s = socket(AF_INET, SOCK_DGRAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(5001);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(s, (struct sockaddr *)&addr, sizeof(addr));
    printf("UDP Server Running...\n");

    while (1)
    {
        n = recvfrom(s, roll, sizeof(roll) - 1, 0,
                     (struct sockaddr *)&client, &len);

        if (n <= 0) continue;

        roll[n] = '\0';
        roll[strcspn(roll, "\r\n")] = '\0';

        int found = 0;
        FILE *fp = fopen("students.csv", "r");

        if (fp)
        {
            while (fgets(line, sizeof(line), fp))
            {
                if (sscanf(line, "%49[^,],%99[^,],%99[^,],%49[^,],%49[^\n]",
                           r, name, dept, sem, cgpa) == 5 &&
                    strcmp(r, roll) == 0)
                {
                    snprintf(result, sizeof(result),
                             "Roll: %s\nName: %s", r, name);
                    found = 1;
                    break;
                }
            }
            fclose(fp);
        }

        if (!found)
            strcpy(result, "Student Record Not Found");

        sendto(s, result, strlen(result), 0,
               (struct sockaddr *)&client, len);
    }

    close(s);
    return 0;
}