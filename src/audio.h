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
 * @file	audio.h
 * @author	Steven Toth <stoth@ltnglobal.com>
 * @copyright	Copyright (c) LiveTimeNet, Inc. 2017. All Rights Reserved.
 * @brief	Process, analyze, convert SDI material.
 */

#ifndef _AUDIO_H
#define _AUDIO_H

#include <stdint.h>
#include <pthread.h>
#include <libltnsdi/ltnsdi.h>

#include "ltnsdi-private.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SDI_AUDIO_GROUPS    4
#define SDI_AUDIO_CHANNELS  4
#define MAXSDI_AUDIO_CHANNELS (SDI_AUDIO_GROUPS * SDI_AUDIO_CHANNELS)

struct smpte337_detector_s;

enum sdiaudio_channel_type_e
{
	AUDIO_TYPE_UNDEFINED = 0,
	AUDIO_TYPE_PCM,
	AUDIO_TYPE_SMPTE337,
};

struct sdiaudio_channel_s
{
	uint32_t groupNr;	/* 1-4 */
	uint32_t channelNr;	/* 0-3 */

	void *userContext;

	enum sdiaudio_channel_type_e type;
	struct {
		struct smpte337_detector_s *detector;
		uint64_t framesWritten;
	} smpte337;
	struct {
		uint64_t samplesWritten;
		uint32_t sampleRateHz; /* Eg. 48000, 44100 */
		/* dbFS for the first sample in the last buffer. 0dbFS maximum volume, -90dbFS basically silence. */
		double dbFS;
	} pcm;

	uint32_t wordLength;	/* 0 (Unset), 16, 20 or 24. */
	struct sdiaudio_channel_s *pairedChannel;
};

struct sdiaudio_channels_s
{
	pthread_mutex_t mutex;
	struct sdiaudio_channel_s ch[MAXSDI_AUDIO_CHANNELS];
};

int sdiaudio_channels_alloc(struct sdiaudio_channels_s **ctx);
void sdiaudio_channels_free(struct sdiaudio_channels_s *ctx);

#ifdef __cplusplus
};
#endif

#endif /* _AUDIO_H */
