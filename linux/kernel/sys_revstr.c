#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/errno.h>

SYSCALL_DEFINE2(revstr, char __user*, str, size_t, n)  
{
    /* set up kernel memory for str */
    char *kbuf = kmalloc(n + 1, GFP_KERNEL);

    if (!kbuf)
        return -ENOMEM;

    if (copy_from_user(kbuf, str, n)){
        kfree(kbuf);
        return -EFAULT;
    }

    kbuf[n] = '\0';

    /* reverse kbuf */
    if (n > 1){
        char *left = kbuf;
        char *right = kbuf + n-1;
    
        while (left < right){
            char temp = *left;
            *left = *right;
            *right = temp;

            left++;
            right--;
        }
    }

    /* pass the reversed str to user space */
    if (copy_to_user(str, kbuf, n)){
        kfree(kbuf);
        return -EFAULT;
    }

    kfree(kbuf);

    return 0;
}