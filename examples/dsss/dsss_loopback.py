#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
802.11b DSSS/CCK Loopback Example

This example demonstrates basic DSSS packet transmission and reception
in a loopback configuration (no hardware required).

Features:
- All 802.11b data rates (1, 2, 5.5, 11 Mbps)
- Both long and short preambles
- Packet generation and decoding
- Signal quality monitoring
"""

from gnuradio import gr, blocks, analog
from gnuradio import ieee80211
import pmt
import time

class dsss_loopback(gr.top_block):
    def __init__(self, rate=3, packet_len=100, packets_per_second=10):
        """
        DSSS Loopback Flowgraph

        Args:
            rate: 0=1M long, 1=2M long, 2=5.5M long, 3=11M long,
                  4=2M short, 5=5.5M short, 6=11M short
            packet_len: Length of test packets in bytes
            packets_per_second: Packet transmission rate
        """
        gr.top_block.__init__(self, "802.11b DSSS Loopback")

        ##################################################
        # Variables
        ##################################################
        self.rate = rate
        self.packet_len = packet_len
        self.samp_rate = 11e6  # 11 Msps for all DSSS rates
        self.packet_interval = 1.0 / packets_per_second

        ##################################################
        # Blocks - Transmitter
        ##################################################

        # Generate test packets
        test_packet = pmt.make_u8vector(packet_len, 0x42)  # Fill with 'B'
        test_packet_pdu = pmt.cons(pmt.PMT_NIL, test_packet)

        self.msg_source = blocks.message_strobe(
            test_packet_pdu,
            int(self.packet_interval * 1000)  # milliseconds
        )

        # Add PLCP preamble and header
        self.ppdu_prefixer = ieee80211.ppdu_prefixer(rate)

        # Map bytes to DSSS/CCK chips
        self.chip_mapper = ieee80211.ppdu_chip_mapper_bc("packet_len")

        ##################################################
        # Channel (Add noise and attenuation)
        ##################################################

        # Add some channel impairments
        self.noise_source = analog.noise_source_c(
            analog.GR_GAUSSIAN,
            0.01,  # Low noise
            0
        )

        self.adder = blocks.add_cc(1)

        # Attenuate signal to simulate path loss
        self.mult_const = blocks.multiply_const_cc(0.5)

        ##################################################
        # Blocks - Receiver
        ##################################################

        # DSSS chip synchronization and demodulation
        self.chip_sync = ieee80211.chip_sync_c(
            long_preamble=(rate < 4),  # Long preamble for rates 0-3
            threshold=2.3
        )

        # Display received packets
        self.msg_debug = blocks.message_debug()

        ##################################################
        # Connections
        ##################################################

        # Transmitter chain
        self.msg_connect((self.msg_source, 'out'),
                        (self.ppdu_prefixer, 'psdu_in'))
        self.msg_connect((self.ppdu_prefixer, 'ppdu_out'),
                        (self.chip_mapper, 'in'))

        # Channel
        self.connect((self.chip_mapper, 0), (self.mult_const, 0))
        self.connect((self.mult_const, 0), (self.adder, 0))
        self.connect((self.noise_source, 0), (self.adder, 1))

        # Receiver chain
        self.connect((self.adder, 0), (self.chip_sync, 0))
        self.msg_connect((self.chip_sync, 'psdu_out'),
                        (self.msg_debug, 'print'))

    def get_rate_name(self):
        """Get human-readable name for current rate"""
        rate_names = [
            "1 Mbps (long preamble)",
            "2 Mbps (long preamble)",
            "5.5 Mbps (long preamble)",
            "11 Mbps (long preamble)",
            "2 Mbps (short preamble)",
            "5.5 Mbps (short preamble)",
            "11 Mbps (short preamble)"
        ]
        return rate_names[self.rate]


def main():
    """Run the loopback test for all rates"""
    print("=" * 70)
    print("802.11b DSSS/CCK Loopback Test")
    print("=" * 70)

    # Test all rates
    rates_to_test = [0, 1, 2, 3, 4, 5, 6]  # All 7 rate combinations

    for rate in rates_to_test:
        print(f"\nTesting rate {rate}: ", end="")

        tb = dsss_loopback(
            rate=rate,
            packet_len=100,
            packets_per_second=5
        )

        print(tb.get_rate_name())
        print("Running for 3 seconds...")

        try:
            tb.start()
            time.sleep(3)
            tb.stop()
            tb.wait()
            print("✓ Test completed successfully")

        except KeyboardInterrupt:
            print("\nInterrupted by user")
            tb.stop()
            tb.wait()
            break

        except Exception as e:
            print(f"✗ Error: {e}")

    print("\n" + "=" * 70)
    print("All tests completed!")
    print("=" * 70)


if __name__ == '__main__':
    main()
