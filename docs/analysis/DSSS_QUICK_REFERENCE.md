# DSSS/CCK Reception Chain - Quick Reference Guide

## Key File Locations

| Component | Header File | Implementation File |
|-----------|-------------|-------------------|
| **Synchronization & Demodulation** | `include/gnuradio/ieee80211/dsss/chip_sync_c.h` | `lib/dsss/chip_sync_c_impl.cc` |
| **Chip Mapping (TX)** | `include/gnuradio/ieee80211/dsss/ppdu_chip_mapper_bc.h` | `lib/dsss/ppdu_chip_mapper_bc_impl.cc` |
| **PPDU Prefixer (TX)** | `include/gnuradio/ieee80211/dsss/ppdu_prefixer.h` | `lib/dsss/ppdu_prefixer.cc` |
| **Utility Functions** | `include/gnuradio/ieee80211/utils.h` | `lib/utils.cc` |
| **Unit Tests** | - | `lib/dsss/qa_dsss.cc` |

---

## Core Algorithm Parameters

### Barker Code (11-chip synchronization sequence)
```
Sequence: [1, -1, 1, 1, -1, 1, 1, 1, -1, -1, -1]
Autocorrelation peak: 11 (at lag 0)
Sidelobe suppression: ≤ 1 (at non-zero lags)
```

### PLL Configuration
```
Loop bandwidth: 0.0314 rad/sample (≈500 kHz @ 11 MHz)
Damping ratio: √2/2 (critically damped)
Phase coefficients:
  - d_alpha = phase update gain
  - d_beta = frequency update gain
```

### Threshold Settings
```
Default: 2.3 (normalized, 0-11 range)
Formula: user_threshold / sqrt(11)
CCK Adjustment: threshold × 16×√22/121 ≈ threshold × 0.496
```

---

## Reception States

```
SEARCH  → Detect packet start (Barker sync)
  ↓ (Sync found)
SYNC    → Decode PLCP header (48 bits)
  ↓ (Valid CRC-16)
PSDU    → Decode data payload (rate-dependent)
  ↓ (Frame complete)
Output  → Send PMT message with decoded data
```

---

## Rate Detection Table

| SIGNAL Field | Rate | Modulation | Bits/Symbol | Chips/Symbol |
|--------------|------|-----------|-------------|--------------|
| 0x0A | 1 Mbps | DBPSK | 1 | 11 (Barker) |
| 0x14 | 2 Mbps | DQPSK | 2 | 11 (Barker) |
| 0x37 | 5.5 Mbps | CCK | 4 | 8 |
| 0x6E | 11 Mbps | CCK | 8 | 8 |

---

## CCK Chip Correlations

### 5.5 Mbps: 4 sequences (maps 2-bit index + 2-bit phase)
```
d_cck5_5_chips[4][8]  // 4 sequences × 8 chips each
```

### 11 Mbps: 64 sequences (maps 6-bit index + 2-bit phase)
```
d_cck11_chips[64][8]  // 64 sequences × 8 chips each
```

---

## Descrambler Configuration

| Preamble Type | Initial State |
|---------------|---------------|
| Long (16 bytes) | 0x1B |
| Short (9 bytes) | 0x6C |

**Polynomial:** x^7 + x^4 + 1 (7-bit LFSR)
**Taps:** Bit 3 and Bit 6
**Operation:** Self-inverse (same circuit for scrambling/descrambling)

---

## PLCP Header Structure (6 bytes)

```
Byte 0: SIGNAL (rate indicator)
Byte 1: SERVICE (flags)
Bytes 2-3: LENGTH (16 bits, microseconds)
Bytes 4-5: CRC-16 (CCITT)

CRC Polynomial: x^16 + x^12 + x^5 + 1
Initial Value: 0xFFFF
Validation: (CRC_calculated XOR CRC_received) == 0xFFFF
```

---

## Important Constants

