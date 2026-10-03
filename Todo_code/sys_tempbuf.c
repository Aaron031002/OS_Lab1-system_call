#include <unistd.h>

#define __NR_tempbuf 452

enum mode{
    PRINT,
    ADD,
    REMOVE
};

int syscall(__NR_tempbuf, enum mode, void *data, size_t size)
{
    
}