/*
 * libmeram: A library for accesssing SH-Mobile MERAM
 * Copyright (C) 2010 Igel Co., Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA  02110-1301 USA
 */
#ifndef __MERAM_H__
#define __MERAM_H__

#ifdef __cplusplus
extern "C" {
#endif

#define MExxCTRL	0x0
#define MExxBSIZE	0x4
#define MExxMCNF	0x8
#define MExxSSARA	0x10
#define MExxSSARB	0x14
#define MExxSBSIZE	0x18
#define MSAR_OFF	0x3C0

#define MEVCR1		0x4
#define MEACTS		0x10
#define MEQSEL1		0x40
#define MEQSEL2		0x44

#define MAX_ICB_INDEX	127	/* common: 0 to 31, extend: 32 to 127 */

/** \file
  * The libmeram C API
  *
  */

/**
  * Opaque handles to meram control structures
  */

struct MERAM;
typedef struct MERAM MERAM;

struct ICB;
typedef struct ICB ICB;

struct MERAM_REG;
typedef struct MERAM_REG MERAM_REG;

/**
  * Open a handle to MERAM
  * \retval 0 Failure, otherwise MERAM handle
  */
MERAM *meram_open(void);

/**
  * Close a MERAM handle
  * \param meram MERAM handle
  */
void meram_close(MERAM *meram);

/**
  * Lock access to a MERAM ICB registers
  * The application should hold this lock for as long as it will use
  * the specified ICB.
  * \param meram MERAM handle
  * \param index index of the ICB to lock
  * \retval 0 Failure, otherwise handle to the locked ICB
  */
ICB *meram_lock_icb(MERAM *meram, int index);

/**
  * Lock access to a MERAM ICB registers only if it is free at the time of invocation.
  * The application should hold this lock for as long as it will use
  * the specified ICB.
  * \param meram MERAM handle
  * \param index index of the ICB to lock
  * \retval 0 Failure, otherwise handle to the locked ICB
  */
ICB *meram_trylock_icb(MERAM *meram, int index);

/**
  * Unlock a MERAM ICB registers when no longer needed
  * The application should unlock the ICB only after is has finished
  * using it. (i.e. after configuration AND data transfer)
  * \param meram MERAM handle
  * \param icb ICB handle
  */
void meram_unlock_icb(MERAM *meram, ICB *icb);

/**
  * Lock access to MERAM common registers
  * The application should only hold this lock when accessing the registers
  * \param meram MERAM handle
  * \retval 0 Failure, otherwise handle to the locked registers
  */
MERAM_REG *meram_lock_reg(MERAM *meram);

/**
  * Unlock MERAM common registers
  * The application should unlock the registers immediately after
  * accessing them, so that other applications may access them
  * \param meram MERAM handle
  * \param meram_reg MERAM register handle
  */
void meram_unlock_reg(MERAM *meram, MERAM_REG *meram_reg);

/** Allocate MERAM memory blocks and associate with an ICB
  * The library will keep track of which memory has been allocated
  * and can be free with meram_free_icb_meram
  * \param meram MERAM handle
  * \param icb ICB handle
  * \param size size of block to allocate in 1K units (e.g. 4 = 4K)
  * \retval -1 Failure, otherwise offset of allocated block
	(range = 0 to 1535)
  */
int meram_alloc_icb_memory(MERAM *meram, ICB *icb, int size);

/**
  * Free MERAM internal memory
  * \param meram MERAM handle
  * \param icb ICB handle for which memory has been allocated with
  *            meram_alloc_icb_memory
  */
void meram_free_icb_memory(MERAM *meram, ICB *icb);

/**
  * Directly lock MERAM internal memory
  *   Using this method the applicatin is responsible for keeping
  *   track of which memory has been locked for proper unlocking
  *   with meram_unlock_memory_block
  * \param meram MERAM handle
  * \param offset offset of block to lock
  * \param size size of block to lock in 1K units (e.g. 4 = 4K)
  * \retval -1 Failure
  * 	     0 Success
  */
int meram_lock_memory_block(MERAM *meram, int offset, int size);

/**
  * Unlock MERAM internal memory
  * \param meram MERAM handle
  * \param offset offset of the locked block
  * \param size size of block to unlock in 1K units (e.g. 4 = 4K)
  */
void meram_unlock_memory_block(MERAM *meram, int offset, int size);

/**
  * Directly allocte MERAM internal memory
  *   Using this method the applicatin is responsible for keeping
  *   track of which memory has been allocated for propler freeing
  *   with meram_free_memory_block
  * \param meram MERAM handle
  * \param size size of block to allocate in 1K units (e.g. 4 = 4K)
  * \retval -1 Failure, otherwise offset of allocated block
	(range = 0 to 1535)
  */
int meram_alloc_memory_block(MERAM *meram, int size);

/**
  * Free MERAM internal memory
  * \param meram MERAM handle
  * \param offset offset of the allocated block
  * \param size size of block to allocate in 1K units (e.g. 4 = 4K)
  */
void meram_free_memory_block(MERAM *meram, int offset, int size);

/**
  * Fill MERAM internal memory with a value
  * \param meram MERAM handle
  * \param offset offset of the allocated block
  * \param n_blocks number of blocks to allocate in 1K units (e.g. 4 = 4K)
  * \param value value with which blocks will be filled
  */
void meram_fill_memory_block(MERAM *meram, int offset, int n_blocks, unsigned int value);

/**
  * Read data from MERAM ICB register
  * \param meram MERAM handle
  * \param icb handle to ICB
  * \param offset address offset within ICB register block
  * \param read_val pointer to store read result data
  * \retval -1 Failure
  * 	     0 Success
  */
int meram_read_icb(MERAM *meram, ICB *icb, int offset,
		unsigned long *read_val);

/**
  * Write data to MERAM ICB register
  * \param meram MERAM handle
  * \param icb handle to ICB
  * \param offset address offset within ICB register block
  * \param val data to write
  * \retval -1 Failure
  * 	     0 Success
  */
int meram_write_icb(MERAM *meram, ICB *icb, int offset, unsigned long val);

/**
  * Read data from MERAM common register
  * \param meram MERAM handle
  * \param meram_reg handle to common registers
  * \param offset address offset within ICB register block
  * \param read_val pointer to store read result data
  * \retval -1 Failure
  * 	     0 Success
  */
int meram_read_reg(MERAM *meram, MERAM_REG *meram_reg, int offset,
		unsigned long *read_val);

/**
  * Write data to MERAM common register
  * \param meram MERAM handle
  * \param meram_reg handle to common registers
  * \param offset address offset within ICB register block
  * \param val data to write
  * \retval -1 Failure
  * 	     0 Success
  */
int meram_write_reg(MERAM *meram, MERAM_REG *meram_reg, int offset,
		unsigned long val);

/**
  * Get the address used to access a specific ICB
  * \param meram MERAM handle
  * \param icb handle to ICB
  * \param ab specify which bank to access 0 = a, 1 = b
  * \retval 0 Failure, othewise address to access
  */
unsigned long meram_get_icb_address(MERAM *meram, ICB *icb, int ab);

/**
 * Get the required MERAM memory size calculated from a stride and number of
 * cache lines
 * \param stride stride to be cached on MERAM
 * \param line_num number of lines to be cached on MERAM
 * \retval size of block calculated from the parameters in 1K units
 */
int meram_get_required_memory_size(int stride, int line_num);
#ifdef __cplusplus
}
#endif

#endif /* __MERAM_H__ */
