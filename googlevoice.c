/* * Standard includes */ 
#include <stdio.h>
#include <sys/ioctl.h> 
#include <unistd.h> 
#include <fcntl.h> 
#include <stdlib.h>
#include <sys/soundcard.h> 
#include <errno.h>
/* * Mandatory variables. */ 
#define BUF_SIZE 160000 
#define CHANNELS 1 /*mono seems enough*/
#define SPEED 8000

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

static void wav_init_header(WAVEHDR *fileheader)
{
    /* stolen from cdda2wav */
    int nBitsPerSample = 16;
    int channels       = CHANNELS;
    int rate           = SPEED;

    unsigned long nBlockAlign = channels * ((nBitsPerSample + 7) / 8);
    unsigned long nAvgBytesPerSec = nBlockAlign * rate;
    unsigned long temp = 0 + sizeof(WAVEHDR) - sizeof(CHUNKHDR); /* data length */ 

    fileheader->chkRiff.ckid    = cpu_to_le32(FOURCC_RIFF);
    fileheader->fccWave         = cpu_to_le32(FOURCC_WAVE);
    fileheader->chkFmt.ckid     = cpu_to_le32(FOURCC_FMT);
    fileheader->chkFmt.dwSize   = cpu_to_le32(16);
    fileheader->wFormatTag      = cpu_to_le16(WAVE_FORMAT_PCM);
    fileheader->nChannels       = cpu_to_le16(channels);
    fileheader->nSamplesPerSec  = cpu_to_le32(rate);
    fileheader->nAvgBytesPerSec = cpu_to_le32(nAvgBytesPerSec);
    fileheader->nBlockAlign     = cpu_to_le16(nBlockAlign);
    fileheader->wBitsPerSample  = cpu_to_le16(nBitsPerSample);
    fileheader->chkData.ckid    = cpu_to_le32(FOURCC_DATA);
    fileheader->chkRiff.dwSize  = cpu_to_le32(temp);
    fileheader->chkData.dwSize  = cpu_to_le32(0); /* data length */
}

extern int upload(void* ptr, size_t len);

int main()
{
    int audio_fd;
    void* buffer;
    void* audio_buffer;
    int len;
    int format = AFMT_S16_LE; 
    int channels = CHANNELS; 
    int speed = SPEED; 
    int buffersize = sizeof(WAVEHDR) + BUF_SIZE;
    buffer = malloc(buffersize);

    /* Init header */
    WAVEHDR* wavheader = (WAVEHDR*)buffer;
    wav_init_header(wavheader);
    audio_buffer = buffer + sizeof(WAVEHDR);

    /* Open device */
    if ((audio_fd = open("/dev/dsp", O_RDONLY, 0)) == -1) 
    { 
        /* Open of device failed */ 
        perror("/dev/dsp"); 
        exit(1); 
    } 

    /* Set format */
    if (ioctl(audio_fd, SNDCTL_DSP_SETFMT, &format) == -1) 
    { 
        /* fatal error */ 
        perror("SNDCTL_DSP_SETFMT"); 
        exit(1); 
    } 
    if (format != AFMT_S16_LE) 
    { 
        /* format not supported. */
        //TODO
    }
    if (ioctl(audio_fd, SNDCTL_DSP_CHANNELS, &channels) == -1) 
    { 
        /* Fatal error */ 
        perror("SNDCTL_DSP_CHANNELS"); 
        exit(1); 
    }
    if (channels != CHANNELS )
    {
        /* channel number not supported */
        //TODO
    }
    if (ioctl(audio_fd, SNDCTL_DSP_SPEED, &speed)==-1) 
    { 
        /* Fatal error */ 
        perror("SNDCTL_DSP_SPEED"); 
        exit(1); 
    } 
    if (speed != SPEED) 
    {  
        /* speed not supported */ 
        //TODO
    }

    /* Record */
    if ((len = read(audio_fd, audio_buffer, BUF_SIZE)) == -1) 
    { 
        perror("audio read"); 
        exit(1); 
    }

    unsigned long temp = BUF_SIZE + sizeof(WAVEHDR) - sizeof(CHUNKHDR);
    wavheader->chkRiff.dwSize = cpu_to_le32(temp);
    wavheader->chkData.dwSize = cpu_to_le32(BUF_SIZE);

    /* upload it */
    upload(buffer, (size_t)buffersize);

    return 0;
}

