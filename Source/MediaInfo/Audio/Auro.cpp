/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
// Auro-3D detection in 24-bit PCM (LSB embedding). Code from Auro/AuroDetectConsole.
//---------------------------------------------------------------------------

#include "MediaInfo/Setup.h"

#include "MediaInfo/Audio/Auro.h"
#include <algorithm>
#include <map>
#include <vector>
#include <cstdint>
#include <cstring>

namespace MediaInfoLib
{

//---------------------------------------------------------------------------
// Internal types (from Auro/AuroDetectConsole/main.cpp)
//---------------------------------------------------------------------------
struct A3DSyncInfo {
    int block_size;
    int m;
};

struct AdolInstruction {
    int opcode;
    bool has_u1;
    uint32_t u1;
    bool has_u2;
    uint32_t u2;
};

struct MuxIteratorFalse {
    const std::vector<int32_t>* samples;
    int base;
    int m;
    int pos;
    int param;
};

struct MuxIteratorTrue {
    const std::vector<int32_t>* samples;
    int base;
    int m;
    int pos;
};

static uint32_t u32(int32_t x) { return static_cast<uint32_t>(x); }
static uint16_t u16(uint32_t x) { return static_cast<uint16_t>(x & 0xFFFFu); }
static uint8_t byte_n(uint32_t x, int n) { return static_cast<uint8_t>((x >> (8 * n)) & 0xFFu); }
static uint16_t lo8_u16(uint16_t x) { return static_cast<uint16_t>(x & 0x00FFu); }

static uint16_t crc16_ccitt_update(uint16_t crc, uint8_t b) {
    uint16_t c = static_cast<uint16_t>(crc ^ (static_cast<uint16_t>(b) << 8));
    for (int i = 0; i < 8; ++i) {
        if (c & 0x8000u)
            c = static_cast<uint16_t>((c << 1) ^ 0x1021u);
        else
            c = static_cast<uint16_t>(c << 1);
    }
    return c;
}

static uint16_t compute_crc16_ccitt_over_pcm24_at(const std::vector<int32_t>& samples, int start, int length) {
    uint16_t crc = 0;
    const int first = std::min(length, 16);
    for (int i = 0; i < first; ++i) {
        uint32_t w = u32(samples[start + i]);
        crc = crc16_ccitt_update(crc, static_cast<uint8_t>(byte_n(w, 0) & 0xFDu));
        crc = crc16_ccitt_update(crc, byte_n(w, 1));
        crc = crc16_ccitt_update(crc, byte_n(w, 2));
    }
    for (int i = first; i < length; ++i) {
        uint32_t w = u32(samples[start + i]);
        crc = crc16_ccitt_update(crc, byte_n(w, 0));
        crc = crc16_ccitt_update(crc, byte_n(w, 1));
        crc = crc16_ccitt_update(crc, byte_n(w, 2));
    }
    return static_cast<uint16_t>(~crc);
}

static int mux_bit_from_pos_false(const MuxIteratorFalse& it) {
    const int bit_pos = it.pos;
    const int m = it.m;
    const int p = it.param;
    int idx = 0;
    uint32_t mask = 0;
    if (bit_pos > 0x2F) {
        if (bit_pos >= 16 * m) {
            int adjusted = (bit_pos - m) / p + bit_pos;
            idx = adjusted / m;
            int rem = adjusted % m;
            mask = 1u << (m - 1 - rem);
        } else {
            idx = (bit_pos - 48) / (m - 3);
            int rem = (bit_pos - 48) % (m - 3);
            mask = 1u << (m - 1 - rem);
        }
    } else {
        if (bit_pos >= 0x10) {
            if (bit_pos <= 0x1F) { idx = bit_pos - 16; mask = 2; }
            else { idx = bit_pos - 32; mask = 4; }
        } else { idx = bit_pos; mask = 1; }
    }
    return (u32((*it.samples)[it.base + idx]) & mask) ? 1 : 0;
}

static int mux_bit_from_pos_true(const MuxIteratorTrue& it) {
    const int bit_pos = it.pos;
    const int m = it.m;
    int idx = 0;
    uint32_t mask = 0;
    if (bit_pos > 0x2F) {
        if (bit_pos >= 16 * m) {
            int rem = bit_pos % m;
            idx = bit_pos / m;
            mask = 1u << (m - 1 - rem);
        } else {
            int pos_mid = bit_pos - 48;
            idx = pos_mid / (m - 3);
            int rem2 = pos_mid - idx * (m - 3);
            mask = 1u << (m - 1 - rem2);
        }
    } else {
        if (bit_pos >= 0x10) {
            if (bit_pos <= 0x1F) { idx = bit_pos - 16; mask = 2; }
            else { idx = bit_pos - 32; mask = 4; }
        } else { idx = bit_pos; mask = 1; }
    }
    return (u32((*it.samples)[it.base + idx]) & mask) ? 1 : 0;
}

static bool read_unsigned_u8_false(int nbits, MuxIteratorFalse& it, int& bits_left, uint8_t& out) {
    if (bits_left < nbits) return false;
    bits_left -= nbits;
    uint32_t v = 0;
    for (int i = 0; i < nbits; ++i) {
        v = (v << 1) | static_cast<uint32_t>(mux_bit_from_pos_false(it));
        it.pos += 1;
    }
    out = static_cast<uint8_t>(v & 0xFFu);
    return true;
}

static bool read_unsigned_u32_false(int nbits, MuxIteratorFalse& it, int& bits_left, uint32_t& out) {
    if (bits_left < nbits) return false;
    bits_left -= nbits;
    uint32_t v = 0;
    for (int i = 0; i < nbits; ++i) {
        v = (v << 1) | static_cast<uint32_t>(mux_bit_from_pos_false(it));
        it.pos += 1;
    }
    out = v;
    return true;
}

static bool read_unsigned_u64_true(int nbits, MuxIteratorTrue& it, int& bits_left, uint64_t& out) {
    if (bits_left < nbits) return false;
    bits_left -= nbits;
    uint64_t v = 0;
    for (int i = 0; i < nbits; ++i) {
        v = (v << 1) | static_cast<uint64_t>(mux_bit_from_pos_true(it));
        it.pos += 1;
    }
    out = v;
    return true;
}

static bool read_bool_false(MuxIteratorFalse& it, int& bits_left, bool& out) {
    if (bits_left <= 0) return false;
    bits_left -= 1;
    out = mux_bit_from_pos_false(it) != 0;
    it.pos += 1;
    return true;
}

static bool parse_sync_pcm24_at(const std::vector<int32_t>& samples, int start, int avail_len, bool strict_len, A3DSyncInfo& out) {
    if (avail_len < 16) return false;
    for (int i = 0; i < 16; ++i) {
        if ((u32(samples[start + i]) & 1u) == 0) return false;
    }
    uint32_t s0 = u32(samples[start + 0]), s1 = u32(samples[start + 1]), s2 = u32(samples[start + 2]), s3 = u32(samples[start + 3]);
    uint32_t s4 = u32(samples[start + 4]), s5 = u32(samples[start + 5]), s6 = u32(samples[start + 6]), s7 = u32(samples[start + 7]);
    uint32_t a = (s0 & 4u) | ((s1 >> 1) & 3u);
    uint32_t b = ((4u * a) & 0xF8u) | (s2 & 4u) | ((s3 >> 1) & 3u);
    uint32_t c = ((4u * b) & 0x78u) | (s4 & 4u) | ((s5 >> 1) & 2u) | ((s6 & 4u) ? 1u : 0u);
    uint32_t block_raw = ((4u * s7) & 0x10u) | (32u * c);
    int block_size = (block_raw == 0x3D0u) ? 1000 : static_cast<int>(block_raw + 16u);
    MuxIteratorTrue it_true = {&samples, start, 3, 44};
    int bits_left = 8;
    uint64_t encoded_m = 0;
    if (!read_unsigned_u64_true(4, it_true, bits_left, encoded_m) || (encoded_m + 13u) >= 0x19u) return false;
    int m = 14 - static_cast<int>(encoded_m);
    if (m < 3) return false;
    if (strict_len) {
        if (avail_len != block_size) return false;
    } else {
        if (avail_len < block_size) return false;
    }
    const int reserved_stride = 16 * m;
    const int eff_m = (m <= 3) ? 3 : m;
    const int total_bits = m * block_size;
    int reserved_pos = 17 * m - 1;
    if (reserved_pos < total_bits) {
        const int mid_divisor = eff_m - 3;
        while (reserved_pos < total_bits) {
            int idx = 0;
            uint32_t mask = 0;
            if (reserved_pos > 0x2F) {
                if (reserved_pos >= 16 * eff_m) {
                    idx = reserved_pos / eff_m;
                    int rem = reserved_pos % eff_m;
                    mask = 1u << (eff_m - 1 - rem);
                } else {
                    idx = (reserved_pos - 48) / mid_divisor;
                    int rem = (reserved_pos - 48) % mid_divisor;
                    mask = 1u << (eff_m - 1 - rem);
                }
            } else {
                idx = reserved_pos - 16;
                if (reserved_pos >= 0x10) {
                    if (reserved_pos <= 0x1F) mask = 2;
                    else { idx = reserved_pos - 32; mask = 4; }
                } else { idx = reserved_pos; mask = 1; }
            }
            if ((u32(samples[start + idx]) & mask) != 0) return false;
            reserved_pos += reserved_stride;
        }
    }
    out.block_size = block_size;
    out.m = m;
    return true;
}

static std::vector<uint16_t> split_samples_to_words16_le(const std::vector<int32_t>& samples16) {
    std::vector<uint16_t> out;
    out.reserve(samples16.size() * 2);
    for (size_t i = 0; i < samples16.size(); ++i) {
        uint32_t w = u32(samples16[i]);
        out.push_back(u16(w));
        out.push_back(u16(w >> 16));
    }
    return out;
}

static uint16_t extract_stored_crc16_from_header_first16_at(const std::vector<int32_t>& samples, int start) {
    std::vector<int32_t> first16(samples.begin() + start, samples.begin() + start + 16);
    std::vector<uint16_t> header_words16 = split_samples_to_words16_le(first16);
    uint16_t word4 = header_words16[4], word8 = header_words16[8];
    uint16_t word6_low8 = lo8_u16(header_words16[6]);
    uint16_t header_nibble_seed = static_cast<uint16_t>((((2u * header_words16[0]) & 0xFFFCu) | (header_words16[2] & 3u)) >> 1);
    auto extract_bit2_from_low8 = [&header_words16](size_t idx) -> uint16_t {
        return static_cast<uint16_t>((2u * lo8_u16(header_words16[idx])) & 4u);
    };
    uint16_t step_a = static_cast<uint16_t>((((2u * word4) & 4u) | (8u * (header_nibble_seed & 3u)) | (word6_low8 & 3u)) >> 1);
    step_a &= 0xFu;
    uint16_t step_b = static_cast<uint16_t>((((2u * word8) & 4u) | (8u * step_a) | (header_words16[10] & 3u)) >> 1);
    uint16_t step_c = static_cast<uint16_t>((extract_bit2_from_low8(12) | (8u * step_b) | (header_words16[14] & 3u)) >> 1);
    uint16_t step_d = static_cast<uint16_t>((extract_bit2_from_low8(16) | (8u * step_c) | (header_words16[18] & 3u)) >> 1);
    uint16_t step_e = static_cast<uint16_t>((extract_bit2_from_low8(20) | (8u * step_d) | (header_words16[22] & 3u)) >> 1);
    uint16_t step_f = static_cast<uint16_t>(extract_bit2_from_low8(24) | (8u * step_e) | (header_words16[26] & 3u));
    uint16_t step_f_shifted = static_cast<uint16_t>(2u * step_f);
    return static_cast<uint16_t>((step_f_shifted & 0xFFFCu) | (header_words16[28] & 2u) | ((header_words16[30] >> 1) & 1u));
}

static bool adol_parse(MuxIteratorFalse& it, int& bits_left, std::vector<AdolInstruction>& out) {
    while (true) {
        uint8_t opcode = 0;
        if (!read_unsigned_u8_false(8, it, bits_left, opcode)) return false;
        AdolInstruction ins;
        ins.opcode = static_cast<int>(opcode);
        ins.has_u1 = false;
        ins.u1 = 0;
        ins.has_u2 = false;
        ins.u2 = 0;
        uint32_t operand = 0;
        switch (ins.opcode) {
            case 1: case 3: case 0x5A: case 0x5B: case 0x5C: case 0x5D: case 0x5E:
            case 0x5F: case 0x60: case 0x61: case 0x62:
                if (!read_unsigned_u32_false(16, it, bits_left, operand)) return false;
                ins.has_u1 = true; ins.u1 = operand; break;
            case 2: case 4: case 0x1E: case 0x1F: case 0x41: case 0x47: case 0x50:
            case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57: case 0x58:
                if (!read_unsigned_u32_false(8, it, bits_left, operand)) return false;
                ins.has_u1 = true; ins.u1 = operand; break;
            case 0x0E: case 0x46: case 0x6E: case 0x6F: case 0x70: case 0x71: case 0x72:
            case 0x73: case 0x74: case 0x75: case 0x76: case 0x80: case 0x81: case 0x82:
            case 0x83: case 0x84: case 0x85: case 0x8C: case 0x8D: case 0x8E: case 0x8F: case 0x90:
                if (!read_unsigned_u32_false(32, it, bits_left, operand)) return false;
                ins.has_u1 = true; ins.u1 = operand; break;
            case 0x40: {
                uint32_t operand2 = 0;
                if (!read_unsigned_u32_false(8, it, bits_left, operand)) return false;
                if (!read_unsigned_u32_false(8, it, bits_left, operand2)) return false;
                ins.has_u1 = true; ins.u1 = operand;
                ins.has_u2 = true; ins.u2 = operand2;
                break;
            }
            case 0x64: case 0x65: case 0x66: case 0x67: case 0x68: case 0x69: case 0x6A: case 0x6B: case 0x6C:
                if (!read_unsigned_u32_false(24, it, bits_left, operand)) return false;
                ins.has_u1 = true; ins.u1 = operand; break;
            case 0:
                out.push_back(ins);
                return true;
            default:
                return false;
        }
        out.push_back(ins);
    }
}

static bool auro_adol_channel_input_config_get_original_layout(uint32_t cfg_id, uint32_t& out_layout) {
    static const std::map<uint32_t, uint32_t> table = {
        {1, 55}, {2, 63}, {8, 71}, {11, 1587}, {12, 51}, {15, 1599}, {20, 26163}, {30, 26175},
        {40, 30271}, {50, 32319}, {54, 26559}, {62, 32703}, {64, 3}, {66, 7}, {67, 119},
        {68, 127}, {69, 439}, {70, 447}, {71, 26167}, {72, 30263}, {73, 32311}, {74, 26551},
        {75, 1983}, {76, 30647}, {77, 30655}, {78, 32695}, {128, 4}, {129, 2052}, {130, 6148},
    };
    std::map<uint32_t, uint32_t>::const_iterator it = table.find(cfg_id);
    if (it == table.end()) return false;
    out_layout = it->second;
    return true;
}

static bool auro_adol_channel_input_config_get_carrier_layout(uint32_t cfg_id, uint32_t& out_layout) {
    switch (cfg_id) {
        case 1: case 8: case 11: case 12: case 66: out_layout = 3; return true;
        case 2: out_layout = 11; return true;
        case 15: case 30: case 40: case 50: case 68: case 70: out_layout = 63; return true;
        case 20: out_layout = 51; return true;
        case 54: case 62: case 75: case 77: out_layout = 447; return true;
        case 64: case 128: case 129: case 130: out_layout = 4; return true;
        case 67: case 69: case 71: case 72: case 73: out_layout = 55; return true;
        case 74: case 76: case 78: out_layout = 439; return true;
        default: return false;
    }
}

static bool block_has_valid_sync_and_crc(const std::vector<int32_t>& samples) {
    A3DSyncInfo sync;
    if (!parse_sync_pcm24_at(samples, 0, static_cast<int>(samples.size()), true, sync))
        return false;
    uint16_t crc = compute_crc16_ccitt_over_pcm24_at(samples, 0, static_cast<int>(samples.size()));
    uint16_t stored_crc16 = extract_stored_crc16_from_header_first16_at(samples, 0);
    return crc == stored_crc16;
}

static bool extract_layout_and_cfg_from_a3d_block_at(const std::vector<int32_t>& samples, int start, int block_size, uint32_t& out_layout_id, uint32_t& out_cfg_id) {
    A3DSyncInfo sync;
    if (!parse_sync_pcm24_at(samples, start, block_size, true, sync))
        return false;
    uint16_t crc = compute_crc16_ccitt_over_pcm24_at(samples, start, block_size);
    uint16_t stored_crc16 = extract_stored_crc16_from_header_first16_at(samples, start);
    if (crc != stored_crc16) return false;
    int m = sync.m;
    int bits_left = (block_size * m - ((block_size - 1) / 16)) - 32;
    if (bits_left <= 0) return false;
    MuxIteratorFalse it = {&samples, start, m, 32, 16 * m - 1};
    uint8_t u8 = 0;
    if (!read_unsigned_u8_false(8, it, bits_left, u8)) return false;
    for (int i = 0; i < 4; ++i) { bool b = false; if (!read_bool_false(it, bits_left, b)) return false; }
    if (!read_unsigned_u8_false(4, it, bits_left, u8)) return false;
    for (int i = 0; i < 2; ++i) { if (!read_unsigned_u8_false(8, it, bits_left, u8)) return false; }
    for (int i = 0; i < 2; ++i) { bool b = false; if (!read_bool_false(it, bits_left, b)) return false; }
    if (!read_unsigned_u8_false(2, it, bits_left, u8)) return false;
    if (!read_unsigned_u8_false(4, it, bits_left, u8)) return false;
    if (!read_unsigned_u8_false(8, it, bits_left, u8)) return false;
    uint8_t adol_blocks = 0;
    if (!read_unsigned_u8_false(8, it, bits_left, adol_blocks)) return false;
    if (!read_unsigned_u8_false(8, it, bits_left, u8)) return false;
    uint32_t value = 0;
    int count = 0;
    for (int i = 0; i < 4; ++i) {
        if (!read_unsigned_u32_false(8, it, bits_left, value)) return false;
        if (value != 255u) count += 1;
    }
    for (int i = 0; i < 4; ++i) { if (!read_unsigned_u8_false(8, it, bits_left, u8)) return false; }
    int vec_sz = -1;
    if (count == 0 || count == 1) vec_sz = 0;
    else if (count == 2) vec_sz = 2;
    else if (count == 3) vec_sz = 5;
    if (vec_sz < 0) return false;
    for (int i = 0; i < vec_sz; ++i) { if (!read_unsigned_u32_false(32, it, bits_left, value)) return false; }
    std::vector<AdolInstruction> instructions;
    for (int i = 0; i < static_cast<int>(adol_blocks); ++i) {
        uint32_t tag = 0;
        if (!read_unsigned_u32_false(8, it, bits_left, tag) || tag != 1u) return false;
        if (!adol_parse(it, bits_left, instructions)) return false;
    }
    for (size_t i = 0; i < instructions.size(); ++i) {
        const AdolInstruction& ins = instructions[i];
        if (ins.opcode == 30 && ins.has_u1) {
            out_cfg_id = ins.u1;
            if (auro_adol_channel_input_config_get_original_layout(ins.u1, out_layout_id))
                return true;
        }
    }
    return false;
}

static bool parse_sync_header_first16(const std::vector<int32_t>& samples16, A3DSyncInfo& out) {
    if (samples16.size() < 16) return false;
    for (int i = 0; i < 16; ++i) {
        if ((samples16[i] & 1) == 0) return false;
    }
    uint32_t s0 = u32(samples16[0]), s1 = u32(samples16[1]), s2 = u32(samples16[2]), s3 = u32(samples16[3]);
    uint32_t s4 = u32(samples16[4]), s5 = u32(samples16[5]), s6 = u32(samples16[6]), s7 = u32(samples16[7]);
    uint32_t a = (s0 & 4u) | ((s1 >> 1) & 3u);
    uint32_t b = ((4u * a) & 0xF8u) | (s2 & 4u) | ((s3 >> 1) & 3u);
    uint32_t c = ((4u * b) & 0x78u) | (s4 & 4u) | ((s5 >> 1) & 2u) | ((s6 & 4u) ? 1u : 0u);
    uint32_t block_raw = ((4u * s7) & 0x10u) | (32u * c);
    int block_size = (block_raw == 0x3D0u) ? 1000 : static_cast<int>(block_raw + 16u);
    MuxIteratorTrue it_true = {&samples16, 0, 3, 44};
    int bits_left = 8;
    uint64_t encoded_m = 0;
    if (!read_unsigned_u64_true(4, it_true, bits_left, encoded_m) || (encoded_m + 13u) >= 0x19u) return false;
    int m = 14 - static_cast<int>(encoded_m);
    if (m < 3) return false;
    out.block_size = block_size;
    out.m = m;
    return true;
}

//---------------------------------------------------------------------------
// Public API
//---------------------------------------------------------------------------
std::string Auro_ChannelLayoutToString(std::uint32_t layout_id) {
    static const std::map<uint32_t, std::string> table = {
        {3, "2.0"}, {4, "1.0"}, {7, "3.0"}, {8, "0.1"}, {11, "2.1"}, {12, "1.1"},
        {15, "3.1"}, {51, "4.0"}, {55, "5.0"}, {59, "4.1"}, {63, "5.1"}, {71, "LCRS"},
        {119, "6.0"}, {127, "6.1"}, {435, "7.0_no_C"}, {439, "7.0"}, {443, "7.1_no_C"},
        {447, "7.1"}, {1539, "2.0_2H"}, {1543, "3.0_2H"}, {1547, "2.1_2H"}, {1551, "3.1_2H"},
        {1587, "4.0_2H"}, {1591, "5.0_2H"}, {1595, "4.1_2H"}, {1599, "5.1_2H"},
        {1971, "7.0_2H_no_C"}, {1975, "7.0_2H"}, {1979, "7.1_2H_no_C"}, {1983, "7.1_2H"},
        {3591, "3.0_3H"}, {3599, "3.1_3H"}, {26163, "4.0_4H"}, {26167, "5.0_4H"},
        {26171, "4.1_4H"}, {26175, "5.1_4H"}, {26547, "7.0_4H_no_C"}, {26551, "7.0_4H"},
        {26555, "7.1_4H_no_C"}, {26559, "7.1_4H"}, {28211, "4.0_5H"}, {28215, "5.0_5H"},
        {28219, "4.1_5H"}, {28223, "5.1_5H"}, {28595, "7.0_5H_no_C"}, {28599, "7.0_5H"},
        {28603, "7.1_5H_no_C"}, {28607, "7.1_5H"}, {30259, "4.0_4H_1T"}, {30263, "5.0_4H_1T"},
        {30267, "4.1_4H_1T"}, {30271, "5.1_4H_1T"}, {30643, "7.0_4H_1T_no_C"}, {30647, "7.0_4H_1T"},
        {30651, "7.1_4H_1T_no_C"}, {30655, "7.1_4H_1T"}, {32307, "4.0_5H_1T"}, {32311, "5.0_5H_1T"},
        {32315, "4.1_5H_1T"}, {32319, "5.1_5H_1T"}, {32691, "7.0_5H_1T_no_C"}, {32695, "7.0_5H_1T"},
        {32699, "7.1_5H_1T_no_C"}, {32703, "7.1_5H_1T"}, {805332543, "5.1_4H_2T"},
        {805332927, "7.1_4H_2T"}, {805334591, "5.1_5H_2T"}, {805334975, "7.1_5H_2T"},
        {1006659519, "9.1_4H_2T"}, {1006661567, "9.1_5H_2T"}, {0xFFFFFFu, "NHK_22.2"},
        {15728631, "NHK_22.0"}, {56649216, "Cube"}, {2052, "TestMix2.0"}, {6148, "TestMix3.0"},
    };
    std::map<uint32_t, std::string>::const_iterator it = table.find(layout_id);
    if (it != table.end()) return it->second;
    return std::string();
}

std::string Auro_DetectInPcm24Le(const unsigned char* buffer, size_t size, int channels) {
    if (!buffer || channels <= 0) return std::string();
    const size_t frame_bytes = 3 * (size_t)channels;
    if (frame_bytes == 0) return std::string();
    size_t num_frames = size / frame_bytes;
    if (num_frames < 17) return std::string();

    const int max_block = 4096;
    const size_t ring_size = (size_t)(max_block + 64);
    std::vector<std::vector<int32_t> > rings((size_t)channels);
    for (int c = 0; c < channels; ++c)
        rings[c].resize(ring_size, 0);
    std::vector<int> run_ones((size_t)channels, 0);
    std::vector<int64_t> sample_idx((size_t)channels, -1);
    std::vector<std::vector<std::pair<int64_t, int> > > pending((size_t)channels);

    for (size_t fr = 0; fr < num_frames; ++fr) {
        const unsigned char* frame = buffer + fr * frame_bytes;
        for (int ci = 0; ci < channels; ++ci) {
            const unsigned char* p = frame + 3 * (size_t)ci;
            int32_t v = (int32_t)(p[0] | (p[1] << 8) | (p[2] << 16));
            if (v & 0x800000) v -= (1 << 24);

            sample_idx[ci]++;
            size_t ring_idx = (size_t)(sample_idx[ci] % (int64_t)ring_size);
            rings[ci][ring_idx] = v;

            if ((v & 1) != 0) {
                run_ones[ci]++;
            } else {
                run_ones[ci] = 0;
                continue;
            }

            if (run_ones[ci] >= 16) {
                int64_t start = sample_idx[ci] - 15;
                if (start >= 0 && start + 15 <= (int64_t)num_frames - 1) {
                    std::vector<int32_t> first16;
                    first16.reserve(16);
                    for (int k = 0; k < 16; ++k) {
                        int64_t idx = start + k;
                        first16.push_back(rings[ci][(size_t)(idx % (int64_t)ring_size)]);
                    }
                    A3DSyncInfo sh;
                    if (parse_sync_header_first16(first16, sh))
                        pending[ci].push_back(std::make_pair(start, sh.block_size));
                }
            }

            if (!pending[ci].empty()) {
                int64_t cur = sample_idx[ci];
                std::vector<std::pair<int64_t, int> > new_pending;
                for (size_t pi = 0; pi < pending[ci].size(); ++pi) {
                    int64_t st = pending[ci][pi].first;
                    int bs = pending[ci][pi].second;
                    int64_t end = st + bs - 1;
                    if (end > cur) {
                        new_pending.push_back(std::make_pair(st, bs));
                        continue;
                    }
                    if (st < 0 || end >= (int64_t)num_frames) continue;
                    std::vector<int32_t> block;
                    block.reserve((size_t)bs);
                    for (int k = 0; k < bs; ++k) {
                        int64_t idx = st + k;
                        block.push_back(rings[ci][(size_t)(idx % (int64_t)ring_size)]);
                    }
                    if (block_has_valid_sync_and_crc(block)) { /* sync_found */ }
                    uint32_t layout_id = 0;
                    uint32_t cfg_id = 0;
                    if (extract_layout_and_cfg_from_a3d_block_at(block, 0, bs, layout_id, cfg_id)) {
                        uint32_t carrier_layout_unused = 0;
                        bool has_carrier = auro_adol_channel_input_config_get_carrier_layout(cfg_id, carrier_layout_unused);
                        (void)carrier_layout_unused;
                        if (has_carrier) {
                            std::string layout_str = Auro_ChannelLayoutToString(layout_id);
                            if (!layout_str.empty())
                                return layout_str;
                        }
                    }
                }
                pending[ci] = new_pending;
            }
        }
    }
    return std::string();
}

} // namespace MediaInfoLib
