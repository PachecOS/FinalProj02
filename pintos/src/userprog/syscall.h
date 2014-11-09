#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include "threads/synch.h"
#include "threads/thread.h"

#define WRITE 1
#define READ 0

struct lock lock;

struct file_attr {
	int fd;
	struct file *file;
	struct list_elem elem;
};

struct child_info {
	int status;
    bool child_exit;
    bool parent_exit;
    bool parent_waiting_on_child;
	int pid;
	struct wait_info *w_info;
	int load;
	struct semaphore lock_sema;
	struct list_elem *elem;
};

#endif /* userprog/syscall.h */