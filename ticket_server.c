#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main()
{
    int s,c,n;

    char movie[50];
    char line[200];
    char result[200];

    char id[50],name[100],seats[50];

    struct sockaddr_in a={0};

    s=socket(AF_INET,SOCK_STREAM,0);

    a.sin_family=AF_INET;
    a.sin_port=htons(5001);
    a.sin_addr.s_addr=INADDR_ANY;

    bind(s,(struct sockaddr*)&a,sizeof(a));

    listen(s,1);

    printf("Ticket Booking Server Running...\n");

    c=accept(s,NULL,NULL);

    n=recv(c,movie,sizeof(movie)-1,0);

    movie[n]='\0';

    int found=0;

    FILE *fp=fopen("movies.csv","r");

    while(fp && fgets(line,sizeof(line),fp))
    {
        sscanf(line,"%49[^,],%99[^,],%49[^\n]",
               id,name,seats);

        if(strcmp(movie,id)==0)
        {
            snprintf(result,sizeof(result),
                    "Movie: %s\nAvailable Seats: %s",
                    name,seats);

            found=1;
            break;
        }
    }

    if(fp) fclose(fp);

    if(!found)
        strcpy(result,"Movie Not Found");

    send(c,result,strlen(result),0);

    close(c);
    close(s);
}