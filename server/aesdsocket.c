/***********************************************************
* Author: Dazong Chen
* Date: 09.30.2021
* Reference: 
* https://beej.us/guide/bgnet/html/
* http://www.microhowto.info/howto/cause_a_process_to_become_a_daemon_in_c.html
* https://github.com/cu-ecen-aeld/aesd-lectures/blob/master/lecture9/timer_thread.c
* https://github.com/stockrt/queue.h/blob/master/sample.c
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

#include <sys/queue.h>
#include <pthread.h>
#include <time.h>



#define       PORT                   9000       // the port users will be connecting to
#define       OUTPUT_FILE            "/var/tmp/aesdsocketdata"
#define       MAX_CONNECTION         4         // maximum length to which the queue of pending connections for sockfd may grow.
#define       BUFFER_SIZE            500





typedef struct
{
    pthread_t     thread;
    int           thread_id;
    int           client_fd;
    int           fd;
    char*         read_buf;
    char*         write_buf;
    sigset_t      mask;
    bool          is_completed;

}threadParams_t;

typedef struct slist_data_s   slist_data_t;
struct slist_data_s
{

    threadParams_t threadParams;
    SLIST_ENTRY(slist_data_s) entries;
};


typedef struct
{
    int fd;
    
}timer_data_t;


static inline void timespec_add( struct timespec *result,
                        const struct timespec *ts_1, const struct timespec *ts_2)
{
    result->tv_sec = ts_1->tv_sec + ts_2->tv_sec;
    result->tv_nsec = ts_1->tv_nsec + ts_2->tv_nsec;
    if( result->tv_nsec > 1000000000L ) 
    {
        result->tv_nsec -= 1000000000L;
        result->tv_sec ++;
    }
}


void* send_receive_packet(void* threadp);
void sig_handler(int signo);
void* get_in_addr(struct sockaddr *sa);
static void timer_thread(union sigval sigval);

pthread_mutex_t locker = PTHREAD_MUTEX_INITIALIZER;
struct sockaddr_in    server_addr;
struct sockaddr_in    client_addr;
int                   server_fd;
int                   client_fd;
int                   fd;
bool shut_down_flag = false;


int main(int argc, char *argv[])
{
    pid_t          pid = 0;
    bool           daemon_flag = false;
    socklen_t      addr_size;
    sigset_t       mask;
    int            thread_id = 1;
    char           buf[BUFFER_SIZE];
    bool           shut_down_flag = false;

    memset(buf, 0, sizeof(buf));
    
    slist_data_t *datap = NULL;
    
    struct sigevent    sev;
    timer_data_t       td;
    int clock_id = CLOCK_MONOTONIC;
    
    SLIST_HEAD(slisthead, slist_data_s) head;
    SLIST_INIT(&head);

    // setup syslog
    openlog(NULL, 0, LOG_USER);
    
    // setup signal handler for SIGINT and SIGTERM
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    
    // signals to be masked
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    
    if(argc == 2)
    {
        if(strcmp(argv[1], "-d") == 0)
        {
            daemon_flag = true;
        }
    }
    
    server_fd = socket(PF_INET, SOCK_STREAM, 0);
    if(server_fd == -1)
    {
    	perror("Socket is not created successfully\n");
    	return -1;
    }
    
    int option = 1;
    /* Forcefully attaching socket to the port 9000 for bind error: address in use */
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option, sizeof(option)) == -1)
    {
        perror("setsockopt failed");
       	exit(-1);
    }
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    
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
        
        if(setsid() == -1)
        {
            printf("failed create new session\n");
            return -1;
        }
        
        // change to root
        chdir("/");
        
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
    }
    
    if(listen(server_fd, MAX_CONNECTION) == -1)
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
    
    
    td.fd = fd;
    /**
    * Setup a call to timer_thread passing in the td structure as the sigev_value
    * argument
    */
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_value.sival_ptr = &td;
    sev.sigev_notify_function = timer_thread;
    
    struct timespec     start_time;
    timer_t             timerid;
    
    if ( timer_create(clock_id, &sev, &timerid) != 0 )
    {
        printf("Error %d (%s) creating timer!\n",errno,strerror(errno));
    }
    
    
    
    if ( clock_gettime(clock_id, &start_time) != 0 ) 
    {
        printf("Error %d (%s) getting clock %d time\n", errno, strerror(errno), clock_id);
    }
    
    struct itimerspec itimerspec;
    itimerspec.it_interval.tv_sec = 10;
    itimerspec.it_interval.tv_nsec = 10;
    
    timespec_add(&itimerspec.it_value,&start_time,&itimerspec.it_interval);
    
    if( timer_settime(timerid, TIMER_ABSTIME, &itimerspec, NULL ) != 0 ) 
    {
        printf("Error %d (%s) setting timer\n",errno,strerror(errno));
    }
    
    while(!shut_down_flag)
    {
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_size);
        
        if(shut_down_flag == true)
        {
            break;
        }
        
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
    	    
    	    
    	    // create new thread
    	    datap = malloc(sizeof(slist_data_t));
    	    datap->threadParams.thread_id = thread_id;
    	    datap->threadParams.client_fd = client_fd;
    	    datap->threadParams.fd = fd;
    	    datap->threadParams.mask = mask;
    	    datap->threadParams.is_completed = false;
    	    
    	    SLIST_INSERT_HEAD(&head, datap, entries);
    	    
    	    pthread_create(&(datap->threadParams.thread),(void*)0, send_receive_packet,(void*)&(datap->threadParams));
    	    SLIST_FOREACH(datap, &head, entries)
    	    {
    	        if((datap->threadParams).is_completed == true)
	        {
              	     pthread_join((datap->threadParams).thread, NULL);	
		}
    	    
    	    }
    	}
    }
    
    close(fd);
    close(client_fd);
    close(server_fd);
    remove(OUTPUT_FILE);

    while (!SLIST_EMPTY(&head))
    {
        datap = SLIST_FIRST(&head);
	SLIST_REMOVE_HEAD(&head, entries);
	free(datap);
    }
    
    return 0;
}


