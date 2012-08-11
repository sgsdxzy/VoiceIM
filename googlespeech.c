/* * Standard includes */ 
#include <stdio.h>
#include <sys/ioctl.h> 
#include <unistd.h> 
#include <fcntl.h> 
#include <stdlib.h>
#include <sys/soundcard.h> 
#include <errno.h>
#include "commontypes.h"
/* * Mandatory variables. */ 
#define BUF_SIZE 960000 /* up to 1 min */ 
#define FRAME_SIZE 4000 /* 1/4 second */
#define CHANNELS 1 /*mono seems enough*/
#define SPEED 8000

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
CURL* handle;

static void wav_finalize_header(WAVEHDR *fileheader, int wav_size)
{
    int nBitsPerSample = 16;
    int channels       = CHANNELS;
    int rate           = SPEED;
    unsigned long temp = wav_size + sizeof(WAVEHDR) - sizeof(CHUNKHDR);
    unsigned long nBlockAlign = channels * ((nBitsPerSample + 7) / 8);
    unsigned long nAvgBytesPerSec = nBlockAlign * rate;

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
    fileheader->chkData.dwSize  = cpu_to_le32(wav_size);
}

static int intdevice(int format, int channels, int speed)
{
    int audio_fd;
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

    return audio_fd;
}

static int kbhit()
{
    int n;
    ioctl(0, FIONREAD, &n);
    return n;
}

static short shortabs(short i)
{
    return i>=0 ? i : -i;
}

static int maxwav(void* buffer_point)
{
    int max;
    short* point = buffer_point;
    short abs;
    int i = FRAME_SIZE / 2;
    int j;
    max = shortabs(point[0]);
    for (j=1;i<i;j++)
    {
        abs = shortabs(point[j]);
        if (max < abs)
        {
            max = abs;
        }
    }
    return max;
}

extern void* upload(void* arg);

int main()
{
    int audio_fd;
    void* buffer;
    void* audio_buffer;
    void* buffer_point;
    WAVPROP* wavp;
    WAVEHDR* wavheader;
    pthread_t tid;
    int len, i, j, final_size, err, counter;
    int ctl = 0;
    int threshold = 10000; /* Threshold of wave strength to be considered speaking */
    int buffersize = sizeof(WAVEHDR) + BUF_SIZE;

    audio_fd = intdevice(AFMT_S16_LE, CHANNELS, SPEED);
    struct curl_slist *headers = NULL;
    upload_init(headers);

    while (1)
    {
        buffer = malloc(buffersize);
        audio_buffer = buffer + sizeof(WAVEHDR);
        buffer_point = audio_buffer;

        /* Wait until speek */
        counter = 0; /* n/4 secs of silence */
        while(1)
        {
            if ((len = read(audio_fd, buffer_point, FRAME_SIZE)) == -1) 
            { 
                perror("audio read"); 
                exit(1); 
            }
            //printf("%d\n", maxwav(buffer_point));
            if (maxwav(buffer_point) > threshold)
            {
                printf("%s\n", "Starting record");
                buffer_point += FRAME_SIZE;
                break;
            }
        }



        /* Record */
        for (i = 0; i < 239; i++)
        {    
            if ((len = read(audio_fd, buffer_point, FRAME_SIZE)) == -1) 
            { 
                perror("audio read"); 
                exit(1); 
            }
            if (maxwav(buffer_point) < threshold)
            {
                counter += 1;
            }
            else
            {
                counter = 0;
            }
            if (counter > 11) /* 3 secs */
            {
                printf("%s\n", "Stop recording");
                i -= 12;
                break;
            }
            buffer_point += FRAME_SIZE;
            ctl = kbhit();
            if (ctl > 0)
            {
                printf("%s\n", "Key pressed");
                for (j=0; j < ctl; j++)
                {
                    getchar();
                }
                break;
            }
        }
        if (i == 239)
        {
            printf("%s\n", "Time up!");
            final_size = BUF_SIZE;
        }
        else
        {
            final_size = (i + 1) * FRAME_SIZE;
        }

        /* Finalize header */
        wavheader = (WAVEHDR*)buffer;
        wav_finalize_header(wavheader, final_size);

        /* upload it */
        wavp = (WAVPROP*)malloc(sizeof(WAVPROP));
        wavp->ptr = buffer;
        wavp->len = (size_t)(final_size + sizeof(WAVEHDR));
        err = pthread_create(&tid, NULL, upload, (void*)wavp);
        if (err != 0) {
		fprintf(stderr, "can't create thread: %s\n", strerror(err));
		exit(1);
	}

    }

    upload_clean(headers);

    return 0;
}

