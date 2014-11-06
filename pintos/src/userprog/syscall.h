#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include "threads/synch.h"

#define WRITE 1
#define READ 0

struct lock lock;

struct file_attr {
	int fd;
	struct file *file;
	struct list_elem elem;
};

#endif /* userprog/syscall.h */