# 802.11b DSSS/CCK Transmission Chain - Complete Documentation

## Overview

This directory contains comprehensive analysis of the DSSS/CCK (Direct Sequence Spread Spectrum / Complementary Code Keying) transmission chain implementation in gr-ieee80211 for 802.11b physical layer encoding.

## Key Components Analyzed

### 1. PLCP Prefixer (`ppdu_prefixer.cc/h`)
- Preamble generation (Long: 18 bytes, Short: 9 bytes)
- PLCP header construction (SIGNAL, SERVICE, LENGTH, CRC-16)
- 7-bit LFSR scrambler implementation
- Rate-specific processing (0-6 rates supported)

### 2. PPDU Chip Mapper (`ppdu_chip_mapper_bc_impl.cc/h`)
- Rate detection and configuration
- Four distinct modulation schemes:
  - **1M:** DBPSK (Differential Binary PSK) + Barker-11 spreading
  - **2M:** DQPSK (Differential Quadrature PSK) + Barker-11 spreading
  - **5.5M:** CCK (Complementary Code Keying) - 16 chips/byte
  - **11M:** CCK optimized - 8 chips/byte
- Phase accumulation for differential encoding
- Barker code spreading (1M, 2M, and appended terminator)

## Supported Rates

| Rate | Modulation | Chips/Byte | Preamble | Symbol Duration | Use Case |
|------|-----------|-----------|----------|------------------|----------|
| 1M | DBPSK | 88 (11×8) | Long | 88 µs | Long range, low SNR |
| 2M | DQPSK | 44 (11×4) | Long | 44 µs | Baseline throughput |
| 5.5M | CCK | 16 | Long/Short | 1.45 µs | Medium range, good SNR |
| 11M | CCK | 8 | Long/Short | 0.727 µs | High throughput, near range |

With both long and short preambles, supporting 7 total rate configurations.

## Documentation Files

### 1. `DSSS_ANALYSIS.md` (28 KB)
**Complete technical analysis document**

Contents:
- Executive summary and physical layer overview
- Detailed component analysis (ppdu_prefixer, ppdu_chip_mapper)
- Complete PLCP preamble and header structure
- Scrambler implementation details (7-bit LFSR)
- Rate-specific modulation schemes with code examples:
  - DBPSK (1M) with phase mapping
  - DQPSK (2M) with symbol extraction
  - CCK (5.5M) encoding process
  - CCK (11M) optimized encoding
- Phase accumulation mechanism
- Message flow examples (100-byte packet at 11M)
- Timing analysis and transmission efficiency calculations
- Tag propagation through message chain
- Implementation details and critical constants
- Performance characteristics and overhead analysis

**Best for:**
- Understanding the complete TX chain architecture
- Detailed modulation scheme reference
- Message format and timing calculations
- Implementation code reference

### 2. `DSSS_FLOWCHARTS.md` (49 KB)
**Detailed transmission flow diagrams and visualizations**

Contents:
- High-level packet transmission pipeline (3 stages)
- PPDU Prefixer detailed message handler flow
- Chip Mapper rate selection and processing
- Phase accumulation state machine
- CCK 5.5M encoding flow with phase component extraction
- Timeline diagram: 100-byte packet at 11M
  - Byte-by-byte breakdown
  - Cumulative chip counts
  - Overhead analysis
  - Performance metrics
- PLCP header CRC-16 calculation with polynomial feedback
- Tag propagation through message chain
- Barker code spreading visualization
- Symbol timeline for 5.5 Mbps
- State transitions in chip mapper

**Best for:**
- Visual understanding of data flow
- Step-by-step processing sequences
- Timing and duration calculations
- Tag propagation understanding
- Implementation debugging

### 3. `DSSS_QUICK_REFERENCE.md` (6.4 KB)
**Quick lookup reference guide**

Contents:
- Rate enumeration and constants
- Preamble and header field values
- Phase lookup tables
- Barker code definition
- Key function signatures
- Rate-specific chip output counts

**Best for:**
- Quick lookups during development
- Field value reference
- Constant verification

### 4. `DSSS_CCK_RECEPTION_ANALYSIS.md` (33 KB)
**Receiver-side analysis and complementary information**

Contents:
- Reception chain overview
- Chip synchronization (chip_sync_c)
- Barker code correlation properties
- Rate detection from SIGNAL field
- Demodulation strategies for each rate
- Phase tracking in receiver

**Best for:**
- Understanding receiver implementation
- Correlating TX and RX processing
- Testing and validation

## Message Flow Summary

