# 802.11b DSSS/CCK Integration Guide

## Overview

This document describes the integration of IEEE 802.11b DSSS/CCK physical layer support into gr-ieee80211, creating a unified WiFi platform supporting 802.11b/a/g/n/ac (1 Mbps to 866 Mbps).

## Architecture

### Block Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                  802.11b Transmit Chain                      │
└─────────────────────────────────────────────────────────────┘

MAC Frame (bytes)
     │
     ▼
┌──────────────────┐
│ ppdu_prefixer    │  ← Adds PLCP preamble & header
│ (message block)  │  ← Scrambles data
└────────┬─────────┘  ← Calculates CRC-16
         │ PPDU (bytes)
         ▼
┌──────────────────┐
│ ppdu_chip_mapper │  ← Maps bytes to chips
│   (stream block) │  ← DBPSK/DQPSK/CCK modulation
└────────┬─────────┘  ← Barker/CCK spreading
         │ Complex chips (11 Msps)
         ▼
    [USRP Sink]


┌─────────────────────────────────────────────────────────────┐
│                  802.11b Receive Chain                       │
└─────────────────────────────────────────────────────────────┘

[USRP Source]
     │ Complex samples (11 Msps)
     ▼
┌──────────────────┐
│   chip_sync_c    │  ← Barker correlation sync
│ (stream→message) │  ← PLL carrier tracking
└────────┬─────────┘  ← DBPSK/DQPSK/CCK demod
         │ MAC Frame (message)
         ▼
  [MAC Processing]
```

## Implementation Details

### Namespace Organization

All DSSS/CCK code resides in the unified `gr::ieee80211` namespace:

```cpp
namespace gr {
namespace ieee80211 {
    // OFDM blocks (existing)
    class sync : public gr::block { ... };
    class demod : public gr::block { ... };

    // DSSS/CCK blocks (new)
    class chip_sync_c : public gr::block { ... };
    class ppdu_chip_mapper_bc : public gr::block { ... };
    class ppdu_prefixer : public gr::block { ... };
}
}
```

### File Structure

```
gr-ieee80211/
├── lib/
│   ├── dsss/
│   │   ├── chip_sync_c_impl.cc        (Receiver implementation)
│   │   ├── chip_sync_c_impl.h
│   │   ├── ppdu_chip_mapper_bc_impl.cc (Transmitter implementation)
│   │   ├── ppdu_chip_mapper_bc_impl.h
│   │   └── ppdu_prefixer.cc           (PLCP header generation)
│   └── [existing OFDM files]
├── include/gnuradio/ieee80211/
│   ├── dsss/
│   │   ├── chip_sync_c.h              (Public API)
│   │   ├── ppdu_chip_mapper_bc.h
│   │   └── ppdu_prefixer.h
│   ├── wifi_rates.h                    (Unified rate enumeration)
│   ├── utils.h                         (FCS, scrambling utilities)
│   └── [existing OFDM headers]
├── python/ieee80211/bindings/
│   ├── chip_sync_c_python.cc          (Pybind11 bindings)
│   ├── ppdu_chip_mapper_bc_python.cc
│   ├── ppdu_prefixer_python.cc
│   └── [existing OFDM bindings]
├── grc/
│   ├── ieee80211_chip_sync_c.block.yml     (GRC definitions)
│   ├── ieee80211_ppdu_chip_mapper_bc.block.yml
│   ├── ieee80211_ppdu_prefixer.block.yml
│   └── [existing OFDM blocks]
└── docs/dsss/
    ├── INTEGRATION_GUIDE.md (this file)
    └── USER_GUIDE.md
```

## Technical Specifications

### PLCP Frame Format

```
┌────────────┬─────┬────────────┐
│  PREAMBLE  │ SFD │   HEADER   │   PSDU (MAC frame)
└────────────┴─────┴────────────┘

Long Preamble:
  PREAMBLE: 128 bits of alternating 0s and 1s (scrambled)
  SFD: 16 bits = 0xF3A0
  HEADER: 48 bits (SIGNAL, SERVICE, LENGTH, CRC-16)

Short Preamble:
  PREAMBLE: 56 bits of alternating 0s and 1s (scrambled)
  SFD: 16 bits = 0x05CF
  HEADER: 48 bits (same as long)
```

### SIGNAL Field Encoding

| Rate    | SIGNAL Byte | Modulation |
|---------|-------------|------------|
| 1 Mbps  | 0x0A        | DBPSK      |
| 2 Mbps  | 0x14        | DQPSK      |
| 5.5 Mbps| 0x37        | CCK        |
| 11 Mbps | 0x6E        | CCK        |

### SERVICE Field

- Bits 0-2: Reserved (set to 0)
- Bit 3: Length extension (11 Mbps only)
- Bits 4-7: Reserved (set to 0)

### Scrambler

7-bit LFSR with polynomial: S(x) = x^7 + x^4 + 1

Initial states:
- Long preamble: 0x1B
- Short preamble: 0x6C

### CRC-16 (PLCP Header)

Polynomial: x^16 + x^12 + x^5 + 1 (CCITT CRC-16)
Covers: SIGNAL, SERVICE, LENGTH fields (32 bits)

## Modulation Details

### 1 Mbps - DBPSK with Barker Spreading

```
Input: 1 bit
Output: 11 chips (Barker sequence)

