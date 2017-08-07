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
#include <inttypes.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <assert.h>
#include <zlib.h>
#if HAVE_CURSES_H
#include <curses.h>
#endif
#include <libgen.h>
#include <signal.h>
#include <libltnsdi/ltnsdi.h>

#include "hexdump.h"
#include "version.h"
#include "DeckLinkAPI.h"

#define WIDE 160

class DeckLinkCaptureDelegate : public IDeckLinkInputCallback
{
public:
	DeckLinkCaptureDelegate();
	~DeckLinkCaptureDelegate();

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, LPVOID * ppv) { return E_NOINTERFACE; }
	virtual ULONG STDMETHODCALLTYPE AddRef(void);
	virtual ULONG STDMETHODCALLTYPE Release(void);
	virtual HRESULT STDMETHODCALLTYPE VideoInputFormatChanged(BMDVideoInputFormatChangedEvents, IDeckLinkDisplayMode *, BMDDetectedVideoInputFormatFlags);
	virtual HRESULT STDMETHODCALLTYPE VideoInputFrameArrived(IDeckLinkVideoInputFrame *, IDeckLinkAudioInputPacket *); 

private:
	ULONG m_refCount;
	pthread_mutex_t m_mutex;
};

#define RELEASE_IF_NOT_NULL(obj) \
        if (obj != NULL) { \
                obj->Release(); \
                obj = NULL; \
        }

/*
48693539 Mode:  9 HD 1080i 59.94    Mode:  6 HD 1080p 29.97  

Decklink Hardware supported modes:
[decklink @ 0x25da300] Mode:  0 NTSC                     6e747363 [ntsc]
[decklink @ 0x25da300] Mode:  1 NTSC 23.98               6e743233 [nt23]
[decklink @ 0x25da300] Mode:  2 PAL                      70616c20 [pal ]
[decklink @ 0x25da300] Mode:  3 HD 1080p 23.98           32337073 [23ps]
[decklink @ 0x25da300] Mode:  4 HD 1080p 24              32347073 [24ps]
[decklink @ 0x25da300] Mode:  5 HD 1080p 25              48703235 [Hp25]
[decklink @ 0x25da300] Mode:  6 HD 1080p 29.97           48703239 [Hp29]
[decklink @ 0x25da300] Mode:  7 HD 1080p 30              48703330 [Hp30]
[decklink @ 0x25da300] Mode:  8 HD 1080i 50              48693530 [Hi50]
[decklink @ 0x25da300] Mode:  9 HD 1080i 59.94           48693539 [Hi59]
[decklink @ 0x25da300] Mode: 10 HD 1080i 60              48693630 [Hi60]
[decklink @ 0x25da300] Mode: 11 HD 720p 50               68703530 [hp50]
[decklink @ 0x25da300] Mode: 12 HD 720p 59.94            68703539 [hp59]
[decklink @ 0x25da300] Mode: 13 HD 720p 60               68703630 [hp60]
*/

static pthread_mutex_t sleepMutex;
static pthread_cond_t sleepCond;
static int g_showStartupMemory = 0;
static int g_verbose = 0;

static IDeckLink *deckLink;
static IDeckLinkInput *deckLinkInput;
static IDeckLinkDisplayModeIterator *displayModeIterator;

static BMDTimecodeFormat g_timecodeFormat = 0;
static int g_videoModeIndex = -1;
static int g_shutdown = 0;
static int g_monitor_reset = 0;
static int g_monitor_mode = 0;
static int g_no_signal = 1;
static BMDDisplayMode g_detected_mode_id = 0;
static BMDDisplayMode g_requested_mode_id = 0;

static unsigned long audioFrameCount = 0;
static struct frameTime_s {
	unsigned long long lastTime;
	unsigned long long frameCount;
	unsigned long long remoteFrameCount;
} frameTimes[2];

struct ltnsdi_context_s *g_sdi_ctx = NULL;

