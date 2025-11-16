/* -*- c++ -*- */
/*
 * Copyright 2025 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/ieee80211/wifi_rates.h>
#include <stdexcept>
#include <map>

namespace gr {
namespace ieee80211 {

// Static rate information table
static const std::map<wifi_rate, rate_info> rate_table = {
    // 802.11b DSSS/CCK rates
    {wifi_rate::DSSS_1M_LONG, {wifi_rate::DSSS_1M_LONG, 1.0f, modulation_type::DSSS, coding_type::NONE, 22, "1 Mbps DBPSK (long preamble)", true}},
    {wifi_rate::DSSS_2M_LONG, {wifi_rate::DSSS_2M_LONG, 2.0f, modulation_type::DSSS, coding_type::NONE, 22, "2 Mbps DQPSK (long preamble)", true}},
    {wifi_rate::DSSS_5_5M_LONG, {wifi_rate::DSSS_5_5M_LONG, 5.5f, modulation_type::CCK, coding_type::NONE, 22, "5.5 Mbps CCK (long preamble)", true}},
    {wifi_rate::DSSS_11M_LONG, {wifi_rate::DSSS_11M_LONG, 11.0f, modulation_type::CCK, coding_type::NONE, 22, "11 Mbps CCK (long preamble)", true}},
    {wifi_rate::DSSS_2M_SHORT, {wifi_rate::DSSS_2M_SHORT, 2.0f, modulation_type::DSSS, coding_type::NONE, 22, "2 Mbps DQPSK (short preamble)", false}},
    {wifi_rate::DSSS_5_5M_SHORT, {wifi_rate::DSSS_5_5M_SHORT, 5.5f, modulation_type::CCK, coding_type::NONE, 22, "5.5 Mbps CCK (short preamble)", false}},
    {wifi_rate::DSSS_11M_SHORT, {wifi_rate::DSSS_11M_SHORT, 11.0f, modulation_type::CCK, coding_type::NONE, 22, "11 Mbps CCK (short preamble)", false}},

    // 802.11a/g OFDM rates
    {wifi_rate::OFDM_6M, {wifi_rate::OFDM_6M, 6.0f, modulation_type::OFDM, coding_type::BCC, 20, "6 Mbps OFDM (BPSK 1/2)", false}},
    {wifi_rate::OFDM_9M, {wifi_rate::OFDM_9M, 9.0f, modulation_type::OFDM, coding_type::BCC, 20, "9 Mbps OFDM (BPSK 3/4)", false}},
    {wifi_rate::OFDM_12M, {wifi_rate::OFDM_12M, 12.0f, modulation_type::OFDM, coding_type::BCC, 20, "12 Mbps OFDM (QPSK 1/2)", false}},
    {wifi_rate::OFDM_18M, {wifi_rate::OFDM_18M, 18.0f, modulation_type::OFDM, coding_type::BCC, 20, "18 Mbps OFDM (QPSK 3/4)", false}},
    {wifi_rate::OFDM_24M, {wifi_rate::OFDM_24M, 24.0f, modulation_type::OFDM, coding_type::BCC, 20, "24 Mbps OFDM (16-QAM 1/2)", false}},
    {wifi_rate::OFDM_36M, {wifi_rate::OFDM_36M, 36.0f, modulation_type::OFDM, coding_type::BCC, 20, "36 Mbps OFDM (16-QAM 3/4)", false}},
    {wifi_rate::OFDM_48M, {wifi_rate::OFDM_48M, 48.0f, modulation_type::OFDM, coding_type::BCC, 20, "48 Mbps OFDM (64-QAM 2/3)", false}},
    {wifi_rate::OFDM_54M, {wifi_rate::OFDM_54M, 54.0f, modulation_type::OFDM, coding_type::BCC, 20, "54 Mbps OFDM (64-QAM 3/4)", false}},

    // 802.11n HT rates (20 MHz)
    {wifi_rate::HT_MCS0_20MHz, {wifi_rate::HT_MCS0_20MHz, 6.5f, modulation_type::OFDM, coding_type::BCC, 20, "HT MCS0 6.5 Mbps (BPSK 1/2)", false}},
    {wifi_rate::HT_MCS1_20MHz, {wifi_rate::HT_MCS1_20MHz, 13.0f, modulation_type::OFDM, coding_type::BCC, 20, "HT MCS1 13 Mbps (QPSK 1/2)", false}},
    {wifi_rate::HT_MCS2_20MHz, {wifi_rate::HT_MCS2_20MHz, 19.5f, modulation_type::OFDM, coding_type::BCC, 20, "HT MCS2 19.5 Mbps (QPSK 3/4)", false}},
    {wifi_rate::HT_MCS3_20MHz, {wifi_rate::HT_MCS3_20MHz, 26.0f, modulation_type::OFDM, coding_type::BCC, 20, "HT MCS3 26 Mbps (16-QAM 1/2)", false}},
    {wifi_rate::HT_MCS4_20MHz, {wifi_rate::HT_MCS4_20MHz, 39.0f, modulation_type::OFDM, coding_type::BCC, 20, "HT MCS4 39 Mbps (16-QAM 3/4)", false}},
    {wifi_rate::HT_MCS5_20MHz, {wifi_rate::HT_MCS5_20MHz, 52.0f, modulation_type::OFDM, coding_type::BCC, 20, "HT MCS5 52 Mbps (64-QAM 2/3)", false}},
    {wifi_rate::HT_MCS6_20MHz, {wifi_rate::HT_MCS6_20MHz, 58.5f, modulation_type::OFDM, coding_type::BCC, 20, "HT MCS6 58.5 Mbps (64-QAM 3/4)", false}},
    {wifi_rate::HT_MCS7_20MHz, {wifi_rate::HT_MCS7_20MHz, 65.0f, modulation_type::OFDM, coding_type::BCC, 20, "HT MCS7 65 Mbps (64-QAM 5/6)", false}},

    // 802.11ac VHT rates (20 MHz)
    {wifi_rate::VHT_MCS0_20MHz, {wifi_rate::VHT_MCS0_20MHz, 6.5f, modulation_type::OFDM, coding_type::BCC, 20, "VHT MCS0 6.5 Mbps (BPSK 1/2)", false}},
    {wifi_rate::VHT_MCS1_20MHz, {wifi_rate::VHT_MCS1_20MHz, 13.0f, modulation_type::OFDM, coding_type::BCC, 20, "VHT MCS1 13 Mbps (QPSK 1/2)", false}},
    {wifi_rate::VHT_MCS2_20MHz, {wifi_rate::VHT_MCS2_20MHz, 19.5f, modulation_type::OFDM, coding_type::BCC, 20, "VHT MCS2 19.5 Mbps (QPSK 3/4)", false}},
    {wifi_rate::VHT_MCS3_20MHz, {wifi_rate::VHT_MCS3_20MHz, 26.0f, modulation_type::OFDM, coding_type::BCC, 20, "VHT MCS3 26 Mbps (16-QAM 1/2)", false}},
    {wifi_rate::VHT_MCS4_20MHz, {wifi_rate::VHT_MCS4_20MHz, 39.0f, modulation_type::OFDM, coding_type::BCC, 20, "VHT MCS4 39 Mbps (16-QAM 3/4)", false}},
    {wifi_rate::VHT_MCS5_20MHz, {wifi_rate::VHT_MCS5_20MHz, 52.0f, modulation_type::OFDM, coding_type::BCC, 20, "VHT MCS5 52 Mbps (64-QAM 2/3)", false}},
    {wifi_rate::VHT_MCS6_20MHz, {wifi_rate::VHT_MCS6_20MHz, 58.5f, modulation_type::OFDM, coding_type::BCC, 20, "VHT MCS6 58.5 Mbps (64-QAM 3/4)", false}},
    {wifi_rate::VHT_MCS7_20MHz, {wifi_rate::VHT_MCS7_20MHz, 65.0f, modulation_type::OFDM, coding_type::BCC, 20, "VHT MCS7 65 Mbps (64-QAM 5/6)", false}},
    {wifi_rate::VHT_MCS8_20MHz, {wifi_rate::VHT_MCS8_20MHz, 78.0f, modulation_type::OFDM, coding_type::BCC, 20, "VHT MCS8 78 Mbps (256-QAM 3/4)", false}},
    {wifi_rate::VHT_MCS9_20MHz, {wifi_rate::VHT_MCS9_20MHz, 86.7f, modulation_type::OFDM, coding_type::BCC, 20, "VHT MCS9 86.7 Mbps (256-QAM 5/6)", false}},
};

const rate_info& get_rate_info(wifi_rate r)
{
    auto it = rate_table.find(r);
    if (it == rate_table.end()) {
        throw std::invalid_argument("Unknown WiFi rate");
    }
    return it->second;
}

std::string get_rate_name(wifi_rate r)
{
    return get_rate_info(r).description;
}

float get_rate_mbps(wifi_rate r)
{
    return get_rate_info(r).mbps;
}

bool is_dsss_rate(wifi_rate r)
{
    const auto& info = get_rate_info(r);
    return (info.mod == modulation_type::DSSS || info.mod == modulation_type::CCK);
}

bool is_ofdm_rate(wifi_rate r)
{
    const auto& info = get_rate_info(r);
    return (info.mod == modulation_type::OFDM);
}

} // namespace ieee80211
} // namespace gr
