#include <meram/meram.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <uiomux/uiomux.h>
#include <stdint.h>
#include "meram_priv.h"

#define ALIGN2UP(_p, _w)		\
{					\
	(_p) = ((_w) - 1);		\
	(_p) = (_p) | ((_p) >> 1);	\
	(_p) = (_p) | ((_p) >> 2);	\
	(_p) = (_p) | ((_p) >> 4);	\
	(_p) = (_p) | ((_p) >> 8);	\
	(_p) = (_p) | ((_p) >> 16);	\
	(_p) += 1;			\
}

typedef uint8_t u8;

static UIOMux *uiomux = NULL;
static pthread_mutex_t uiomux_mutex = PTHREAD_MUTEX_INITIALIZER;

static unsigned long icb_inuse[(MAX_ICB_INDEX + 1) >> 5];
static pthread_mutex_t icb_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t icb_wq = PTHREAD_COND_INITIALIZER;

static int ref_count = 0;

static const char *uios[] = {
	"MERAM",
	"IPMMU",
	NULL
};

MERAM *meram_open(void)
{
	MERAM *meram;
	int ret;
	static struct reserved_address *reserved_mem;
	static struct ipmmui_settings *ipmmui_config;

	meram = calloc(1, sizeof(*meram));
	if (!meram)
		return NULL;

	pthread_mutex_lock(&uiomux_mutex);
	ref_count++;
	if (uiomux == NULL) {
		uiomux = uiomux_open_named(uios);
		parse_config_file(CONFIG_FILE, &reserved_mem, &ipmmui_config);
	}
	pthread_mutex_unlock(&uiomux_mutex);

	if (!uiomux) {
		free(meram);
		return NULL;
	}
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
		free(meram);
		return NULL;
	}
	meram->reserved_mem = reserved_mem;
	meram->ipmmui_config = ipmmui_config;
	return meram;
}

void meram_close(MERAM *meram)
{
	pthread_mutex_lock(&uiomux_mutex);
	ref_count--;
	if (ref_count == 0) {
		uiomux_close(uiomux);
		uiomux = NULL;
		delete_reserved_addr_list(meram->reserved_mem);
		meram->reserved_mem = NULL;
		delete_ipmmui_settings(meram->ipmmui_config);
		meram->ipmmui_config = NULL;
	}
	pthread_mutex_unlock(&uiomux_mutex);
	free(meram);
}

static inline ICB *__meram_lock_icb(MERAM *meram, int index, int sync)
{
	ICB *icb;
        int pagesize = sysconf(_SC_PAGESIZE);
	unsigned long mask;
	int slot, ret;

	if ((index < 0) || (index > MAX_ICB_INDEX))
		return NULL;

	/* wait until the target icb is available */
	slot = index >> 5;
	mask = 1UL << (index & 31);
	pthread_mutex_lock(&icb_mutex);
	while (icb_inuse[slot] & mask) {
		if (!sync) {
			pthread_mutex_unlock(&icb_mutex);
			return NULL;
		}
		pthread_cond_wait(&icb_wq, &icb_mutex);
	}
	icb_inuse[slot] |= mask;
	pthread_mutex_unlock(&icb_mutex);

	icb = calloc (1, sizeof (*icb));
	/*lock indeces 1 per icb positioned after memory pages*/
	icb->lock_offset = ((meram->mem_len + pagesize - 1 )/pagesize) + index;
	/*offset and size determination*/
	icb->offset = 0x400 + index * 0x20;
	icb->len = 0x20;
	icb->index = index;
	icb->mem_block = icb->mem_size = -1;
#ifdef EXPERIMENTAL
	if (uiomux_partial_lock(meram->uiomux, UIOMUX_SH_MERAM,
		icb->lock_offset, icb->len) < 0) {
		free (icb);
		return NULL;
	}
#endif
	icb->locked = 1;
	return icb;
}