#if HAVE_CURSES_H
pthread_t threadId;
//static struct vanc_cache_s *selected = 0;

static void cursor_expand_all()
{
}

static void cursor_expand()
{
}

static void cursor_down()
{
}

static void cursor_up()
{
}

static void vanc_monitor_stats_dump_curses()
{
	int linecount = 0;
	int headLineColor = 1;
	int cursorColor = 5;

	char head_c[160];
	if (g_no_signal)
		sprintf(head_c, "NO SIGNAL");
	else
	if (g_requested_mode_id == g_detected_mode_id)
		sprintf(head_c, "SIGNAL LOCKED");
	else {
		//sprintf(head_c, "CHECK SIGNAL SETTINGS %x == %x", g_detected_mode_id, g_requested_mode_id);
		sprintf(head_c, "CHECK SIGNAL SETTINGS");
		headLineColor = 4;
	}

	char head_a[160];
	sprintf(head_a, "LTNSDI_AUDIO_ANALYZER (%s)", GIT_VERSION);

	char head_b[160];
	int blen = (WIDE - 5) - (strlen(head_a) + strlen(head_c));
	memset(head_b, 0x20, sizeof(head_b));
	head_b[blen] = 0;

	attron(COLOR_PAIR(headLineColor));
	mvprintw(linecount++, 0, "%s%s%s", head_a, head_b, head_c);
        attroff(COLOR_PAIR(headLineColor));

	attron(COLOR_PAIR(6));
	mvprintw(linecount++, 0, "               Sample");
	mvprintw(linecount++, 0, "Group  Channel  Width  Payload Description   Type  Name           Bitrate/ps  Last Update     Codec Payload 1..9 Bytes    Status");
	attroff(COLOR_PAIR(6));

	linecount++;
	mvprintw(linecount++, 0, "    1  0: DIGITAL  24  SMPTE 337             0x03  A/52 Audio     384kb       08-03 17:23:58  FC 1B 08 21 39 22 18 17 21  OK");
	mvprintw(linecount++, 0, "    1  1: DIGITAL  20  SMPTE 337             0x03  A/52 Audio     192kb       08-03 17:23:56  FC 1B 08 21 39 22 16 03 41  OK");
	mvprintw(linecount++, 0, "    1  2: DIGITAL  16  SMPTE 337             0x07  AAC Part 4     192kb       08-03 17:22:31  FF FB 08 09 C1 80 80 91 18  Missing Payload");
	mvprintw(linecount++, 0, "    1  3: UNUSED");

	linecount++;
	mvprintw(linecount++, 0, "    2  0: PCM      16  Raw PCM                     Channel Left   28750       08-03 17:22:31  > |||||||||||||||||         OK");
	mvprintw(linecount++, 0, "    2  1: PCM      16  Raw PCM                     Channel Right  220         08-03 17:22:31  > ||                        Silent?");
	mvprintw(linecount++, 0, "    2  2: UNUSED");
	mvprintw(linecount++, 0, "    2  3: UNUSED");

	linecount++;
	mvprintw(linecount++, 0, "    3  0: UNUSED");
	mvprintw(linecount++, 0, "    3  1: UNUSED");
	mvprintw(linecount++, 0, "    3  2: UNUSED");
	mvprintw(linecount++, 0, "    3  3: UNUSED");

	linecount++;
	mvprintw(linecount++, 0, "    4  0: UNUSED");
	mvprintw(linecount++, 0, "    4  1: UNUSED");
	mvprintw(linecount++, 0, "    4  2: UNUSED");
	mvprintw(linecount++, 0, "    4  3: UNUSED");

	linecount++;
	attron(COLOR_PAIR(2));
        mvprintw(linecount++, 0, "q)uit r)eset e)xpand E)xpand all");
	attroff(COLOR_PAIR(2));

	char tail_c[160];
	time_t now = time(0);
	sprintf(tail_c, "%s", ctime(&now));

	char tail_a[160];
	sprintf(tail_a, "LTNSDI_AUDIO_ANALYZER");

	char tail_b[160];
	blen = (WIDE - 4) - (strlen(tail_a) + strlen(tail_c));
	memset(tail_b, 0x20, sizeof(tail_b));
	tail_b[blen] = 0;

	attron(COLOR_PAIR(1));
	mvprintw(linecount++, 0, "%s%s%s", tail_a, tail_b, tail_c);
        attroff(COLOR_PAIR(1));
}

