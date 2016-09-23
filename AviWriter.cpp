//
// Created by akruglov on 20.09.16.
//

#include "AviWriter.h"

#include <iostream>


AviWriter::AviWriter(const char *outputFileName, unsigned int bufferSize, unsigned short movieWidth,
                     unsigned short movieHeight, unsigned int movieFPS) :
    fBufferSize(bufferSize), fMovieWidth(movieWidth), fMovieHeight(movieHeight), fMovieFPS(movieFPS)
{
    fOutFid = fopen(outputFileName, "wb");

    if (fOutFid == NULL)
        std::cerr << "Unable to open file " << outputFileName << std::endl;

    fNumSubsessions = 2;

    fNumBytesWritten = 0;

    fRIFFSizePosition = fRIFFSizeValue = 0;
    fJunkNumber = 0;
    fMoviSizePosition = fMoviSizeValue = 0;
    fAVIHMaxBytesPerSecondPosition = 0;
    fAVIHFrameCountPosition = 0;
    fVideoMaxBytesPerSecond = 0;
    fNumVideoFrames = 0;
    fPrevVideoPts = 0;
    fAVIScale = 0;
    fAVIRate = 0;
    fAVISize = 0;
    fAVISubsessionTag = 0;
    fAVICodecHandlerType = 0;
    fAudioMaxBytesPerSecond = 0;
    fNumAudioFrames = 0;
    fPrevAudioPts = 0;
    fAVISamplingFrequency = 0;
    fWAVCodecTag = 0;
    fVideoSTRHFrameCountPosition = 0;
    fAudioSTRHFrameCountPosition = 0;
    fNumAudioChannels = 0;

    addFileHeader_AVI();
}


AviWriter::~AviWriter()
{
    if (fOutFid != NULL)
        fclose(fOutFid);

    if (indexRecords.size())
    {
        auto record = indexRecords.front();
        delete record;
        indexRecords.pop();
    }
}

void AviWriter::useFrame(const avi_frame_t &frame)
{
    if (frame.type == AVI_AUDIO_FRAME) {
        if (fPrevAudioPts) {
            int uSecondsDiff = frame.pts - fPrevAudioPts;
            unsigned bytePerSeconds = frame.size * 1000000.0 / uSecondsDiff;
            if (bytePerSeconds > fAudioMaxBytesPerSecond)
                fAudioMaxBytesPerSecond = bytePerSeconds;
        }

        fPrevAudioPts = frame.pts;
    } else {
        if (fPrevVideoPts) {
            int uSecondsDiff = frame.pts - fPrevVideoPts;
            unsigned bytePerSeconds = frame.size * 1000000.0 / uSecondsDiff;
            if (bytePerSeconds > fVideoMaxBytesPerSecond)
                fVideoMaxBytesPerSecond = bytePerSeconds;
        }

        fPrevVideoPts = frame.pts;
    }

    unsigned subsessionTag = (frame.type == AVI_AUDIO_FRAME ? fourChar('0', '1', 'w', 'b') : fourChar('0', '0', 'd', 'c'));

    auto indexRecord = new AVIIndexRecord;

    indexRecord->fChunkId = subsessionTag;
    int testIndex = (frame.type == AVI_AUDIO_FRAME ? 0 : 4);
    indexRecord->fFlags = frame.data[testIndex] == 0x67 ? 0x10 : 0;
    indexRecord->fOffset = 4 + fNumBytesWritten; // riff chunk header offset from movi chunk start (4 = movi)
    indexRecord->fSize = frame.size; // chunk size excluding riff header size (only frame data)

    indexRecords.push(indexRecord);

    fNumBytesWritten += addWord(subsessionTag);
    fNumBytesWritten += addWord(frame.size);

    fwrite(frame.data, 1, frame.size, fOutFid);
    fNumBytesWritten += frame.size;

    // Pad to an even length:

    if (frame.size % 2 == 0)
        fNumBytesWritten += addByte(0);

    (frame.type == AVI_AUDIO_FRAME ? fNumAudioFrames : fNumVideoFrames)++;
}

void AviWriter::completeOutputFile()
{
    unsigned maxBytesPerSecond = fAudioMaxBytesPerSecond + fVideoMaxBytesPerSecond;

    setWord(fVideoSTRHFrameCountPosition, fNumVideoFrames);

    if (fNumAudioFrames)
        setWord(fAudioSTRHFrameCountPosition, fNumAudioFrames);

    add4ByteString("idx1");
    int indexSize = indexRecords.size() * 4 * 4;
    addWord(indexRecords.size() * 4 * 4);

    while(!indexRecords.empty()) {
        auto record = indexRecords.front();
        addWord(record->fChunkId);
        addWord(record->fFlags);
        addWord(record->fOffset);
        addWord(record->fSize);
        delete record;
        indexRecords.pop();
    }

    fRIFFSizeValue += fNumBytesWritten + indexSize;
    setWord(fRIFFSizePosition, fRIFFSizeValue);

    setWord(fAVIHMaxBytesPerSecondPosition, maxBytesPerSecond);
    setWord(fAVIHFrameCountPosition, fNumVideoFrames > 0 ? fNumVideoFrames : fNumAudioFrames);

    fMoviSizeValue += fNumBytesWritten;
    setWord(fMoviSizePosition, fMoviSizeValue);
}

