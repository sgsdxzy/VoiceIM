#include <stdio.h>
#include <stdlib.h>
#include <AL/al.h>
#include <AL/alc.h>

ALCdevice* record_init(ALCenum format, int speed, int buffersize)
{
    ALCenum error;
    alGetError();
    ALCdevice *device = alcCaptureOpenDevice(NULL, speed, format, buffersize);
    if ((error = alcGetError(device)) != ALC_NO_ERROR)
    {
        printf("%s\n","Error occured while alcCaptureOpenDevice");
        exit(1);
    }   
    
    return device;
}

void record_clean(ALCdevice* device)
{
    ALCenum error;
    alGetError();
    alcCaptureCloseDevice(device);
    if ((error = alcGetError(device)) != ALC_NO_ERROR)
    {
        printf("%s\n","Error occured while alcCaptureCloseDevice");
        exit(1);
    }
    
    return;
}




