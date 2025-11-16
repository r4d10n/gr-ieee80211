# WiFi Evolution Demonstrator

A comprehensive demonstration platform showcasing the complete evolution of IEEE 802.11 WiFi physical layer standards, from 802.11b (1 Mbps) to 802.11ac (866 Mbps+).

## Overview

The WiFi Evolution Demonstrator provides both command-line and graphical interfaces to explore and test all WiFi PHY standards supported by gr-ieee80211:

- **802.11b DSSS/CCK**: 1, 2, 5.5, 11 Mbps (Direct Sequence Spread Spectrum / Complementary Code Keying)
- **802.11a/g OFDM**: 6, 9, 12, 18, 24, 36, 48, 54 Mbps (Orthogonal Frequency Division Multiplexing)
- **802.11n HT**: Up to 150 Mbps (High Throughput, 20/40 MHz)
- **802.11ac VHT**: Up to 866 Mbps (Very High Throughput, 80/160 MHz)

## Demo Applications

### 1. Python Command-Line Demo with Rich Text UI

**File**: `wifi_evolution_demo.py`

A feature-rich Python application with real-time statistics display using the `rich` library for beautiful terminal output.

#### Features

- **Real-time Statistics Dashboard**:
  - TX/RX packet counts
  - Throughput (current, average, peak)
  - Packet Error Rate (PER)
  - Signal quality metrics (SNR, RSSI)
  - Runtime statistics

- **Interactive Controls**:
  - Keyboard-based rate selection
  - Automatic rate adaptation toggle
  - Statistics reset
  - All WiFi standards accessible via hotkeys

- **Operating Modes**:
  - **Loopback**: Internal testing with simulated channel
  - **USRP**: Over-the-air with USRP hardware

#### Installation

```bash
# Install required dependencies
pip3 install rich

# Make executable
chmod +x wifi_evolution_demo.py
```

#### Usage

**Basic loopback test (no hardware required)**:
```bash
./wifi_evolution_demo.py --mode loopback --rate 11M_SHORT
```

**USRP hardware operation**:
```bash
./wifi_evolution_demo.py --mode usrp --freq 2.437e9 --tx-gain 20 --rx-gain 20
```

**Full options**:
```bash
./wifi_evolution_demo.py \
    --mode loopback \
    --rate 1M_LONG \
    --freq 2.437e9 \
    --tx-gain 20 \
    --rx-gain 20 \
    --packet-size 100 \
    --pps 10 \
    --auto-rate \
    --usrp-args "type=b200"
```

#### Command-Line Arguments

| Argument | Description | Default |
|----------|-------------|---------|
| `--mode` | Operating mode: `loopback` or `usrp` | `loopback` |
| `--rate` | Initial rate (e.g., `1M_LONG`, `11M_SHORT`, `OFDM_54M`) | `1M_LONG` |
| `--freq` | Center frequency in Hz | `2.437e9` (Channel 6) |
| `--tx-gain` | TX gain in dB | `20` |
| `--rx-gain` | RX gain in dB | `20` |
| `--packet-size` | Packet size in bytes | `100` |
| `--pps` | Packets per second | `10` |
| `--auto-rate` | Enable automatic rate adaptation | `False` |
| `--usrp-args` | USRP device arguments | `""` |
| `--simple-ui` | Use simple text UI (no rich library) | `False` |

#### Available Rates

**802.11b DSSS/CCK Rates**:
- `1M_LONG` - 1 Mbps DSSS with long preamble
- `2M_LONG` - 2 Mbps DSSS with long preamble
- `5.5M_LONG` - 5.5 Mbps CCK with long preamble
- `11M_LONG` - 11 Mbps CCK with long preamble
- `2M_SHORT` - 2 Mbps DSSS with short preamble
- `5.5M_SHORT` - 5.5 Mbps CCK with short preamble
- `11M_SHORT` - 11 Mbps CCK with short preamble

**802.11a/g OFDM Rates**:
- `OFDM_6M` - 6 Mbps (BPSK 1/2)
- `OFDM_9M` - 9 Mbps (BPSK 3/4)
- `OFDM_12M` - 12 Mbps (QPSK 1/2)
- `OFDM_18M` - 18 Mbps (QPSK 3/4)
- `OFDM_24M` - 24 Mbps (16-QAM 1/2)
- `OFDM_36M` - 36 Mbps (16-QAM 3/4)
- `OFDM_48M` - 48 Mbps (64-QAM 2/3)
- `OFDM_54M` - 54 Mbps (64-QAM 3/4)

#### Keyboard Controls (Rich UI)

| Key | Action |
|-----|--------|
| `1-7` | Select 802.11b rate |
| `8-9`, `0` | Select 802.11a/g OFDM rate |
| `a` | Toggle automatic rate adaptation |
| `r` | Reset all statistics |
| `q` | Quit application |

### 2. GNU Radio Companion Flowgraph

**File**: `wifi_evolution_demonstrator.grc`