static void vanc_monitor_stats_dump()
{
}

static void signal_handler(int signum);
static void *thread_func_input(void *p)
{
	while (!g_shutdown) {
		int ch = getch();
		if (ch == 'q') {
			signal_handler(1);
			break;
		}
		if (ch == 'r')
			g_monitor_reset = 1;
		if (ch == 'e')
			cursor_expand();
		if (ch == 'E')
			cursor_expand_all();
		if (ch == 0x1b) {
			ch = getch();

			/* Cursor keys */
			if (ch == 0x5b) {
				ch = getch();
				if (ch == 0x41) {
					/* Up arrow */
					cursor_up();
				} else
				if (ch == 0x42) {
					/* Down arrow */
					cursor_down();
				} else
				if (ch == 0x43) {
					/* Right arrow */
				} else
				if (ch == 0x44) {
					/* Left arrow */
				}
			}
		}
	}
	return 0;
}

static void *thread_func_draw(void *p)
{
	noecho();
	curs_set(0);
	start_color();
	init_pair(1, COLOR_WHITE, COLOR_BLUE);
	init_pair(2, COLOR_CYAN, COLOR_BLACK);
	init_pair(3, COLOR_RED, COLOR_BLACK);
	init_pair(4, COLOR_RED, COLOR_BLUE);
	init_pair(5, COLOR_BLACK, COLOR_WHITE);
	init_pair(6, COLOR_YELLOW, COLOR_BLACK);

	while (!g_shutdown) {
		if (g_monitor_reset) {
			g_monitor_reset = 0;
                        //vanc_cache_reset(vanchdl);
		}

		clear();
                vanc_monitor_stats_dump_curses();

		refresh();
		usleep(100 * 1000);
	}

	return 0;
}
#endif /* HAVE_CURSES_H */

static void signal_handler(int signum)
{
	pthread_cond_signal(&sleepCond);
	g_shutdown = 1;
}

static void showMemory(FILE * fd)
{
	char fn[64];
	char s[80];
	sprintf(fn, "/proc/%d/statm", getpid());

	FILE *fh = fopen(fn, "rb");
	if (!fh)
		return;

	memset(s, 0, sizeof(s));
	size_t wlen = fread(s, 1, sizeof(s) - 1, fh);
	fclose(fh);

	if (wlen > 0) {
		fprintf(fd, "%s: %s", fn, s);
	}
}

static unsigned long long msecsX10()
{
	unsigned long long elapsedMs;

	struct timeval now;
	gettimeofday(&now, 0);

	elapsedMs = (now.tv_sec * 10000.0);	/* sec to ms */
	elapsedMs += (now.tv_usec / 100.0);	/* us to ms */

	return elapsedMs;
}

static char g_mode[5];		/* Racey */
static const char *display_mode_to_string(BMDDisplayMode m)
{
	g_mode[4] = 0;
	g_mode[3] = m;
	g_mode[2] = m >> 8;
	g_mode[1] = m >> 16;
	g_mode[0] = m >> 24;

	return &g_mode[0];
}

DeckLinkCaptureDelegate::DeckLinkCaptureDelegate()
: m_refCount(0)
{
	pthread_mutex_init(&m_mutex, NULL);
}

DeckLinkCaptureDelegate::~DeckLinkCaptureDelegate()
{
	pthread_mutex_destroy(&m_mutex);
}

