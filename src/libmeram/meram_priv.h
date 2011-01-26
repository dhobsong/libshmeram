#ifndef MERAM_PRIV_H
#define MERAM_PRIV_H
#define UIOMUX_SH_MERAM 1
#define UIOMUX_SH_IPMMUI 2

struct MERAM {
	UIOMux *uiomux;
	unsigned long paddr;
	void *vaddr;
	unsigned long len;
	unsigned long mem_paddr;
	void *mem_vaddr;
	unsigned long mem_len;
	struct reserved_address *reserved_mem;
	struct ipmmui_settings *ipmmui_config;
};

struct ICB {
	int locked;
	unsigned long offset;
	unsigned long lock_offset;
	unsigned long len;
	int index;
};

struct MERAM_REG {
	int locked;
	unsigned long offset;
	unsigned long len;
};

struct IPMMUI {
	MERAM *meram;
	UIOMux *uiomux;
	unsigned long paddr;
	void *vaddr;
	unsigned long len;
};

struct PMB {
	unsigned long offset;
	unsigned long lock_offset;
	int index;
};
struct IPMMUI_REG {
	int locked;
	unsigned long offset;
	unsigned long len;
};

struct reserved_address {
	int start_block;
	int end_block;
	struct reserved_address *next;
};

struct ipmmui_settings {
	char *tag;
	unsigned long vaddr;
	int size;
	struct ipmmui_settings *next;
};

int
parse_config_file(char *infile,
	struct reserved_address **add_head,
	struct ipmmui_settings **ipmmui_head);
void
delete_reserved_addr_list(struct reserved_address *head);

void
delete_ipmmui_settings(struct ipmmui_settings *head);
#endif
