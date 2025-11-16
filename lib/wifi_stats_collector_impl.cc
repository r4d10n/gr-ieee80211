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

#include "wifi_stats_collector_impl.h"
#include <gnuradio/io_signature.h>
#include <algorithm>

namespace gr {
namespace ieee80211 {

wifi_stats_collector::sptr wifi_stats_collector::make(int update_interval_ms)
{
    return gnuradio::make_block_sptr<wifi_stats_collector_impl>(update_interval_ms);
}

wifi_stats_collector_impl::wifi_stats_collector_impl(int update_interval_ms)
    : gr::block("wifi_stats_collector",
                gr::io_signature::make(0, 0, 0),
                gr::io_signature::make(0, 0, 0)),
      d_update_interval_ms(update_interval_ms),
      d_rx_packets_total(0),
      d_rx_packets_success(0),
      d_rx_packets_error(0),
      d_tx_packets_total(0),
      d_total_bytes(0),
      d_snr_current(0.0f),
      d_rssi_current(0.0f),
      d_bytes_since_last_update(0),
      d_throughput_current(0.0),
      d_throughput_peak(0.0),
      d_current_rate(wifi_rate::DSSS_1M_LONG),
      d_current_modulation(modulation_type::DSSS)
{
    // Register message ports
    message_port_register_in(pmt::mp("rx_packets"));
    message_port_register_in(pmt::mp("tx_packets"));
    message_port_register_in(pmt::mp("rx_errors"));
    message_port_register_in(pmt::mp("signal_quality"));
    message_port_register_out(pmt::mp("stats"));

    // Set message handlers
    set_msg_handler(pmt::mp("rx_packets"),
                    [this](pmt::pmt_t msg) { handle_rx_packet(msg); });
    set_msg_handler(pmt::mp("tx_packets"),
                    [this](pmt::pmt_t msg) { handle_tx_packet(msg); });
    set_msg_handler(pmt::mp("rx_errors"),
                    [this](pmt::pmt_t msg) { handle_rx_error(msg); });
    set_msg_handler(pmt::mp("signal_quality"),
                    [this](pmt::pmt_t msg) { handle_signal_quality(msg); });

    d_snr_history.reserve(100);
    d_rssi_history.reserve(100);
}

wifi_stats_collector_impl::~wifi_stats_collector_impl() {}

bool wifi_stats_collector_impl::start()
{
    std::lock_guard<std::mutex> lock(d_mutex);
    d_start_time = std::chrono::steady_clock::now();
    d_last_update = d_start_time;
    d_last_packet_time = d_start_time;
    return block::start();
}

bool wifi_stats_collector_impl::stop()
{
    return block::stop();
}

void wifi_stats_collector_impl::handle_rx_packet(pmt::pmt_t msg)
{
    std::lock_guard<std::mutex> lock(d_mutex);

    d_rx_packets_total++;
    d_rx_packets_success++;
    d_last_packet_time = std::chrono::steady_clock::now();

    // Extract packet length if available
    if (pmt::is_dict(msg)) {
        pmt::pmt_t len_pmt = pmt::dict_ref(msg, pmt::mp("length"), pmt::PMT_NIL);
        if (!pmt::eqv(len_pmt, pmt::PMT_NIL)) {
            size_t len = pmt::to_long(len_pmt);
            d_total_bytes += len;
            d_bytes_since_last_update += len;
        }

        // Extract rate if available
        pmt::pmt_t rate_pmt = pmt::dict_ref(msg, pmt::mp("rate"), pmt::PMT_NIL);
        if (!pmt::eqv(rate_pmt, pmt::PMT_NIL)) {
            d_current_rate = static_cast<wifi_rate>(pmt::to_long(rate_pmt));
        }
    }

    update_throughput();
}

void wifi_stats_collector_impl::handle_tx_packet(pmt::pmt_t msg)
{
    std::lock_guard<std::mutex> lock(d_mutex);
    d_tx_packets_total++;

    if (pmt::is_dict(msg)) {
        pmt::pmt_t len_pmt = pmt::dict_ref(msg, pmt::mp("length"), pmt::PMT_NIL);
        if (!pmt::eqv(len_pmt, pmt::PMT_NIL)) {
            size_t len = pmt::to_long(len_pmt);
            d_total_bytes += len;
            d_bytes_since_last_update += len;
        }
    }
}

void wifi_stats_collector_impl::handle_rx_error(pmt::pmt_t msg)
{
    std::lock_guard<std::mutex> lock(d_mutex);
    d_rx_packets_total++;
    d_rx_packets_error++;
}

void wifi_stats_collector_impl::handle_signal_quality(pmt::pmt_t msg)
{
    std::lock_guard<std::mutex> lock(d_mutex);

    if (pmt::is_dict(msg)) {
        // Extract SNR
        pmt::pmt_t snr_pmt = pmt::dict_ref(msg, pmt::mp("snr"), pmt::PMT_NIL);
        if (!pmt::eqv(snr_pmt, pmt::PMT_NIL)) {
            d_snr_current = pmt::to_float(snr_pmt);
            d_snr_history.push_back(d_snr_current);
            if (d_snr_history.size() > 100) {
                d_snr_history.erase(d_snr_history.begin());
            }
        }

        // Extract RSSI
        pmt::pmt_t rssi_pmt = pmt::dict_ref(msg, pmt::mp("rssi"), pmt::PMT_NIL);
        if (!pmt::eqv(rssi_pmt, pmt::PMT_NIL)) {
            d_rssi_current = pmt::to_float(rssi_pmt);
            d_rssi_history.push_back(d_rssi_current);
            if (d_rssi_history.size() > 100) {
                d_rssi_history.erase(d_rssi_history.begin());
            }
        }
    }
}

void wifi_stats_collector_impl::update_throughput()
{
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - d_last_update).count();