A comprehensive visual flowgraph for GNU Radio Companion with complete GUI controls and real-time visualization.

#### Features

- **Multi-Standard Support**:
  - Automatic switching between 802.11b/a/g/n/ac standards
  - Intelligent TX/RX path selection
  - Standard-specific parameter control

- **GUI Controls**:
  - WiFi standard selector (dropdown)
  - Rate selection for each standard
  - Frequency control (2.4 GHz / 5 GHz bands)
  - TX/RX gain sliders
  - Noise level adjustment
  - Correlation threshold tuning
  - TX enable/disable toggle

- **Real-Time Visualization**:
  - TX signal time domain plot
  - RX signal time domain plot
  - Dual-channel spectrum analyzer
  - Message debug windows for packet inspection

- **Channel Simulation**:
  - AWGN noise injection
  - Multipath fading simulation
  - Adjustable channel impairments

#### Usage

1. **Open in GNU Radio Companion**:
   ```bash
   gnuradio-companion wifi_evolution_demonstrator.grc
   ```

2. **Configure Parameters**:
   - Select WiFi standard from dropdown
   - Choose appropriate rate
   - Adjust frequency (WiFi channels: 2.412-2.484 GHz or 5.150-5.875 GHz)
   - Set TX/RX gains
   - Configure noise level for testing

3. **Run Flowgraph**:
   - Click "Execute" (F6) or press the play button
   - Observe real-time signal visualization
   - Monitor packet reception in message debug windows

4. **Interactive Operation**:
   - Change rates on-the-fly using GUI controls
   - Adjust noise to test different channel conditions
   - Toggle TX on/off to observe receiver behavior
   - Switch between WiFi standards dynamically

#### GUI Layout

```
┌────────────────────────────────────────────────────────────┐
│ Row 0: WiFi Standard Selector                              │
├────────────────────────────────────────────────────────────┤
│ Row 1: 802.11b Rate Selector                               │
│ Row 2: 802.11a/g Rate Selector                             │
│ Row 3: Frequency Control (2.4-5.9 GHz)                     │
│ Row 4: TX Gain | RX Gain                                   │
│ Row 5: Noise Level | Correlation Threshold                 │
│ Row 6: Enable TX                                           │
├────────────────────────────────────────────────────────────┤
│ Row 7-10: TX Signal (Time) | RX Signal (Time)              │
├────────────────────────────────────────────────────────────┤
│ Row 11-14: Spectrum Analyzer (TX + RX)                     │
└────────────────────────────────────────────────────────────┘
```

## WiFi Channel Frequencies

### 2.4 GHz Band (802.11b/g/n)

| Channel | Frequency | Notes |
|---------|-----------|-------|
| 1 | 2.412 GHz | Non-overlapping |
| 6 | 2.437 GHz | Non-overlapping (default) |
| 11 | 2.462 GHz | Non-overlapping |

### 5 GHz Band (802.11a/n/ac)

| Channel | Frequency | Notes |
|---------|-----------|-------|
| 36 | 5.180 GHz | U-NII-1 |
| 40 | 5.200 GHz | U-NII-1 |
| 44 | 5.220 GHz | U-NII-1 |
| 48 | 5.240 GHz | U-NII-1 |
| 149 | 5.745 GHz | U-NII-3 |
| 153 | 5.765 GHz | U-NII-3 |
| 157 | 5.785 GHz | U-NII-3 |
| 161 | 5.805 GHz | U-NII-3 |

## Testing Scenarios

### Scenario 1: DSSS Rate Comparison

Compare throughput and reliability across all 802.11b rates:

```bash
# Test each rate for 60 seconds
for rate in 1M_LONG 2M_LONG 5.5M_LONG 11M_LONG 11M_SHORT; do
    echo "Testing rate: $rate"
    timeout 60 ./wifi_evolution_demo.py --mode loopback --rate $rate --pps 100
done
```

### Scenario 2: Noise Resilience Testing

Test how different rates handle channel noise:

```bash
# In GRC flowgraph:
# 1. Set rate to 1M_LONG
# 2. Gradually increase noise level from 0.001 to 0.1
# 3. Observe PER increase in message debug
# 4. Repeat with 11M_SHORT and compare robustness
```

### Scenario 3: Multi-Mode Operation

Demonstrate automatic mode switching:

```bash
./wifi_evolution_demo.py --mode loopback --auto-rate --pps 50
# System will automatically adapt rate based on simulated channel conditions
```

### Scenario 4: Over-the-Air with USRP

Two-radio setup for actual RF transmission:

**Radio 1 (TX):**
```bash
# Terminal 1 - Transmitter
./wifi_evolution_demo.py \
    --mode usrp \
    --rate 11M_SHORT \
    --freq 2.437e9 \
    --tx-gain 30 \
    --usrp-args "type=b200,serial=ABC123"
```

