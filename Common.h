//
// Created by akruglov on 20.09.16.
//

#ifndef CONVERTTOAVI_COMMON_H
#define CONVERTTOAVI_COMMON_H

#include <cstdint>


#define fourChar(x,y,z,w) ( ((w)<<24)|((z)<<16)|((y)<<8)|(x) )  /*little-endian*/

typedef struct avi_frame_s
{
#define AVI_VIDEO_IFRAME	'I'
#define AVI_VIDEO_PFRAME	'P'
#define AVI_VIDEO_JPEG		'J'
#define AVI_AUDIO_FRAME		'A'
    char type;
    char res[3];
    int number;
    int64_t pts;
    int anumber;
    int size;
    unsigned char *data;
} avi_frame_t;


#endif //CONVERTTOAVI_COMMON_H
