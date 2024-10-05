#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include "filesys/off_t.h"

typedef int pid_t;

#define FILE_TABLE_LIMIT 1<<10
#define FDT_PAGE_SIZE 2

tid_t process_create_initd (const char *file_name);
tid_t process_fork (const char *name, struct intr_frame *if_);
int process_exec (void *f_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (struct thread *next);
off_t process_set_file(struct file *f);
struct thread *get_child_process(tid_t tid);

void file_lock_init();
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


#endif /* userprog/process.h */