ULONG DeckLinkCaptureDelegate::AddRef(void)
{
	pthread_mutex_lock(&m_mutex);
	m_refCount++;
	pthread_mutex_unlock(&m_mutex);

	return (ULONG) m_refCount;
}

ULONG DeckLinkCaptureDelegate::Release(void)
{
	pthread_mutex_lock(&m_mutex);
	m_refCount--;
	pthread_mutex_unlock(&m_mutex);

	if (m_refCount == 0) {
		delete this;
		return 0;
	}

	return (ULONG) m_refCount;
}

HRESULT DeckLinkCaptureDelegate::VideoInputFrameArrived(IDeckLinkVideoInputFrame *videoFrame, IDeckLinkAudioInputPacket *audioFrame)
{
	if (g_shutdown == 1) {
		g_shutdown = 2;
		return S_OK;
	}
	if (g_shutdown == 2)
		return S_OK;

	IDeckLinkVideoFrame *rightEyeFrame = NULL;
	IDeckLinkVideoFrame3DExtensions *threeDExtensions = NULL;
	void *frameBytes;
	void *audioFrameBytes;
	struct frameTime_s *frameTime;

	if (g_showStartupMemory) {
		showMemory(stderr);
		g_showStartupMemory = 0;
	}

	if (videoFrame) {
		if (videoFrame->GetFlags() & bmdFrameHasNoInputSource && !g_monitor_mode) {
			g_no_signal = 1;
		} else {
			g_no_signal = 0;
		}

	}

	// Handle Audio Frame
	if (audioFrame) {
		audioFrameCount++;
		frameTime = &frameTimes[1];

		uint32_t sampleSize = audioFrame->GetSampleFrameCount() * 16 * (32 / 8);

		unsigned long long t = msecsX10();
		double interval = t - frameTime->lastTime;
		interval /= 10;

		uint32_t sfc = audioFrame->GetSampleFrameCount();
		if (g_verbose) {
			fprintf(stdout,
				"Audio received (#%10lu) - Size: %u sfc: %lu channels: %u depth: %u bytes  (%7.2f ms)\n",
				audioFrameCount,
				sampleSize,
				sfc,
				16,
				32 / 8,
				interval);
		}

		audioFrame->GetBytes(&audioFrameBytes);
		uint32_t strideBytes = 16 * (32 / 8);
		if (ltnsdi_audio_channels_write(g_sdi_ctx, (uint8_t *)audioFrameBytes, sfc, 32, 16, strideBytes) < 0) {
		}

		frameTime->frameCount++;
		frameTime->lastTime = t;

	}
	return S_OK;
}

HRESULT DeckLinkCaptureDelegate::VideoInputFormatChanged(BMDVideoInputFormatChangedEvents events, IDeckLinkDisplayMode * mode, BMDDetectedVideoInputFormatFlags)
{
	g_detected_mode_id = mode->GetDisplayMode();
	return S_OK;
}

static void listDisplayModes()
{
	int displayModeCount = 0;
	IDeckLinkDisplayMode *displayMode;
	while (displayModeIterator->Next(&displayMode) == S_OK) {

		char *displayModeString = NULL;
		HRESULT result = displayMode->GetName((const char **)&displayModeString);
		if (result == S_OK) {
			BMDTimeValue frameRateDuration, frameRateScale;
			displayMode->GetFrameRate(&frameRateDuration, &frameRateScale);

			fprintf(stderr, "        %2d:  %-20s \t %li x %li \t %g FPS [0x%08x]\n",
				displayModeCount, displayModeString,
				displayMode->GetWidth(),
				displayMode->GetHeight(),
				(double)frameRateScale /
				(double)frameRateDuration,
				displayMode->GetDisplayMode());

			free(displayModeString);
			displayModeCount++;
		}

		displayMode->Release();
	}
}

