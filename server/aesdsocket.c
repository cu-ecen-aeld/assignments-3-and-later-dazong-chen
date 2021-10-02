/***********************************************************
* Author: Dazong Chen
* Date: 09.30.2021
* Reference: 
* https://beej.us/guide/bgnet/html/
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



#define       PORT                   9000       // the port users will be connecting to
#define       OUTPUT_FILE            "/var/tmp/aesdsocketdata"
#define       MAX_CONNECTION         4         // maximum length to which the queue of pending connections for sockfd may grow.
#define       BUFFER_SIZE            500


void *get_in_addr(struct sockaddr *sa);
void termination_handler();
void write_append_file(char* filename, char* content, int lenth);

struct sockaddr_in    server_addr;
struct sockaddr_in    client_addr;
int                   server_fd;
int                   client_fd;



int main(int argc, char *argv[])
{
    pid_t          pid = 0;
    bool           daemon_flag = false;
    socklen_t      addr_size;

    // setup syslog
    openlog(NULL, 0, LOG_USER);
    
    // setup signal handler for SIGINT and SIGTERM
    signal(SIGINT, termination_handler);
    signal(SIGTERM, termination_handler);
    
    
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
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
    
    if(daemon_flag == true)
    {
        pid = fork();
    
        if(pid < 0)
        {
            perror("fork failed\n");
            return -1;
        }
    
        else if(pid > 0)
        {
    	    printf("parent of pid = %d\n", pid);
    	    exit(0);
        }
        
        else
        {
            printf("child process created\n");
        }
    }
    
    
        
    if(listen(server_fd, MAX_CONNECTION)<0)
    {
    	perror("Server listen failed\n");
    	return -1;
    }
    
    // create output file
    fd = open(OUTPUT_FILE, O_RDWR | O_CREAT | O_APPEND, 0644);
    
    if(fd < 0)
    {
        syslog(LOG_ERR, "open() failed\n");
        return -1;
    }
    
    while(1)
    {
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_size);
        
        if(client_fd == -1)
    	{
    	    perror("socket is not accepting successfully\n");
    	    return -1;
    	}
    	else
    	{
    	    char client_ip6[INET6_ADDRSTRLEN]; // space to hold the IPv6 string
    	    inet_ntop(AF_INET, get_in_addr((struct sockaddr*)&client_addr), client_ip6, sizeof client_ip6);
    	    syslog(LOG_DEBUG, "Accepted connection from %s", client_ip6);
    	}
    	
    	int received_bytes = 0;
    	int current_in_buf_bytes = 0;
    	char buf[BUFFER_SIZE];
    	memset(buf, 0, sizeof(buf));
    	
    	char* content_buf = NULL;
    	int content_buf_size = BUFFER_SIZE;
    	content_buf = (char*)malloc(sizeof(char) * content_buf_size);
    	
    	do
    	{
    	    received_bytes = recv(client_fd, buf, sizeof buf, 0);
    	    
    	    if(received_bytes == -1)
    	    {
    	        printf("recv failed\n");
    	    }
    	    
    	    // check if malloced size is enough to hold new appended contents, otherwise realloc
    	    while( (received_bytes + current_in_buf_bytes) >= content_buf_size )
    	    {
    	        contect_buf_size += BUFFER_SIZE;
    	    }
    	    
    	    content_buf = (char*)realloc(sizeof(char) * content_buf_size);
    	    
    	    memcpy(&content_buf[current_in_buf_bytes], buf, received_bytes);
    	    
    	    current_in_buf_bytes += received_bytes;
    	    	
    	}while(buf[received_bytes-1] != '\n');
    	
    	ssize_t write_bytes = write(fd, content_buf, current_in_buf_bytes);
    	
    	if(write_bytes != current_in_buf_bytes)
    	{
    	    printf("not completely written\n");
    	}
    	
    	
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


void termination_handler()
{
    syslog(LOG_DEBUG, "Caught signal, exiting\n");
    
    close(fd);
    close(server_fd);
    closelog();
    
    if(remove(OUTPUT_FILE) < 0)
    {
    	syslog(LOG_DEBUG, "Remove %s failed\n", OUTPUT_FILE);
    }
    
    exit(0);
}