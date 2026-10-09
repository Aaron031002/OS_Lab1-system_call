#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/printk.h>
#include <linux/errno.h>
#include <linux/string.h>

#define ADD 0
#define REMOVE 1
#define PRINT 2

struct tempbuf_node {
    char *data;
    size_t len;
    struct list_head list;
};

/* initialize the shared list (create sentinel head) */
static LIST_HEAD(tempbuf_list);

static long tempbuf_add(void __user *data, size_t size)
{
    struct tempbuf_node* node;

    if (size == (size_t) - 1)   // input size is too large
        return -ENOMEM;

    /* allocate memory space for 'node' struct */
    node = kmalloc(sizeof(*node), GFP_KERNEL);   
    if (!node){
        kfree(node);
        return -ENOMEM;
    }

    /* allocate memory space for node->data */
    node->data = kmalloc(size + 1, GFP_KERNEL);
    if (!node->data){
        kfree(node->data);
        kfree(node);
        return -ENOMEM;
    }

    /* copy the content of 'data' into kernel memory space */
    if (copy_from_user(node->data, data, size)){      
        kfree(node->data);
        kfree(node);
        return -EFAULT;
    }
    node->data[size] = '\0';

    node->len = size;

    list_add_tail(&node->list, &tempbuf_list);

    printk(KERN_INFO "[tempbuf] Added: %s\n", node->data);

    return 0;
}

static long tempbuf_remove(void __user *data, size_t size)
{
    struct tempbuf_node *node, *next;   // node: point to the current node in loop, next: point to the next node (for safe deletion)
    char* target;

    if (size == (size_t) - 1)   // input size is too large
        return -ENOMEM;

    /* allocate memory space for target in kernel space */
    target = kmalloc(size + 1, GFP_KERNEL);     
    if (!target)
        return -ENOMEM;

    /* copy the content of 'data' to kernel space */
    if (copy_from_user(target, data, size)){
        kfree(target);
        return -EFAULT;
    }     
    target[size] = '\0';

    /* traverse the node list to find the target node */
    list_for_each_entry_safe(node, next, &tempbuf_list, list){
        if (node->len == size && !strcmp(target, node->data)){
            list_del(&node->list);      // remove the first found target node
        
            printk(KERN_INFO "[tempbuf] Removed: %s\n", node->data);

            kfree(node->data);
            kfree(node);
            kfree(target);

            return 0;
        }
    }

    kfree(target);
    return -ENOENT;
}

static long tempbuf_print(void __user *data, size_t size)
{
    struct tempbuf_node *node;

    char *result;           // concatenating string
    size_t alloc_size;      // size that should allocate to 'result'
    size_t pos = 0;         // calculate the position to put the single string to 
    bool first_node = true; // check if it is the first node
    size_t copied;

    /* calculate the size to allocate to 'result'(concatenating string) */
    list_for_each_entry(node, &tempbuf_list, list){
        if (first_node)
            alloc_size += node->len;
        else
            alloc_size += node->len + 1;

        first_node = false;
    }   

    /* allocate memory space to 'result' */
    result = kmalloc(alloc_size + 1, GFP_KERNEL);
    if (!result)
        return -ENOMEM;

    /* concatenate all the strings to 'result' */
    list_for_each_entry(node, &tempbuf_list, list){
        if (pos > 0){
            result[pos] = ' ';
            pos++;
        }
        
        memcpy(result + pos, node->data, node->len);
        
        pos += node->len;
    }
    result[pos] = '\0';

    /* write the result to kernel buffer */
    printk(KERN_INFO "[tempbuf] %s\n", result);

    /* handle the buffer overflow */
    copied = min(alloc_size - 1, size - 1);
    result[copied] = '\0';

    /* copy the result string to user space */
    if (copy_to_user(data, result, copied + 1)){
        kfree(result);
        return -EFAULT;
    }

    kfree(result);

    return (long)copied;    // return the real copy size excluding '\0'
}

SYSCALL_DEFINE3(tempbuf, int, mode, void __user*, data, size_t, size)
{       
    if (!data || size == 0)
        return -EFAULT;
    
    switch (mode){
        case ADD:
            return tempbuf_add(data, size);

        case REMOVE:
            return tempbuf_remove(data, size);

        case PRINT:
            return tempbuf_print(data, size);

        default:
            return -EINVAL;     // invalid argument
    }

    return 0;
}