#include <meram/meram.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <uiomux/uiomux.h>
#include "meram_priv.h"

#define UIOMUX_SH_MERAM 1
#define UIOMUX_SH_MERAM_MEM 2

static UIOMux *uiomux = NULL;
static pthread_mutex_t uiomux_mutex = PTHREAD_MUTEX_INITIALIZER;

static int ref_count = 0;

static const char *uios[] = {
	"MERAM",
	NULL
};

struct MERAM {
	UIOMux *uiomux;
	unsigned long paddr;
	void *vaddr;
	unsigned long len;
	unsigned long mem_paddr;
	void *mem_vaddr;
	unsigned long mem_len;
	struct reserved_address *reserved_mem;
};

struct ICB {
	int locked;
	unsigned long offset;
	unsigned long lock_offset;
	unsigned long len;
};

struct MERAM_REG {
	int locked;
	unsigned long offset;
	unsigned long len;
};


MERAM *meram_open(void)
{
	MERAM *meram;
	int ret;
	static struct reserved_address *reserved_mem;

	meram = calloc(1, sizeof(*meram));
	if (!meram)
		return NULL;

	pthread_mutex_lock(&uiomux_mutex);
	ref_count++;
	if (uiomux == NULL) {
		uiomux = uiomux_open_named(uios);
	}
	make_reserved_addr_list(CONFIG_FILE, &reserved_mem);
	pthread_mutex_unlock(&uiomux_mutex);

	if (!uiomux)
		return NULL;
	meram->uiomux = uiomux;
	ret = uiomux_get_mmio(uiomux, UIOMUX_SH_MERAM,
		&meram->paddr,
		&meram->len,
		&meram->vaddr);

	ret &= uiomux_get_mem(uiomux, UIOMUX_SH_MERAM,
		&meram->mem_paddr,
		&meram->mem_len,
		&meram->mem_vaddr);
	if (!ret) {
		pthread_mutex_lock(&uiomux_mutex);
		if (ref_count == 1) {
			uiomux_close(uiomux);
			uiomux = NULL;
		}	
		pthread_mutex_unlock(&uiomux_mutex);
		/*TODO: check ref count*/
		return NULL;	
	}
	meram->reserved_mem = reserved_mem;
	return meram;
}

void meram_close(MERAM *meram)
{
	pthread_mutex_lock(&uiomux_mutex);
	ref_count--;
	if (ref_count == 1) {
		uiomux_close(uiomux);
		uiomux = NULL;
		delete_reserved_addr_list(meram->reserved_mem);
	}	
	pthread_mutex_unlock(&uiomux_mutex);
	free(meram);
}


ICB *meram_lock_icb(MERAM *meram, int index)
{
	ICB *icb;
        int pagesize = sysconf(_SC_PAGESIZE);
	icb = calloc (1, sizeof (*icb));
	/*lock indeces 1 per icb positioned after memory pages*/
	icb->lock_offset = ((meram->mem_len + pagesize - 1 )/pagesize) + index;
	/*offset and size determination*/
	icb->offset = 0x400 + index * 0x20;
	icb->len = 0x20;
	
	if (uiomux_partial_lock(meram->uiomux, UIOMUX_SH_MERAM,
		icb->lock_offset, icb->len) < 0) {
		free (icb);
		return NULL;
	}
	icb->locked = 1;
	return icb; 
}


void meram_unlock_icb(MERAM *meram, ICB *icb)
{
	/*partial uiomux unlock*/
	icb->locked = 0;
	uiomux_partial_unlock(meram->uiomux, UIOMUX_SH_MERAM,
		icb->lock_offset, icb->len);

	free(icb);	
}