static int usage(const char *progname, int status)
{
	fprintf(stderr, "Analyze SDI audio stream.\n");
	fprintf(stderr, "Usage: %s -m <mode id> [OPTIONS]\n"
		"\n" "    -m <mode id>:\n", basename((char *)progname));

	fprintf(stderr,
		"    -p <pixelformat>\n"
		"         0:   8 bit YUV (4:2:2)\n"
		"         1:  10 bit YUV (4:2:2) (default)\n"
		"         2:  10 bit RGB (4:4:4)\n"
		"    -L              List available display modes\n"
		"    -v              Increase level of verbosity (def: 0)\n"
		"    -i <number>     Capture from input port (def: 0)\n"
#if HAVE_CURSES_H
		"    -M              Display an interractive UI.\n"
#endif
		"\n"
		"Display and show audio related materials for a 1080i 59.94 stream:\n"
		"    %s -m9 -p1 -M\n\n",
		basename((char *)progname)
		);

	exit(status);
}

static int _main(int argc, char *argv[])
{
	IDeckLinkIterator *deckLinkIterator = CreateDeckLinkIteratorInstance();
	DeckLinkCaptureDelegate *delegate;
	IDeckLinkDisplayMode *displayMode;
	BMDVideoInputFlags inputFlags = bmdVideoInputEnableFormatDetection;
	BMDDisplayMode selectedDisplayMode = bmdModeNTSC;
	BMDPixelFormat pixelFormat = bmdFormat10BitYUV;
	int displayModeCount = 0;
	int exitStatus = 1;
	int ch;
	int portnr = 0;
	bool foundDisplayMode = false;
	bool wantHelp = false;
	bool wantDisplayModes = false;
	HRESULT result;

	pthread_mutex_init(&sleepMutex, NULL);
	pthread_cond_init(&sleepCond, NULL);

	while ((ch = getopt(argc, argv, "?h3c:s:f:a:m:n:p:t:vV:I:i:l:LP:MS")) != -1) {
		switch (ch) {
		case 'm':
			g_videoModeIndex = atoi(optarg);
			break;
		case 'i':
			portnr = atoi(optarg);
			break;
		case 'L':
			wantDisplayModes = true;
			break;
#if HAVE_CURSES_H
		case 'M':
			g_monitor_mode = 1;
			break;
#endif
		case 'v':
			g_verbose++;
			break;
		case 'p':
			switch (atoi(optarg)) {
			case 0:
				pixelFormat = bmdFormat8BitYUV;
				break;
			case 1:
				pixelFormat = bmdFormat10BitYUV;
				break;
			case 2:
				pixelFormat = bmdFormat10BitRGB;
				break;
			default:
				fprintf(stderr, "Invalid argument: Pixel format %d is not valid", atoi(optarg));
				goto bail;
			}
			break;
		case '?':
		case 'h':
			wantHelp = true;
		}
	}

	if (wantHelp) {
		usage(argv[0], 0);
		goto bail;
	}

	if (!deckLinkIterator) {
		fprintf(stderr, "This application requires the DeckLink drivers installed.\n");
		goto bail;
	}

	for (int i = 0; i <= portnr; i++) {
		/* Connect to the nth DeckLink instance */
		result = deckLinkIterator->Next(&deckLink);
		if (result != S_OK) {
			fprintf(stderr, "No capture devices found.\n");
			goto bail;
		}
	}

	if (deckLink->QueryInterface(IID_IDeckLinkInput, (void **)&deckLinkInput) != S_OK) {
		fprintf(stderr, "No input capture devices found.\n");
		goto bail;
	}

	delegate = new DeckLinkCaptureDelegate();
	deckLinkInput->SetCallback(delegate);

	/* Obtain an IDeckLinkDisplayModeIterator to enumerate the display modes supported on output */
	result = deckLinkInput->GetDisplayModeIterator(&displayModeIterator);
	if (result != S_OK) {
		fprintf(stderr, "Could not obtain the video output display mode iterator - result = %08x\n", result);
		goto bail;
	}

	if (wantDisplayModes) {
		listDisplayModes();
		goto bail;
	}

	if (g_videoModeIndex < 0) {
		fprintf(stderr, "No video mode specified\n");
		usage(argv[0], 0);
	}

	while (displayModeIterator->Next(&displayMode) == S_OK) {
		if (g_videoModeIndex == displayModeCount) {

			foundDisplayMode = true;

			const char *displayModeName;
			displayMode->GetName(&displayModeName);
			selectedDisplayMode = displayMode->GetDisplayMode();
			g_detected_mode_id = displayMode->GetDisplayMode();
			g_requested_mode_id = displayMode->GetDisplayMode();

			BMDDisplayModeSupport result;
			deckLinkInput->DoesSupportVideoMode(selectedDisplayMode, pixelFormat, bmdVideoInputFlagDefault, &result, NULL);
			if (result == bmdDisplayModeNotSupported) {
				fprintf(stderr, "The display mode %s is not supported with the selected pixel format\n", displayModeName);
				goto bail;
			}

			if (inputFlags & bmdVideoInputDualStream3D) {
				if (!(displayMode->GetFlags() & bmdDisplayModeSupports3D)) {
					fprintf(stderr, "The display mode %s is not supported with 3D\n", displayModeName);
					goto bail;
				}
			}

			break;
		}
		displayModeCount++;
		displayMode->Release();
	}

	if (!foundDisplayMode) {
		fprintf(stderr, "Invalid mode %d specified\n", g_videoModeIndex);
		goto bail;
	}

	result = deckLinkInput->EnableVideoInput(selectedDisplayMode, pixelFormat, inputFlags);
	if (result != S_OK) {
		fprintf(stderr, "Failed to enable video input. Is another application using the card?\n");
		goto bail;
	}

	result = deckLinkInput->EnableAudioInput(bmdAudioSampleRate48kHz, 32, 16);
	if (result != S_OK) {
		fprintf(stderr, "Failed to enable audio input. Is another application using the card?\n");
		goto bail;
	}

	if (ltnsdi_context_alloc(&g_sdi_ctx) < 0) {
		fprintf(stderr, "Error allocating a general SDI context.\n");
		goto bail;
	}

	result = deckLinkInput->StartStreams();
	if (result != S_OK) {
		fprintf(stderr, "Failed to start stream. Is another application using the card?\n");
		goto bail;
	}

	signal(SIGINT, signal_handler);

#if HAVE_CURSES_H
	if (g_monitor_mode) {
		initscr();
		pthread_create(&threadId, 0, thread_func_draw, NULL);
		pthread_create(&threadId, 0, thread_func_input, NULL);
	}
#endif

	/* All Okay. */
	exitStatus = 0;

	/* Block main thread until signal occurs */
	pthread_mutex_lock(&sleepMutex);
	pthread_cond_wait(&sleepCond, &sleepMutex);
	pthread_mutex_unlock(&sleepMutex);

	while (g_shutdown != 2)
		usleep(50 * 1000);

	fprintf(stdout, "Stopping Capture\n");
	result = deckLinkInput->StopStreams();
	if (result != S_OK) {
		fprintf(stderr, "Failed to start stream. Is another application using the card?\n");
	}

#if HAVE_CURSES_H
	vanc_monitor_stats_dump();
#endif

#if HAVE_CURSES_H
	if (g_monitor_mode)
		endwin();
#endif

bail:
	if (g_sdi_ctx)
		ltnsdi_context_free(g_sdi_ctx);

	RELEASE_IF_NOT_NULL(displayModeIterator);
	RELEASE_IF_NOT_NULL(deckLinkInput);
	RELEASE_IF_NOT_NULL(deckLink);
	RELEASE_IF_NOT_NULL(deckLinkIterator);

	return exitStatus;
}

extern "C" int audio_analyzer_main(int argc, char *argv[])
{
	return _main(argc, argv);
}

