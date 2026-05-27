#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define MYCDEV_MAGIC        'M'
#define MYCDEV_CLEAR        _IO(MYCDEV_MAGIC, 1)
#define MYCDEV_SAY_HELLO    _IO(MYCDEV_MAGIC, 2)
#define MYCDEV_USER_READ    _IOR(MYCDEV_MAGIC, 3, int) //user read - kernel write
#define MYCDEV_USER_WRITE   _IOW(MYCDEV_MAGIC, 4, int) //user write - kernel read

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

    if(ioctl(fd, MYCDEV_SAY_HELLO) < 0){
        perror("Failed ioctl MYCDEV_SAY_HELLO\n");
        goto close_fd;
    }
    printf("Kernel said hello\n");

    int value;
    if(ioctl(fd, MYCDEV_USER_READ, &value) < 0){
        perror("Failed ioctl MYCDEV_READ\n");
        goto close_fd;
    }
    printf("Value: 0x%x copied from kernel\n", value);

    value = 0xf00d1e;
    if(ioctl(fd, MYCDEV_USER_WRITE, &value) < 0){
        perror("Failed ioctl MYCDEV_WRITE\n");
        goto close_fd;
    }
    printf("Value: 0x%x written to kernel\n", value);
    
    char buffer[DEV_BUFFER_SIZE];
    
    if(ioctl(fd, MYCDEV_CLEAR) < 0){
        perror("Failed ioctl MYCDEV_CLEAR\n");
        goto close_fd;
    }
    printf("Kernel private buffer cleared\n", value);
    memset(buffer, 0, DEV_BUFFER_SIZE);


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
