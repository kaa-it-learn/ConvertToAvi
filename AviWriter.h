//
// Created by akruglov on 20.09.16.
//

#ifndef CONVERTTOAVI_AVIWRITER_H
#define CONVERTTOAVI_AVIWRITER_H


#include "Common.h"

#include <cstdio>
#include <cstdint>
#include <queue>


struct AVIIndexRecord
{
    unsigned fChunkId;
    unsigned fFlags;
    unsigned fOffset;
    unsigned fSize;
};


class AviWriter {

public:
    AviWriter(const char* outputFileName,
              unsigned bufferSize = 20000,
              unsigned short movieWidth = 240,
              unsigned short movieHeight = 180,
              unsigned movieFPS = 15);

    ~AviWriter();

    void useFrame(const avi_frame_t& frame);
    void completeOutputFile();

    unsigned written() { return fNumBytesWritten; }

private:
    int64_t SeekFile64(FILE *fid, int64_t offset, int whence);

    int64_t TellFile64(FILE *fid);

    ///// Definitions specific to the AVI file format:

    unsigned addWord(unsigned word); // outputs "word" in little-endian order
    unsigned addHalfWord(unsigned short halfWord);

    unsigned addByte(unsigned char byte) {
        putc(byte, fOutFid);
        return 1;
    }

    unsigned addZeroWords(unsigned numWords);

    unsigned add4ByteString(char const *str);

    void setWord(unsigned filePosn, unsigned size);

    // Define member functions for outputting various types of file header:
#define _header(name) unsigned addFileHeader_##name()

    _header(AVI);

    _header(hdrl);

    _header(avih);

    _header(strl);

    _header(strh);

    _header(strf);

    _header(JUNK);

    _header(movi);

private:
    FILE *fOutFid;
    unsigned fBufferSize;
    unsigned short fMovieWidth, fMovieHeight;
    unsigned fMovieFPS;
    unsigned fNumSubsessions;
    unsigned fRIFFSizePosition, fRIFFSizeValue;
    unsigned fJunkNumber;
    unsigned fMoviSizePosition, fMoviSizeValue;
    unsigned fAVIHMaxBytesPerSecondPosition;
    unsigned fAVIHFrameCountPosition;
    unsigned fVideoMaxBytesPerSecond;
    unsigned fNumVideoFrames;
    int64_t  fPrevVideoPts;
    unsigned fAVIScale;
    unsigned fAVIRate;
    unsigned fAVISize;
    unsigned fAVISubsessionTag;
    unsigned fAVICodecHandlerType;
    unsigned fAudioMaxBytesPerSecond;
    unsigned fNumAudioFrames;
    int64_t  fPrevAudioPts;
    unsigned fAVISamplingFrequency;
    unsigned short fWAVCodecTag;
    unsigned fVideoSTRHFrameCountPosition;
    unsigned fAudioSTRHFrameCountPosition;
    unsigned fNumAudioChannels;
    bool fWriteVideoHeader;
    std::queue<AVIIndexRecord*> indexRecords;
    unsigned fNumBytesWritten;
};

#endif //CONVERTTOAVI_AVIWRITER_H
