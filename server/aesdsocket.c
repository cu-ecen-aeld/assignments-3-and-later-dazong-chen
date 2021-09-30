/***********************************************************
* Author: Dazong Chen
* Date: 09.30.2021
* Reference: 
* https://beej.us/guide/bgnet/html/
* https://www.educative.io/edpresso/how-to-implement-tcp-sockets-in-c
* https://www.csd.uoc.gr/~hy556/material/tutorials/cs556-3rd-tutorial.pdf
***********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#include <syslog.h>
#include <sys/stat.h>
#include <fcntl.h>



#define       PORT               9000       // the port users will be connecting to
#define       OUTPUT_FILE        "/var/tmp/aesdsocketdata"
#define       MAX_CONNECTION     4         // maximum length to which the queue of pending connections for sockfd may grow.


void *get_in_addr(struct sockaddr *sa);


struct sockaddr_in    server_addr;
struct sockaddr_in    client_addr;
int server_fd;

int main(int argc, char *argv[])
{

    // setup syslog
    openlog(NULL, 0, LOG_USER);
    
    // setup signal handler for SIGINT and SIGTERM
    signal(SIGINT, termination_handler);
    signal(SIGTERM, termination_handler);
    
    
    server_fd = socket(PF_INET, SOCK_STREAM, 0);
    if(server_fd == -1)
    {
    	perror("Socket is not created successfully\n");
    	return -1;
    }
    
    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(PORT);
    server_addr->sin_addr.s_addr = INADDR_ANY;
    
    // Bind to the set port and IP:
    if(bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr))<0)
    {
        printf("Couldn't bind to the port\n");
        return -1;
    }
    
    printf("Done with binding\n");
    
    if(listen(server_fd, MAX_CONNECTION)<0)
    {
    	perror("Server listen failed\n");
    	return -1;
    }
    
    // create output file
    fd = open(OUTPUT_FILE, O_RDWR | O_CREAT, 0644);
    
    if(fd < 0)
    {
        syslog(LOG_ERR, "open() failed\n");
        return -1;
    }
}



// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) 
    {
    	return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
