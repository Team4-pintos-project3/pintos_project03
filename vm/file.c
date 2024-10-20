/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "threads/vaddr.h"
#include "userprog/process.h"
#include "threads/mmu.h"

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
	file_page->file = aux->file;
	file_page->offset = aux->offset;
	file_page->read_bytes = aux->read_bytes;
	file_page->zero_bytes = aux->zero_bytes;

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

	if(page->frame != NULL){

	if(pml4_is_dirty (thread_current()->pml4, page->va)){
            file_write_at (file_page->file, page->frame->kva, file_page->read_bytes,
            file_page->offset);
			pml4_set_dirty(thread_current()->pml4, page->va, false);
    }

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
	if (!file || !addr || pg_ofs(addr) || offset % PGSIZE || !length)
		return NULL;

	// if (!is_user_vaddr(addr) || !is_user_vaddr(addr + length)) {
    //     return NULL;  // addr이나 addr + length가 커널 공간에 속할 경우 매핑 실패
    // }

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
		aux->file = file_reopen (file);
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
		length -= length<PGSIZE ? length : PGSIZE;
	}
	palloc_free_page(buffer);

	return addr;
}

/* Do the munmap */
void do_munmap(void *addr) {
    struct thread *cur = thread_current();
    size_t start = 0;

    while (true) {
        // 현재 가상 주소를 기준으로 SPT에서 페이지 정보 가져오기
        void *upage = addr + start;
        struct page *page = spt_find_page(&cur->spt, upage);

        // SPT에서 페이지를 찾지 못하면 루프 종료
        if (!page) {
            break;
        }

    //     // 더티 비트 확인
    // bool is_dirty = pml4_is_dirty(cur->pml4, page->va);

    // // 더티 비트를 디버그 로그에 출력
    // if (is_dirty) {
    //     printf("Page at address %p is dirty.\n", page->va);
    // } else {
    //     printf("Page at address %p is not dirty.\n", page->va);
    // }

		// SPT에서 페이지 정보 제거
			hash_delete (&thread_current()->spt.hash_table, &page->elem);
            spt_remove_page(&cur->spt, page);
		
        // 다음 페이지로 이동
        start += PGSIZE;
    }
}
