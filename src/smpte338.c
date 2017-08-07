#include <libltnsdi/smpte338.h>

static const char *smpte338_dataTypes[32] =
{
	/* 0 .. 4 */
	"ST 339 - Null Data",
	"ST 340 - AC-3 (audio) data",
	"ST 339 - Time stamp data",
	"ST 339 - Pause data",
	"ST 2041-1 - MPEG-1 Layer I (audio) data",

	/* 5 .. 9 */
	"ST 2041-1 - MPEG-1 Layer II or III (audio) data or MPEG-2 Layer 1, II, or III (audio) data without extension",
	"ST 2041-1 - MPEG-2 Layer I, II, or III (audio) data with extension",
	"ST 2041-2 - MPEG 2 AAC (audio) data in ADTS",
	"Reserved for compatibility with IEC",
	"Reserved for compatibility with IEC",

	/* 10 .. 14 */
	"ST 2041-2 - MPEG-4 AAC (audio) data in ADTS or LATM/LOAS",
	"ST 2041-2 - MPEG-4 HE-AAC (audio) data in ADTS or LATM/LOAS",
	"Reserved for compatibility with IEC",
	"Reserved for compatibility with IEC",
	"Reserved for compatibility with IEC",

	/* 15 .. 19 */
	"Reserved for compatibility with IEC",
	"ST 340 - Enhanced AC-3 (audio) data (professional applications)",
	"ST 2106 - DTS (audio) data",
	"Reserved for compatibility with IEC",
	"Reserved for compatibility with IEC",

	/* 20 .. 24 */
	"Reserved for compatibility with IEC",
	"Reserved for compatibility with IEC",
	"Reserved for compatibility with IEC",
	"Reserved for compatibility with IEC",
	"ST 2101 - AC-4 (audio) data",

	/* 25 .. 29 */
	"Reserved",
	"ST 339 - Utility data type (V sync)",
	"ST 355 - SMPTE KLV data",
	"RDD 33 - Dolby E (audio) data",
	"ST 341 - Captioning data"

	/* 30 .. 31 */
	"ST 339 - User defined data",
	"ST 337 - Data Type in Extended Code Space"
};

const char *smpte338_lookupDataTypeDescription(uint32_t nr)
{
	return smpte338_dataTypes[ nr & 0x1f ];
}
