#include "basisu_transcoder.h"
#include <vector>

#ifdef _WIN32
    #define DLL_EXPORT __declspec(dllexport)
#else
    #define DLL_EXPORT
#endif

extern "C" {
    DLL_EXPORT bool TranscodeBasisToRGBA(const char* basisFilePath, unsigned char* outputData, int* width, int* height) {
        // Load .basis file data into memory
        FILE* file = fopen(basisFilePath, "rb");
        if (!file) return false;

        fseek(file, 0, SEEK_END);
        size_t fileSize = ftell(file);
        fseek(file, 0, SEEK_SET);

        std::vector<uint8_t> basisFileData(fileSize);
        fread(basisFileData.data(), 1, fileSize, file);
        fclose(file);

        // Create BasisU transcoder
        basist::basisu_transcoder transcoder;
        if (!transcoder.start_transcoding(basisFileData.data(), fileSize)) {
            return false; // Failed to start transcoding
        }

        // Initialize basisu_image_level_info structure
        basist::basisu_image_level_info levelInfo;
        if (!transcoder.get_image_level_info(basisFileData.data(), fileSize, levelInfo, 0, 0)) {
            return false; // Failed to get image level info
        }

        // Retrieve width and height
        *width = levelInfo.m_orig_width;
        *height = levelInfo.m_orig_height;

        // Transcode to RGBA32 format
        if (!transcoder.transcode_image_level(
            basisFileData.data(), fileSize, 0, 0, outputData, *width * *height * 4, basist::transcoder_texture_format::cTFRGBA32))
        {
            return false; // Transcoding failed
        }

        return true; // Success
    }
}
