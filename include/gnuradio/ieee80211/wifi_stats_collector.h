/* -*- c++ -*- */
/*
 * Copyright 2025 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#ifndef INCLUDED_IEEE80211_WIFI_STATS_COLLECTOR_H
#define INCLUDED_IEEE80211_WIFI_STATS_COLLECTOR_H

#include <gnuradio/ieee80211/api.h>
#include <gnuradio/block.h>
#include <gnuradio/ieee80211/wifi_rates.h>

namespace gr {
namespace ieee80211 {

/**
 * @brief WiFi Statistics Collector
 *
 * Collects and reports comprehensive statistics for WiFi transmissions:
 * - Packet counts (RX/TX, success/error)
 * - Throughput (current, average, peak)
 * - Signal quality (SNR, RSSI)
 * - Packet error rate (PER)
 * - Rate distribution
 *
 * Subscribes to message ports for packet events and publishes
 * periodic statistics reports.
 */
class IEEE80211_API wifi_stats_collector : virtual public gr::block
{
public:
    typedef std::shared_ptr<wifi_stats_collector> sptr;

    /**
     * @brief Statistics report structure
     */
    struct stats_report {
        // Packet counters
        uint64_t rx_packets_total;
        uint64_t rx_packets_success;
        uint64_t rx_packets_error;
        uint64_t tx_packets_total;

        // Throughput (bytes per second)
        double throughput_current;
        double throughput_average;
        double throughput_peak;

        // Signal quality
        float snr_current;
        float snr_average;
        float rssi_current;
        float rssi_average;

        // Error statistics
        double per_current;      // Packet Error Rate
        double per_average;

        // Rate statistics
        wifi_rate current_rate;
        modulation_type current_modulation;

        // Timing
        double elapsed_time;
        uint64_t total_bytes;
    };

    /**
     * @brief Create WiFi statistics collector
     *
     * @param update_interval_ms Statistics update interval in milliseconds
     * @return Shared pointer to stats collector block
     */
    static sptr make(int update_interval_ms = 1000);

    /**
     * @brief Get current statistics report
     */
    virtual stats_report get_stats() = 0;

    /**
     * @brief Reset all statistics
     */
    virtual void reset_stats() = 0;

    /**
     * @brief Set update interval
     */
    virtual void set_update_interval(int interval_ms) = 0;
};

} // namespace ieee80211
} // namespace gr

#endif /* INCLUDED_IEEE80211_WIFI_STATS_COLLECTOR_H */
