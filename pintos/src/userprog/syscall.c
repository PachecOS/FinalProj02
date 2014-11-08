#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <user/syscall.h>
#include "devices/input.h"
#include "devices/shutdown.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/interrupt.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"

#define BOTTOM 0x08048000


static void syscall_handler (struct intr_frame *);
void syscall_init (void);
int mem_switch_to_kernel(const void * ptr );
void check_ptr(const void* addr);
void strip_args(struct intr_frame *f, int total, int* arg);
struct file* get_file (int fd);
void check_arg(const void* arg);

void
syscall_init (void) 
{
  lock_init(&lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  //printf("I am here. ");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int arg[3];
  int esp = mem_switch_to_kernel((const void*) f->esp);
  switch(*(int *) esp)
  {
  	case SYS_HALT:
  	{
  		//printf("HALT\n");
  		halt();
  		break;
  	}
  	case SYS_EXIT:
  	{  		
  		//printf("EXIT\n");
  		strip_args(f, 1, &arg[0]);
  		exit(arg[0]);
  		break;
  	}
  	case SYS_EXEC:
  	{
  		//printf("EXEC\n");
  		strip_args(f, 1, &arg[0]);
  		check_arg((const void*) arg[0]);

  		arg[0] = mem_switch_to_kernel((const void*)arg[0]);
  		f->eax = exec((const char*) arg[0]);
  		break;
  	}
  	case SYS_WAIT:
  	{
  		//printf("WAIT\n");
  		strip_args(f, 1, &arg[0]);
  		f->eax = wait(arg[0]);
  		break;
  	}
  	case SYS_REMOVE:
  	{
  		//printf("REMOVE\n");
  		strip_args(f, 1, &arg[0]);
  		check_arg((const void*) arg[0]);

  		arg[0] = mem_switch_to_kernel((const void*) arg[0]);
  		f->eax = remove((const char*) arg[0]);
  		break;
  	}
  	case SYS_CREATE:
  	{
  		//printf("CREATE\n");
  		strip_args(f, 2, &arg[0]);
  		check_arg((const void*) arg[0]);

  		arg[0] = mem_switch_to_kernel((const void*) arg[0]);
  		f->eax = create((const char*) arg[0], (unsigned)arg[1]);
  		break;
  	}
  	case SYS_OPEN:
  	{
  		//printf("OPEN\n");
  		strip_args(f, 1, &arg[0]);
  		check_arg((const void*) arg[0]);

  		arg[0] = mem_switch_to_kernel((const void*) arg[0]);
  		f->eax = open((const char *) arg[0]);
  		break;
  	}
  	case SYS_FILESIZE:
  	{
  		//printf("FILESIZE\n");
  		strip_args(f, 1, &arg[0]);
  		f->eax = filesize(arg[0]);
  		break;
  	}
  	case SYS_READ:
  	{
  		//printf("READ\n");
  		strip_args(f, 3, &arg[0]);

  		arg[1] = mem_switch_to_kernel((const void *) arg[1]);
  		f->eax = read(arg[0], (void *) arg[1], (unsigned) arg[2]);
  		break;
  	}
  	case SYS_WRITE:
  	{
  		//printf("WRITE\n");
  		strip_args(f, 3, &arg[0]);

  		arg[1] = mem_switch_to_kernel((const void *) arg[1]);
  		f->eax = write(arg[0], (const void *) arg[1], (unsigned) arg[2]);
  		break;
  	}
  	case SYS_SEEK:
  	{
  		//printf("SEEK\n");
  		strip_args(f, 2, &arg[0]);
  		seek(arg[0], (unsigned) arg[1]);
  		break;
  	}
  	case SYS_TELL:
  	{
  		//printf("TELL\n");
  		strip_args(f, 1, &arg[0]);
  		f->eax = tell(arg[0]);
  		break;
  	}
  	case SYS_CLOSE:
  	{
  		//printf("CLOSE\n");
  		strip_args(f, 1, &arg[0]);
  		close(arg[0]);
  		break;
  	}

  }
}

void
check_arg(const void* arg) {

	while (* (char *) mem_switch_to_kernel(arg) != 0)
	{
		arg = (char *) arg + 1;
	}
}

/* The purpose of this funciton is to get the
   page that maps the kernel address corressponding
   to the physical address. Returns the pointer
   to that space */
int
mem_switch_to_kernel(const void * ptr )
{

	check_ptr(ptr);

	struct thread *t = thread_current();
	uint32_t page = (uint32_t)t->pagedir;
	void * valid = pagedir_get_page((uint32_t *)page, ptr);

	if(!valid)
	{
		exit(-1);
	} 

	return (int) valid;
}

/* Checks if the pointer is valid i.e if it is 
   in between PHYS_BASE and 0x08048000 */
void
check_ptr(const void* addr)
{
	if(addr < (void *)BOTTOM) 
	{
		exit(-1);
	}
	if(!(addr < (void *)PHYS_BASE))
	{
		exit(-1);
	}
	if(!is_user_vaddr(addr))
	{
		exit(-1);
	}

}

/* Gets the args off of the stack */
void
strip_args(struct intr_frame *f, int total, int* arg)
{
	int i;
	int *temp;
	for(i = 0; i < total; i++)
	{
		temp = (int *)f->esp + i + 1;
		check_ptr((const void*)temp);
		arg[i] = *temp;
	}
}

/* Terminates Pintos by calling shutdown_power_off() */
void
halt(void)
{
	shutdown_power_off();
}

/* Terminates the current user program */
void
exit (int status) 
{
 struct thread *curr_thread = thread_current();
 struct list_elem *e;

 if (curr_thread->parent)
 {
 	 curr_thread->status = status;
 }  

  printf ("%s: exit(%d)\n", curr_thread->name, status);
  thread_exit();
}
/* Runs the executable whose name is given in cmd_line*/
pid_t
exec(const char* cmd_line)
{
	pid_t name = process_execute(cmd_line);
	/*struct wait_info *wait_info;
	wait_resources_init(&wait_info);
	struct child_info *child;
	struct thread *t = thread_current();
	struct list_elem *e, *next;

	for(e = list_begin(&t->child_list); e!= list_end(&t->child_list);
				e = list_next(e))
	{
		struct child_info *keep_child = list_entry(e, struct child_info, elem);
		if(name == child->pid)
		{
			keep_child = child->pid;
		}
		else {
			keep_child = NULL;
		}
	}

	while(keep_child->load == 0)
	{
		cond_wait(&(wait_info->condition), &(wait_info->mutex_lock));
	}

	if(keep_child->load == 2)
	{
		while(e!=list_end(&t->child_list))
		{
			next = list_next(e);
			child = list_entry(e, struct child_info, elem);
			list_remove(&child->elem);
			free(child);
			e = next;
		}

	}

	*/
	return name;
		
}

/* Waits for a child process pid and retrieves the childs
   exit status */
int
wait (pid_t pid) 
{
	return process_wait(pid);
}

/* Creates a new file *file intially sized initial_size
   in bytes */
bool
create (const char *file, unsigned initial_size)
{
	bool created = false;
	lock_acquire(&lock);
	if(filesys_create(file, initial_size))
	{
		created = true;
		return created;
	} else {
		return created;
	}
	lock_release(&lock);
}

bool
remove (const char *file)
{
	bool removed = false;
	lock_acquire(&lock);
	if(filesys_remove(file) == true)
	{
		removed = true;
		return removed;
	} else {
		return removed;
	}
	lock_release(&lock);
}

int
open (const char *file)
{
	lock_acquire(&lock);
	struct file *f = filesys_open(file);
	struct thread *t = thread_current();
	// Space for the file attr struct 
	struct file_attr *fa = malloc(sizeof (struct file_attr));
	fa ->file = f;

	// Need to check if the file attr is valid
	if(!fa) 
	{
		lock_release(&lock);
		return -1;
	}
	// Update the fd's and the thread's fd
	fa->fd = t->fd;
	t->fd++;

	// Put the file elem on that thread's file list
	list_push_back(t->files, &fa->elem);
	free(fa);
	lock_release(&lock);
	return fa->fd;
	

}

/* Returns the size, in bytes, of the file open as fd */
int
filesize (int fd)
{
	lock_acquire(&lock);
	struct file *f = get_file(fd);
	int size = file_length(f);
	lock_release(&lock);
	return size;
}

/* Reads size bytes from the file open as fd into buffer.
   Returns the number of bytes actually read (0 at the
   end of a file), or -1 if the file could not be read.
   Fd 0 reads from the keyboard using input_getc(). */
int
read (int fd, void *buffer, unsigned size)
{
	int i;
	uint8_t* l_buff = (uint8_t*) buffer;

	if(fd == READ)
	{
		for(i = 0; i < (int)size; i++)
		{
			l_buff[i] = input_getc();
 		}
 		return size;
	}

	lock_acquire(&lock);
	struct file *f = get_file(fd);
	int read_f = file_read(f, buffer, size);
	
	lock_release(&lock);
	return read_f;
}

int
write (int fd, const void *buffer, unsigned size)
{	
	if(fd == WRITE)
	{
		putbuf(buffer, size);
		return size;
	}

	lock_acquire(&lock);
	struct file *f = get_file(fd);
	int file_w = file_write(f, buffer, size);
	lock_release(&lock);
	return file_w;
}

void
seek (int fd, unsigned position)
{	
	lock_acquire(&lock);
	struct file *f = get_file(fd);
	file_seek(f, position);
	lock_release(&lock);
}

/* Returns the position of the next byte to be read
   or written in open file fd, expressed as in bytes
   from the beginning of the file. */
unsigned 
tell (int fd)
{
	lock_acquire(&lock);
	struct file *f = get_file(fd);
	int next = 0;
	// Get the file attributed to this fd
	if(f != NULL) 
	{
		// file_tell gets the offset of the next byte
		next = file_tell(f);
	}
	lock_release(&lock);
	return next;
	

}

void
close (int fd)
{
	lock_acquire(&lock);
	struct thread *t = thread_current();
	struct list_elem *e;
	for(e=list_begin(t->files); e != list_end(t->files); e = list_next(e))
	{
		struct file_attr *fa = list_entry(e, struct file_attr, elem);
		if(fa->fd == fd)
		{
			file_close(fa->file);
			list_remove(&fa->elem);
			free(fa);
		}
		if(fd == -1)
		{
			file_close(fa->file);
			list_remove(&fa->elem);
			free(fa);
		}
		else if(fd > -1)
		{
			return;
		}
	}
	lock_release(&lock);

}


/* Helper function that returns the file associated to the FD
   int fd. */
struct 
file* get_file (int fd)
{
	struct thread *t = thread_current();
	struct list_elem *e;

	for(e = list_begin(t->files); e != list_end(t->files); e = list_next(e))
	{
		struct file_attr *fa = list_entry(e, struct file_attr, elem);
		if(fa->fd == fd)
		{
			return fa->file;
		} 
		else 
		{
			return NULL;
		}
	}
}
