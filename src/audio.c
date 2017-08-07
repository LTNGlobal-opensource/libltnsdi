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


#include <stdint.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <libltnsdi/ltnsdi.h>

#include "ltnsdi-private.h"

static void *detector_callback(void *userContext,
	struct smpte337_detector_s *ctx,
	uint8_t datamode, uint8_t datatype, uint32_t payload_bitCount, uint8_t *payload)
{
	struct sdiaudio_channel_s *ch = (struct sdiaudio_channel_s *)userContext;

	printf("libltnsdi:%s() groupnr: %d channelnr: %d datamode = %d [%sbit], datatype = %d [payload: %s]"
		", payload_bitcount = %d, payload = %p\n",
		__func__,
		ch->groupNr,
		ch->channelNr,
		datamode,
		datamode == 0 ? "16" :
		datamode == 1 ? "20" :
		datamode == 2 ? "24" : "Reserved",
		datatype,
		smpte338_lookupDataTypeDescription(datatype),
		payload_bitCount,
		payload);

	ch->type = AUDIO_TYPE_SMPTE337;
	ch->smpte337.framesWritten++;

	if (datamode == 0)
		ch->wordLength = 16;
	else
	if (datamode == 1)
		ch->wordLength = 20;
	else
	if (datamode == 2)
		ch->wordLength = 24;

	return 0;
}

/* Write many channels at once to the internal channels.
 * The buffer is assumed to start with group 1 channel 0.
 * G1C0 | G1C1 | G1C2 | G1C3 | G2C0 | G2C1 | .... up to G4C3
 * zero on success else < zero.
 * */
int ltnsdi_audio_channels_write(struct ltnsdi_context_s *ctx, uint8_t *buf,
        uint32_t audioFrames, uint32_t sampleDepth, uint32_t channelsPerFrame, uint32_t frameStrideBytes)
{
	struct sdiaudio_channels_s *channels = getChannels(ctx);

	if (channelsPerFrame >= MAXSDI_AUDIO_CHANNELS)
		return -1;

	pthread_mutex_lock(&channels->mutex);

	for (int i = 0; i < channelsPerFrame; i++) {
		struct sdiaudio_channel_s *ch = &channels->ch[i];

		int sampleOffset = (i * (sampleDepth / 8));
		if (ch->smpte337.detector) {
			smpte337_detector_write(ch->smpte337.detector, buf + sampleOffset, audioFrames, sampleDepth,
				channelsPerFrame, frameStrideBytes, 1);
		}
	}

	pthread_mutex_unlock(&channels->mutex);
	return 0;
}

int sdiaudio_channels_alloc(struct sdiaudio_channels_s **ctx)
{
	struct sdiaudio_channels_s *o = calloc(1, sizeof(*o));
	if (!o)
		return -1;

	*ctx = o;

	pthread_mutex_init(&o->mutex, NULL);
	memset(&o->ch, 0, sizeof(o->ch));

	for (int g = 0; g < SDI_AUDIO_GROUPS; g++) {
		for (int c = 0; c < SDI_AUDIO_CHANNELS; c++) {
			struct sdiaudio_channel_s *ch = &o->ch[ (g * SDI_AUDIO_GROUPS) + c ];

			ch->groupNr = g;
			ch->channelNr = c;
			ch->userContext = NULL;
			ch->type = AUDIO_TYPE_UNDEFINED;
			ch->wordLength = 0;
			ch->pairedChannel = NULL;

			/* PCM */
			ch->pcm.samplesWritten = 0;
			ch->pcm.sampleRateHz = 0;

			/* SMPTE 337 */
			ch->smpte337.framesWritten = 0;
			ch->smpte337.detector = smpte337_detector_alloc((smpte337_detector_callback)detector_callback, ch);
			if (!ch->smpte337.detector) {
				return -1;
			}
		}
	}

	return 0;
}

void sdiaudio_channels_free(struct sdiaudio_channels_s *ctx)
{
	/* Acquire the mutex, prevent futher callbacks, prevent further use, and destroy the channels. */
	pthread_mutex_lock(&ctx->mutex);

	for (int i = 0; i < MAXSDI_AUDIO_CHANNELS; i++) {
		struct sdiaudio_channel_s *ch = &ctx->ch[i];

		/* Destroy the channel, regardless of type. */
		/* PCM */

		/* SMPTE 337 */
		if (ch->smpte337.detector) {
			smpte337_detector_free(ch->smpte337.detector);
		}
	}

	/* Intensionally leave the mutex locked. */
	free(ctx);
}