ICB *meram_lock_icb(MERAM *meram, int index)
{
	return __meram_lock_icb(meram, index, 1);
}

ICB *meram_trylock_icb(MERAM *meram, int index)
{
	return __meram_lock_icb(meram, index, 0);
}

void meram_unlock_icb(MERAM *meram, ICB *icb)
{
	int index = icb->index & 31;
	int slot = icb->index >> 5;

	/*partial uiomux unlock*/
#ifdef EXPERIMENTAL
	uiomux_partial_unlock(meram->uiomux, UIOMUX_SH_MERAM,
		icb->lock_offset, icb->len);
#endif
	icb->locked = 0;
	meram_free_icb_memory(meram, icb);
	free(icb);

	pthread_mutex_lock(&icb_mutex);
	icb_inuse[slot] &= ~(1UL << index);
	pthread_cond_broadcast(&icb_wq);
	pthread_mutex_unlock(&icb_mutex);
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

int meram_lock_memory_block(MERAM *meram, int offset, int size)
{
	int alloc_size = size << 10;
	void *alloc_ptr = (u8 *) meram->mem_vaddr + offset;
	struct reserved_address *current;

	/* check if specified blocks would overlap reserved regions */
	current = meram->reserved_mem;
	while (current) {
		unsigned long alloc_beg = (unsigned long)
			((u8 *) alloc_ptr - (u8 *) meram->mem_vaddr);
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
		return -1;
	}

	return uiomux_mlock(meram->uiomux, UIOMUX_SH_MERAM, alloc_ptr, alloc_size);
}

void meram_unlock_memory_block(MERAM *meram, int offset, int size)
{
	int alloc_size = size << 10;
	void *alloc_ptr = (u8 *) meram->mem_vaddr + offset;
	uiomux_munlock(meram->uiomux, UIOMUX_SH_MERAM, alloc_ptr, alloc_size);
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
			alloc_size, alloc_size);
		if (!alloc_ptr)
			return -1;
		current = meram->reserved_mem;
		while (current) {
			unsigned long alloc_beg = (unsigned long)
				((u8 *) alloc_ptr - (u8 *) meram->mem_vaddr);
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
				uiomux_free(meram->uiomux, UIOMUX_SH_MERAM,
					alloc_ptr,
					cur_beg - alloc_beg);
			}
			if (alloc_end > cur_end) {
				uiomux_free(meram->uiomux, UIOMUX_SH_MERAM,
					(u8 *)meram->mem_vaddr + cur_end +
					(1 << 10), alloc_end - cur_end);
			}
			alloc_ptr = NULL;
			break;
		}
	}

	return ((unsigned long) ((u8 *)alloc_ptr -
			(u8 *)meram->mem_vaddr)) >> 10;
}
void meram_free_memory_block(MERAM *meram, int offset, int size)
{
	int alloc_size = size << 10;
	void *alloc_ptr = (u8 *) meram->mem_vaddr + (offset << 10);
	uiomux_free(meram->uiomux, UIOMUX_SH_MERAM, alloc_ptr, alloc_size);
}

void meram_fill_memory_block(MERAM *meram, int offset,
			     int n_blocks, unsigned int val)
{
	unsigned int bytes = n_blocks << 10;
	unsigned long *ptr32 = (unsigned long *)
		((u8 *) meram->mem_vaddr + (offset << 10));
	unsigned int i;

	for (i=0; i< bytes/sizeof(unsigned long); i++)
		*(ptr32 + i) = val;
}

int meram_alloc_icb_memory(MERAM *meram, ICB *icb, int size)
{
	if (!meram || !icb)
		return -1;
	icb->mem_block = meram_alloc_memory_block(meram, size);
	if (icb->mem_block >= 0)
		icb->mem_size = size;
	return icb->mem_block;
}

