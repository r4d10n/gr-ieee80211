# 802.11b DSSS/CCK Examples

This directory contains example flowgraphs and scripts demonstrating 802.11b DSSS/CCK functionality in gr-ieee80211.

## Examples

### 1. `dsss_loopback.py` - Basic Loopback Test

**Purpose:** Demonstrates basic 802.11b packet transmission and reception in a loopback configuration.

**Features:**
- All 7 rate/preamble combinations (1, 2, 5.5, 11 Mbps)
- No hardware required (simulated channel)
- Automatic testing of all rates
- Signal quality monitoring

**Usage:**
```bash
cd examples/dsss
python3 dsss_loopback.py
```

**Expected Output:**
```
======================================================================
802.11b DSSS/CCK Loopback Test
======================================================================

Testing rate 0: 1 Mbps (long preamble)
Running for 3 seconds...
✓ Test completed successfully

Testing rate 1: 2 Mbps (long preamble)
Running for 3 seconds...
✓ Test completed successfully

[... continues for all rates ...]
```

**What it tests:**
- PPDU prefixer (PLCP header generation)
- Chip mapper (DSSS/CCK modulation)
- Chip sync (demodulation and synchronization)
- End-to-end packet flow

---

### 2. `multi_mode_transceiver.py` - Multi-Mode Operation

**Purpose:** Demonstrates unified transceiver supporting both DSSS and OFDM modes with automatic switching.

**Features:**
- Automatic mode detection (DSSS vs OFDM)
- Rate adaptation based on link quality
- Cross-mode operation
- Realistic channel model

**Usage:**
```bash
cd examples/dsss
python3 multi_mode_transceiver.py
```

**Expected Output:**
```
======================================================================
Multi-Mode WiFi Transceiver Demo
802.11b/a/g/n/ac with Automatic Mode Selection
======================================================================

[1] Starting in DSSS mode (802.11b)...
    Using 11 Mbps CCK modulation

[2] Simulating high SNR → staying in DSSS max rate...

[3] Simulating low SNR → fallback to 1 Mbps...

[4] SNR improved → increase to 5.5 Mbps...

[5] Stopping transceiver...

✓ Demo completed successfully!
```

**What it demonstrates:**
- Mode selection logic
- Rate adaptation algorithm
- DSSS fallback for low SNR
- Integration with OFDM modes

---

## GRC Flowgraphs

For GNU Radio Companion users, you can create flowgraphs using these blocks:

### DSSS Transmitter Chain
```
┌─────────────────┐
│ Message Strobe  │ → Generate test packets
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ PPDU Prefixer   │ → Add PLCP header (rate: 0-6)
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ Chip Mapper     │ → Generate DSSS/CCK chips
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ USRP Sink       │ → Transmit (or File Sink for testing)
└─────────────────┘
```

### DSSS Receiver Chain
```
┌─────────────────┐
│ USRP Source     │ → Receive signal (or File Source)
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ Chip Sync       │ → Synchronize and demodulate
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ Message Debug   │ → Display packets
└─────────────────┘
```

### Block Parameters

**ieee80211_ppdu_prefixer**
- Rate: 0-6 (0=1M long, 1=2M long, 2=5.5M long, 3=11M long, 4=2M short, 5=5.5M short, 6=11M short)
- Input: Message PDU (PSDU)
- Output: Message PDU (PPDU with PLCP header)

**ieee80211_ppdu_chip_mapper_bc**
- Length Tag Name: "packet_len" (default)
- Input: Byte stream
- Output: Complex chips (11 Msps)

**ieee80211_chip_sync_c**
- Long Preamble: True/False
- Correlation Threshold: 0.0-11.0 (default: 2.3)
- Input: Complex stream (11 Msps)
- Output: Message PDU (received packets)

---

## Hardware Testing with USRP

### Requirements
- USRP B210, B200, or similar (11 Msps capable)
- Two antennas or loopback cable
- GNU Radio 3.10+ with UHD

