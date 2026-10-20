
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdint.h>

void *handle_client(void *arg)
{
    int client = (intptr_t)arg;
    char roll[100], line[300], result[500] = "";
    char r[50], name[100], dept[100], sem[50], cgpa[50];
    int found = 0;

    int n = recv(client, roll, sizeof(roll) - 1, 0);
    if (n <= 0) {
        close(client);
        return NULL;
    }

    roll[n] = '\0';
    roll[strcspn(roll, "\r\n")] = '\0';

    FILE *fp = fopen("students.csv", "r");

    if (fp != NULL) {
        while (fgets(line, sizeof(line), fp)) {
            if (sscanf(line, "%49[^,],%99[^,],%99[^,],%49[^,],%49[^\n]",
                       r, name, dept, sem, cgpa) == 5) {
                if (strcmp(r, roll) == 0) {
                    snprintf(result, sizeof(result),
                             "Roll Number: %s\nName: %s\nDepartment: %s\nSemester: %s\nCGPA: %s\n",
                             r, name, dept, sem, cgpa);
                    found = 1;
                    break;
                }
            }
        }
        fclose(fp);
    }

    if (!found)
        strcpy(result, "Student Record Not Found");

    send(client, result, strlen(result), 0);
    close(client);
    return NULL;
}

int main()
{
    int server, client;
    struct sockaddr_in addr;
    pthread_t tid;

    server = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(5000);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server, (struct sockaddr *)&addr, sizeof(addr));
    listen(server, 5);

    printf("TCP server running on port 5000...\n");

    while (1) {
        client = accept(server, NULL, NULL);

        if (client >= 0) {
            pthread_create(&tid, NULL, handle_client,
                           (void *)(intptr_t)client);
            pthread_detach(tid);
        }
    }

    close(server);
    return 0;
}