    if (elapsed >= d_update_interval_ms) {
        // Calculate current throughput (bytes/sec)
        if (elapsed > 0) {
            d_throughput_current = (d_bytes_since_last_update * 1000.0) / elapsed;
            d_throughput_peak = std::max(d_throughput_peak, d_throughput_current);
        }

        d_bytes_since_last_update = 0;
        d_last_update = now;

        // Publish statistics
        publish_stats();
    }
}

void wifi_stats_collector_impl::publish_stats()
{
    stats_report report = get_stats();

    // Create PMT dictionary with statistics
    pmt::pmt_t stats_dict = pmt::make_dict();
    stats_dict = pmt::dict_add(stats_dict, pmt::mp("rx_packets_total"),
                               pmt::from_uint64(report.rx_packets_total));
    stats_dict = pmt::dict_add(stats_dict, pmt::mp("rx_packets_success"),
                               pmt::from_uint64(report.rx_packets_success));
    stats_dict = pmt::dict_add(stats_dict, pmt::mp("rx_packets_error"),
                               pmt::from_uint64(report.rx_packets_error));
    stats_dict = pmt::dict_add(stats_dict, pmt::mp("tx_packets_total"),
                               pmt::from_uint64(report.tx_packets_total));
    stats_dict = pmt::dict_add(stats_dict, pmt::mp("throughput_current"),
                               pmt::from_double(report.throughput_current));
    stats_dict = pmt::dict_add(stats_dict, pmt::mp("throughput_average"),
                               pmt::from_double(report.throughput_average));
    stats_dict = pmt::dict_add(stats_dict, pmt::mp("throughput_peak"),
                               pmt::from_double(report.throughput_peak));
    stats_dict = pmt::dict_add(stats_dict, pmt::mp("snr_current"),
                               pmt::from_float(report.snr_current));
    stats_dict = pmt::dict_add(stats_dict, pmt::mp("snr_average"),
                               pmt::from_float(report.snr_average));
    stats_dict = pmt::dict_add(stats_dict, pmt::mp("per_current"),
                               pmt::from_double(report.per_current));

    message_port_pub(pmt::mp("stats"), stats_dict);
}

wifi_stats_collector::stats_report wifi_stats_collector_impl::get_stats()
{
    std::lock_guard<std::mutex> lock(d_mutex);

    stats_report report;
    report.rx_packets_total = d_rx_packets_total;
    report.rx_packets_success = d_rx_packets_success;
    report.rx_packets_error = d_rx_packets_error;
    report.tx_packets_total = d_tx_packets_total;
    report.total_bytes = d_total_bytes;

    report.throughput_current = d_throughput_current;
    report.throughput_peak = d_throughput_peak;

    // Calculate average throughput
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - d_start_time).count();
    report.throughput_average = (elapsed > 0) ? (d_total_bytes / (double)elapsed) : 0.0;

    report.snr_current = d_snr_current;
    report.rssi_current = d_rssi_current;

    // Calculate average SNR and RSSI
    if (!d_snr_history.empty()) {
        float sum = std::accumulate(d_snr_history.begin(), d_snr_history.end(), 0.0f);
        report.snr_average = sum / d_snr_history.size();
    } else {
        report.snr_average = 0.0f;
    }

    if (!d_rssi_history.empty()) {
        float sum = std::accumulate(d_rssi_history.begin(), d_rssi_history.end(), 0.0f);
        report.rssi_average = sum / d_rssi_history.size();
    } else {
        report.rssi_average = 0.0f;
    }

    // Calculate PER
    if (d_rx_packets_total > 0) {
        report.per_current = (double)d_rx_packets_error / d_rx_packets_total;
        report.per_average = report.per_current;
    } else {
        report.per_current = 0.0;
        report.per_average = 0.0;
    }

    report.current_rate = d_current_rate;
    report.current_modulation = d_current_modulation;
    report.elapsed_time = elapsed;

    return report;
}

void wifi_stats_collector_impl::reset_stats()
{
    std::lock_guard<std::mutex> lock(d_mutex);

    d_rx_packets_total = 0;
    d_rx_packets_success = 0;
    d_rx_packets_error = 0;
    d_tx_packets_total = 0;
    d_total_bytes = 0;
    d_bytes_since_last_update = 0;
    d_throughput_current = 0.0;
    d_throughput_peak = 0.0;
    d_snr_history.clear();
    d_rssi_history.clear();

    d_start_time = std::chrono::steady_clock::now();
    d_last_update = d_start_time;
}

void wifi_stats_collector_impl::set_update_interval(int interval_ms)
{
    std::lock_guard<std::mutex> lock(d_mutex);
    d_update_interval_ms = interval_ms;
}

} // namespace ieee80211
} // namespace gr