int64_t AviWriter::SeekFile64(FILE *fid, int64_t offset, int whence)
{
    if (fid == NULL) return -1;

    clearerr(fid);
    fflush(fid);
#if (defined(__WIN32__) || defined(_WIN32)) && !defined(_WIN32_WCE)
    return _lseeki64(_fileno(fid), offset, whence) == (int64_t)-1 ? -1 : 0;
#else
#if defined(_WIN32_WCE)
    return fseek(fid, (long)(offset), whence);
#else
    return fseeko(fid, (off_t)(offset), whence);
#endif
#endif
}

int64_t AviWriter::TellFile64(FILE *fid)
{
    if (fid == NULL) return -1;

    clearerr(fid);
    fflush(fid);
#if (defined(__WIN32__) || defined(_WIN32)) && !defined(_WIN32_WCE)
    return _telli64(_fileno(fid));
#else
#if defined(_WIN32_WCE)
    return ftell(fid);
#else
    return ftello(fid);
#endif
#endif
}

////////// AVI-specific implementation //////////


unsigned AviWriter::addWord(unsigned word) {
    // Add "word" to the file in little-endian order:
    addByte(word); addByte(word>>8);
    addByte(word>>16); addByte(word>>24);

    return 4;
}

unsigned AviWriter::addHalfWord(unsigned short halfWord) {
    // Add "halfWord" to the file in little-endian order:
    addByte((unsigned char)halfWord); addByte((unsigned char)(halfWord>>8));

    return 2;
}

unsigned AviWriter::addZeroWords(unsigned numWords) {
    for (unsigned i = 0; i < numWords; ++i) {
        addWord(0);
    }

    return numWords*4;
}

unsigned AviWriter::add4ByteString(char const* str) {
    addByte(str[0]); addByte(str[1]); addByte(str[2]);
    addByte(str[3] == '\0' ? ' ' : str[3]); // e.g., for "AVI "

    return 4;
}

void AviWriter::setWord(unsigned filePosn, unsigned size) {
    do {
        if (SeekFile64(fOutFid, filePosn, SEEK_SET) < 0) break;
        addWord(size);
        if (SeekFile64(fOutFid, 0, SEEK_END) < 0) break; // go back to where we were

        return;
    } while (0);

    // One of the SeekFile64()s failed, probable because we're not a seekable file
    std::cerr << "AVIFileSink::setWord(): SeekFile64 failed (err " << errno << ")\n";
}

// Methods for writing particular file headers.  Note the following macros:

