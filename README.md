---
title: Lab1_report

---

# Basic infomation
![image](https://hackmd.io/_uploads/Bk1qjV0qze.png)

# Compiling kernel
1. `make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- defconfig`
2. `make ARCH=riscv menuconfig`
3. change kernel local version
4. `make kernelrelease`
5. `make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- -j$(nproc)`

# Testing
1. put testing program into `initramfs.cpio.gz` (要先解壓縮再放進去)
2. 把寫好的 program (如 `sys_revstr.c` or `sys_tempbuf.c`) 放入 linux folder (如 `/linux/kernel`)

3. compile kernel with `make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- -j$(nproc)` 
    :::info
    run 3. in the container
    :::

4. run:
    ```    
    qemu-system-riscv64 -nographic -machine virt \
      -kernel linux/arch/riscv/boot/Image \
      -initrd initramfs.cpio.gz \
      -append "console=ttyS0 loglevel=3"
    ```
    which makes qemu have kernel image & file system
    
5. run the test program (ex: `./test_revstr`)

# Result screenshots
## `test_revstr`
![image](https://hackmd.io/_uploads/Hy9j_Owifx.png)

For `dmesg`:
![image](https://hackmd.io/_uploads/SyxlY_vjMl.png)

## `test_tempbuf`
![image](https://hackmd.io/_uploads/SypzKdwifl.png)

For `dmesg`:
![image](https://hackmd.io/_uploads/HJJEtuDsfg.png)


# Recording
## Prerequisite
1. 在 `include/linux/syscalls.h` 裡面寫 prototype
    :::spoiler Example:
    ![image](https://hackmd.io/_uploads/B1XuZs0qGl.png)
    :::

2. 在 `include/uapi/asm-generic/unistd.h` 裡面添加 system call number 以及 mapping (這個編號對應哪一個 function (prototype))
    :::spoiler Example:
    ![image](https://hackmd.io/_uploads/rkne1Nbsze.png)

    :::

3. 在 `kernel/sys_ni.c` 加上 system call fallback (當該 system call 沒有被編入 kernel 的時候執行並回傳 -ENOSYS)
    :::info
    system call 是否需要編入 kernel is optional
    :::
    :::spoiler Example:
    ![image](https://hackmd.io/_uploads/H1FlVTR5fl.png)
    :::
    
:::info 
**System call execution:**

kernel compile 時會把`SYSCALL_DEFINE2(revstr, ...)`裡面的`revstr` 利用 macro 擴展成 `sys_revstr`，而在`unistd.h`裡面有`__SYSCALL(451, sys_revstr)`，所以 system call table 會建立成以下:


| index | function (這個會變成右邊 address 的模樣)      |  function address   |
| ----- | --------  | --- |
| 451   | sys_revstr| 0x12345678    |

所以 user 在 userspace 執行 `syscall(__NR_revstr)` 時，kernel 會知道 `__NR_revstr` 是 451，而去 `0x12345678` 執行 `SYSCALL_DEFINE2(revstr, ...)`

::: 
    
# Implementation
## `sys_revstr.c`

1. Function definition:
![image](https://hackmd.io/_uploads/H1h6DN-ofe.png)
`SYSCALL_DEFINE2`: this function has two arguments, so write '2' after SYSCALL_DEFINE. 
And documentation says the argument should be present as (type, name) pair, so write it as above.

    `__user` 表示是指向 user space 的 pointer，不可隨意 dereference (即資料存在 user space)

2. string set up in the kernel:
![image](https://hackmd.io/_uploads/HJAJUTXsze.png)
since `str` is located in user space, so we should copy them to the kernel space 
(first allocate memory space for `str` in the kernel, and use `copy_from_user()` to copy the content of `str` to the kernel space)

3. write data to kernel ring buffer (for easy debugging):
![image](https://hackmd.io/_uploads/ByzrOpmszx.png)

4. reverse `str`(or `kbuf`) using two pointer method:
![image](https://hackmd.io/_uploads/r1fF_amiGe.png)

5. write the reversed `str`(or `kbuf`) to kernel ring buffer:
![image](https://hackmd.io/_uploads/H1ppu6Qize.png)

6. pass reversed `str` back to the kernel space using `copy_to_user()`:
![image](https://hackmd.io/_uploads/HJ5etTXoMg.png) 
finally, free the kernel memory sapce used for kbuf, and return 0.

## `sys_tempbuf.c`
1. Initialization:
![image](https://hackmd.io/_uploads/HJIYwBPszl.png)
    :::info
    initializae the header node for node list (it will not be traversed)
    :::

2. `SYSCALL_DEFINE3`:
![image](https://hackmd.io/_uploads/rJaNurvjfl.png)
use `switch` `case` to identify the modes

3. `tempbuf_add()`:
    :::success
    **Flow concept**: 
    create a new node struct => place data into the node (node->data) => add the new node to the node list using `list_add_tail()`
    :::
    a. ![image](https://hackmd.io/_uploads/B1jE9SPjfe.png)
create a node struct and allocate the memory space in kernel

    b. ![image](https://hackmd.io/_uploads/HkRh5rwjGg.png)
copy the data(str) into the node (from user space to kernel space) and handle the string termination sign

    c. ![image](https://hackmd.io/_uploads/ryxUsrwizx.png)
add the new node to the node list and then put the message into kernel ring buffer

4. `tempbuf_remove()`:
    :::success
    **Flow concept**:
    copy the target data into kernel space => traverse the node list to find the targeted node => if found, delete the node
    :::
    a. ![image](https://hackmd.io/_uploads/HkeahBPiGe.png)
define `node` & `next` for traversing (the current node and the next node)

    copy the data from user space to kernel space

    b. ![image](https://hackmd.io/_uploads/r1EYarPiMe.png)
traverse the node list (using `list_for_each_entry_safe` to find all or `list_for_each_entry` to find one) and delete the target node (if found)

5. `tempbuf_print()`:
    :::success
    **Flow concept**:
    count the size that should be allocated to the concatenated string and allocate it => concate all the strings to `result` string => pass it back to user space and place it to kernel ring buffer
    :::
    a. ![image](https://hackmd.io/_uploads/HJ6EIdDoze.png)
count the total size that should be allocated to `result` string

    b. ![image](https://hackmd.io/_uploads/HkqqL_wofl.png)
travrse each node and concate all to the tail of the result string (using `pos` variable)
    
    c. ![image](https://hackmd.io/_uploads/H12zPdPoGg.png)
place the message into kernel ring buffer using `printk()` and send it back to user space using `copy_to_user()`
    
# Things optional to be done
* write makefile for compiling kernel and emulation
* write kconfig