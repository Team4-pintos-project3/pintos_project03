/* anon.c: Implementation of page for non-disk image (a.k.a. anonymous page). */

#include "vm/vm.h"
#include "threads/mmu.h"
#include <string.h>
#include <bitmap.h>

/* DO NOT MODIFY BELOW LINE */
static struct disk *swap_disk;
static bool anon_swap_in (struct page *page, void *kva);
static bool anon_swap_out (struct page *page);
static void anon_destroy (struct page *page);

static disk_sector_t get_swap_slot(void);
static void clear_swap_slot(disk_sector_t clear_sector);

struct swap_table {
	struct lock lock;               /* Mutual exclusion. */
	struct bitmap *used_map;        /* Bitmap of free pages. */
};

struct swap_table st;

/* DO NOT MODIFY this struct */
static const struct page_operations anon_ops = {
	.swap_in = anon_swap_in,
	.swap_out = anon_swap_out,
	.destroy = anon_destroy,
	.type = VM_ANON,
};

/* Initialize the data for anonymous pages */
void
vm_anon_init (void) {
	/* TODO: Set up the swap_disk. */
	// swap_disk = NULL;
	swap_disk = disk_get (1, 1);
	if (swap_disk == NULL)
		PANIC ("hd1:1 (hdb) not present, file system initialization failed");
}

void
swap_table_init(void) {
	st.used_map = bitmap_create(disk_size(swap_disk));
	lock_init(&st.lock);
}

/* Initialize the file mapping */
bool
anon_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &anon_ops;

	struct anon_page *anon_page = &page->anon;
	return true;
}

/* Swap in the page by read contents from the swap disk. */
static bool
anon_swap_in (struct page *page, void *kva) {
	struct anon_page *anon_page = &page->anon;
	
	lock_acquire(&st.lock);
	clear_swap_slot(anon_page->sector_start);
	disk_sector_t sector_index;
	for (sector_index = 0; sector_index < PGSIZE/DISK_SECTOR_SIZE; sector_index ++) {
		disk_read(swap_disk, anon_page->sector_start + sector_index, page->frame->kva + sector_index*DISK_SECTOR_SIZE);
	}
	lock_release(&st.lock);
	if (!pml4_set_page(thread_current()->pml4, page->va, page->frame->kva, page->writable))
		return false;
	
	return true;
}

/* Swap out the page by writing contents to the swap disk. */
static bool
anon_swap_out (struct page *page) {
	struct anon_page *anon_page = &page->anon;
	if (page->frame != NULL) {
		lock_acquire(&st.lock);
		anon_page->sector_start = get_swap_slot();
		disk_sector_t sector_index;
		for (sector_index = 0; sector_index < PGSIZE/DISK_SECTOR_SIZE; sector_index ++) {
			disk_write(swap_disk, anon_page->sector_start + sector_index, page->frame->kva + sector_index*DISK_SECTOR_SIZE);
		}
		memset(page->frame->kva, 0, PGSIZE);
		lock_release(&st.lock);
		page->frame = NULL;
		pml4_clear_page(thread_current()->pml4, page->va);
	} else {
		return false;
	}
	return true;
}

/* Destroy the anonymous page. PAGE will be freed by the caller. */
static void
anon_destroy (struct page *page) {
	struct anon_page *anon_page = &page->anon;
	if (page->frame != NULL) {
		palloc_free_page(page->frame->kva);
		list_remove(&page->frame->elem);
		free(page->frame);
		pml4_clear_page(thread_current()->pml4, page->va);
	} else {
		clear_swap_slot(anon_page->sector_start);
	}
	
}

static disk_sector_t
get_swap_slot(void) {
	return bitmap_scan_and_flip(st.used_map, 0, (PGSIZE/DISK_SECTOR_SIZE), false);
}

static void
clear_swap_slot(disk_sector_t clear_sector) {
	bitmap_set_multiple(st.used_map, clear_sector, (PGSIZE/DISK_SECTOR_SIZE), false);
}