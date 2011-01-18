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

#define MERAM_REG_BASE 0xE8000000
#define MERAM_REG_SIZE 0x200000
#define MERAM_ICB0BASE 0x400
#define MERAM_ICB28BASE 0x780
#define MExxCTL 0x0
#define MExxSIZE 0x4
#define MExxMNCF 0x8
#define MExxSARA 0x10
#define MExxSARB 0x14
#define MExxBSIZE 0x18
#define MSAR_OFF 0x3C0

#define MEVCR1 0x4
#define MEACTS 0x10
#define MEQSEL1 0x40
#define MEQSEL2 0x44

#define BLOCK_SIZE 64

#define MERAM_START(ind, ab) (0xC0000000 | ((ab & 0x1) << 23) | \
	((ind & 0x1F) << 24))

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

/**
  * Allocate MERAM internal memory
  * \param meram MERAM handle
  * \param size size of block to allocate in 1K units (e.g. 4 = 4K)
  * \retval -1 Failure, otherwise offset of allocated block (range = 0 to 1535)
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
  * Read data from MERAM ICB register
  * \param meram MERAM handle
  * \param icb handle to ICB
  * \param offset address offset within ICB register block
  * \retval read data
  */
void meram_read_icb(MERAM *meram, ICB *icb, int offset,
		unsigned long *read_val);

/**
  * Write data to MERAM ICB register
  * \param meram MERAM handle
  * \param icb handle to ICB
  * \param offset address offset within ICB register block
  * \param val data to write 
  */
void meram_write_icb(MERAM *meram, ICB *icb, int offset, unsigned long val);

/**
  * Read data from MERAM common register
  * \param meram MERAM handle
  * \param meram_reg handle to common registers
  * \param offset address offset within ICB register block
  * \retval read data
  */
void meram_read_reg(MERAM *meram, MERAM_REG *meram_reg, int offset,
		unsigned long *read_val);

/**
  * Write data to MERAM common register
  * \param meram MERAM handle
  * \param meram_reg handle to common registers
  * \param offset address offset within ICB register block
  * \param val data to write 
  */
void meram_write_reg(MERAM *meram, MERAM_REG *meram_reg, int offset,
		unsigned long val);

#ifdef __cplusplus
}
#endif

#endif /* __MERAM_H__ */
