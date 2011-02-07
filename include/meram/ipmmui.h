/*
 * libipmmui: A library for accesssing SH-Mobile IPMMUI
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
#ifndef __IPMMUI_H__
#define __IPMMUI_H__

#ifdef __cplusplus
extern "C" {
#endif

#define IMCTR1 0x0
#define IMCTR2 0x4
#define IMPMBA 0x0
#define IMPMBD 0x40

/** \file
  * The IPMMUII access C API
  *
  */

/**
  * Opaque handles to ipmmui control structures
  */

struct IPMMUI;
typedef struct IPMMUI IPMMUI;

struct PMB;
typedef struct PMB PMB;

struct IPMMUI_REG;
typedef struct IPMMUI_REG IPMMUI_REG;

/**
  * Open a handle to IPMMUI
  * \retval 0 Failure, otherwise IPMMUI handle
  */
IPMMUI *ipmmui_open(void);

/**
  * Close a IPMMUI handle
  * \param ipmmui IPMMUI handle
  */
void ipmmui_close(IPMMUI *ipmmui);

/**
  * Lock access to a IPMMUI PMB registers
  * The application should hold this lock for as long as it will use
  * the specified PMB.
  * \param ipmmui IPMMUI handle
  * \param index index of the PMB to lock
  * \retval 0 Failure, otherwise handle to the locked PMB
  */
PMB *ipmmui_lock_pmb(IPMMUI *ipmmui, int index);

/**
  * Unlock a IPMMUI PMB registers when no longer needed
  * The application should unlock the PMB only after is has finished
  * using it. (i.e. after configuration AND data transfer)
  * \param ipmmui IPMMUI handle
  * \param pmb PMB handle
  */
void ipmmui_unlock_pmb(IPMMUI *ipmmui, PMB *pmb);

/**
  * Lock access to IPMMUI common registers
  * The application should only hold this lock when accessing the registers
  * \param ipmmui IPMMUI handle
  * \retval 0 Failure, otherwise handle to the locked registers
  */
IPMMUI_REG *ipmmui_lock_reg(IPMMUI *ipmmui);

/**
  * Unlock IPMMUI common registers
  * The application should unlock the registers immediately after
  * accessing them, so that other applications may access them
  * \param ipmmui IPMMUI handle
  * \param ipmmui_reg IPMMUI register handle
  */
void ipmmui_unlock_reg(IPMMUI *ipmmui, IPMMUI_REG *ipmmui_reg);

/**
  * Read data from IPMMUI PMB register
  * \param ipmmui IPMMUI handle
  * \param pmb handle to PMB
  * \param offset address offset within PMB register block
  * \param read_val pointer to store read result data
  * \retval -1 Failure
  * 	     0 Success
  */
int ipmmui_read_pmb(IPMMUI *ipmmui, PMB *pmb, int offset,
		unsigned long *read_val);

/**
  * Write data to IPMMUI PMB register
  * \param ipmmui IPMMUI handle
  * \param pmb handle to PMB
  * \param offset address offset within PMB register block
  * \param val data to write
  * \retval -1 Failure
  * 	     0 Success
  */
int ipmmui_write_pmb(IPMMUI *ipmmui, PMB *pmb, int offset, unsigned long val);

/**
  * Read data from IPMMUI common register
  * \param ipmmui IPMMUI handle
  * \param ipmmui_reg handle to common registers
  * \param offset address offset within PMB register block
  * \param read_val pointer to store read result data
  * \retval -1 Failure
  * 	     0 Success
  */
int ipmmui_read_reg(IPMMUI *ipmmui, IPMMUI_REG *ipmmui_reg, int offset,
		unsigned long *read_val);

/**
  * Write data to IPMMUI common register
  * \param ipmmui IPMMUI handle
  * \param ipmmui_reg handle to common registers
  * \param offset address offset within PMB register block
  * \param val data to write
  * \retval -1 Failure
  * 	     0 Success
  */
int ipmmui_write_reg(IPMMUI *ipmmui, IPMMUI_REG *ipmmui_reg, int offset,
		unsigned long val);

/**
  * Get the virutal address used to access a specific PMB
  * \param ipmmui IPMMUI handle
  * \param tag tag string indicating the application using the PMB
  * \param vaddr address to store the vaddr to access
  * \param size size of virtal address space (in MiB)
  * \retval -1 invalid IPMMUI handle or tag
  *          0 Success
  */
int
ipmmui_get_vaddr(IPMMUI *ipmmui,
		 char *tag,
		 unsigned long *vaddr,
		 int *size);
#ifdef __cplusplus
}
#endif

#endif /* __IPMMUI_H__ */
