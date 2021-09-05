/******************************************************
* Author: Dazong Chen
* Date: 09.04.2021
* Reference:
* 1. https://pubs.opengroup.org/onlinepubs/007904975/functions/write.html
* 2. https://makefiletutorial.com/
*******************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <syslog.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <unistd.h>

#include <errno.h>




int main(int argc, char *argv[])
{
    // check if valid input arguments
    if(argc != 3)
    {
    	printf("Error: Please enter 2 arguments for path of file and content be written to the file\n");
        syslog(LOG_ERR, "Error: Please enter 2 arguments for path of file and content be written to the file\n");
	syslog(LOG_USER, "Sample command: ./writer path-of-file content-be-written\n");
	exit(1);
    }
    
    
    char *file_path = argv[1];
    char *file_str = argv[2];
    
    int fd = open(file_path, O_CREAT | O_RDWR | O_TRUNC, S_IRWXU); // create, read/write, owner can RD,WR, EX
    
    if(fd == -1)
    {
    	printf("File cannot be opened\n");
    	syslog(LOG_ERR, "file cannot be opened\n");
    }
    
    int nbytes = strlen(file_str);
    ssize_t bytes_written = write(fd, file_str, nbytes);
    
    if(bytes_written != nbytes)
    {
    	printf("Write request NOT success\n");
    	syslog(LOG_ERR, "Write request NOT success\n");
    }
    
    syslog(LOG_DEBUG, "Writing %s to %s\n", file_str, file_path);
    
    // close file and exit
    close(fd);
    
    if(fd == -1)
    {
    	printf("File cannot be closed\n");
    	syslog(LOG_ERR, "File cannot be closed\n");
    }
    
    return 0;
}
