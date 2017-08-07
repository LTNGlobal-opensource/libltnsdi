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

#include <stdio.h>
#include <stdlib.h>
#include <libltnsdi/ltnsdi.h>


#define DATA_MODE_16BIT 0
#define DATA_MODE_24BIT 2
static void generate_smpte337_header(uint8_t *dst, uint16_t payloadLength, uint8_t dataType, uint8_t dataMode)
{
	if (dataMode == DATA_MODE_16BIT) {
		uint8_t *x = dst;

		/* Pa */
		*(x++) = 0xf8;
		*(x++) = 0x72;

		/* Pb */
		*(x++) = 0x4e;
		*(x++) = 0x1f;

		/* Pc Burst info */
		*(x++) = 0x00;
		*(x++) = (dataType & 0x1f) | ((dataMode & 0x03) << 5);

		/* Pd */
		*(x++) = (payloadLength >> 8);
		*(x++) = payloadLength;

		/* Pe */
		*(x++) = 0xaa;
		*(x++) = 0xaa;
		
		/* Pf */
		*(x++) = 0x55;
		*(x++) = 0x55;
	} else
	if (dataMode == DATA_MODE_24BIT) {
		uint8_t *x = dst;

		/* Pa */
		*(x++) = 0x96;
		*(x++) = 0xf8;
		*(x++) = 0x72;

		/* Pb */
		*(x++) = 0xa5;
		*(x++) = 0x4e;
		*(x++) = 0x1f;

		/* Pc Burst info */
		*(x++) = 0x00;
		*(x++) = 0x00;
		*(x++) = (dataType & 0x1f) | ((dataMode & 0x03) << 5);

		/* Pd */
		*(x++) = 0x00;
		*(x++) = (payloadLength >> 8);
		*(x++) = payloadLength;

		/* Pe */
		*(x++) = 0xaa;
		*(x++) = 0xaa;
		*(x++) = 0xaa;
		
		/* Pf */
		*(x++) = 0x55;
		*(x++) = 0x55;
		*(x++) = 0x55;
	}
}

static int test_16bitPayload_in_32bit_words_G1C0(struct ltnsdi_context_s *ctx)
{
	int ret = 0;

	uint32_t channels = 1;
	uint32_t wordLength = 32;
	uint32_t stride = (wordLength / 8) * channels;
	uint32_t audioFrames = 32;

	uint8_t *buf = malloc(audioFrames * stride * 4);

	uint8_t header[12];
	generate_smpte337_header(&header[0], 0x0010, 0x07, DATA_MODE_16BIT);

	int cnt = 0;
	while (cnt++ < 16) {
		uint32_t *sample = (uint32_t *)buf;
		uint32_t *s32 = (uint32_t *)buf;
		for (int i = 0; i < (stride / 2) * audioFrames; i++) {
			*(sample + i) = -1;
		}

		/* Write the 337 header into a buffer G1C0 */
		for (int i = 0; i < 6; i++) {
			uint32_t v = (header[(i * 2) + 0] << 24) | (header[(i * 2) + 1] << 16);
			*(s32 + i) = v;

			printf("%08x ", *(s32 + i));
		}

		printf("\n");
		sample = (uint32_t *)buf;
		for (int i = 0; i < 8; i++) {
			printf("%08x ", *(sample + i));
		}
		printf("\n");

		int r = ltnsdi_audio_channels_write(ctx, buf, audioFrames, wordLength, channels, stride);
		if (r < 0) {
			fprintf(stderr, "Error writing audio into minitoring cores.\n");
			ret = -1;
		}
		break;
	}
	free(buf);

	return ret;
}

static int test_24bitPayload_in_32bit_words_G1C0(struct ltnsdi_context_s *ctx)
{
	int ret = 0;

	uint32_t channels = 1;
	uint32_t wordLength = 32;
	uint32_t stride = (wordLength / 8) * channels;
	uint32_t audioFrames = 128;

	uint8_t *buf = malloc(audioFrames * stride * 4);

	uint8_t header[32];
	generate_smpte337_header(&header[0], 0x0010, 0x07, DATA_MODE_24BIT);

	int cnt = 0;
	while (cnt++ < 16) {
		uint32_t *sample = (uint32_t *)buf;
		uint32_t *s32 = (uint32_t *)buf;
		for (int i = 0; i < (stride / 2) * audioFrames; i++) {
			*(sample + i) = -1;
		}

		/* Write the 337 header into a buffer G1C0 */
		for (int i = 0; i < 6; i++) {
			uint32_t v = (header[(i * 3) + 0] << 24) | (header[(i * 3) + 1] << 16) | (header[(i * 3) + 2] << 8);
			*(s32 + i) = v;

			printf("%08x ", *(s32 + i));
		}

		printf("\n");
		sample = (uint32_t *)buf;
		for (int i = 0; i < 20; i++) {
			printf("%08x ", *(sample + i));
		}
		printf("\n");

		int r = ltnsdi_audio_channels_write(ctx, buf, audioFrames, wordLength, channels, stride);
		if (r < 0) {
			fprintf(stderr, "Error writing audio into minitoring cores.\n");
			ret = -1;
		}
	}
	free(buf);

	return ret;
}

static int test_16bitPCM_G1C0_G1C1(struct ltnsdi_context_s *ctx)
{
	int ret = 0;

	uint32_t channels = 2;
	uint32_t wordLength = 32;
	uint32_t stride = (wordLength / 8) * channels;
	uint32_t audioFrames = 128;

	int cnt = 0;
	while (cnt++ < 2) {
		uint32_t *buf = malloc(audioFrames * stride);
		uint32_t *sample = buf;

		for (int j = 0; j < audioFrames; j++) {
			for (int i = 0; i < channels; i++) {
				uint32_t val = 0xffff;
				*sample = val;
				*sample &= 0x0000ffff;
				sample++;
			}
		}

		int r = ltnsdi_audio_channels_write(ctx, (uint8_t *)buf, audioFrames, wordLength, channels, stride);
		if (r < 0) {
			fprintf(stderr, "Error writing audio into minitoring cores.\n");
			ret = -1;
		}
		free(buf);
	}

	return ret;
}

int demo_main(int argc, char *argv[])
{
	struct ltnsdi_context_s *ctx;
	if (ltnsdi_context_alloc(&ctx) < 0) {
		fprintf(stderr, "Error allocating a general SDI context.\n");
		return -1;
	}
	printf("Allocated a SDI helper context.\n");

	int results = 0;
	//results += test_16bitPayload_in_32bit_words_G1C0(ctx);
	//results += test_24bitPayload_in_32bit_words_G1C0(ctx);
	results += test_16bitPCM_G1C0_G1C1(ctx);

	ltnsdi_context_free(ctx);
	printf("Free'd the SDI helper context.\n");

	return 0;
}
