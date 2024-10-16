/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "threads/vaddr.h"
#include "userprog/process.h"

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

	struct file_page *file_page = &page->file;
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
}

/* Do the mmap */
void *
do_mmap (void *addr, size_t length, int writable,
		struct file *file, off_t offset) {
	if (!file || !addr || pg_ofs(addr) || offset % PGSIZE || !length)
		return NULL;

	char a;
	if (!file_read_at(file, &a, 1, 0))
		return NULL;

	void *upage = addr;
	uint64_t *buffer = palloc_get_page(0);
	size_t read_bytes;
	size_t zero_bytes;

	while (length > 0) {
		read_bytes = file_read_at(file, buffer, PGSIZE, offset);
		zero_bytes = PGSIZE - read_bytes;
		
		struct file_page *aux = (struct file_page *)malloc(sizeof(struct file_page));
		aux->file = file;
		aux->offset = offset;
		aux->read_bytes = read_bytes;
		aux->zero_bytes = zero_bytes;
		if (!read_bytes) {
			if (!vm_alloc_page_with_initializer (VM_ANON, upage,
						writable, lazy_load_segment, aux))
				return NULL;
		} else {
			if (!vm_alloc_page_with_initializer (VM_FILE, upage,
						writable, lazy_load_segment, aux))
				return NULL;
		}
		
		offset += PGSIZE;
		upage += PGSIZE;
		length -= PGSIZE;
	}
	palloc_free_page(buffer);

	return addr;
}

/* Do the munmap */
void
do_munmap (void *addr) {
}
