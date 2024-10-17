/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "threads/vaddr.h"
#include "userprog/process.h"
#include <string.h>
#include "threads/mmu.h"
#include "hash.h"

static bool file_backed_swap_in (struct page *page, void *kva);
static bool file_backed_swap_out (struct page *page);
static void file_backed_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations file_ops = {
	.swap_in = file_backed_swap_in,
	.swap_out = file_backed_swap_out,
	.destroy = file_backed_destroy,
	.type = VM_FILE,
};

/* The initializer of file vm */
void
vm_file_init (void) {
}

/* Initialize the file backed page */
bool
file_backed_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &file_ops;
	struct file_page *aux = page->uninit.aux;
	struct file_page *file_page = &page->file;
	memcpy(file_page, aux, sizeof(struct file_page));
}

/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in (struct page *page, void *kva) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
	if (page->frame != NULL) {
		if (pml4_is_dirty(thread_current()->pml4, page->va))
			file_write_at(page->file.file, page->frame->kva, page->file.read_bytes, page->file.offset);
		palloc_free_page(page->frame->kva);
		free(page->frame);
	}
	
	pml4_clear_page(thread_current()->pml4, page->va);
	free(file_page->file);
}

/* Do the mmap */
void *
do_mmap (void *addr, size_t length, int writable,
		struct file *file, off_t offset) {
	if (!file || !addr || pg_ofs(addr) || is_kernel_vaddr(addr) || offset % PGSIZE || (long)length <= 0)
		return NULL;

	char a;
	if (!file_read_at(file, &a, 1, 0))
		return NULL;

	void *upage = addr;
	uint64_t *buffer = palloc_get_page(0);
	size_t read_bytes;
	size_t zero_bytes;
	
	struct list *mapped_list = (struct list *)malloc(sizeof(struct list));
	list_init(mapped_list);

	while (length > 0) {
		read_bytes = file_read_at(file, buffer, PGSIZE, offset);
		zero_bytes = PGSIZE - read_bytes;
		
		struct file_page *aux = (struct file_page *)malloc(sizeof(struct file_page));

		aux->offset = offset;
		aux->read_bytes = read_bytes;
		aux->zero_bytes = zero_bytes;
		if (!read_bytes) {
			aux->file = file;
			if (!vm_alloc_page_with_initializer (VM_ANON, upage,
						writable, lazy_load_segment, aux))
				return NULL;
		} else {
			aux->file = file_reopen(file);
			if (!vm_alloc_page_with_initializer (VM_FILE, upage,
						writable, lazy_load_segment, aux))
				return NULL;
		}
		
		struct page *page = spt_find_page(&thread_current()->spt, upage);
		list_push_back(mapped_list, &page->mapped_elem);

		offset += PGSIZE;
		upage += PGSIZE;
		length -= length < PGSIZE ? length : PGSIZE;
	}
	palloc_free_page(buffer);

	return addr;
}

/* Do the munmap */
void
do_munmap (void *addr) {
	struct page *page = spt_find_page(&thread_current()->spt, addr);
	struct list_elem *head = list_prev(&page->mapped_elem);
	if (head->prev)
		exit(-1);
	struct list *list = list_entry(head, struct list, head);
	while (!list_empty (list)) {
		struct list_elem *e = list_pop_front (list);
		page = list_entry(e, struct page, mapped_elem);
		hash_delete(&thread_current()->spt.hash_table,&page->elem);
		vm_dealloc_page(page);
	}

	free(list);
}