Barker code: [1, -1, 1, 1, -1, 1, 1, 1, -1, -1, -1]

Phase mapping:
  0 → 0°
  1 → 180°

Differential encoding:
  symbol[n] = symbol[n-1] * phase[bit]
```

### 2 Mbps - DQPSK with Barker Spreading

```
Input: 2 bits (dibits)
Output: 11 chips (Barker sequence)

Phase mapping (Gray code):
  00 → 0°
  01 → 90°
  11 → 180°
  10 → 270°

Differential encoding applied
```

### 5.5 Mbps - CCK

```
Input: 4 bits per symbol (two dibits)
Output: 8 complex chips

CCK encoding:
  4 phase terms (φ1, φ2, φ3, φ4)
  φ1: QPSK from dibit 1
  φ2, φ3, φ4: DQPSK from dibit 2

Chip sequence:
  c0 = exp(j(φ1 + φ2 + φ3 + φ4))
  c1 = exp(j(φ1 + φ3 + φ4))
  c2 = exp(j(φ1 + φ2 + φ4))
  c3 = exp(j(φ1 + φ4 + π))
  c4 = exp(j(φ1 + φ2 + φ3))
  c5 = exp(j(φ1 + φ3))
  c6 = exp(j(φ1 + φ2 + π))
  c7 = exp(j(φ1))

4 possible CCK codes per symbol
```

### 11 Mbps - CCK

```
Input: 8 bits per symbol (four dibits)
Output: 8 complex chips

Similar to 5.5 Mbps but with all 4 phase terms varying

64 possible CCK codes per symbol
```

## Receiver Architecture

### State Machine

```
┌─────────┐
│ SEARCH  │ ← Scan for Barker correlation peak
└────┬────┘
     │ Peak detected (threshold exceeded)
     ▼
┌─────────┐
│  SYNC   │ ← Lock PLL, decode PLCP header
└────┬────┘
     │ Header CRC valid
     ▼
┌─────────┐
│  PSDU   │ ← Decode MAC frame
└────┬────┘
     │ Frame complete
     ▼
Output message
     │
     └──→ Return to SEARCH
```

### Synchronization

**Barker Correlation:**
```
corr = Σ(received[i] * conj(barker[i])) for i = 0..10

If |corr| > threshold * sqrt(11):
    Packet detected
```

**PLL for Carrier Tracking:**
```
Loop filter: 2nd order
Damping: sqrt(0.5)
Loop bandwidth: 0.0314

Phase update:
  error = arg(symbol * conj(expected))
  freq += beta * error
  phase += alpha * error + freq
```

## Build System Integration

### CMakeLists.txt Changes

**lib/CMakeLists.txt:**
```cmake
list(APPEND ieee80211_sources
    # ... existing files ...
    dsss/chip_sync_c_impl.cc
    dsss/ppdu_chip_mapper_bc_impl.cc
    dsss/ppdu_prefixer.cc
)
```

**include/gnuradio/ieee80211/CMakeLists.txt:**
```cmake
install(FILES
    # ... existing headers ...
    wifi_rates.h
    utils.h
    DESTINATION include/gnuradio/ieee80211
)

install(FILES
    dsss/chip_sync_c.h
    dsss/ppdu_chip_mapper_bc.h
    dsss/ppdu_prefixer.h
    DESTINATION include/gnuradio/ieee80211/dsss
)
```

**python/ieee80211/bindings/CMakeLists.txt:**
```cmake
list(APPEND ieee80211_python_files
    # ... existing bindings ...
    chip_sync_c_python.cc
    ppdu_chip_mapper_bc_python.cc
    ppdu_prefixer_python.cc
    python_bindings.cc
)
```

**grc/CMakeLists.txt:**
```cmake
install(FILES
    # ... existing blocks ...
    ieee80211_chip_sync_c.block.yml
    ieee80211_ppdu_chip_mapper_bc.block.yml
    ieee80211_ppdu_prefixer.block.yml
    DESTINATION share/gnuradio/grc/blocks
)
```

## GNU Radio 3.10 Compatibility

### Modernization Changes

**Threading:**
```cpp
// Before (GR 3.7)
#include <gr/thread/thread.h>
gr::thread::mutex d_mutex;
gr::thread::scoped_lock guard(d_mutex);

