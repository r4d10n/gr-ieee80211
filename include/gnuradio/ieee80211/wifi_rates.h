/* -*- c++ -*- */
/*
 * Copyright 2025 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#ifndef INCLUDED_IEEE80211_WIFI_RATES_H
#define INCLUDED_IEEE80211_WIFI_RATES_H

#include <gnuradio/ieee80211/api.h>
#include <string>

namespace gr {
namespace ieee80211 {

/**
 * @brief WiFi rate enumeration covering 802.11b/a/g/n/ac
 *
 * This enum provides a unified rate representation across all supported
 * WiFi standards, enabling seamless mode switching and rate adaptation.
 */
enum class wifi_rate {
    // ===== 802.11b DSSS/CCK Rates =====
    DSSS_1M_LONG,       ///< 1 Mbps DBPSK with long preamble (144 bits)
    DSSS_2M_LONG,       ///< 2 Mbps DQPSK with long preamble
    DSSS_5_5M_LONG,     ///< 5.5 Mbps CCK with long preamble
    DSSS_11M_LONG,      ///< 11 Mbps CCK with long preamble
    DSSS_2M_SHORT,      ///< 2 Mbps DQPSK with short preamble (72 bits)
    DSSS_5_5M_SHORT,    ///< 5.5 Mbps CCK with short preamble
    DSSS_11M_SHORT,     ///< 11 Mbps CCK with short preamble

    // ===== 802.11a/g OFDM Rates (20 MHz bandwidth) =====
    OFDM_6M,            ///< 6 Mbps BPSK 1/2
    OFDM_9M,            ///< 9 Mbps BPSK 3/4
    OFDM_12M,           ///< 12 Mbps QPSK 1/2
    OFDM_18M,           ///< 18 Mbps QPSK 3/4
    OFDM_24M,           ///< 24 Mbps 16-QAM 1/2
    OFDM_36M,           ///< 36 Mbps 16-QAM 3/4
    OFDM_48M,           ///< 48 Mbps 64-QAM 2/3
    OFDM_54M,           ///< 54 Mbps 64-QAM 3/4

    // ===== 802.11n HT Rates (extendable for MCS 0-31) =====
    HT_MCS0_20MHz,      ///< HT MCS 0: 6.5 Mbps (BPSK 1/2, 20 MHz)
    HT_MCS1_20MHz,      ///< HT MCS 1: 13 Mbps (QPSK 1/2, 20 MHz)
    HT_MCS2_20MHz,      ///< HT MCS 2: 19.5 Mbps (QPSK 3/4, 20 MHz)
    HT_MCS3_20MHz,      ///< HT MCS 3: 26 Mbps (16-QAM 1/2, 20 MHz)
    HT_MCS4_20MHz,      ///< HT MCS 4: 39 Mbps (16-QAM 3/4, 20 MHz)
    HT_MCS5_20MHz,      ///< HT MCS 5: 52 Mbps (64-QAM 2/3, 20 MHz)
    HT_MCS6_20MHz,      ///< HT MCS 6: 58.5 Mbps (64-QAM 3/4, 20 MHz)
    HT_MCS7_20MHz,      ///< HT MCS 7: 65 Mbps (64-QAM 5/6, 20 MHz)

    // ===== 802.11ac VHT Rates (extendable for MCS 0-9) =====
    VHT_MCS0_20MHz,     ///< VHT MCS 0: 6.5 Mbps (BPSK 1/2, 20 MHz)
    VHT_MCS1_20MHz,     ///< VHT MCS 1: 13 Mbps (QPSK 1/2, 20 MHz)
    VHT_MCS2_20MHz,     ///< VHT MCS 2: 19.5 Mbps (QPSK 3/4, 20 MHz)
    VHT_MCS3_20MHz,     ///< VHT MCS 3: 26 Mbps (16-QAM 1/2, 20 MHz)
    VHT_MCS4_20MHz,     ///< VHT MCS 4: 39 Mbps (16-QAM 3/4, 20 MHz)
    VHT_MCS5_20MHz,     ///< VHT MCS 5: 52 Mbps (64-QAM 2/3, 20 MHz)
    VHT_MCS6_20MHz,     ///< VHT MCS 6: 58.5 Mbps (64-QAM 3/4, 20 MHz)
    VHT_MCS7_20MHz,     ///< VHT MCS 7: 65 Mbps (64-QAM 5/6, 20 MHz)
    VHT_MCS8_20MHz,     ///< VHT MCS 8: 78 Mbps (256-QAM 3/4, 20 MHz)
    VHT_MCS9_20MHz,     ///< VHT MCS 9: 86.7 Mbps (256-QAM 5/6, 20 MHz)
};

/**
 * @brief Modulation type enumeration
 */
enum class modulation_type {
    DSSS,               ///< Direct Sequence Spread Spectrum (802.11b)
    CCK,                ///< Complementary Code Keying (802.11b)
    OFDM,               ///< Orthogonal Frequency Division Multiplexing
};

/**
 * @brief Coding type enumeration
 */
enum class coding_type {
    NONE,               ///< No coding (DSSS/CCK)
    BCC,                ///< Binary Convolutional Coding
    LDPC,               ///< Low-Density Parity Check (802.11n/ac)
};

/**
 * @brief Rate information structure
 *
 * Comprehensive metadata for each WiFi rate including modulation,
 * coding, bandwidth, and data rate.
 */
struct rate_info {
    wifi_rate rate;             ///< Rate enumeration value
    float mbps;                 ///< Data rate in Mbps
    modulation_type mod;        ///< Modulation scheme
    coding_type coding;         ///< Channel coding scheme
    int bandwidth_mhz;          ///< Channel bandwidth in MHz
    std::string description;    ///< Human-readable description
    bool long_preamble;         ///< True for long preamble (802.11b only)
};

/**
 * @brief Get rate information for a given WiFi rate
 *
 * @param r WiFi rate enumeration
 * @return const rate_info& Reference to rate information structure
 */
IEEE80211_API const rate_info& get_rate_info(wifi_rate r);

/**
 * @brief Get rate name as string
 *
 * @param r WiFi rate enumeration
 * @return std::string Human-readable rate name
 */
IEEE80211_API std::string get_rate_name(wifi_rate r);

/**
 * @brief Get data rate in Mbps
 *
 * @param r WiFi rate enumeration
 * @return float Data rate in megabits per second
 */
IEEE80211_API float get_rate_mbps(wifi_rate r);

/**
 * @brief Check if rate is DSSS/CCK (802.11b)
 *
 * @param r WiFi rate enumeration
 * @return bool True if rate is 802.11b DSSS or CCK
 */
IEEE80211_API bool is_dsss_rate(wifi_rate r);

/**
 * @brief Check if rate is OFDM-based (802.11a/g/n/ac)
 *
 * @param r WiFi rate enumeration
 * @return bool True if rate is OFDM-based
 */
IEEE80211_API bool is_ofdm_rate(wifi_rate r);

} // namespace ieee80211
} // namespace gr

#endif /* INCLUDED_IEEE80211_WIFI_RATES_H */
