#include<iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
using namespace std;

int main(){
    int fd = open("datafile",O_CREAT|O_EXCL|O_RDWR,S_IRWXU);
    uint8_t data[3] = {0x56,0x57,0x59};
    int b = write(fd,data,3);
    int a = close(fd);
    return 0;
}


/*

*/