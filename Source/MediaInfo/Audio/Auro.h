/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef MediaInfo_AuroH
#define MediaInfo_AuroH
//---------------------------------------------------------------------------

#include <string>
#include <cstddef>
#include <cstdint>

namespace MediaInfoLib
{

//---------------------------------------------------------------------------
/// @brief Convert Auro channel layout ID to string (e.g. 63 -> "5.1")
std::string Auro_ChannelLayoutToString(std::uint32_t layout_id);

//---------------------------------------------------------------------------
/// @brief Detect Auro-3D embedded in 24-bit little-endian PCM.
/// @param buffer Interleaved PCM, 3 bytes per sample, little-endian
/// @param size   Size in bytes (must be multiple of 3*channels)
/// @param channels Number of channels
/// @return Layout string if detected (e.g. "5.1"), empty string otherwise
std::string Auro_DetectInPcm24Le(const unsigned char* buffer, size_t size, int channels);

} //NameSpace

#endif