```cpp
// Synchronization words (post-descrambling)
Long preamble:  0x05CF
Short preamble: 0xF3A0

// Threshold adjustment for CCK
d_cck_thres_adjust = 16.0 * sqrt(22.0) / 121.0 ≈ 0.496

// Chip processing timing
DBPSK/DQPSK: 11 chips/symbol
CCK 5.5M: 8 chips/symbol, 2 bits from phase encoding
CCK 11M: 8 chips/symbol, 6 bits from CCK index + 2 bits from phase
```

---

## VOLK Optimized Functions

```cpp
// Energy calculation (conjugate dot product)
volk_32fc_x2_conjugate_dot_prod_32fc(&energy, in, in, length);

// Barker correlation
volk_32fc_32f_dot_prod_32fc(&corr, in, d_barker, 11);

// CCK sequence correlation
volk_32fc_x2_conjugate_dot_prod_32fc(&corr, in, d_cck_chips[i], 8);
```

---

## Error Handling

| State | Failure Condition | Recovery |
|-------|------------------|----------|
| SEARCH | abs(correlation) < threshold | Continue searching |
| SYNC | CRC-16 mismatch | Return to SEARCH |
| SYNC | Unknown SIGNAL value | Return to SEARCH |
| PSDU | abs(correlation) < threshold | Return to SEARCH |

---

## Python API Example

```python
from gnuradio import ieee80211

# Create receiver
chip_sync = ieee80211.chip_sync_c(
    long_preamble=True,   # True for long, False for short
    threshold=2.3         # 0-11, higher = more robust
)

# Connect signals
self.connect((input_signal, 0), (chip_sync, 0))

# Connect message output
self.msg_connect((chip_sync, 'psdu_out'), (output_handler, 'in'))

# Runtime preamble type switching
chip_sync.set_preamble_type(False)  # Switch to short
```

---

## Performance Expectations

| Metric | Value | Notes |
|--------|-------|-------|
| Sampling Rate | 11 Msps | Standard for all rates |
| Min SNR (11M) | ~5-7 dB | Depends on threshold |
| PLL Convergence | ~100-200 µs | Typical acquisition time |
| Memory per Frame | 8 KB max | PSDU buffer size |
| CPU Load | Low | Single-threaded, real-time capable |

---

## Troubleshooting Checklist

1. **Not detecting packets?**
   - Check threshold setting (too high?)
   - Verify input signal level (normalized?)
   - Check preamble type selection (long vs short)

2. **CRC failures?**
   - Signal might be at wrong rate
   - Check for phase offsets
   - Verify descrambler initialization

3. **Corrupted PSDU data?**
   - Increase SNR (reduce noise)
   - Adjust threshold lower (if not losing detection)
   - Check CCK threshold adjustment is applied

4. **Poor sensitivity?**
   - Lower threshold setting
   - Check PLL parameters (loop bandwidth)
   - Verify input impedance matching

---

## Signal Flow Diagram (Quick Version)

```
Raw I/Q → Barker Sync → Rate Detection → Rate-Specific Decode
                ↓              ↓                    ↓
          (11 chips)      (PLCP Header)      (PSDU Bits)
                ↓              ↓                    ↓
            PLL(BPSK)    CRC-16 Check         Descramble
                ↓              ↓                    ↓
          Normalize        Extract Rate       Extract Bits
                ↓              ↓                    ↓
         Compare Threshold   Select Decoder   Buffer & Output
                ↓              ↓                    ↓
          Sync Found?    Header Valid?        Frame Complete?
                │              │                    │
                └──────────────┴────────────────────┘
                              │
                              ▼
                      Output: PSDU Message
```

---

## References

- **IEEE 802.11-2020:** Wireless LANs standard
- **GNU Radio Documentation:** https://wiki.gnuradio.org/
- **Test Files:** `lib/dsss/qa_dsss.cc`, `examples/dsss/dsss_loopback.py`