void meram_free_icb_memory(MERAM *meram, ICB *icb)
{
	if (!meram || !icb || icb->mem_block < 0 || icb->mem_size < 0)
		return;
	meram_free_memory_block(meram, icb->mem_block, icb->mem_size);
	icb->mem_block = icb->mem_size = -1;
}

int meram_read_icb(MERAM *meram, ICB *icb, int offset,
		unsigned long *read_val)
{
	volatile unsigned long *reg;
	if (!meram || !icb)
		return -1;
	reg = (unsigned long *) ((u8 *)meram->vaddr + icb->offset + offset);
	*read_val = *reg;
	return 0;
}
int meram_write_icb(MERAM *meram, ICB *icb, int offset, unsigned long val)
{
	volatile unsigned long *reg;
	if (!meram || !icb)
		return -1;
	reg = (unsigned long *) ((u8 *)meram->vaddr + icb->offset + offset);
	*reg = val;
	return 0;
}
int meram_read_reg(MERAM *meram, MERAM_REG *meram_reg, int offset,
		unsigned long *read_val)
{
	volatile unsigned long *reg;
	if (!meram || !meram_reg)
		return -1;
	reg = (unsigned long *) ((u8 *)meram->vaddr + meram_reg->offset +
		offset);
	*read_val = *reg;
	return 0;
}
int meram_write_reg(MERAM *meram, MERAM_REG *meram_reg, int offset,
		unsigned long val)
{
	volatile unsigned long *reg;
	if (!meram || !meram_reg)
		return -1;
	reg = (unsigned long *) ((u8 *)meram->vaddr + meram_reg->offset +
		offset);
	*reg = val;
	return 0;
}

unsigned long
meram_get_icb_address(MERAM *meram, ICB *icb, int ab) {
	if (icb)
		return (0xC0000000 | ((ab & 1) << 23) |
				((icb->index & 0x1F) << 24));
	return 0;
}

#define TOKENS " \t"
#define LINE_LEN 255
int
parse_config_file(char *infile,
	struct reserved_address **add_head,
	struct ipmmui_settings **ipmmui_head)
{
	int len = LINE_LEN;
	struct reserved_address *add_current = NULL, *add_prev = NULL;
	struct ipmmui_settings *ipmmui_current = NULL, *ipmmui_prev = NULL;
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
		} else if (!strcmp(id, "ipmmui")) {
			num_fields = 3;
		} else
			continue;

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
		if (!strcmp(id, "reserved")) {
			if (!add_current)
				add_current = *add_head = calloc (1,
					sizeof (struct reserved_address));
			else {
				add_current = calloc (1,
					sizeof (struct reserved_address));
				add_prev->next = add_current;
			}
			add_current->start_block = atoi(fields[0]);
			add_current->end_block = atoi(fields[1]);
			add_current->next = NULL;
			add_prev = add_current;
		} else if (!strcmp(id, "ipmmui")) {
			if (!ipmmui_current)
				ipmmui_current = *ipmmui_head = calloc (1,
					sizeof (struct ipmmui_settings));
			else {
				ipmmui_current = calloc (1,
					sizeof (struct ipmmui_settings));
				ipmmui_prev->next = ipmmui_current;
			}
			ipmmui_current->tag = strdup(fields[0]);
			ipmmui_current->vaddr = strtoul(fields[1], NULL, 0);
			ipmmui_current->size = atoi(fields[2]);
			ipmmui_current->next = NULL;
			ipmmui_prev = ipmmui_current;
		}
		line_cnt ++;
		free(fields);
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

void
delete_ipmmui_settings(struct ipmmui_settings *head)
{
	struct ipmmui_settings *next = head;
	while (head) {
		next = head->next;
		free(head->tag);
		free(head);
		head = next;
	}
}

int
meram_get_required_memory_size(int stride, int line_num)
{
	int bytes_per_line;

	if (stride <= 1024)
		bytes_per_line = 1024;
	else
		ALIGN2UP(bytes_per_line, stride);

	return bytes_per_line * line_num / 1024;
}
