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
 * @file	smpte338.h
 * @author	Steven Toth <stoth@ltnglobal.com>
 * @copyright	Copyright (c) LiveTimeNet, Inc. 2017. All Rights Reserved.
 * @brief	Human printable labels and helper descriptions.
 */

#ifndef _SMPTE338_H
#define _SMPTE338_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

const char *smpte338_lookupDataTypeDescription(uint32_t nr);

#ifdef __cplusplus
};
#endif

#endif /* _SMPTE338_H */
