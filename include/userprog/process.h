#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include "include/filesys/off_t.h"

#define FILE_TABLE_LIMIT 1<<10

tid_t process_create_initd (const char *file_name);
tid_t process_fork (const char *name, struct intr_frame *if_);
int process_exec (void *f_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (struct thread *next);
off_t process_set_file(struct file *f);

#endif /* userprog/process.h */
