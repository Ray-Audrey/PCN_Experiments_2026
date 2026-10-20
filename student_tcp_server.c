#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdint.h>

void *handle(void *arg)
{
    int c = (intptr_t)arg, found = 0;
    char roll[100], line[300], result[500] = "";
    char r[50], name[100], dept[100], sem[50], cgpa[50];

    int n = recv(c, roll, sizeof(roll) - 1, 0);
    if (n <= 0) {
        close(c);
        return NULL;
    }

    roll[n] = '\0';
    roll[strcspn(roll, "\r\n")] = '\0';

    FILE *fp = fopen("students.csv", "r");

    if (fp) {
        while (fgets(line, sizeof(line), fp)) {
            if (sscanf(line, "%49[^,],%99[^,],%99[^,],%49[^,],%49[^\n]",
                       r, name, dept, sem, cgpa) == 5 &&
                strcmp(r, roll) == 0) {
                snprintf(result, sizeof(result),
                         "Roll: %s\nName: %s\nDepartment: %s\nSemester: %s\nCGPA: %s\n",
                         r, name, dept, sem, cgpa);
                found = 1;
                break;
            }
        }
        fclose(fp);
    }

    if (!found)
        strcpy(result, "Student Record Not Found");

    send(c, result, strlen(result), 0);
    close(c);
    return NULL;
}

int main()
{
    int s, c;
    struct sockaddr_in a;
    pthread_t t;

    s = socket(AF_INET, SOCK_STREAM, 0);

    a.sin_family = AF_INET;
    a.sin_port = htons(5000);
    a.sin_addr.s_addr = INADDR_ANY;

    bind(s, (struct sockaddr *)&a, sizeof(a));
    listen(s, 5);

    printf("TCP Server running on port 5000...\n");

    while (1) {
        c = accept(s, NULL, NULL);
        if (c >= 0) {
            pthread_create(&t, NULL, handle, (void *)(intptr_t)c);
            pthread_detach(t);
        }
    }
}