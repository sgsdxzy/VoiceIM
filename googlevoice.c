/* * Standard includes */ 
#include <stdio.h>
#include <sys/ioctl.h> 
#include <unistd.h> 
#include <fcntl.h> 
#include <stdlib.h>
#include <sys/soundcard.h> 
#include <errno.h>
/* * Mandatory variables. */ 
#define BUF_SIZE 960000 /* up to 1 min */ 
#define FRAME_SIZE 4000 /* 1/4 second */
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

int kbhit()
{
    int n;
    ioctl(0, FIONREAD, &n);
    return n;
}

extern int upload(void* ptr, size_t len);

int main()
{
    int audio_fd;
    void* buffer;
    void* audio_buffer;
    void* buffer_point;
    int len, i, j, final_size;
    int ctl = 0;
    int format = AFMT_S16_LE; 
    int channels = CHANNELS; 
    int speed = SPEED; 
    int buffersize = sizeof(WAVEHDR) + BUF_SIZE;
    buffer = malloc(buffersize);

    /* Init header */
    WAVEHDR* wavheader = (WAVEHDR*)buffer;
    wav_init_header(wavheader);
    audio_buffer = buffer + sizeof(WAVEHDR);
    buffer_point = audio_buffer;

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
       fprintf(stderr , "%s\n", "S16LE format is not supported by your device, exiting.");
       exit(1);
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
        fprintf(stderr , "%s\n", "Mono is not supported by your device, exiting.");
        exit(1);
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
        fprintf(stderr , "%s\n", "8000Hz sample rate is not supported by your device, exiting.");
        exit(1);
    }

    /* Record */
    for (i = 0; i < 240; i++)
    {    
        if ((len = read(audio_fd, buffer_point, FRAME_SIZE)) == -1) 
        { 
            perror("audio read"); 
            exit(1); 
        }
        buffer_point += FRAME_SIZE;
        ctl = kbhit();
        if (ctl > 0)
        {
            printf("%s\n","Key pressed");
            for (j=0; j < ctl; j++)
            {
                getchar();
            }
            break;
        }
    }
    if (i == 240)
    {
        printf("%s\n", "Time up!");
        final_size = BUF_SIZE;
    }
    else
    {
        final_size = (i + 1) * FRAME_SIZE;
    }

    /* Finalize header */
    unsigned long temp = final_size + sizeof(WAVEHDR) - sizeof(CHUNKHDR);
    wavheader->chkRiff.dwSize = cpu_to_le32(temp);
    wavheader->chkData.dwSize = cpu_to_le32(final_size);

    /* upload it */
    upload(buffer, (size_t)(final_size + sizeof(WAVEHDR)));

    /* Exiting */
    free(buffer);

    return 0;
}