**Radio 2 (RX):**
```bash
# Terminal 2 - Receiver
./wifi_evolution_demo.py \
    --mode usrp \
    --rate 11M_SHORT \
    --freq 2.437e9 \
    --rx-gain 40 \
    --usrp-args "type=b200,serial=XYZ789"
```

## Performance Expectations

### Loopback Mode (Ideal Channel)

| Standard | Rate | Expected Throughput | PER |
|----------|------|---------------------|-----|
| 802.11b | 1 Mbps | ~0.95 Mbps | <0.1% |
| 802.11b | 11 Mbps (short) | ~10.5 Mbps | <0.1% |
| 802.11a/g | 6 Mbps | ~5.7 Mbps | <0.1% |
| 802.11a/g | 54 Mbps | ~51 Mbps | <0.1% |

### USRP Mode (Real RF Channel)

Performance depends on:
- Distance between radios
- TX/RX gain settings
- Antenna type and placement
- RF environment (interference, multipath)
- USRP model and sample rate accuracy

**Typical ranges**:
- 1 Mbps DSSS: Up to 100m outdoor
- 11 Mbps CCK: Up to 50m outdoor
- 54 Mbps OFDM: Up to 20m outdoor

## Troubleshooting

### Issue: "rich library not available"

**Solution**: Install rich library
```bash
pip3 install rich
```
Or use simple UI mode:
```bash
./wifi_evolution_demo.py --simple-ui
```

### Issue: UHD driver errors with USRP

**Solution**: Check USRP connection and firmware
```bash
uhd_find_devices
uhd_usrp_probe --args="type=b200"
```

### Issue: No packets received in loopback mode

**Possible causes**:
1. Noise level too high - reduce `--noise-voltage`
2. Wrong rate selected for RX - ensure TX/RX rates match
3. Correlation threshold too strict - lower threshold value

### Issue: GRC flowgraph crashes on startup

**Solution**: Check GNU Radio version
```bash
gnuradio-config-info --version  # Should be 3.10+
```

Ensure all gr-ieee80211 blocks are installed:
```bash
cd build
sudo make install
sudo ldconfig
```

### Issue: Poor performance with USRP

**Solutions**:
1. Increase TX gain (but avoid saturation)
2. Increase RX gain (watch for ADC overrange)
3. Improve antenna setup (line-of-sight, proper orientation)
4. Check for interference (use spectrum analyzer view)
5. Reduce distance between radios

## Advanced Usage

### Custom Packet Payloads

Modify `wifi_evolution_demo.py` to send custom data:

```python
# Replace in message_strobe initialization:
custom_data = b"Hello WiFi!"
msg = pmt.cons(pmt.PMT_NIL, pmt.make_u8vector(len(custom_data), ord(custom_data[0])))
self.msg_source = blocks.message_strobe(msg, 100)
```

### Logging and Analysis

Enable detailed logging for analysis:

```bash
# Export GR_LOG_LEVEL for detailed logs
export GR_LOG_LEVEL=DEBUG
./wifi_evolution_demo.py --mode loopback > wifi_test.log 2>&1
```

### Integration with Wireshark

Capture packets for analysis:

```python
# Add to flowgraph:
self.pcap_writer = blocks.file_sink(gr.sizeof_char, "wifi_capture.pcap")
# Connect after packet decode
```

## Educational Use Cases

### 1. Understanding Modulation Schemes

- Compare DBPSK (1 Mbps) vs. DQPSK (2 Mbps) vs. CCK (5.5/11 Mbps)
- Observe constellation diagrams in GRC
- Analyze spectral efficiency

### 2. Channel Coding Trade-offs

- Compare OFDM rates with different coding rates (1/2, 2/3, 3/4)
- Understand robustness vs. throughput trade-offs
- Experiment with adaptive modulation

### 3. Spread Spectrum Concepts

- Visualize DSSS spreading with Barker codes
- Compare narrowband vs. wideband signals
- Understand processing gain

### 4. WiFi Evolution

- Demonstrate 20-year PHY evolution (1997-2017)
- Compare data rates: 1 Mbps → 866+ Mbps (866x improvement!)
- Understand technological advances (MIMO, wider channels, higher-order modulation)

## References

- IEEE 802.11-2020 Standard
- IEEE 802.11b-1999 (DSSS/CCK)
- IEEE 802.11a-1999 (OFDM)
- IEEE 802.11g-2003 (Extended Rate PHY)
- IEEE 802.11n-2009 (High Throughput)
- IEEE 802.11ac-2013 (Very High Throughput)
- gr-ieee80211 documentation: https://github.com/bastibl/gr-ieee80211

## License

GNU General Public License v3.0 or later (SPDX: GPL-3.0-or-later)

## Contributing

Contributions are welcome! Please submit pull requests or open issues on the gr-ieee80211 repository.

## Support

For questions or issues:
1. Check this README and troubleshooting section
2. Review gr-ieee80211 documentation
3. Open an issue on GitHub
4. Join GNU Radio mailing list or discuss.gnuradio.org

---

**Happy experimenting with WiFi evolution!** 📡🚀
