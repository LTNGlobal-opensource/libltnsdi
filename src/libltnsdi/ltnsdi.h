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
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ltnsdi_context_s;

#if 0
typedef void (*ltnsdi_callback_smpte337_discovery)(void *userContext, struct ltnsdi_context_s *ctx, uint32_t groupNr, uint32_t channelNr); 
typedef void (*ltnsdi_callback_smpte337_removal)(void *userContext, struct ltnsdi_context_s *ctx, uint32_t groupNr, uint32_t channelNr);
typedef void (*ltnsdi_callback_pcm_discovery)(void *userContext, struct ltnsdi_context_s *ctx, uint32_t groupNr, uint32_t channelNr);
typedef void (*ltnsdi_callback_pcm_removal)(void *userContext, struct ltnsdi_context_s *ctx, uint32_t groupNr, uint32_t channelNr);
typedef void (*ltnsdi_callback_unused_discovery)(void *userContext, struct ltnsdi_context_s *ctx, uint32_t groupNr, uint32_t channelNr);
typedef void (*ltnsdi_callback_unused_removal)(void *userContext, struct ltnsdi_context_s *ctx, uint32_t groupNr, uint32_t channelNr);

struct ltnsdi_context_callbacks_s
{
	ltnsdi_callback_smpte337_discovery smpte337_discovery;
	ltnsdi_callback_smpte337_removal   smpte337_removal;
	ltnsdi_callback_pcm_discovery      pcm_discovery;
	ltnsdi_callback_pcm_removal        pcm_removal;
	ltnsdi_callback_unused_discovery   unused_discovery;
	ltnsdi_callback_unused_removal     unused_removal;
};
#endif

struct ltnsdi_context_s
{
	/* Internal use by the library */
	int   verbose;
	void *callbackContext;
	void *priv;
#if 0
	/* Callbacks */
	struct ltnsdi_context_callbacks_s cb;
#endif
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

/* Write many channels at once to the internal channels.
 * zero on success else < zero.
 * */
int ltnsdi_audio_channels_write(struct ltnsdi_context_s *ctx, uint8_t *buf,
        uint32_t audioFrames, uint32_t sampleDepth, uint32_t channelsPerFrame, uint32_t frameStrideBytes);

/* Enable PCM loss detection per channel (0-15), with an allowable
 * limit of loss, and an ability to reset the accumulative counts. */
int ltnsdi_audio_channels_analyze_pcm_enable(struct ltnsdi_context_s *ctx, unsigned int channelNr, int truefalse);
int ltnsdi_audio_channels_analyze_pcm_limit(struct ltnsdi_context_s *ctx, unsigned int channelNr, unsigned int limit);
int ltnsdi_audio_channels_analyze_pcm_reset(struct ltnsdi_context_s *ctx);
int ltnsdi_audio_channels_analyze_pcm_console_dump(struct ltnsdi_context_s *ctx, int truefalse);

struct ltnsdi_status_s
{
	struct {
		uint32_t groupNumber;
		uint32_t channelNumber;
		uint32_t wordLength;

		/* 1 = PCM
		 * 2 = SMPTE 337
		 * 3 = UNUSED
		 */
		uint32_t   type;
		const char typeDescription[16];
		struct timeval typeUpdated;
		const char     typeUpdatedDescription[32];

		uint64_t       buffersProcessed;
		struct timeval lastBufferArrival;
		const char     lastBufferArrivalDescription[32];
		const char     lastBufferPayloadHeader[64];
		double         bitratePs;
		const char     bitratePsDescriptionKb[16];

		/* PCM */
		double     pcm_dbFS;
		const char pcm_dbFSDescription[8];
		const char pcm_channelDescription[32];
		uint64_t   pcm_Hz;
		uint64_t   pcm_missingAudioCount;

		/* SMPTE 337 */
		uint32_t   smpte337_dataMode;
		uint32_t   smpte337_dataType;
		const char smpte337_dataTypeDescription[64];

	} channels[16];
};

/**
 * @brief	Query the SDi context for a breakdown of overall status.\n
 *              The caller MUST free the returned status reporte.
 * @param[in]	struct ltnsdi_context_s *ctx - Context.
 * @param[out]	struct ltnsdi_status_s **status - Status report..
 * @return      0 - Success
 * @return      < 0 - Error
 */
int ltnsdi_status_alloc(struct ltnsdi_context_s *ctx, struct ltnsdi_status_s **status);

void ltnsdi_status_free(struct ltnsdi_context_s *ctx, struct ltnsdi_status_s *status);

#ifdef __cplusplus
};
#endif

#endif /* _LTNSDI_H */
