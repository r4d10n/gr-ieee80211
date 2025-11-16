/* -*- c++ -*- */
/*
 * Copyright 2025 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include <gnuradio/attributes.h>
#include <gnuradio/ieee80211/dsss/chip_sync_c.h>
#include <gnuradio/ieee80211/dsss/ppdu_chip_mapper_bc.h>
#include <gnuradio/ieee80211/dsss/ppdu_prefixer.h>
#include <gnuradio/ieee80211/utils.h>
#include <gnuradio/ieee80211/wifi_rates.h>
#include <boost/test/unit_test.hpp>
#include <complex>
#include <vector>
#include <cmath>

namespace gr {
namespace ieee80211 {

// Test utility functions
BOOST_AUTO_TEST_SUITE(qa_ieee80211_utils)

BOOST_AUTO_TEST_CASE(test_fcs_calculation)
{
    // Test FCS calculation with known data
    uint8_t test_data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    size_t len = sizeof(test_data);

    uint32_t fcs = utils::calc_fcs(test_data, len);

    // FCS should be non-zero for non-zero data
    BOOST_CHECK_NE(fcs, 0u);

    // Same data should produce same FCS
    uint32_t fcs2 = utils::calc_fcs(test_data, len);
    BOOST_CHECK_EQUAL(fcs, fcs2);
}

BOOST_AUTO_TEST_CASE(test_fcs_validation)
{
    // Create test frame with FCS
    uint8_t frame[100];
    for (size_t i = 0; i < 96; i++) {
        frame[i] = static_cast<uint8_t>(i);
    }

    // Calculate and append FCS
    uint32_t fcs = utils::calc_fcs(frame, 96);
    memcpy(&frame[96], &fcs, 4);

    // Validate should pass
    BOOST_CHECK(utils::validate_fcs(frame, 100));

    // Corrupt data
    frame[50] ^= 0xFF;

    // Validation should fail
    BOOST_CHECK(!utils::validate_fcs(frame, 100));
}

BOOST_AUTO_TEST_CASE(test_plcp_crc16)
{
    // Test PLCP header CRC-16
    uint8_t header[4] = {0x0A, 0x04, 0x64, 0x00}; // 1 Mbps, service, length=100

    uint16_t crc = utils::calc_plcp_crc16(header);
    BOOST_CHECK_NE(crc, 0u);

    // Create full header with CRC
    uint8_t full_header[6];
    memcpy(full_header, header, 4);
    memcpy(&full_header[4], &crc, 2);

    // Validation should pass
    BOOST_CHECK(utils::validate_plcp_crc16(full_header));
}

BOOST_AUTO_TEST_CASE(test_scrambler)
{
    // Test scrambler/descrambler
    uint8_t data[32];
    uint8_t original[32];

    for (size_t i = 0; i < 32; i++) {
        data[i] = original[i] = static_cast<uint8_t>(i * 17);
    }

    // Scramble
    utils::scramble(data, 32, 0x1B);

    // Data should be different after scrambling
    bool changed = false;
    for (size_t i = 0; i < 32; i++) {
        if (data[i] != original[i]) {
            changed = true;
            break;
        }
    }
    BOOST_CHECK(changed);

    // Descramble (scrambler is self-inverse)
    utils::descramble(data, 32, 0x1B);

    // Should match original
    for (size_t i = 0; i < 32; i++) {
        BOOST_CHECK_EQUAL(data[i], original[i]);
    }
}

BOOST_AUTO_TEST_CASE(test_power_conversions)
{
    // Test dBm to linear and back
    float dbm = 10.0f;
    float linear = utils::dbm_to_linear(dbm);
    float dbm_back = utils::linear_to_dbm(linear);

    BOOST_CHECK_CLOSE(dbm, dbm_back, 0.01f);

    // Test specific values
    BOOST_CHECK_CLOSE(utils::dbm_to_linear(0.0f), 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(utils::dbm_to_linear(10.0f), 10.0f, 0.01f);
    BOOST_CHECK_CLOSE(utils::dbm_to_linear(20.0f), 100.0f, 0.01f);
}

BOOST_AUTO_TEST_SUITE_END()

// Test WiFi rate enumeration
BOOST_AUTO_TEST_SUITE(qa_ieee80211_rates)

BOOST_AUTO_TEST_CASE(test_dsss_rates)
{
    // Test DSSS rate information
    BOOST_CHECK_EQUAL(get_rate_mbps(wifi_rate::DSSS_1M_LONG), 1.0f);
    BOOST_CHECK_EQUAL(get_rate_mbps(wifi_rate::DSSS_2M_LONG), 2.0f);
    BOOST_CHECK_EQUAL(get_rate_mbps(wifi_rate::DSSS_5_5M_LONG), 5.5f);
    BOOST_CHECK_EQUAL(get_rate_mbps(wifi_rate::DSSS_11M_LONG), 11.0f);

    // Test rate classification
    BOOST_CHECK(is_dsss_rate(wifi_rate::DSSS_1M_LONG));
    BOOST_CHECK(is_dsss_rate(wifi_rate::DSSS_11M_SHORT));
    BOOST_CHECK(!is_ofdm_rate(wifi_rate::DSSS_1M_LONG));
}

BOOST_AUTO_TEST_CASE(test_ofdm_rates)
{
    // Test OFDM rate information
    BOOST_CHECK_EQUAL(get_rate_mbps(wifi_rate::OFDM_6M), 6.0f);
    BOOST_CHECK_EQUAL(get_rate_mbps(wifi_rate::OFDM_54M), 54.0f);

    // Test rate classification
    BOOST_CHECK(is_ofdm_rate(wifi_rate::OFDM_6M));
    BOOST_CHECK(!is_dsss_rate(wifi_rate::OFDM_6M));
}

BOOST_AUTO_TEST_CASE(test_rate_info)
{
    // Test getting rate info
    const auto& info = get_rate_info(wifi_rate::DSSS_11M_LONG);
    BOOST_CHECK_EQUAL(info.mbps, 11.0f);
    BOOST_CHECK_EQUAL(info.bandwidth_mhz, 22);
    BOOST_CHECK(info.long_preamble);
    BOOST_CHECK_EQUAL(info.mod, modulation_type::CCK);
    BOOST_CHECK_EQUAL(info.coding, coding_type::NONE);
}

BOOST_AUTO_TEST_CASE(test_rate_names)
{
    // Test getting rate names
    std::string name = get_rate_name(wifi_rate::DSSS_1M_LONG);
    BOOST_CHECK(!name.empty());
    BOOST_CHECK(name.find("1") != std::string::npos);
    BOOST_CHECK(name.find("Mbps") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()

// Test DSSS PPDU Prefixer
BOOST_AUTO_TEST_SUITE(qa_dsss_ppdu_prefixer)

BOOST_AUTO_TEST_CASE(test_prefixer_creation)
{
    // Test creating prefixer with each rate
    for (int rate = 0; rate <= 6; rate++) {
        BOOST_CHECK_NO_THROW({
            auto prefixer = ppdu_prefixer::make(rate);
            BOOST_CHECK(prefixer != nullptr);
        });
    }
}

BOOST_AUTO_TEST_CASE(test_prefixer_rate)
{
    // Test rate getter
    auto prefixer = ppdu_prefixer::make(3); // 11 Mbps long
    // Note: Can't easily test get_rate() without accessing impl
}

BOOST_AUTO_TEST_SUITE_END()

// Test DSSS Chip Mapper
BOOST_AUTO_TEST_SUITE(qa_dsss_chip_mapper)

BOOST_AUTO_TEST_CASE(test_chip_mapper_creation)
{
    BOOST_CHECK_NO_THROW({
        auto mapper = ppdu_chip_mapper_bc::make("packet_len");
        BOOST_CHECK(mapper != nullptr);
    });
}

BOOST_AUTO_TEST_SUITE_END()

// Test DSSS Chip Sync
BOOST_AUTO_TEST_SUITE(qa_dsss_chip_sync)

BOOST_AUTO_TEST_CASE(test_chip_sync_creation)
{
    // Test with long preamble
    BOOST_CHECK_NO_THROW({
        auto sync = chip_sync_c::make(true, 2.3f);
        BOOST_CHECK(sync != nullptr);
    });

    // Test with short preamble
    BOOST_CHECK_NO_THROW({
        auto sync = chip_sync_c::make(false, 2.3f);
        BOOST_CHECK(sync != nullptr);
    });
}

BOOST_AUTO_TEST_CASE(test_chip_sync_threshold)
{
    // Test threshold validation
    BOOST_CHECK_NO_THROW({
        auto sync = chip_sync_c::make(true, 0.5f);
    });

    BOOST_CHECK_NO_THROW({
        auto sync = chip_sync_c::make(true, 10.0f);
    });

    // Negative threshold should throw
    BOOST_CHECK_THROW({
        auto sync = chip_sync_c::make(true, -1.0f);
    }, std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(test_chip_sync_preamble_switching)
{
    auto sync = chip_sync_c::make(true, 2.3f);

    // Test preamble type switching
    BOOST_CHECK_NO_THROW({
        sync->set_preamble_type(false);
        sync->set_preamble_type(true);
    });
}

BOOST_AUTO_TEST_SUITE_END()

// Integration tests
BOOST_AUTO_TEST_SUITE(qa_dsss_integration)

BOOST_AUTO_TEST_CASE(test_barker_code_correlation)
{
    // Test Barker code properties
    // Barker-11: [1, -1, 1, 1, -1, 1, 1, 1, -1, -1, -1]
    const float barker[11] = {1, -1, 1, 1, -1, 1, 1, 1, -1, -1, -1};

    // Autocorrelation at zero lag should be 11
    float autocorr = 0;
    for (int i = 0; i < 11; i++) {
        autocorr += barker[i] * barker[i];
    }
    BOOST_CHECK_CLOSE(autocorr, 11.0f, 0.01f);

    // Autocorrelation at non-zero lags should be small
    for (int lag = 1; lag < 11; lag++) {
        float corr = 0;
        for (int i = 0; i < 11 - lag; i++) {
            corr += barker[i] * barker[i + lag];
        }
        BOOST_CHECK_LE(std::abs(corr), 1.1f); // Should be <= 1
    }
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace ieee80211
} // namespace gr
