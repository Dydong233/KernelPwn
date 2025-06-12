#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
char * buf = "test for read and write.\n";
int main(void)
{
    char ch[0x100];
    int fd = open("/dev/mydevice", 2);	// 只写模式打开
    int len = strlen(buf);
    ioctl(fd, 0x1000, NULL);	// 设置读文件模式
    write(fd, buf, len);
    ioctl(fd, 0x1001, NULL);	// 设置写文件模式
    write(fd, buf, len);
    read(fd, ch, len);
    write(0, ch, len);
    ioctl(fd, 0x1002, NULL);	// 重置缓冲区
    read(fd, ch, len);
    write(0, ch, len);
    close(fd);
    return 0;
}