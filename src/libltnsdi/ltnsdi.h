/*
 * Copyright (c) LiveTimeNet, Inc. 2017. All Rights Reserved.
 *
 * Address: LTN Global Communications, Inc.
 *          Historic Savage Mill
 *          Box 2020 
 *          8600 Foundry Street
 *          Savage, MD 20763
 *
 * Contact: sales@ltnglobal.com
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


/**
 * @file	ltnsdi.h
 * @author	Steven Toth <stoth@ltnglobal.com>
 * @copyright	Copyright (c) LiveTimeNet, Inc. 2017. All Rights Reserved.
 * @brief	Process, analyze, convert SDI material.
 */

#ifndef _LTNSDI_H
#define _LTNSDI_H

#include <stdint.h>
#include <sys/errno.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ltnsdi_context_s
{
	/* Internal use by the library */
	int   verbose;
	void *callback_context;
	void *priv;
};

/**
 * @brief	Create or destroy some basic application/library context.\n
 *              The context is considered private and is likely to change.
 *              release the context with ltnsdi_context_free().
 * @param[out]	struct ltnsdi_context_s **ctx - Context.
 * @return      0 - Success
 * @return      < 0 - Error
 */
int ltnsdi_context_alloc(struct ltnsdi_context_s **ctx);

/**
 * @brief	Deallocate and destroy a context. See ltnsdi_context_alloc()
 * @param[in]	struct ltnsdi_context_s *ctx - Context.
 */
void ltnsdi_context_free(struct ltnsdi_context_s *ctx);

#ifdef __cplusplus
};
#endif

#endif /* _LTNSDI_H */
