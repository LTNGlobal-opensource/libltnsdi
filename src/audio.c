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
#include <math.h>
#include <sys/errno.h>
#include <libltnsdi/ltnsdi.h>
#include <libltnsdi/smpte338.h>

#include "ltnsdi-private.h"

__inline__ static enum sdiaudio_channel_type_e sdiaudio_channel_getType(struct sdiaudio_channel_s *ch)
{
	/* Lets have a centralized place so we can adjust date/times for queries etc. */
	return ch->type;
}

static void sdiaudio_channel_setType(struct sdiaudio_channel_s *ch, enum sdiaudio_channel_type_e type)
{
	if (ch->type == type && type != AUDIO_TYPE_UNDEFINED)
		return;

	if (type != AUDIO_TYPE_UNUSED) {
		ch->unused.unusedSampleCount = 0;
	}

	if (type != AUDIO_TYPE_PCM) {
		ch->pcm.samplesWritten = 0;
		ch->pcm.sampleRateHz = 0;
		ch->pcm.dbFS = 0.0;
	}

	if (type != AUDIO_TYPE_SMPTE337) {
		ch->smpte337.framesWritten = 0;
	} else {
		/* Undefined State */
	}

	ch->type = type;
	gettimeofday(&ch->type_last_update, NULL);
}

__inline__ void sdiaudio_channel_statsUpdate(struct sdiaudio_channel_s *ch)
{
	switch (ch->type) {
	case AUDIO_TYPE_UNUSED:
		ch->unused.unusedSampleCount++;
		gettimeofday(&ch->unused.last_update, NULL);
		break;
	case AUDIO_TYPE_SMPTE337:
		ch->smpte337.framesWritten++;
		gettimeofday(&ch->smpte337.last_update, NULL);
		break;
	case AUDIO_TYPE_PCM:
		ch->pcm.samplesWritten++;
		gettimeofday(&ch->pcm.last_update, NULL);
		break;
	case AUDIO_TYPE_UNDEFINED:
		break;
	}
}

__inline__ uint32_t be_u16(uint16_t n)
{
#if G_BYTE_ORDER == G_LITTLE_ENDIAN 
    return ((n & 0xff00) >> 8) | (n << 8);
#elif G_BYTE_ORDER == G_BIG_ENDIAN
    return n;
#else
#error "G_BYTE_ORDER should be big or little endian."
#endif
}

__inline__ uint32_t be_u32(uint32_t n)
{
#if G_BYTE_ORDER == G_LITTLE_ENDIAN 
    return ((n & 0xff000000) >> 24) | ((n & 0x00ff0000) >> 8) | ((n & 0x0000ff00) << 8) | (n << 24);
#elif G_BYTE_ORDER == G_BIG_ENDIAN
    return n;
#else
#error "G_BYTE_ORDER should be big or little endian."
#endif
}

static const char *sdiaudio_channel_type_name(enum sdiaudio_channel_type_e e)
{
	switch (e) {
	default:
	case AUDIO_TYPE_UNDEFINED: return "UNDEFINED";
	case AUDIO_TYPE_SMPTE337:  return "SMPTE337";
	case AUDIO_TYPE_PCM:       return "PCM";
	case AUDIO_TYPE_UNUSED:    return "UNUSED";
	}
};

static __inline__ void incrementChannelBitsPs(struct ltnsdi_context_s *ctx, struct sdiaudio_channel_s *ch, uint64_t bitCount)
{
	time_t now;
	time(&now);

	if (now != ch->bitsNowTime) {
		ch->bitsPsCurrent = ch->bitsNow;
		ch->bitsNow = 0;
		ch->bitsNowTime = now;
	}

	ch->bitsNow += bitCount;
}

