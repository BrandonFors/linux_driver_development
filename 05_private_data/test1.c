#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#define DEV_BUFFER_SIZE 64

int main(int argc, char *argv[]){
    if(argc < 2){
        printf("Need file to open an argument\n");
        return -1;
    }
    int fd;
    int ret;

    /*************************************** */
    fd = open(argv[1], O_RDWR);
    if(fd < 0){
        perror("Failed to open 0\n");
        return fd;
    }
    printf("%s: filed opened 0\n", argv[1]);


    ssize_t len = write(fd, argv[2], strlen(argv[2]));
    if(len < 0){
        perror("Failed to write\n");
        goto close_fd;
    }
    printf("Written length: %ld \n", len);
    printf("Press enter to continue.");
    getchar();

    char buffer[DEV_BUFFER_SIZE];
    len = read(fd, buffer, DEV_BUFFER_SIZE);
    if(len < 0){
        perror("Failed to read\n");
        goto close_fd;
    }
    printf("read: %d bytes - %s \n", len, buffer);

close_fd:
    ret = close(fd);
    if(ret < 0){
        perror("Failed to close 0\n");
        return ret;
    }
    printf("%s: file closed 0\n", argv[1]);

    

   
    


    return 0;
}
