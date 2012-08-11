#ifndef COMMONTYPES
#define COMMONTYPES

#include <pthread.h>
#include <curl/curl.h>
#include <string.h>

/* this buffer holds the digitized audio */
//unsigned char buf[LENGTH*RATE*SIZE*CHANNELS/8];
typedef unsigned char   BYTE;
typedef unsigned short  WORD;
typedef unsigned int  DWORD;
typedef unsigned int  FOURCC;    /* a four character code */

/* flags for 'wFormatTag' field of WAVEFORMAT */
#define WAVE_FORMAT_PCM 1

/* MMIO macros */
#define mmioFOURCC(ch0, ch1, ch2, ch3) \
  ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) | \
  ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24))

#define FOURCC_RIFF    mmioFOURCC ('R', 'I', 'F', 'F')
#define FOURCC_LIST    mmioFOURCC ('L', 'I', 'S', 'T')
#define FOURCC_WAVE    mmioFOURCC ('W', 'A', 'V', 'E')
#define FOURCC_FMT    mmioFOURCC ('f', 'm', 't', ' ')
#define FOURCC_DATA    mmioFOURCC ('d', 'a', 't', 'a')

typedef struct CHUNKHDR {
    FOURCC ckid;        /* chunk ID */
    DWORD dwSize;             /* chunk size */
} CHUNKHDR;

/* simplified Header for standard WAV files */
typedef struct WAVEHDR {
    CHUNKHDR chkRiff;
    FOURCC fccWave;
    CHUNKHDR chkFmt;
    WORD wFormatTag;       /* format type */
    WORD nChannels;       /* number of channels (i.e. mono, stereo, etc.) */
    DWORD nSamplesPerSec;  /* sample rate */
    DWORD nAvgBytesPerSec; /* for buffer estimation */
    WORD nBlockAlign;       /* block size of data */
    WORD wBitsPerSample;
    CHUNKHDR chkData;
} WAVEHDR;

/* WAVE PROPERTY FOR PTHREAD */
typedef struct WAVPROP {
    void* ptr;
    size_t len;
} WAVPROP;

#if BYTE_ORDER == BIG_ENDIAN
# define cpu_to_le32(x) SWAP4((x))
# define cpu_to_le16(x) SWAP2((x))
# define le32_to_cpu(x) SWAP4((x))
# define le16_to_cpu(x) SWAP2((x))
#else
# define cpu_to_le32(x) (x)
# define cpu_to_le16(x) (x)
# define le32_to_cpu(x) (x)
# define le16_to_cpu(x) (x)
#endif

#endif
