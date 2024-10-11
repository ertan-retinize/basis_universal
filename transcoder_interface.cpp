#include ".\transcoder\basisu_transcoder.h"
#include <vector>
#include <fstream>
#include <iostream>

#ifdef _WIN32
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif

extern "C" {
DLL_EXPORT bool TranscodeKTX2ToRGBA(const char* ktx2FilePath, unsigned char* outputData, int* width, int* height) {
    // Log file for debugging
    std::ofstream log("transcoder_log.txt");
    if (!log.is_open()) {
        std::cerr << "Failed to open log file for debugging." << std::endl;
        return false;
    }

    log << "Starting KTX2 transcoding process..." << std::endl;

    // Load KTX2 file data into memory
    FILE* file = fopen(ktx2FilePath, "rb");
    if (!file) {
        log << "Failed to open KTX2 file: " << ktx2FilePath << std::endl;
        return false;
    }

    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (fileSize == 0) {
        log << "KTX2 file is empty." << std::endl;
        fclose(file);
        return false;
    }

    std::vector<uint8_t> ktx2FileData(fileSize);
    fread(ktx2FileData.data(), 1, fileSize, file);
    fclose(file);

    log << "Loaded KTX2 file successfully, file size: " << fileSize << " bytes." << std::endl;

    // Create KTX2 transcoder
    basist::ktx2_transcoder transcoder;

    // Initialize the transcoder
    if (!transcoder.init(ktx2FileData.data(), fileSize)) {
        log << "Failed to initialize KTX2 transcoder." << std::endl;
        return false;
    }

    log << "KTX2 transcoder initialized successfully." << std::endl;

    // Start transcoding
    if (!transcoder.start_transcoding()) {
        log << "Failed to start KTX2 transcoding." << std::endl;
        return false;
    }

    log << "KTX2 transcoding started successfully." << std::endl;

    // Get image level information
    basist::ktx2_image_level_info levelInfo;
    if (!transcoder.get_image_level_info(levelInfo, 0, 0, 0)) {
        log << "Failed to get KTX2 image level info." << std::endl;
        return false;
    }
    
    log << "Alpha flag: " << (levelInfo.m_alpha_flag ? "Yes" : "No") << std::endl;
    log << "Image level info retrieved: width = " << levelInfo.m_orig_width
        << ", height = " << levelInfo.m_orig_height << std::endl;

    // Retrieve width and height
    *width = levelInfo.m_orig_width;
    *height = levelInfo.m_orig_height;

    // Ensure output buffer is large enough
    if (*width <= 0 || *height <= 0) {
        log << "Invalid width or height." << std::endl;
        return false;
    }
    uint32_t output_size = *width * *height * 4; // 4 bytes per pixel for RGBA32
    // Transcode to RGBA32 format
    const uint32_t transcoding_flags = 0;

    log << "Is UASTC: " << (transcoder.is_uastc() ? "Yes" : "No") << std::endl;
    log << "Is ETC1S: " << (transcoder.is_etc1s() ? "Yes" : "No") << std::endl;

    if (!transcoder.transcode_image_level(0, 0, 0, outputData, output_size, basist::transcoder_texture_format::cTFRGBA32,transcoding_flags))
    {
        log << "Failed to transcode KTX2 image level to RGBA32 with high quality flags." << std::endl;

        log << "Transcoding failed. Format: " << static_cast<int>(transcoder.get_format()) << std::endl;

        log << "First 10 bytes of outputData: ";
        for (int i = 0; i < 10; ++i) {
            log << static_cast<int>(outputData[i]) << " ";
        }
        log << std::endl;

    }
    log << "KTX2 transcoding completed successfully." << std::endl;
    return true;
}
}
