#include "Common.h"
#include "AviWriter.h"

#include <iostream>
#include <fstream>
#include <cstring>
#include <memory>
#include <map>


unsigned char startCode[] = {0x00, 0x00, 0x00, 0x01};

const std::string kVideoPath = "/home/akruglov/Projects/Leha/ConvertToAvi/tvh/0/";
const std::string kAudioPath = "/home/akruglov/Projects/Leha/ConvertToAvi/tvh/3/";
const int kMaxFrameNumber = 1857;
const int kMaxSampleNumber = 3093;

const int kMaxFrameSize = 65000;

const unsigned short kMovieWidth = 1920;
const unsigned short kMovieHeight = 1080;
const unsigned kMovieFPS = 15;

class FileException : public std::runtime_error
{
public:
    FileException(const std::string& message) : std::runtime_error(message) {}
};

unsigned char gFrameData[kMaxFrameSize];

avi_frame_t gFrame {};

std::unique_ptr<AviWriter> writer = nullptr;

std::multimap<int64_t, std::string> gFileMap;

void processVideoFiles()
{
    for (int i = 0; i <= kMaxFrameNumber; i++) {

        std::string filePrefix = (i % 15 ? "P" : "I");

        std::string fileName = kVideoPath + filePrefix + std::to_string(i);

        std::ifstream videoFile(fileName, std::ifstream::in | std::ifstream::binary);

        if (!videoFile) {
            throw FileException("Can not open file: " + fileName);
        } else {
            videoFile.read((char*)&gFrame, sizeof(gFrame) - sizeof(unsigned char*));

            if (!videoFile)
                throw FileException("Can not read a frame from file: " + fileName);

            gFileMap.insert(std::make_pair(gFrame.pts, fileName));
        }
    }
}

void processAudioFiles()
{
    for (int i = 0; i <= kMaxSampleNumber; i++) {

        std::string fileName = kAudioPath + "A" + std::to_string(i);

        std::ifstream audioFile(fileName, std::ifstream::in | std::ifstream::binary);

        if (!audioFile) {
            continue;
            //throw FileException("Can not open file: " + fileName);
        } else {
            audioFile.read((char*)&gFrame, sizeof(gFrame) - sizeof(unsigned char*));

            if (!audioFile)
                throw FileException("Can not read a sample from file: " + fileName);

            gFileMap.insert(std::make_pair(gFrame.pts, fileName));
        }
    }
}

void initFileMap()
{
    processVideoFiles();
    processAudioFiles();
}

void init()
{
    gFrame.data = gFrameData;

    initFileMap();

    writer = std::make_unique<AviWriter>("out.avi", kMaxFrameSize, kMovieWidth, kMovieHeight, kMovieFPS);
}

void readFrames()
{
    for (auto& elem : gFileMap) {

        std::string fileName = elem.second;

        std::ifstream file(fileName, std::ifstream::in | std::ifstream::binary);

        file.read((char*)&gFrame, sizeof(gFrame) - sizeof(unsigned char*));

        file.read((char*)gFrameData, gFrame.size);

        if (gFrame.type != AVI_AUDIO_FRAME) {

            int totalSize = 0;

            unsigned char *pData = gFrameData;

            while (totalSize < gFrame.size) {
                int size = *(int *) pData;
                memcpy(pData, startCode, 4);
                pData += size + 4;
                totalSize += size + 4;
            }
        }

        writer->useFrame(gFrame);
    }
}

void readAudioFrames()
{
    for (int i = 0; i <= kMaxSampleNumber; i++) {

        std::string fileName = kAudioPath + "A" + std::to_string(i);

        std::ifstream audioFile(fileName, std::ifstream::in | std::ifstream::binary);

        if (!audioFile) {
            throw FileException("Can not open file: " + fileName);
        } else {
            audioFile.read((char*)&gFrame, sizeof(gFrame) - sizeof(unsigned char*));

            if (!audioFile)
                throw FileException("Can not read a frame from file: " + fileName);
        }
    }
}

int main() {

    init();

    try {
        readFrames();
    } catch (const FileException error) {
        std::cerr << error.what() << std::endl;
        return 1;
    }

    writer->completeOutputFile();

    writer.reset(nullptr);

    return 0;
}