#define addFileHeader(tag,name) \
    unsigned AviWriter::addFileHeader_##name() { \
        add4ByteString("" #tag ""); \
        unsigned headerSizePosn = (unsigned)TellFile64(fOutFid); addWord(0); \
        add4ByteString("" #name ""); \
        unsigned ignoredSize = 8;/*don't include size of tag or size fields*/ \
        unsigned size = 12

#define addFileHeader1(name) \
    unsigned AviWriter::addFileHeader_##name() { \
        add4ByteString("" #name ""); \
        unsigned headerSizePosn = (unsigned)TellFile64(fOutFid); addWord(0); \
        unsigned ignoredSize = 8;/*don't include size of name or size fields*/ \
        unsigned size = 8

#define addFileHeaderEnd \
  setWord(headerSizePosn, size-ignoredSize); \
  return size; \
}

addFileHeader(RIFF,AVI);
    size += addFileHeader_hdrl();
    size += addFileHeader_movi();
    fRIFFSizePosition = headerSizePosn;
    fRIFFSizeValue = size-ignoredSize;
addFileHeaderEnd;

addFileHeader(LIST,hdrl);
    size += addFileHeader_avih();
    fWriteVideoHeader = true;
    size += addFileHeader_strl();
    fWriteVideoHeader = false;
    size += addFileHeader_strl();

    // Then add another JUNK entry
    ++fJunkNumber;
    size += addFileHeader_JUNK();
addFileHeaderEnd;

#define AVIF_HASINDEX           0x00000010 // Index at end of file?
#define AVIF_MUSTUSEINDEX       0x00000020
#define AVIF_ISINTERLEAVED      0x00000100
#define AVIF_TRUSTCKTYPE        0x00000800 // Use CKType to find key frames?
#define AVIF_WASCAPTUREFILE     0x00010000
#define AVIF_COPYRIGHTED        0x00020000

addFileHeader1(avih);
    unsigned usecPerFrame = fMovieFPS == 0 ? 0 : 1000000/fMovieFPS;
    size += addWord(usecPerFrame); // dwMicroSecPerFrame
    fAVIHMaxBytesPerSecondPosition = (unsigned)TellFile64(fOutFid);
    size += addWord(0); // dwMaxBytesPerSec (fill in later)
    size += addWord(0); // dwPaddingGranularity
    size += addWord(AVIF_TRUSTCKTYPE|AVIF_HASINDEX|AVIF_ISINTERLEAVED); // dwFlags
    fAVIHFrameCountPosition = (unsigned)TellFile64(fOutFid);
    size += addWord(0); // dwTotalFrames (fill in later)
    size += addWord(0); // dwInitialFrame
    size += addWord(fNumSubsessions); // dwStreams
    size += addWord(fBufferSize); // dwSuggestedBufferSize
    size += addWord(fMovieWidth); // dwWidth
    size += addWord(fMovieHeight); // dwHeight
    size += addZeroWords(4); // dwReserved
addFileHeaderEnd;

addFileHeader(LIST,strl);
    size += addFileHeader_strh();
    size += addFileHeader_strf();
    fJunkNumber = 0;
    size += addFileHeader_JUNK();
addFileHeaderEnd;

addFileHeader1(strh);
    if (fWriteVideoHeader)
    {
        fAVISubsessionTag = fourChar('0','0','d','c');
        fAVICodecHandlerType = fourChar('H','2','6','4');
        fAVIScale = 1;
        fAVIRate = fMovieFPS;
        fAVISize = fMovieWidth * fMovieHeight * 3;
    }
    else
    {
        fAVISubsessionTag = fourChar('0', '1', 'w', 'b');
        fAVICodecHandlerType = 1;
        fAVISamplingFrequency = 8000;
        fNumAudioChannels = 1;
        fWAVCodecTag = 0x0007; // PCMU
        fAVIScale = fAVISize = fNumAudioChannels;
        fAVIRate = fAVISize * fAVISamplingFrequency;
    }
    size += add4ByteString(fWriteVideoHeader ? "vids" : "auds"); // fccType
    size += addWord(fAVICodecHandlerType); // fccHandler
    size += addWord(0); // dwFlags
    size += addWord(0); // wPriority + wLanguage
    size += addWord(0); // dwInitialFrames
    size += addWord(fAVIScale); // dwScale
    size += addWord(fAVIRate); // dwRate
    size += addWord(0); // dwStart
    (fWriteVideoHeader ? fVideoSTRHFrameCountPosition : fAudioSTRHFrameCountPosition) = (unsigned)TellFile64(fOutFid);
    size += addWord(0); // dwLength (fill in later)
    size += addWord(fBufferSize); // dwSuggestedBufferSize
    size += addWord((unsigned)-1); // dwQuality
    size += addWord(fAVISize); // dwSampleSize
    size += addWord(0); // rcFrame (start)
    if (fWriteVideoHeader) {
        size += addHalfWord(fMovieWidth);
        size += addHalfWord(fMovieHeight);
    } else {
        size += addWord(0);
    }
addFileHeaderEnd;

addFileHeader1(strf);
    if (fWriteVideoHeader) {
        // Add a BITMAPINFO header:
        unsigned extraDataSize = 0;
        size += addWord(10*4 + extraDataSize); // size
        size += addWord(fMovieWidth);
        size += addWord(fMovieHeight);
        size += addHalfWord(1); // planes
        size += addHalfWord(24); // bits-per-sample #####
        size += addWord(fAVICodecHandlerType); // compr. type
        size += addWord(fAVISize);
        size += addZeroWords(4); // ??? #####
        // Later, add extra data here (if any) #####
    } else {
        // Add a WAVFORMATEX header:
        size += addHalfWord(fWAVCodecTag);
        size += addHalfWord(fNumAudioChannels);
        size += addWord(fAVISamplingFrequency);
        size += addWord(fAVIRate); // bytes per second
        size += addHalfWord(fAVISize); // block alignment
        unsigned bitsPerSample = (fAVISize*8)/fNumAudioChannels;
        size += addHalfWord(bitsPerSample);
    }
addFileHeaderEnd;

#define AVI_MASTER_INDEX_SIZE   256

addFileHeader1(JUNK);
    if (fJunkNumber == 0) {
        size += addHalfWord(4); // wLongsPerEntry
        size += addHalfWord(0); // bIndexSubType + bIndexType
        size += addWord(0); // nEntriesInUse #####
        size += addWord(fAVISubsessionTag); // dwChunkId
        size += addZeroWords(2); // dwReserved
        size += addZeroWords(AVI_MASTER_INDEX_SIZE*4);
    } else {
        size += add4ByteString("odml");
        size += add4ByteString("dmlh");
        unsigned wtfCount = 248;
        size += addWord(wtfCount); // ??? #####
        size += addZeroWords(wtfCount/4);
    }
addFileHeaderEnd;

addFileHeader(LIST,movi);
    fMoviSizePosition = headerSizePosn;
    fMoviSizeValue = size-ignoredSize;
addFileHeaderEnd