static uint8_t *demuxChannelWords(struct ltnsdi_context_s *ctx, uint8_t *buf,
        uint32_t audioFrames, uint32_t sampleDepth, uint32_t channelsPerFrame, uint32_t frameStrideBytes, int channelIndex,
	uint32_t *largestSample)
{
	*largestSample = 0;
#if 0
	printf("%s depth = %d -- ", __func__, sampleDepth);
	for (int i = 0; i < 12; i++)
		printf("%02x ", *(buf + i));
	printf("\n");
#endif

	if (sampleDepth == 32) {
		int step = (frameStrideBytes / sizeof(uint32_t));
		uint32_t *b   = malloc(audioFrames * sizeof(uint32_t));
		uint32_t *dst = b;
		uint32_t *src = (uint32_t *)buf;
		src += channelIndex;
//printf("%s() 32 bit i = %d, step = %d\n", __func__, channelIndex, step);

		for (int i = 0; i < audioFrames; i++) {
			*dst = be_u32(*src);
			if (*dst > *largestSample)
				*largestSample = *dst;
			src += step;
			dst++;
		}
		return (uint8_t *)b;
	}

	if (sampleDepth == 16) {
// TODO: requires testing. */
printf("%s() 16 bit\n", __func__);
		uint32_t step = (frameStrideBytes / sizeof(uint16_t));
		uint32_t *b = malloc(audioFrames * sizeof(uint32_t));
		uint32_t *dst = b;
		uint16_t *src = (uint16_t *)buf;
		src += (channelIndex * (sampleDepth / 8));

		for (int i = 0; i < audioFrames; i++) {
			*dst = be_u16(*src);
			if (*dst > *largestSample)
				*largestSample = *dst;
			src += step;
			dst++;
		}
		return (uint8_t *)b;
	}

	return NULL;
}

