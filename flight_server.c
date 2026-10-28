#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main()
{
    int s, c, n;
    char flight[50], line[200], result[200];
    char id[50], dest[100], seats[50];

    struct sockaddr_in a = {0};

    s = socket(AF_INET, SOCK_STREAM, 0);

    a.sin_family = AF_INET;
    a.sin_port = htons(5000);
    a.sin_addr.s_addr = INADDR_ANY;

    bind(s, (struct sockaddr *)&a, sizeof(a));

    listen(s, 1);

    printf("Flight Server Running...\n");

    c = accept(s, NULL, NULL);

    n = recv(c, flight, sizeof(flight)-1, 0);

    flight[n] = '\0';

    int found = 0;

    FILE *fp = fopen("flights.csv", "r");

    while(fp && fgets(line,sizeof(line),fp))
    {
        sscanf(line,"%49[^,],%99[^,],%49[^\n]",
               id,dest,seats);

        if(strcmp(id,flight)==0)
        {
            snprintf(result,sizeof(result),
                    "Flight: %s\nDestination: %s\nAvailable Seats: %s",
                    id,dest,seats);

            found = 1;
            break;
        }
    }

    if(fp) fclose(fp);

    if(!found)
        strcpy(result,"Flight Not Found");

    send(c,result,strlen(result),0);

    close(c);
    close(s);
}