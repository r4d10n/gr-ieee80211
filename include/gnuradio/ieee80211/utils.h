/* -*- c++ -*- */
/*
 * Copyright 2025 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#ifndef INCLUDED_IEEE80211_UTILS_H
#define INCLUDED_IEEE80211_UTILS_H

#include <gnuradio/ieee80211/api.h>
#include <cstdint>
#include <cstddef>

namespace gr {
namespace ieee80211 {
namespace utils {

/**
 * @brief Calculate IEEE 802.11 FCS (Frame Check Sequence)
 *
 * Computes the CRC-32 checksum for 802.11 MAC frames according to
 * the standard polynomial: G(x) = x^32 + x^26 + x^23 + x^22 + x^16 +
 * x^12 + x^11 + x^10 + x^8 + x^7 + x^5 + x^4 + x^2 + x + 1
 *
 * This is the same as the Ethernet FCS (ITU-T V.42 polynomial).
 *
 * @param data Pointer to data buffer
 * @param len Length of data in bytes
 * @return uint32_t Calculated CRC-32 checksum (FCS)
 *
 * @note The FCS is calculated over the MAC header and frame body,
 *       but NOT including the FCS field itself.
 *
 * Example usage:
 * @code
 * uint8_t mac_frame[100];
 * size_t frame_len = 96; // Not including 4-byte FCS
 * uint32_t fcs = calc_fcs(mac_frame, frame_len);
 * memcpy(&mac_frame[frame_len], &fcs, 4); // Append FCS
 * @endcode
 */
IEEE80211_API uint32_t calc_fcs(const uint8_t* data, size_t len);

/**
 * @brief Validate IEEE 802.11 FCS
 *
 * Verifies the FCS of a received MAC frame by recalculating the
 * CRC-32 and comparing with the received FCS.
 *
 * @param data Pointer to complete frame (including FCS)
 * @param len Total length including 4-byte FCS
 * @return bool True if FCS is valid, false otherwise
 *
 * Example usage:
 * @code
 * uint8_t rx_frame[100];
 * size_t total_len = 100; // Including 4-byte FCS
 * if (validate_fcs(rx_frame, total_len)) {
 *     // Frame is valid
 * }
 * @endcode
 */
IEEE80211_API bool validate_fcs(const uint8_t* data, size_t len);

/**
 * @brief Extract FCS from frame
 *
 * Extracts the 4-byte FCS from the end of a MAC frame.
 *
 * @param data Pointer to complete frame (including FCS)
 * @param len Total length including 4-byte FCS
 * @return uint32_t FCS value in host byte order
 */
IEEE80211_API uint32_t extract_fcs(const uint8_t* data, size_t len);

/**
 * @brief Calculate CRC-16 for 802.11b PLCP header
 *
 * Computes the CRC-16 for the PLCP header in 802.11b frames.
 * Polynomial: x^16 + x^12 + x^5 + 1 (CCITT CRC-16)
 *
 * @param header Pointer to 4-byte PLCP header (SIGNAL, SERVICE, LENGTH)
 * @return uint16_t Calculated CRC-16
 *
 * @note This is used internally by the DSSS PLCP implementation
 *       and is different from the MAC frame FCS.
 */
IEEE80211_API uint16_t calc_plcp_crc16(const uint8_t* header);

/**
 * @brief Validate PLCP header CRC-16
 *
 * @param header Pointer to 6-byte PLCP header (SIGNAL, SERVICE, LENGTH, CRC)
 * @return bool True if CRC-16 is valid
 */
IEEE80211_API bool validate_plcp_crc16(const uint8_t* header);

/**
 * @brief Calculate legacy CRC-32 table entry
 *
 * Helper function to generate CRC-32 lookup table entries.
 * Can be used for runtime table generation if needed.
 *
 * @param crc Current CRC value
 * @param byte Input byte
 * @return uint32_t Updated CRC value
 */
IEEE80211_API uint32_t crc32_byte(uint32_t crc, uint8_t byte);

/**
 * @brief IEEE 802.11 scrambler (7-bit LFSR)
 *
 * Implements the scrambler defined in IEEE 802.11b for DSSS/CCK.
 * Polynomial: S(x) = x^7 + x^4 + 1
 *
 * @param data Pointer to data buffer (modified in-place)
 * @param len Length of data in bytes
 * @param init Initial scrambler state (0x1B for long, 0x6C for short preamble)
 *
 * @note The same function is used for both scrambling and descrambling
 *       since the scrambler is its own inverse.
 */
IEEE80211_API void scramble(uint8_t* data, size_t len, uint8_t init = 0x1B);

/**
 * @brief Descrambler (same as scrambler for 802.11)
 *
 * Convenience wrapper for scramble() to make descrambling explicit.
 *
 * @param data Pointer to data buffer (modified in-place)
 * @param len Length of data in bytes
 * @param init Initial scrambler state
 */
inline void descramble(uint8_t* data, size_t len, uint8_t init = 0x1B) {
    scramble(data, len, init);
}

/**
 * @brief Convert dBm to linear power
 *
 * @param dbm Power in dBm
 * @return float Power in linear scale (milliwatts)
 */
IEEE80211_API float dbm_to_linear(float dbm);

/**
 * @brief Convert linear power to dBm
 *
 * @param linear Power in linear scale (milliwatts)
 * @return float Power in dBm
 */
IEEE80211_API float linear_to_dbm(float linear);

} // namespace utils
} // namespace ieee80211
} // namespace gr

#endif /* INCLUDED_IEEE80211_UTILS_H */
