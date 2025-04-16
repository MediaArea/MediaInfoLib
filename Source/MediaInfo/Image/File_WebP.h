// File_WebP.h - Header for WebP file parser (MediaInfoLib)
// Place this in Source/MediaInfo/Image/File_WebP.h

#include "MediaInfo/File__Analyze.h"

namespace MediaInfoLib {

class File_WebP : public File__Analyze
{
public:
    File_WebP();

private:
    // Overrides from File__Analyze
    bool FileHeader_Begin();               // Check initial RIFF header
    void FileHeader_Parse();               // Parse RIFF header and set up format
    void Header_Parse();                   // Parse chunk header (FourCC and size)
    void Data_Parse();                     // Parse chunk data based on chunk ID

    // Chunk-specific parsing functions
    void Parse_VP8();   // Parse VP8 (lossy) bitstream header for width/height
    void Parse_VP8L();  // Parse VP8L (lossless) bitstream header for width/height
    void Parse_VP8X();  // Parse extended header chunk (canvas size, flags)
    void Parse_ANIM();  // Parse animation header (loop count, etc.)
    // (We will handle ANMF frames by counting/skipping in Data_Parse directly)

    // Internal flags/properties
    bool HasAlpha;
    bool IsLossless;
    bool IsAnimated;
};

} // namespace MediaInfoLib
