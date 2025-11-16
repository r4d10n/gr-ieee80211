/* -*- c++ -*- */
/*
 * Copyright 2025 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#ifndef INCLUDED_IEEE80211_WIFI_STATS_COLLECTOR_IMPL_H
#define INCLUDED_IEEE80211_WIFI_STATS_COLLECTOR_IMPL_H

#include <gnuradio/ieee80211/wifi_stats_collector.h>
#include <mutex>
#include <chrono>

namespace gr {
namespace ieee80211 {

class wifi_stats_collector_impl : public wifi_stats_collector
{
private:
    std::mutex d_mutex;

    // Configuration
    int d_update_interval_ms;

    // Counters
    uint64_t d_rx_packets_total;
    uint64_t d_rx_packets_success;
    uint64_t d_rx_packets_error;
    uint64_t d_tx_packets_total;
    uint64_t d_total_bytes;

    // Signal quality tracking
    std::vector<float> d_snr_history;
    std::vector<float> d_rssi_history;
    float d_snr_current;
    float d_rssi_current;

    // Throughput tracking
    std::chrono::steady_clock::time_point d_start_time;
    std::chrono::steady_clock::time_point d_last_update;
    std::chrono::steady_clock::time_point d_last_packet_time;
    uint64_t d_bytes_since_last_update;
    double d_throughput_current;
    double d_throughput_peak;

    // Rate tracking
    wifi_rate d_current_rate;
    modulation_type d_current_modulation;

    // Message handlers
    void handle_rx_packet(pmt::pmt_t msg);
    void handle_tx_packet(pmt::pmt_t msg);
    void handle_rx_error(pmt::pmt_t msg);
    void handle_signal_quality(pmt::pmt_t msg);

    // Statistics computation
    void update_throughput();
    void publish_stats();

public:
    wifi_stats_collector_impl(int update_interval_ms);
    ~wifi_stats_collector_impl();

    stats_report get_stats() override;
    void reset_stats() override;
    void set_update_interval(int interval_ms) override;

    bool start() override;
    bool stop() override;
};

} // namespace ieee80211
} // namespace gr

#endif /* INCLUDED_IEEE80211_WIFI_STATS_COLLECTOR_IMPL_H */
