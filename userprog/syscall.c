#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "devices/input.h"
#include "threads/init.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "userprog/process.h"
#include "threads/flags.h"
#include "intrinsic.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "threads/synch.h"

void syscall_entry (void);
void syscall_handler (struct intr_frame *);
void chk_addr(void *addr);
void halt();
void exit(int status);
int exec (const char *cmd_line);
int wait (pid_t pid);
bool create(const char *file, unsigned initial_size);
bool remove(const char *file);
int open (const char *file);
int filesize (int fd);
int read (int fd, void *buffer, unsigned size);
int write (int fd, const void *buffer, unsigned size);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);

/* System call.
 *
 * Previously system call services was handled by the interrupt handler
 * (e.g. int 0x80 in linux). However, in x86-64, the manufacturer supplies
 * efficient path for requesting the system call, the `syscall` instruction.
 *
 * The syscall instruction works by reading the values from the the Model
 * Specific Register (MSR). For the details, see the manual. */

#define MSR_STAR 0xc0000081         /* Segment selector msr */
#define MSR_LSTAR 0xc0000082        /* Long mode SYSCALL target */
#define MSR_SYSCALL_MASK 0xc0000084 /* Mask for the eflags */

struct lock file_lock;

void
syscall_init (void) {
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48  |
			((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t) syscall_entry);

	lock_init(&file_lock);

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
			FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
}

/* The main system call interface */
void
syscall_handler (struct intr_frame *f UNUSED) {
	// TODO: Your implementation goes here.
	switch (f->R.rax){
		case SYS_HALT: halt();
			break;
		case SYS_EXIT: 
			f->R.rax = f->R.rdi;
			exit(f->R.rdi);
			break;
		case SYS_FORK:
			f->R.rax = process_fork(f->R.rdi, f);
			break;
		case SYS_EXEC:
			break;
		case SYS_WAIT:
			break;

		case SYS_CREATE:
			f->R.rax = create(f->R.rdi, f->R.rsi);
			break;
		case SYS_REMOVE:
			f->R.rax = remove(f->R.rdi);
			break;
		case SYS_OPEN:
			f->R.rax = open(f->R.rdi);
			break;
		case SYS_FILESIZE:
			f->R.rax = filesize(f->R.rdi);
			break;
		case SYS_READ:
			f->R.rax = read(f->R.rdi, f->R.rsi, f->R.rdx);
			break;
		case SYS_WRITE:
			f->R.rax = write(f->R.rdi, f->R.rsi, f->R.rdx);
			break;
		case SYS_SEEK:
			seek(f->R.rdi, f->R.rsi);
			break;
		case SYS_TELL:
			f->R.rax = tell(f->R.rdi);
			break;
		case SYS_CLOSE:
			close(f->R.rdi);
			break;
		default:
			break;
	}
	printf ("system call!\n");
	thread_exit ();
}

void chk_addr(void *addr){
	if (addr == NULL)
		exit(-1);
	
	if (!is_user_vaddr(addr))
		exit(-1);
	
}

void halt(){
	power_off();
}

void exit(int status){
	printf("%d", status);
	thread_exit();
}

int exec (const char *cmd_line){

}

int wait (pid_t pid){

}

bool create(const char *file, unsigned initial_size){
	chk_addr(file);
	return filesys_create(file, initial_size);
}

bool remove(const char *file){
	chk_addr(file);
	return filesys_remove(file);
}

int open (const char *file){
	chk_addr(file);
	struct file *f = filesys_open(file);
	if (f == NULL)
		return -1;
	
	return process_set_file(f);
}

int filesize (int fd){
	struct file * f = thread_current()->fdt[fd];
	if(f == NULL)
		return 0;
	return file_length(f);
}

int read (int fd, void *buffer, unsigned size){
	if (FILE_TABLE_LIMIT <= fd)
		return -1;
	
	struct file * f = thread_current()->fdt[fd];
	if(f == NULL || fd == 1)
		return -1;
	if(fd == 0){
		for (int i = 0; i < size; i++)
			*((char*)buffer + i) = input_getc();
	}

	return file_read(f, buffer, size);
}

int write (int fd, const void *buffer, unsigned size){
	if (FILE_TABLE_LIMIT <= fd)
		return -1;
	struct file *f = thread_current()->fdt[fd];
	if(f == NULL || fd == 0)
		return -1;
	off_t write_size = file_write(f, buffer, size);
	if (fd == 1)
		putbuf(buffer, write_size);
	
	return write_size;
}

void seek (int fd, unsigned position){
	if (FILE_TABLE_LIMIT <= fd)
		return -1;
	struct file *f = thread_current()->fdt[fd];
	if(f == NULL || fd < 2)
		return -1;

	file_seek(f, position);
}

unsigned tell (int fd){
	if (FILE_TABLE_LIMIT <= fd)
		return -1;
	struct file *f = thread_current()->fdt[fd];
	if(f == NULL || fd < 2)
		return -1;
	return file_tell(f);
}

void close (int fd){
	if (FILE_TABLE_LIMIT <= fd)
		return -1;
	thread_current()->fdt[fd] = NULL;
}
