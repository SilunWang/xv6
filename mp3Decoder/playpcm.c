/*
 * author: SilunWang
 * date: 2013-12-19
 * description: play wav or pcm by alsa
 * compile: gcc playpcm -o plaupcm.c
 * run: aoss ./playpcm XXX.pcm
 * alarm: the *.wav must be 8000Hz 64kbps  8bits MONO(1)
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h> /*for OSS style sound programing */

#define BUFF_SIZE  512 /*buffer size:512 Bytes */
#define FMT8BITS AFMT_U8 /*unsigned 8 bits(for almost PC) */
#define FMT16BITS AFMT_S16_LE /*signed 16 bits,little endian*/

#define FMT8K    8000 /*default sampling rate */
#define FMT11K   11025 /*11,22,44,48 are three pop rate */
#define FMT22K   22050
#define FMT44K   44100
#define FMT48K   48000

#define MONO     1  //单声道
#define STEREO   2  //双声道

int main(int argc, char *argv[])
{

    if (argc != 2)
        printf("input error! ./playpcm filename.pcm");

    //for device: dsp or alsa
    int fd; 
    fd = open("/dev/dsp", O_WRONLY);
    if (fd < 0)
    {
        perror("Can't open /dev/dsp");
        return -1;
    }

    //readfile
    int outfile;
    outfile = open(argv[1], O_RDONLY);
    if (outfile < 0)
    {
        perror("Cannot open file for writing");
        return -1;
    }


    /* set bit format */
    // 16-bits
    int bits = FMT16BITS;
    if (ioctl(fd, SNDCTL_DSP_SETFMT, &bits) == -1)
    {
        fprintf(stderr, "Set fmt to bit %d failed:%s\n", bits,
                strerror(errno));
        return (-1);
    }
    if (bits != FMT16BITS)
    {
        fprintf(stderr, "do not support bit %d, supported 8、16\n", bits);
        return (-1);
    }

    /*set channel */
    // 2-channels
    int channel = STEREO;
    if ((ioctl(fd, SNDCTL_DSP_CHANNELS, &channel)) == -1)
    {
        fprintf(stderr, "Set Audio Channels %d failed:%s\n", channel,
                strerror(errno));
        return (-1);
    }

    /*set speed */
    // 44K rate
    int speed = FMT44K;
    if (ioctl(fd, SNDCTL_DSP_SPEED, &speed) == -1)
    {
        fprintf(stderr, "Set speed to %d failed:%s\n", speed,
                strerror(errno));
        return (-1);
    }
    if (speed != FMT44K)
    {
        fprintf(stderr, "do not support speed %d\n", speed,
                speed);
        return (-1);
    }

    int nRD;
    unsigned char buff[BUFF_SIZE]; /*sound buffer*/
    while (1)
    {
        if ((nRD = read(outfile, buff, BUFF_SIZE)) < 0)
        {
            perror("read sound data file failed");
            return (-1);
        }
        else if (nRD == 0)
        {
            perror("read over!");
            exit(0);
        }

        if (fd > 0)
            write(fd, buff, nRD);
    }

    return 0;
}