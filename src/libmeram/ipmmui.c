#include <meram/meram.h>
#include <meram/ipmmui.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <uiomux/uiomux.h>
#include "meram_priv.h"

typedef uint8_t u8;

IPMMUI *ipmmui_open(void)
{
	IPMMUI *ipmmui;
	MERAM_REG *reg;
	int ret;
	unsigned long tmp;

	ipmmui = calloc(1, sizeof(*ipmmui));

	ipmmui->meram = meram_open();
	if (ipmmui->meram == NULL)
		return NULL;

        reg = meram_lock_reg(ipmmui->meram);
        meram_read_reg(ipmmui->meram, reg, MEVCR1, &tmp);
        meram_write_reg(ipmmui->meram, reg, MEVCR1, tmp | 0x20000000);
        meram_unlock_reg(ipmmui->meram, reg);

	ipmmui->uiomux = ipmmui->meram->uiomux;

	ret = uiomux_get_mmio(ipmmui->uiomux, UIOMUX_SH_IPMMUI,
		&ipmmui->paddr,
		&ipmmui->len,
		&ipmmui->vaddr);

	if (!ret) {
		meram_close(ipmmui->meram);
		return NULL;
	}
	return ipmmui;
}

void ipmmui_close(IPMMUI *ipmmui)
{
	if (!ipmmui)
		return;
	meram_close(ipmmui->meram);
	free(ipmmui);
}

int
ipmmui_get_vaddr(IPMMUI *ipmmui,
		 const char *tag,
		 unsigned long *vaddr,
		 int *size)
{
	struct ipmmui_settings *current;
	if (!ipmmui)
		return -1;
	current = ipmmui->meram->ipmmui_config;
	while (current) {
		if (!strcmp(current->tag, tag)) {
			*vaddr = current->vaddr;
			*size = current->size;
			return 0;
		}
		current = current->next;
	}
	return -1;
}
PMB *ipmmui_lock_pmb(IPMMUI *ipmmui, int index)
{
	PMB *pmb;
	pmb = calloc (1, sizeof (*pmb));
	pmb->index = index;
	pmb->offset = 0x80 + 4 * index;
	return pmb;
}

void ipmmui_unlock_pmb(IPMMUI *ipmmui, PMB *pmb)
{
	free(pmb);
}

IPMMUI_REG *ipmmui_lock_reg(IPMMUI *ipmmui)
{
	IPMMUI_REG *ipmmui_reg;
	if (!ipmmui)
		return NULL;

	ipmmui_reg = calloc (1, sizeof (*ipmmui_reg));
	/*offset and size determination*/
	ipmmui_reg->offset = 0;
	ipmmui_reg->len = 0x24;

	uiomux_lock(ipmmui->uiomux, UIOMUX_SH_IPMMUI);

	return ipmmui_reg; //* or NULL on locking error*/
}

void ipmmui_unlock_reg(IPMMUI *ipmmui, IPMMUI_REG *ipmmui_reg)
{
	if (!ipmmui)
		return;
	uiomux_unlock(ipmmui->uiomux, UIOMUX_SH_IPMMUI);
	free(ipmmui_reg);
}
int ipmmui_read_pmb(IPMMUI *ipmmui, PMB *pmb, int offset,
		unsigned long *read_val)
{
	volatile unsigned long *reg;
	if (!ipmmui || !pmb)
		return -1;
	reg = (unsigned long *) ((u8 *) ipmmui->vaddr + pmb->offset + offset);
	*read_val = *reg;
	return 0;
}
int ipmmui_write_pmb(IPMMUI *ipmmui, PMB *pmb, int offset, unsigned long val)
{
	volatile unsigned long *reg;
	if (!ipmmui || !pmb)
		return -1;
	reg = (unsigned long *) ((u8 *) ipmmui->vaddr + pmb->offset + offset);
	*reg = val;
	return 0;
}
int ipmmui_read_reg(IPMMUI *ipmmui, IPMMUI_REG *ipmmui_reg, int offset,
		unsigned long *read_val)
{
	volatile unsigned long *reg;
	if (!ipmmui || !ipmmui_reg)
		return -1;

	reg = (unsigned long *) ((u8 *) ipmmui->vaddr + ipmmui_reg->offset +
		offset);
	*read_val = *reg;
	return 0;
}
int ipmmui_write_reg(IPMMUI *ipmmui, IPMMUI_REG *ipmmui_reg, int offset,
		unsigned long val)
{
	volatile unsigned long *reg;
	if (!ipmmui || !ipmmui_reg)
		return -1;

        reg = (unsigned long *) ((u8 *) ipmmui->vaddr +
		ipmmui_reg->offset + offset);

	*reg = val;
	return 0;
}
