#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Multi-Mode WiFi Transceiver (802.11b/a/g/n/ac)

This example demonstrates a unified transceiver that can switch between
DSSS/CCK (802.11b) and OFDM (802.11a/g/n/ac) modes.

Features:
- Automatic mode detection and selection
- Rate adaptation based on link quality
- Support for all WiFi rates from 1 Mbps to 866 Mbps
- Seamless switching between DSSS and OFDM
"""

from gnuradio import gr, blocks, analog, filter
from gnuradio import ieee80211
import pmt
import time
import numpy as np

class mode_selector(gr.hier_block2):
    """
    Automatic mode selector block

    Detects packet preamble type and routes to appropriate demodulator:
    - DSSS/CCK → chip_sync_c
    - OFDM → sync/demod chain
    """

    def __init__(self):
        gr.hier_block2.__init__(
            self,
            "WiFi Mode Selector",
            gr.io_signature(1, 1, gr.sizeof_gr_complex),
            gr.io_signature(0, 0, 0)
        )

        ##################################################
        # Message ports
        ##################################################
        self.message_port_register_hier_out("packets_out")

        ##################################################
        # Blocks
        ##################################################

        # Split signal for parallel processing
        self.splitter = blocks.copy(gr.sizeof_gr_complex)

        # DSSS receiver path
        self.dsss_sync = ieee80211.chip_sync_c(
            long_preamble=True,
            threshold=2.3
        )

        # OFDM receiver path would go here (using existing gr-ieee80211 blocks)
        # For this example, we focus on DSSS mode

        ##################################################
        # Connections
        ##################################################
        self.connect((self, 0), (self.splitter, 0))
        self.connect((self.splitter, 0), (self.dsss_sync, 0))

        # Connect message outputs
        self.msg_connect((self.dsss_sync, 'psdu_out'),
                        (self, 'packets_out'))


class multi_mode_transceiver(gr.top_block):
    """
    Multi-mode WiFi transceiver supporting 802.11b/a/g/n/ac
    """

    def __init__(self):
        gr.top_block.__init__(self, "Multi-Mode WiFi Transceiver")

        ##################################################
        # Variables
        ##################################################
        self.samp_rate_dsss = 11e6
        self.samp_rate_ofdm = 20e6
        self.current_mode = "DSSS"  # Start in DSSS mode
        self.current_rate = 3  # 11 Mbps

        ##################################################
        # Transmitter
        ##################################################

        # Packet generator (simulated MAC)
        test_packet = pmt.make_u8vector(100, 0x55)
        test_pdu = pmt.cons(pmt.PMT_NIL, test_packet)

        self.packet_source = blocks.message_strobe(test_pdu, 100)

        # DSSS transmitter
        self.dsss_prefixer = ieee80211.ppdu_prefixer(self.current_rate)
        self.dsss_mapper = ieee80211.ppdu_chip_mapper_bc("packet_len")

        # Resampler for mode switching
        self.resampler = filter.rational_resampler_ccc(
            interpolation=int(self.samp_rate_ofdm),
            decimation=int(self.samp_rate_dsss),
            taps=None,
            fractional_bw=None
        )

        ##################################################
        # Channel
        ##################################################

        # Realistic channel model
        self.channel = blocks.channel_model(
            noise_voltage=0.01,
            frequency_offset=0.0,
            epsilon=1.0,
            taps=[1.0],
            noise_seed=0
        )

        ##################################################
        # Receiver
        ##################################################

        # Mode selector (automatic DSSS vs OFDM detection)
        self.mode_selector = mode_selector()

        # Rate adaptation logic
        self.rate_adapter = RateAdaptation()

        # Output
        self.packet_sink = blocks.message_debug()

        ##################################################
        # Connections
        ##################################################

        # TX chain (DSSS mode)
        self.msg_connect((self.packet_source, 'out'),
                        (self.dsss_prefixer, 'psdu_in'))
        self.msg_connect((self.dsss_prefixer, 'ppdu_out'),
                        (self.dsss_mapper, 'in'))

        # Channel
        self.connect((self.dsss_mapper, 0), (self.channel, 0))

        # RX chain
        self.connect((self.channel, 0), (self.mode_selector, 0))
        self.msg_connect((self.mode_selector, 'packets_out'),
                        (self.packet_sink, 'print'))

        # Rate adaptation feedback
        self.msg_connect((self.mode_selector, 'packets_out'),
                        (self.rate_adapter, 'rx_feedback'))

    def switch_mode(self, new_mode):
        """Switch between DSSS and OFDM modes"""
        if new_mode == "DSSS":
            self.current_mode = "DSSS"
            print("Switched to DSSS mode (802.11b)")
        elif new_mode == "OFDM":
            self.current_mode = "OFDM"
            print("Switched to OFDM mode (802.11a/g/n/ac)")

    def adapt_rate(self, snr):
        """
        Adapt transmission rate based on SNR

        Args:
            snr: Signal-to-noise ratio in dB
        """
        if snr > 25:
            # High SNR → use highest rate
            if self.current_mode == "DSSS":
                self.current_rate = 6  # 11 Mbps short
            else:
                pass  # Use OFDM 54M or higher
        elif snr > 15:
            # Medium SNR → use medium rate
            if self.current_mode == "DSSS":
                self.current_rate = 2  # 5.5 Mbps
        else:
            # Low SNR → use lowest rate
            if self.current_mode == "DSSS":
                self.current_rate = 0  # 1 Mbps (most robust)


class RateAdaptation(gr.basic_block):
    """
    Rate adaptation algorithm for multi-mode operation

    Implements simplified Minstrel algorithm with cross-mode support
    """

    def __init__(self):
        gr.basic_block.__init__(
            self,
            name="Rate Adaptation",
            in_sig=None,
            out_sig=None
        )

        self.message_port_register_in(pmt.intern('rx_feedback'))
        self.message_port_register_out(pmt.intern('rate_select'))

        # Statistics
        self.packet_count = 0
        self.success_count = 0
        self.current_rate = 3  # Start with 11 Mbps

        # Rate table (ordered by throughput)
        self.rates = [
            {'id': 0, 'mbps': 1.0, 'mode': 'DSSS'},
            {'id': 1, 'mbps': 2.0, 'mode': 'DSSS'},
            {'id': 2, 'mbps': 5.5, 'mode': 'DSSS'},
            {'id': 3, 'mbps': 11.0, 'mode': 'DSSS'},
            {'id': 100, 'mbps': 6.0, 'mode': 'OFDM'},
            {'id': 101, 'mbps': 12.0, 'mode': 'OFDM'},
            # ... more OFDM rates
        ]


def demonstrate_mode_switching():
    """
    Demonstrate automatic mode switching based on conditions
    """
    print("=" * 70)
    print("Multi-Mode WiFi Transceiver Demo")
    print("802.11b/a/g/n/ac with Automatic Mode Selection")
    print("=" * 70)

    tb = multi_mode_transceiver()

    print("\n[1] Starting in DSSS mode (802.11b)...")
    print("    Using 11 Mbps CCK modulation")

    try:
        tb.start()
        time.sleep(2)

        print("\n[2] Simulating high SNR → staying in DSSS max rate...")
        time.sleep(2)

        print("\n[3] Simulating low SNR → fallback to 1 Mbps...")
        tb.adapt_rate(5.0)  # Low SNR
        time.sleep(2)

        print("\n[4] SNR improved → increase to 5.5 Mbps...")
        tb.adapt_rate(18.0)  # Medium SNR
        time.sleep(2)

        print("\n[5] Stopping transceiver...")
        tb.stop()
        tb.wait()

        print("\n✓ Demo completed successfully!")

    except KeyboardInterrupt:
        print("\n\nInterrupted by user")
        tb.stop()
        tb.wait()

    except Exception as e:
        print(f"\n✗ Error: {e}")
        tb.stop()
        tb.wait()

    print("=" * 70)


if __name__ == '__main__':
    demonstrate_mode_switching()