static void *detector_callback(void *userContext,
	struct smpte337_detector_s *ctx,
	uint8_t datamode, uint8_t datatype, uint32_t payload_bitCount, uint8_t *payload)
{
	struct sdiaudio_channel_s *ch = (struct sdiaudio_channel_s *)userContext;

#if 0
	printf("libltnsdi:%s() groupnr: %d channelnr: %d datamode = %d [%dbit], datatype = %d [payload: %s]"
		", payload_bitcount = %d, payload = %p\n",
		__func__,
		ch->groupNr,
		ch->channelNr,
		datamode, smpte338_lookupDataMode(datamode),
		datatype, smpte338_lookupDataTypeDescription(datatype),
		payload_bitCount,
		payload);
#endif

	if (sdiaudio_channel_getType(ch) == AUDIO_TYPE_PCM) {
		/* PCM removal alert */
	}

	ch->wordLength = smpte338_lookupDataMode(datamode);
	ch->smpte337.dataType = datatype;
	ch->smpte337.dataMode = datamode;

	sdiaudio_channel_setType(ch, AUDIO_TYPE_SMPTE337);
	sdiaudio_channel_statsUpdate(ch);
	incrementChannelBitsPs(NULL, ch, payload_bitCount);

	if (sdiaudio_channel_getType(ch->pairedChannel) != AUDIO_TYPE_SMPTE337) {
		sdiaudio_channel_setType(ch->pairedChannel, AUDIO_TYPE_SMPTE337);
		ch->pairedChannel->smpte337.dataType = datatype;
		ch->pairedChannel->smpte337.dataMode = datamode;
	}
	sdiaudio_channel_statsUpdate(ch->pairedChannel);
	incrementChannelBitsPs(NULL, ch->pairedChannel, payload_bitCount);

#if 0
	if (ctx->spanCount == 2) {
		sdiaudio_channel_setType(ch->pairedChannel, AUDIO_TYPE_UNUSED);
	}
#endif

	/* SMPTE 337 Discovery alert. */

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

	if (channelsPerFrame >  MAXSDI_AUDIO_CHANNELS)
		return -1;

	pthread_mutex_lock(&channels->mutex);

	for (int i = 0; i < channelsPerFrame; i++) {
		struct sdiaudio_channel_s *ch = &channels->ch[i];

		struct timeval tv;
		gettimeofday(&tv, NULL);
		if (sdiaudio_channel_getType(ch) != AUDIO_TYPE_UNUSED) {

			if (sdiaudio_channel_getType(ch) == AUDIO_TYPE_SMPTE337) {
				if (tv.tv_sec > ch->smpte337.last_update.tv_sec + 2) {
					sdiaudio_channel_setType(ch, AUDIO_TYPE_UNUSED);
					ch->wordLength = 0;
					continue;
				}
			} else
			if (sdiaudio_channel_getType(ch) == AUDIO_TYPE_PCM) {
				if (tv.tv_sec > ch->pcm.last_update.tv_sec + 2) {
					sdiaudio_channel_setType(ch, AUDIO_TYPE_UNUSED);
					ch->wordLength = 0;
					continue;
				}
			} else
				sdiaudio_channel_setType(ch, AUDIO_TYPE_UNUSED);
		}

		int sampleOffset = (i * (sampleDepth / 8));
		if (ch->smpte337.detector) {
			smpte337_detector_write(ch->smpte337.detector, buf + sampleOffset, audioFrames, sampleDepth,
				channelsPerFrame, frameStrideBytes);
		}

		if (sdiaudio_channel_getType(ch) == AUDIO_TYPE_SMPTE337)
			continue;

		/* Now process the payload as if its PCM. */

		/* De-interleave the sample. */
		uint32_t largestSample = 0;
		uint8_t *dat = demuxChannelWords(ctx, buf, audioFrames, sampleDepth, channelsPerFrame, frameStrideBytes, i, &largestSample);
		if (!dat)
			continue;

		/* Convert from stream specific format into LE */
		if (largestSample == 0) {
			if (sdiaudio_channel_getType(ch) == AUDIO_TYPE_PCM) {
				ch->pcm.emptyBufferCount++;

				if (ch->pcm.emptyBufferCount > 32) {
					/* No actual data, flag this sample as unused. */
					sdiaudio_channel_setType(ch, AUDIO_TYPE_UNUSED);
					sdiaudio_channel_statsUpdate(ch);
					ch->wordLength = 0;
				}
			}
		} else {
//printf("ch g%dc%dPCM largest sample = 0x%x\n", ch->groupNr, ch->channelNr, largestSample);
			sdiaudio_channel_setType(ch, AUDIO_TYPE_PCM);
			sdiaudio_channel_statsUpdate(ch);
			ch->pcm.emptyBufferCount = 0;
		}
		incrementChannelBitsPs(ctx, ch, ch->wordLength * audioFrames);

		if (sdiaudio_channel_getType(ch) == AUDIO_TYPE_PCM) {
			/* We don't know if these PCM samples ar 16/20/24/32 but, calculate the  size in bitwidth.  */
			/* Lets scan the channel, see if our samples are 16, 20 or 24 bit constrained. */
			int bits = 0;
			for (int z = 31; z > 0; z--) {
				if (largestSample & (1 << z)) {
					bits = z + 1;
					break;
				}
			}

			/* Measuring the largest sample only hints at the bitwidth.
			 * For example, the smallest sample in actuality can only be 16bit.
			 * Round accordingly. */
			if (bits <= 16)
				bits = 16;
			else
			if (bits <= 32)
				bits = 32;

			ch->wordLength = bits;


			/* Generate a dbFS measurement. Where maximum power is 0dbFS, and minimum is -90dbFS. */
			double x = largestSample;
			if (bits <= 16) {
				/* 16 bit */
				ch->pcm.dbFS = 20 * log10( x / 32767.0);
			} else
			if (bits <= 20) {
				/* TODO: Test this. */
				/* 20bit */
				ch->pcm.dbFS = 20 * log10( x / 524287.0);
			} else
			if (bits <= 24) {
				/* TODO: Test this. */
				/* 24bit */
				ch->pcm.dbFS = 20 * log10( x / 8388607.0);
			} else
			if (bits <= 32) {
				/* TODO: Test this. */
				/* 32bit */
				ch->pcm.dbFS = 20 * log10( x / 2147483647.0);
			}
		} /* If ch == PCM */

//		printf("g%dc%d: %.03fdbFS largest: %08x\n", ch->groupNr, ch->channelNr, ch->pcm.dbFS, largestSample);

		free(dat);
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

			ch->groupNr = g + 1;
			ch->channelNr = c;

			if (c == 0 || c == 2)
				ch->pairedChannel = &o->ch[ (g * SDI_AUDIO_GROUPS) + c + 1 ];
			else
			if (c == 1 || c == 3)
				ch->pairedChannel = &o->ch[ (g * SDI_AUDIO_GROUPS) + c - 1 ];

			ch->userContext = NULL;
			sdiaudio_channel_setType(ch, AUDIO_TYPE_UNDEFINED);
			ch->wordLength = 0;

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

#if 0
struct ltnsdi_status_s
{
        struct {
                /* 1 = PCM
 *                  * 2 = SMPTE 337
 *                                   * 3 = UNUSED
 *                                                    */
                uint32_t   type;
                const char typeDescription[16];
                struct timeval typeUpdated;
                const char     typeUpdatedDescription[32];

                uint64_t       buffersProcessed;
                struct timeval lastBufferArrival;
                const char     lastBufferArrivalDescription[32];
                const char     lastBufferPayloadHeader[32];

                /* PCM */
                double     pcm_dbFS;
                const char pcm_dbFSDescription[8];

                /* SMPTE 337 */
                uint32_t   smpte337_dataMode;
                uint32_t   smpte337_dataType;
                const char smpte337_dataTypeDescription[64];

        } channels[16];
};
#endif

static void createDateString(time_t n, char *dst)
{
	struct tm x;
	localtime_r(&n, &x);
	sprintf(dst, "%02d/%02d/%04d %02d:%02d:%02d",
		x.tm_mon + 1,
		x.tm_mday,
		x.tm_year + 1900,
		x.tm_hour,
		x.tm_min,
		x.tm_sec);
}

int ltnsdi_status_alloc(struct ltnsdi_context_s *ctx, struct ltnsdi_status_s **status)
{
	struct ltnsdi_status_s *s = calloc(1, sizeof(*s));
	if (!s)
		return -1;

	struct sdiaudio_channels_s *channels = getChannels(ctx);

	pthread_mutex_lock(&channels->mutex);
	for (int i = 0; i < MAXSDI_AUDIO_CHANNELS; i++) {
		struct sdiaudio_channel_s *ch = &channels->ch[i];

		s->channels[i].groupNumber = ch->groupNr;
		s->channels[i].channelNumber = ch->channelNr;
		s->channels[i].wordLength = ch->wordLength;

		switch (sdiaudio_channel_getType(ch)) {
		case AUDIO_TYPE_PCM:
			s->channels[i].type = 1;
			s->channels[i].buffersProcessed = ch->pcm.samplesWritten;
			s->channels[i].lastBufferArrival = ch->pcm.last_update;
			s->channels[i].pcm_dbFS = ch->pcm.dbFS;
			sprintf((char *)s->channels[i].pcm_dbFSDescription, "%04.02f", ch->pcm.dbFS);
			sprintf((char *)s->channels[i].smpte337_dataTypeDescription, "N/A");

			if (s->channels[i].channelNumber == 1 || s->channels[i].channelNumber == 3)
				sprintf((char *)s->channels[i].pcm_channelDescription, "Right");
			else
				sprintf((char *)s->channels[i].pcm_channelDescription, "Left");

			s->channels[i].pcm_Hz = ch->bitsPsCurrent / ch->wordLength;
			break;
		case AUDIO_TYPE_SMPTE337:
			s->channels[i].buffersProcessed = ch->smpte337.framesWritten;
			s->channels[i].lastBufferArrival = ch->smpte337.last_update;
			s->channels[i].type = 2;
			s->channels[i].smpte337_dataMode = ch->smpte337.dataType;
			s->channels[i].smpte337_dataType = ch->smpte337.dataMode;
			strncpy((char *)s->channels[i].smpte337_dataTypeDescription,
				smpte338_lookupDataTypeDescription(ch->smpte337.dataType),
				sizeof(s->channels[i].smpte337_dataTypeDescription));
			break;
		default:
		case AUDIO_TYPE_UNUSED:
			s->channels[i].buffersProcessed = ch->unused.unusedSampleCount;
			s->channels[i].lastBufferArrival = ch->unused.last_update;
			s->channels[i].type = 3;
			break;
		}
		createDateString(s->channels[i].lastBufferArrival.tv_sec, (char *)s->channels[i].lastBufferArrivalDescription);

		strncpy((char *)s->channels[i].typeDescription, sdiaudio_channel_type_name(sdiaudio_channel_getType(ch)), sizeof(s->channels[i].typeDescription));
		s->channels[i].typeUpdated = ch->type_last_update; /* Implicit struct copy. */

		createDateString(s->channels[i].typeUpdated.tv_sec, (char *)s->channels[i].typeUpdatedDescription);

		s->channels[i].bitratePs = ch->bitsPsCurrent;
		sprintf((char *)s->channels[i].bitratePsDescriptionKb, "%7.1f", (double)s->channels[i].bitratePs / 1000.0);

		/* Payload header */
		{
			/* TODO, get this from the channel. */
			uint8_t dat[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
			for (int x = 0; x < 8; x++) {
				sprintf((char *)s->channels[i].lastBufferPayloadHeader + strlen(s->channels[i].lastBufferPayloadHeader), "%02x ",
					dat[x]);
			}
		}
	}
	pthread_mutex_unlock(&channels->mutex);

	*status = s;
	return 0; /* Success */
}

void ltnsdi_status_free(struct ltnsdi_context_s *ctx, struct ltnsdi_status_s *status)
{
	free(status);
}
