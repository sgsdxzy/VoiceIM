#include <stdio.h>
#include <sys/ioctl.h> 
#include <unistd.h> 
#include <fcntl.h> 
#include <stdlib.h>
#include <sys/soundcard.h> 


int record_init(int format, int channels, int speed)
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
    if (channels != 1 )
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
    if (speed != 8000) 
    {  
        /* speed not supported */ 
        fprintf(stderr , "%s\n", "8000Hz sample rate is not supported by your device, exiting.");
        exit(1);
    }

    return audio_fd;
}

void record_clean(int audio_fd)
{
    if (close(audio_fd) == -1) 
    { 
        /* Close device failed */ 
        perror("/dev/dsp"); 
        exit(1); 
    } 
}