MERAM_REG *meram_lock_reg(MERAM *meram)
{
	MERAM_REG *meram_reg;
	
	meram_reg = calloc (1, sizeof (MERAM_REG));
	/*offset and size determination*/
	meram_reg->offset = 0;
	meram_reg->len = 0x80;

	uiomux_lock(meram->uiomux, UIOMUX_SH_MERAM);

	return meram_reg; //* or NULL on locking error*/
}
void meram_unlock_reg(MERAM *meram, MERAM_REG *meram_reg)
{
	uiomux_unlock(meram->uiomux, UIOMUX_SH_MERAM);
	free(meram_reg);	
}
int meram_alloc_memory_block(MERAM *meram, int size)
{
	int alloc_size = size << 10;
	void *alloc_ptr = NULL;
	struct reserved_address *current;
	current = meram->reserved_mem;
	/* uiomux_malloc has minimum 1 page (4k) alignment*/
	while (!alloc_ptr) {	
		alloc_ptr = uiomux_malloc(meram->uiomux, UIOMUX_SH_MERAM,
			alloc_size, 1024);
		if (!alloc_ptr) 
			return -1;
		current = meram->reserved_mem;
		while (current) {
			unsigned long alloc_beg = (unsigned long)
				(alloc_ptr - meram->mem_vaddr);
			unsigned long alloc_end = alloc_beg + alloc_size - 
				(1 << 10);
			unsigned long cur_beg = current->start_block << 10;
			unsigned long cur_end = current->end_block << 10;
			if (alloc_beg > cur_end ) {
				current = current->next;
				continue;
			}
			if ((alloc_end) < cur_beg) {
				current = current->next;
				continue;
			}
			if (alloc_beg < cur_beg) {
				fprintf(stderr, "1 Freeing %x bytes at %p\n",
					cur_beg - alloc_beg, alloc_ptr);
				uiomux_free(meram->uiomux, UIOMUX_SH_MERAM,
					alloc_ptr,
					cur_beg - alloc_beg);
			}
			if (alloc_end > cur_end) {
				fprintf(stderr, "2 Freeing %x bytes at %p\n",
					alloc_end - cur_end,
					meram->mem_vaddr + cur_end);
				uiomux_free(meram->uiomux, UIOMUX_SH_MERAM,
					meram->mem_vaddr + cur_end +
					(1 << 10), alloc_end - cur_end);
			}
			alloc_ptr = NULL;
			break;
		}
	}
		
	return ((unsigned long) (alloc_ptr - meram->mem_vaddr)) >> 10;
}
void meram_free_memory_block(MERAM *meram, int offset, int size)
{
	int alloc_size = size << 10;
	void *alloc_ptr = meram->mem_vaddr + (offset << 10);
	uiomux_free(meram->uiomux, UIOMUX_SH_MERAM, alloc_ptr, alloc_size);
}

void meram_read_icb(MERAM *meram, ICB *icb, int offset,
		unsigned long *read_val)
{
	volatile unsigned long *reg = meram->vaddr + icb->offset + offset;
	*read_val = *reg;
}
void meram_write_icb(MERAM *meram, ICB *icb, int offset, unsigned long val)
{
	volatile unsigned long *reg = meram->vaddr + icb->offset + offset;
	*reg = val;
}
void meram_read_reg(MERAM *meram, MERAM_REG *meram_reg, int offset,
		unsigned long *read_val)
{
	volatile unsigned long *reg = meram->vaddr + meram_reg->offset + offset;
	*read_val = *reg;
}
void meram_write_reg(MERAM *meram, MERAM_REG *meram_reg, int offset,
		unsigned long val)
{
	volatile unsigned long *reg = meram->vaddr + meram_reg->offset + offset;
	*reg = val;
}
#define TOKENS " \t"
#define LINE_LEN 255
int
make_reserved_addr_list(char *infile, struct reserved_address **head)
{
	int len = LINE_LEN;
	struct reserved_address *current = NULL, *prev;
	int i, num_fields;
	char **fields;
	FILE *cfg_file;
	char linedata[LINE_LEN];
	int line_cnt = 1;
	char *id;

	cfg_file = fopen (infile, "r");
	if (!cfg_file) 
		return -1;

	while (fgets(linedata, len, cfg_file)) {
		id = strtok(linedata, TOKENS);
		if (id[0] == '#' || id[0] == '\n' || id[0] == 0) {
			line_cnt ++;
			continue;
		}
		if (!strcmp(id, "reserved")) {
			num_fields = 2;
			fields = calloc (num_fields, sizeof (char *));
			for (i = 0; i < num_fields; i++) {
				if (!(fields[i] = strtok(NULL, TOKENS))) {
					fprintf(stderr, "Line %d: "
						"Invalid data\n", line_cnt);
					fclose(cfg_file);
					return -1;			
				}
			}
			if (strtok(NULL, TOKENS)) {
				fprintf(stderr, "Line %d: Too few tokens\n",
					line_cnt);
				fclose(cfg_file);
				return -1;			
			}
			if (!current) 
				current = *head = calloc (1, 
					sizeof (struct reserved_address));
			else {
				current = calloc (1, 
					sizeof (struct reserved_address));
				prev->next = current;
			}
			current->start_block = atoi(fields[0]);
			current->end_block = atoi(fields[1]);
			current->next = NULL;
			prev = current;
		}
		line_cnt ++;
	}
	fclose(cfg_file);
	return 0;
}
void
delete_reserved_addr_list(struct reserved_address *head)
{
	struct reserved_address *next = head;
	while (head) {
		next = head->next;
		free(head);
		head = next;	
	}
}