```
User Packets
    ↓
PPDU Prefixer
├─ Adds preamble (18 or 9 bytes)
├─ Generates PLCP header (6 bytes)
├─ Applies LFSR scrambling
└─ Embeds rate tag in first byte

Message: [rate_tag | scrambled_ppdu]
    ↓
PPDU Chip Mapper
├─ Detects rate from tag
├─ Processes preamble @ 1M (88 chips/byte)
├─ Processes header @ 1M or 2M (88 or 44 chips/byte)
├─ Processes PSDU @ rate-specific modulation
│  ├─ 1M: 88 chips/byte
│  ├─ 2M: 44 chips/byte
│  ├─ 5.5M: 16 chips/byte
│  └─ 11M: 8 chips/byte
└─ Appends Barker terminator (11 chips)

Stream: Complex baseband chips @ 11 MHz
    ↓
Downstream Processing
├─ Optional: Frequency offset correction
├─ Optional: Pulse shaping filter
└─ Output: Modulated RF or file
```

## Transmission Timeline Example

**100-byte packet at 11 Mbps (long preamble):**

| Component | Bytes | Chips | Duration (µs) | % Total |
|-----------|-------|-------|---------------|---------|
| Preamble | 18 | 1,584 | 144.0 | 54.1% |
| Header | 6 | 528 | 48.0 | 18.1% |
| PSDU | 100 | 800 | 72.7 | 27.4% |
| Append | - | 11 | 1.0 | 0.4% |
| **TOTAL** | **124** | **2,923** | **265.7** | **100%** |

**Efficiency:** 27.4% (800 bits payload / 2,923 chips)

## Key Technical Insights

### 1. Differential Phase Encoding
- All modulations use phase accumulation: `phase = Σ(increments) mod 2π`
- Maintains phase continuity across symbols
- Enables receiver to track phase drift without absolute reference

### 2. Barker Code Properties
- Barker-11: [1, -1, 1, 1, -1, 1, 1, 1, -1, -1, -1]
- Autocorrelation: 11 at lag 0, ≤1 at other lags
- Provides 10.4 dB processing gain
- Used in 1M and 2M (with Barker spreading)

### 3. CCK Efficiency
- 5.5M: 4 bits encoded in 16 chips = 0.25 bits/chip
- 11M: 8 bits encoded in 8 chips = 1 bit/chip
- No explicit spreading code like Barker
- Implicit spreading through phase combinations

### 4. LFSR Scrambler
- 7-bit feedback taps at positions 3 and 6
- Two initial states:
  - Long preamble: 0x1B
  - Short preamble: 0x6C
- Ensures bit transitions for clock recovery
- Self-synchronizing (no training sequence needed)

### 5. Rate Flexibility
- Single block implementation supports all 7 rate configurations
- Rate selected at runtime via embedded tag
- Function pointer enables efficient rate switching
- Preamble/header always at low rate (1M) for detection

## File Locations

```
/home/user/gr-ieee80211/
├── DSSS_ANALYSIS.md                    # Main technical analysis
├── DSSS_FLOWCHARTS.md                  # Flow diagrams and timelines
├── DSSS_QUICK_REFERENCE.md             # Quick lookup reference
├── DSSS_CCK_RECEPTION_ANALYSIS.md      # Receiver-side analysis
│
├── include/gnuradio/ieee80211/dsss/
│   ├── ppdu_prefixer.h                 # Header interface
│   ├── ppdu_chip_mapper_bc.h           # Chip mapper interface
│   └── chip_sync_c.h                   # Receiver synchronization
│
├── lib/dsss/
│   ├── ppdu_prefixer.cc                # PLCP prefixer implementation
│   ├── ppdu_chip_mapper_bc_impl.cc     # Chip mapper implementation
│   ├── ppdu_chip_mapper_bc_impl.h      # Chip mapper internal header
│   ├── chip_sync_c_impl.cc             # Receiver synchronization
│   ├── chip_sync_c_impl.h              # Receiver sync internal header
│   └── qa_dsss.cc                      # Unit tests
│
└── examples/dsss/
    ├── dsss_loopback.py                # Loopback example (TX/RX)
    ├── dsss_loopback.grc               # GRC flowgraph
    ├── multi_mode_transceiver.py       # Multi-rate transceiver
    └── usrp_dsss_transceiver.grc       # USRP hardware example
```

## Code Structure Reference