// After (GR 3.10)
#include <mutex>
std::mutex d_mutex;
std::lock_guard<std::mutex> guard(d_mutex);
```

**Smart Pointers:**
```cpp
// Before
#include <boost/shared_ptr.hpp>
typedef boost::shared_ptr<chip_sync_c> sptr;

// After
#include <memory>
typedef std::shared_ptr<chip_sync_c> sptr;
```

**Message Handlers:**
```cpp
// Before
set_msg_handler(port, boost::bind(&impl::handler, this, _1));

// After
set_msg_handler(port, [this](pmt::pmt_t msg){ handler(msg); });
```

**Python Bindings:**
```cpp
// Before: SWIG
%include "chip_sync_c.h"

// After: Pybind11
#include <pybind11/pybind11.h>
void bind_chip_sync_c(py::module& m) {
    py::class_<chip_sync_c, std::shared_ptr<chip_sync_c>>(m, "chip_sync_c")
        .def(py::init(&chip_sync_c::make), ...);
}
```

**GRC Blocks:**
```yaml
# Before: XML format (.xml)
<?xml version="1.0"?>
<block>
  <name>Chip Sync</name>
  ...
</block>

# After: YAML format (.block.yml)
id: ieee80211_chip_sync_c
label: 802.11b DSSS Packet Sink
category: '[IEEE 802.11 GR-WiFi]'
```

## Performance Considerations

### Sample Rate

All DSSS/CCK modes use **11 Msps** (Mega-samples per second):
- 1 Mbps: 11 samples per bit (Barker spreading)
- 2 Mbps: 5.5 samples per bit
- 5.5 Mbps: 2 samples per bit
- 11 Mbps: 1 sample per bit

### Processing Complexity

| Block | Operation | Complexity |
|-------|-----------|------------|
| ppdu_prefixer | Scrambling, CRC | O(n) |
| chip_mapper | Table lookup | O(1) per symbol |
| chip_sync | Barker correlation | O(11) per sample |
| chip_sync | CCK correlation | O(64) per symbol (11M) |

### Optimization Opportunities

1. **VOLK kernels** for CCK correlation (future enhancement)
2. **Dynamic buffer sizing** instead of fixed 8192 bytes
3. **Zero-copy message passing** where possible

## Testing

### Unit Tests

Create test files in `lib/dsss/`:
```cpp
// qa_chip_sync_c.cc
BOOST_AUTO_TEST_CASE(test_barker_correlation) {
    // Test Barker code correlation
}

BOOST_AUTO_TEST_CASE(test_cck_demod) {
    // Test CCK demodulation
}
```

### Integration Tests

1. **Loopback test:** TX → RX in same process
2. **File-based test:** Pre-recorded signals
3. **USRP test:** Hardware validation

## Troubleshooting

### Common Issues

**1. No packet detection:**
- Check correlation threshold (try 2.0 - 3.0)
- Verify sample rate is 11 Msps
- Check AGC gain settings

**2. CRC failures:**
- Verify scrambler initialization state
- Check PLCP header generation
- Validate length calculation

**3. Build errors:**
- Ensure GNU Radio 3.10+ installed
- Check C++17 compiler support
- Verify all headers installed

### Debug Output

Enable debug output in implementation:
```cpp
#define d_debug 1  // In chip_sync_c_impl.cc
```

This enables console logging:
```
Barker peak detected: 8.43
PLCP header decoded: rate=11M, len=100
PSDU complete: 100 bytes
```

## Future Enhancements

### Planned Features

1. **VOLK Optimization**
   - CCK correlation kernels
   - Vectorized differential detection

2. **Signal Quality Metrics**
   - RSSI estimation
   - SNR calculation
   - Per-packet statistics

3. **Rate Adaptation**
   - Automatic rate selection
   - Link quality monitoring
   - Fallback mechanisms

4. **Advanced Features**
   - PBCC support (22/33 Mbps)
   - Antenna diversity
   - Transmit power control

### Integration Roadmap

1. **Unified MAC Layer**
   - Common frame structure for DSSS and OFDM
   - Mode-aware packet scheduler
   - Seamless rate adaptation

2. **Multi-Mode Transceiver**
   - Automatic mode detection (DSSS vs OFDM)
   - Dynamic mode switching
   - Hybrid flowgraphs

3. **Interoperability Testing**
   - Commercial WiFi device testing
   - Conformance test suite
   - Performance benchmarking

## References

- IEEE 802.11-2020 Standard
- Section 17: HR/DSSS PHY specification
- gr-wifi-dsss original implementation: https://github.com/r4d10n/gr-wifi-dsss

## Support

For issues or questions:
- GitHub Issues: https://github.com/cloud9477/gr-ieee80211/issues
- Original gr-wifi-dsss: https://github.com/r4d10n/gr-wifi-dsss

---

**Document Version:** 1.0
**Last Updated:** 2025-11-16
**Author:** Integration Team