### USRP TX Example
```python
from gnuradio import uhd

# Create USRP sink
usrp_sink = uhd.usrp_sink(
    device_addr="",
    stream_args=uhd.stream_args(
        cpu_format="fc32",
        channels=range(1),
    ),
)

# Configure for 802.11b channel 6 (2.437 GHz)
usrp_sink.set_center_freq(2.437e9, 0)
usrp_sink.set_samp_rate(11e6)
usrp_sink.set_gain(30, 0)

# Connect to chip mapper output
tb.connect((chip_mapper, 0), (usrp_sink, 0))
```

### USRP RX Example
```python
# Create USRP source
usrp_source = uhd.usrp_source(
    device_addr="",
    stream_args=uhd.stream_args(
        cpu_format="fc32",
        channels=range(1),
    ),
)

# Configure
usrp_source.set_center_freq(2.437e9, 0)
usrp_source.set_samp_rate(11e6)
usrp_source.set_gain(40, 0)

# Connect to chip sync input
tb.connect((usrp_source, 0), (chip_sync, 0))
```

---

## Troubleshooting

### No Packets Received

**Possible causes:**
1. Threshold too high → Lower correlation threshold (try 1.5-2.0)
2. Wrong preamble type → Match TX and RX preamble settings
3. Sample rate mismatch → Ensure both TX and RX use 11 Msps
4. Insufficient signal level → Increase TX gain or decrease distance

### High Packet Error Rate

**Possible causes:**
1. Too much noise → Increase TX power or improve antenna
2. Frequency offset → Check USRP clock synchronization
3. Timing offset → Verify sample rates are exact
4. Interference → Change WiFi channel

### Build Errors

**Missing GNU Radio:**
```bash
sudo apt install gnuradio-dev
```

**Missing UHD:**
```bash
sudo apt install uhd-host libuhd-dev
```

---

## Performance Expectations

| Rate | Expected PER @ 10dB SNR | Expected PER @ 20dB SNR |
|------|-------------------------|-------------------------|
| 1 Mbps | < 1% | < 0.1% |
| 2 Mbps | < 5% | < 0.5% |
| 5.5 Mbps | < 10% | < 1% |
| 11 Mbps | < 15% | < 2% |

*PER = Packet Error Rate*

**Throughput:**
- 1 Mbps: ~0.8 Mbps (application layer)
- 2 Mbps: ~1.6 Mbps
- 5.5 Mbps: ~4.4 Mbps
- 11 Mbps: ~8.8 Mbps

---

## Advanced Usage

### Packet Capture to PCAP

```python
# Add PCAP writer to save packets
from scapy.all import wrpcap, Ether

class pcap_writer(gr.basic_block):
    def __init__(self, filename):
        gr.basic_block.__init__(self, name="PCAP Writer",
                                in_sig=None, out_sig=None)
        self.message_port_register_in(pmt.intern('pdus'))
        self.set_msg_handler(pmt.intern('pdus'), self.handle_pdu)
        self.filename = filename
        self.packets = []

    def handle_pdu(self, pdu):
        meta = pmt.car(pdu)
        data = pmt.cdr(pdu)

        if pmt.is_u8vector(data):
            packet_bytes = bytes(pmt.u8vector_elements(data))
            # Save to PCAP
            self.packets.append(Ether(packet_bytes))

    def close(self):
        wrpcap(self.filename, self.packets)
```

### Monitor Mode Compatibility

To receive real 802.11b traffic from commercial WiFi devices:

1. Set USRP to monitor WiFi channel
2. Use chip_sync with appropriate threshold
3. Decode MAC frames
4. Analyze with Wireshark

---

## References

- IEEE 802.11-2020 Standard (Section 17: DSSS PHY)
- GNU Radio Documentation: https://www.gnuradio.org/doc/
- gr-wifi-dsss: https://github.com/r4d10n/gr-wifi-dsss

---

**Last Updated:** 2025-11-16
