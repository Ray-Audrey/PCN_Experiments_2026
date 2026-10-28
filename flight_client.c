#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

int main()
{
    int s,n;
    char flight[50],result[200];

    struct sockaddr_in a={0};

    struct timespec start,end;
    double rtt;

    s = socket(AF_INET,SOCK_STREAM,0);

    a.sin_family = AF_INET;
    a.sin_port = htons(5000);

    inet_pton(AF_INET,"127.0.0.1",&a.sin_addr);

    connect(s,(struct sockaddr*)&a,sizeof(a));

    printf("Enter Flight ID: ");
    scanf("%s",flight);

    clock_gettime(CLOCK_MONOTONIC,&start);

    send(s,flight,strlen(flight),0);

    n = recv(s,result,sizeof(result)-1,0);

    clock_gettime(CLOCK_MONOTONIC,&end);

    result[n]='\0';

    printf("\n%s\n",result);

    rtt=(end.tv_sec-start.tv_sec)*1000.0+
        (end.tv_nsec-start.tv_nsec)/1000000.0;

    printf("\n--- Metrics ---\n");
    printf("RTT: %.2f ms\n",rtt);
    printf("Throughput: %.2f bits/sec\n",
           strlen(result)*8.0/(rtt/1000.0));

    close(s);
}