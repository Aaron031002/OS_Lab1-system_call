#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/list.h>

#define ADD 0
#define REMOVE 1
#define PRINT 2

struct tempbuf_node {
    char *data;
    size_t len;
    struct list_head list;
};

/* initialize the list (create sentinel head) */
static LIST_HEAD(tempbuf_list);

static long tempbuf_add(void __user *data, size_t size)
{
    struct tempbuf_node* node;
}

static long tempbuf_remove(void __user *data, size_t size)
{

}

static long tempbuf_print(void __user *data, size_t size)
{

}

SYSCALL_DEFINE3(__NR_tempbuf, enum , mode, void __user*, data, size_t, size)
{       
    switch (mode){
        case ADD:
            return tempbuf_add(data, size);

        case REMOVE:
            return tempbuf_remove(data, size);

        case PRINT:
            return tempbuf_print(data, size);

        default:
            return 
    }

    return 0;
}