### ppdu_prefixer Implementation
```cpp
class ppdu_prefixer_impl : public ppdu_prefixer {
    // Rate enumeration (0-6)
    enum DSSSRATE { LONG1M, LONG2M, LONG5_5M, LONG11M, 
                    SHORT2M, SHORT5_5M, SHORT11M };
    
    // Message handler
    void psdu_in(pmt::pmt_t msg);
    
    // Internal processing
    void placeHeader(int psduLen);      // SIGNAL/SERVICE/LENGTH/CRC
    void scrambler();                    // LFSR scrambling
    void update_rate(int rate);          // Rate configuration
    
    // Data storage
    unsigned char d_buf[8192];           // PPDU construction
    unsigned char d_spread_buf[8192];   // Scrambled output
    int d_ppdu_index;                    // Current position
};
```

### ppdu_chip_mapper_bc_impl Implementation
```cpp
class ppdu_chip_mapper_bc_impl : public ppdu_chip_mapper_bc {
    // Rate enumeration (same as prefixer)
    enum DSSSRATE { ... };
    
    // Modulation functions
    int dbpsk_1M_chips(gr_complex* out, uint8_t byte, bool even);
    int dqpsk_2M_chips(gr_complex* out, uint8_t byte, bool even);
    int cck_5_5M_chips(gr_complex* out, uint8_t byte, bool even);
    int cck_11M_chips(gr_complex* out, uint8_t byte, bool even);
    
    // Processing
    int chipGen(...);                    // Rate-agnostic generator
    bool updateRate(unsigned char raw);  // Configure for rate
    
    // State management
    int d_count;                         // Total PPDU bytes
    int d_copy;                          // Bytes processed
    float d_phase_acc;                   // Phase accumulator
    int d_psdu_symbol_num;               // Symbols per byte
};
```

## Testing and Validation

The repository includes unit tests in `qa_dsss.cc`:

```cpp
// Test Barker code properties
BOOST_AUTO_TEST_CASE(test_barker_code_correlation) {
    // Autocorrelation at lag 0 should be 11
    // Autocorrelation at non-zero lags should be ≤ 1
}

// Test DSSS rate configuration
BOOST_AUTO_TEST_CASE(test_prefixer_creation) {
    // Test creating prefixer with each rate (0-6)
}

// Test chip mapper functionality
BOOST_AUTO_TEST_CASE(test_chip_mapper_creation) {
    // Verify mapper can be instantiated
}

// Test chip sync integration
BOOST_AUTO_TEST_CASE(test_chip_sync_creation) {
    // Test with long and short preambles
}
```

Run tests with:
```bash
cd /home/user/gr-ieee80211
make test
```

## Example Usage

### Python Loopback Example
```python
from gnuradio import gr, blocks, analog, ieee80211

tb = gr.top_block()

# Create packet source
pkt = pmt.make_u8vector(100, 0x42)
msg_source = blocks.message_strobe(pkt, 100)

# DSSS transmitter chain
ppdu_prefixer = ieee80211.ppdu_prefixer(rate=3)  # 11M long
chip_mapper = ieee80211.ppdu_chip_mapper_bc("packet_len")

# DSSS receiver (loopback)
chip_sync = ieee80211.chip_sync_c(long_preamble=True, threshold=2.3)

# Connections
tb.msg_connect((msg_source, 'out'), (ppdu_prefixer, 'psdu_in'))
tb.msg_connect((ppdu_prefixer, 'ppdu_out'), (chip_mapper, 'in'))
tb.connect((chip_mapper, 0), (chip_sync, 0))

tb.start()
tb.run()
```

## Related Documentation

- **IEEE 802.11b Standard:** Direct Sequence Spread Spectrum (DSSS) for 1, 2, 5.5, and 11 Mbps
- **CCIR Report 328-3:** Revision of modulation and bandwidth allocations
- **GNU Radio Documentation:** Message passing architecture, stream processing

## Summary

This documentation provides a complete reference for understanding and working with the 802.11b DSSS/CCK transmission implementation in gr-ieee80211:

1. **DSSS_ANALYSIS.md** - Comprehensive technical details and code analysis
2. **DSSS_FLOWCHARTS.md** - Visual representations and step-by-step processing flows
3. **DSSS_QUICK_REFERENCE.md** - Lookup tables and constants
4. **DSSS_CCK_RECEPTION_ANALYSIS.md** - Complementary receiver analysis

All documents cross-reference source files in:
- `/home/user/gr-ieee80211/lib/dsss/`
- `/home/user/gr-ieee80211/include/gnuradio/ieee80211/dsss/`

For questions or updates, refer to the gr-ieee80211 repository documentation and IEEE 802.11b specifications.

---

**Generated:** November 21, 2025
**Repository:** gr-ieee80211
**Branch:** claude/integrate-dsss-support-01VK7BT3wJt62TUfYDwpgn8t
