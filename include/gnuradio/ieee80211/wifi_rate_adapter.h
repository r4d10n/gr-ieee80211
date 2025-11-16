/* -*- c++ -*- */
/*
 * Copyright 2025 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#ifndef INCLUDED_IEEE80211_WIFI_RATE_ADAPTER_H
#define INCLUDED_IEEE80211_WIFI_RATE_ADAPTER_H

#include <gnuradio/ieee80211/api.h>
#include <gnuradio/block.h>
#include <gnuradio/ieee80211/wifi_rates.h>

namespace gr {
namespace ieee80211 {

/**
 * @brief WiFi Rate Adapter
 *
 * Implements automatic rate adaptation algorithms for WiFi:
 * - Minstrel rate adaptation (default)
 * - ARF (Auto Rate Fallback)
 * - AARF (Adaptive ARF)
 * - SampleRate
 *
 * Monitors channel conditions (SNR, PER) and automatically selects
 * the optimal data rate to maximize throughput while maintaining
 * acceptable packet error rates.
 *
 * Supports all WiFi standards: 802.11b/a/g/n/ac
 */
class IEEE80211_API wifi_rate_adapter : virtual public gr::block
{
public:
    typedef std::shared_ptr<wifi_rate_adapter> sptr;

    /**
     * @brief Rate adaptation algorithms
     */
    enum algorithm {
        MINSTREL,    ///< Minstrel rate adaptation (default)
        ARF,         ///< Auto Rate Fallback
        AARF,        ///< Adaptive Auto Rate Fallback
        SAMPLE_RATE, ///< SampleRate algorithm
        FIXED_RATE   ///< Fixed rate (no adaptation)
    };

    /**
     * @brief Create WiFi rate adapter
     *
     * @param algo Rate adaptation algorithm
     * @param min_rate Minimum allowed rate
     * @param max_rate Maximum allowed rate
     * @param target_per Target packet error rate (0.0-1.0)
     * @return Shared pointer to rate adapter block
     */
    static sptr make(algorithm algo = MINSTREL,
                     wifi_rate min_rate = wifi_rate::DSSS_1M_LONG,
                     wifi_rate max_rate = wifi_rate::VHT_MCS9_20MHz,
                     float target_per = 0.1);

    /**
     * @brief Get current rate
     */
    virtual wifi_rate get_current_rate() = 0;

    /**
     * @brief Set algorithm
     */
    virtual void set_algorithm(algorithm algo) = 0;

    /**
     * @brief Set rate limits
     */
    virtual void set_rate_limits(wifi_rate min_rate, wifi_rate max_rate) = 0;

    /**
     * @brief Set target PER
     */
    virtual void set_target_per(float per) = 0;

    /**
     * @brief Enable/disable rate adaptation
     */
    virtual void set_enabled(bool enabled) = 0;
};

} // namespace ieee80211
} // namespace gr

#endif /* INCLUDED_IEEE80211_WIFI_RATE_ADAPTER_H */