// get sockaddr, IPv4 or IPv6:
void* get_in_addr(struct sockaddr* sa)
{
    if (sa->sa_family == AF_INET) 
    {
    	return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


void* send_receive_packet(void* threadp)
{
    threadParams_t        *threadParams = (threadParams_t *)threadp;
    int                   current_in_buf_bytes = 0;   // overwrite content buf
    int                   received_bytes = 0;
    char* 	          tmp = NULL;
    int 		  content_buf_size = BUFFER_SIZE;
    bool 	          newline_flag = false;
    char                  buf[BUFFER_SIZE];
    
    
    threadParams->read_buf = (char*)malloc(sizeof(char) * BUFFER_SIZE);
    threadParams->write_buf = (char*)malloc(sizeof(char) * BUFFER_SIZE);
    
    
    do	// reading a line
    {
        received_bytes = recv(threadParams->client_fd, buf, BUFFER_SIZE, 0);
	    
	//printf("buf %s",buf);
	    
	if(received_bytes == -1)
	{
	    printf("recv failed\n");
	    pthread_exit(threadParams);
	}
	    
	if(strchr(buf, '\n') != NULL)
	{
	    newline_flag = true;
	}
	    
	// check if malloced size is enough to hold new appended contents, otherwise realloc
	if( (received_bytes + current_in_buf_bytes) >= content_buf_size )
	{
	    content_buf_size += BUFFER_SIZE;
		
	    tmp = (char*)realloc(threadParams->read_buf, sizeof(char) * content_buf_size);
	    
	    if(tmp == NULL)
	    {
	        printf("readBuf realloc failed\n");
	        free(threadParams->read_buf);
	        pthread_exit(threadParams);
            }
            
            else
            {
                threadParams->read_buf = tmp;
            }
	 }
	 
	 memcpy(threadParams->read_buf+current_in_buf_bytes, buf, received_bytes);
    	 current_in_buf_bytes += received_bytes;
    	 
    }while(!newline_flag);
    

    pthread_mutex_lock(&locker);
    
    // stop receiving signal while writing data to file
    if(sigprocmask(SIG_BLOCK, &threadParams->mask, NULL) == -1)
    {
        printf("failed blocking signal\n");
    	exit(-1);
    } 
    
    ssize_t write_bytes = write(threadParams->fd, threadParams->read_buf, current_in_buf_bytes);
    	
    //printf("write bytes %ld\n", write_bytes);
    if(write_bytes != current_in_buf_bytes)
    {
        printf("not completely written\n");
    }
    
    // Unmask signals after receive/send
    if (sigprocmask(SIG_UNBLOCK,&(threadParams->mask),NULL) == -1)
    {
        perror("\nERROR sigprocmask():");
        exit(-1);
    }
    
    pthread_mutex_unlock(&locker);
    
    lseek(threadParams->fd, 0, SEEK_SET);
    
    //read one byte at a time and sending one packet with newline at a time
    char byte_content;
    int nbytes = 0;
    int send_buf_size = BUFFER_SIZE;
    int packet_size = 0;
    
    pthread_mutex_lock(&locker);
    
    // Block signals to avoid partial write
    if(sigprocmask(SIG_BLOCK, &(threadParams->mask), NULL) == -1)
    {
        printf("failed blocking signal\n");
    	exit(-1);
    }
    
    while( (nbytes = read(threadParams->fd, &byte_content,1)) > 0 )
    {
        if(packet_size >= send_buf_size)   // reallocate memory if not enough space
        {
            send_buf_size += BUFFER_SIZE;
            tmp = realloc(threadParams->write_buf, sizeof(char) * send_buf_size);
            if(tmp == NULL)
	    {
	        printf("writeBuf realloc failed\n");
	        free(threadParams->write_buf);
	        pthread_exit(threadParams);
            }
            
            else
            {
                threadParams->write_buf = tmp;
            }
            
            
        }
        
        threadParams->write_buf[packet_size++] = byte_content;
        
        if(byte_content == '\n')    // read in newline, send the packet
        {   
            ssize_t send_bytes = send(threadParams->client_fd, threadParams->write_buf, packet_size, 0);
            
            if(send_bytes == -1)
            {
                printf("send() failed\n");
	        pthread_exit(threadParams);
            }
            
            packet_size = 0;
        }
    }
    
    pthread_mutex_unlock(&locker);
    
    if (sigprocmask(SIG_UNBLOCK,&(threadParams->mask),NULL) == -1)
    {
        perror("\nERROR sigprocmask():");
        pthread_exit(threadParams);
    }

    pthread_mutex_unlock(&locker);

    close(threadParams->client_fd);

    free(threadParams->read_buf);
    free(threadParams->write_buf);
    
    threadParams->is_completed = true;
    
    pthread_exit(threadParams);
}


// from timer_thread.c example code in lecture 9
static void timer_thread(union sigval sigval)
{
    timer_data_t* td = (timer_data_t*) sigval.sival_ptr;
    char buf[BUFFER_SIZE];
    time_t time_now;
    struct tm *time_info;
    time_info = localtime(&time_now);
    size_t nbytes = strftime(buf,100,"timestamp:%a, %d %b %Y %T %z\n",time_info);
    
    pthread_mutex_lock(&locker);
    ssize_t write_bytes = write(td->fd, buf, nbytes);
    
    if(write_bytes == -1)
    {
        perror("timer_thread write() failed\n");
        exit(-1);
    }
    
    pthread_mutex_unlock(&locker);
}


void sig_handler(int signo)
{
    if(signo == SIGINT || signo==SIGTERM) 
    {
        shutdown(server_fd, SHUT_RDWR);
        shut_down_flag = true;
    }
}
