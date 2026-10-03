#include <unistd.h>

#define __NR_revstr 451
int syscall(__NR_revstr, char *str, size_t n)
{
    
}