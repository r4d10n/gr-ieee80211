# 802.11b DSSS/CCK Transmission Chain Analysis - gr-ieee80211

## Executive Summary

This document provides a detailed analysis of the DSSS/CCK (Direct Sequence Spread Spectrum / Complementary Code Keying) transmission chain in the gr-ieee80211 GNU Radio module. The implementation supports all four 802.11b data rates (1, 2, 5.5, and 11 Mbps) with both long and short PLCP preambles.

---

## 1. Overview of 802.11b Physical Layer

802.11b uses DSSS modulation with the following characteristics:

| Parameter | Value |
|-----------|-------|
| Chip Rate | 11 MHz (for all rates) |
| Spreading Code | Barker-11 code (1, -1, 1, 1, -1, 1, 1, 1, -1, -1, -1) |
| Preamble | Long (192 µs) or Short (96 µs) |
| PLCP Header Duration | 48 µs (at 2 Mbps equivalent) |
| Modulation Schemes | DBPSK (1M), DQPSK (2M), CCK (5.5M, 11M) |
| Bandwidth | 22 MHz |

---

## 2. DSSS/CCK Transmission Chain Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                    802.11b DSSS/CCK TX CHAIN                        │
└─────────────────────────────────────────────────────────────────────┘

User Packets (PSDU)
       │
       ├──────────────────────────────────────────────────────────┐
       │                                                          │
       ▼                                                          │
┌──────────────────────────────────────────┐                     │
│   ppdu_prefixer (PLCP Header Generator)   │◄────────────────┬──┘
│  - Preamble (Long/Short)                 │                 │
│  - SIGNAL/SERVICE Field                  │         update_rate()
│  - Length/CRC Calculation                │                 │
│  - Scrambling                            │                 │
└──────────────────────────────────────────┘                 │
       │                                                      │
       │ PPDU (Message Blob)                                 │
       │ [rate_tag | scrambled_data]                         │
       ▼                                                      │
┌──────────────────────────────────────────┐                 │
│  ppdu_chip_mapper_bc (Chip Generation)    │                 │
│  - Rate detection from tag               │◄────────────────┘
│  - Phase accumulation                    │
│  - Modulation selection                  │
└──────────────────────────────────────────┘
       │
       │ Complex Baseband Chips
       │ (1, 2, 5.5, or 11 MHz symbols)
       ▼
┌──────────────────────────────────────────┐
│   Additional DSP Blocks (Optional)        │
│  - Frequency offset compensation         │
│  - Phase correction                      │
│  - Pulse shaping filter                  │
└──────────────────────────────────────────┘
       │
       ▼
  Complex Baseband Output (11 Msps)
```

---

## 3. Component 1: PLCP Prefixer (`ppdu_prefixer.cc/h`)

### 3.1 Purpose
Adds the PLCP (Physical Layer Convergence Procedure) header which includes:
- Synchronization preamble
- SIGNAL/SERVICE fields
- Length information
- CRC-16 checksum
- Optional scrambling

### 3.2 Rate Enumeration

```cpp
enum DSSSRATE {
    LONG1M,      // 0: 1 Mbps with long preamble
    LONG2M,      // 1: 2 Mbps with long preamble
    LONG5_5M,    // 2: 5.5 Mbps with long preamble
    LONG11M,     // 3: 11 Mbps with long preamble
    SHORT2M,     // 4: 2 Mbps with short preamble
    SHORT5_5M,   // 5: 5.5 Mbps with short preamble
    SHORT11M     // 6: 11 Mbps with short preamble
};
```

### 3.3 PLCP Preamble Structure

#### Long Preamble (18 bytes, 192 µs @ 11 MHz)
```
Sync Field (128 chips):    0xff × 16 bytes = 10101010... (alternating pattern)
SFD (16 chips):            [0xA0, 0xF3] = 1010000011110011
```

#### Short Preamble (9 bytes, 96 µs @ 11 MHz)
```
Sync Field (56 chips):     0x00 × 7 bytes
SFD (16 chips):            [0xCF, 0x05] = 1100111100000101
```

### 3.4 PLCP Header Structure (6 bytes)

```
Byte 0:    SIGNAL Field (data rate indicator)
           ┌─────────────────────────────────────┐
           │ Signal Value for Each Rate:         │
           │ 1M:   0x0A (10 Mbps equivalent)     │
           │ 2M:   0x14 (20 Mbps equivalent)     │
           │ 5.5M: 0x37 (55 Mbps equivalent)     │
           │ 11M:  0x6E (110 Mbps equivalent)    │
           └─────────────────────────────────────┘

Byte 1:    SERVICE Field (extension bit for 11M padding)
           Bit 0-2: Reserved
           Bit 3: Length extension flag (11M only)
           Bit 4-7: Reserved

Bytes 2-3: LENGTH Field (16-bit little-endian)
           = Transmission time in microseconds
             = 8 × PSDU_length / data_rate (in Mbps)

Bytes 4-5: CRC-16 Checksum (CCITT)
           Protects SIGNAL, SERVICE, and LENGTH fields
```

### 3.5 PLCP Header Calculation

```cpp
// For each rate:
switch(d_rate) {
    case LONG1M:
        signal = 0x0A;
        tmpLen = psduLen * 8;              // 1 Mbps = 1 chip/bit
        break;
    case LONG2M:
        signal = 0x14;
        tmpLen = psduLen * 4;              // 2 Mbps = 2 chips/bit
        break;
    case LONG5_5M:
        signal = 0x37;
        tmpLen = psduLen * 8 / 5.5;       // 5.5 Mbps
        break;
    case LONG11M:
        signal = 0x6E;
        tmpLen = 8 * psduLen / 11.0;      // 11 Mbps
        // Extended service bit handling
        if (11.0 * (ceil(tmpLen) - tmpLen) < 8.0)
            service = 0x04;  // No extension
        else
            service = 0x84;  // Extension needed
        break;
}

// LENGTH is transmitted as ceil(tmpLen)
```

### 3.6 Scrambler Implementation

The scrambler uses a 7-bit linear feedback shift register (LFSR):

```
Feedback Taps: Bits 3 and 6
Initial State:
  - Long preamble: 0x1B
  - Short preamble: 0x6C

Scrambling Equation:
  output_bit = input_bit ⊕ state[3] ⊕ state[6]
  new_state = (state << 1) | output_bit
```

**Why Scrambling?**
- Ensures sufficient bit transitions for clock recovery
- Improves spectral characteristics
- Prevents long runs of 1s or 0s

### 3.7 Message Flow (Input/Output)

**Input:** Message port "psdu_in"
```
Incoming PMT message (key, value):
  key:   PMT_NIL
  value: u8vector (packet data, PSDU)
```

**Processing:**
1. Extract PSDU from PMT vector
2. Copy preamble (18 or 9 bytes)
3. Create PLCP header (6 bytes)
4. Copy scrambled PSDU
5. Embed rate tag in first reserved byte

**Output:** Message port "ppdu_out"
```
Outgoing PMT message (key, blob):
  blob contains:
  [rate_byte | scrambled_ppdu_data]
  
  rate_byte:       0-6 (rate identifier)
  scrambled_ppdu:  preamble + header + scrambled_psdu
```

---

## 4. Component 2: PPDU Chip Mapper (`ppdu_chip_mapper_bc*.cc/h`)

### 4.1 Purpose
Converts PPDU bytes to complex baseband chips using rate-specific modulation:
- **1M:** DBPSK (Differential Binary Phase Shift Keying) + Barker spreading
- **2M:** DQPSK (Differential Quadrature PSK) + Barker spreading
- **5.5M:** CCK (Complementary Code Keying) - 16 chips/byte
- **11M:** CCK - 8 chips/byte

### 4.2 Rate-Specific Output Chip Counts

| Rate | Modulation | Chips/Byte | Input Symbol Duration |
|------|------------|-----------|----------------------|
| 1M | DBPSK | 88 (11×8) | 88 µs |
| 2M | DQPSK | 44 (11×4) | 44 µs |
| 5.5M | CCK | 16 | 1.45 µs |
| 11M | CCK | 8 | 0.73 µs |

### 4.3 Rate Detection and Configuration

```cpp
bool updateRate(unsigned char raw) {
    // raw comes from first byte of PPDU (rate tag from prefixer)
    
    switch(raw) {
        case 0: // LONG1M
            d_rateVal = 1.0;
            d_chip_mapper = &ppdu_chip_mapper_bc_impl::dbpsk_1M_chips;
            d_psdu_symbol_num = 8;  // 8 symbols per byte
            d_preType = true;        // Long preamble
            break;
        case 1: // LONG2M
            d_rateVal = 2.0;
            d_chip_mapper = &ppdu_chip_mapper_bc_impl::dqpsk_2M_chips;
            d_psdu_symbol_num = 4;  // 4 symbols per byte
            d_preType = true;
            break;
        case 2: // LONG5_5M
            d_rateVal = 5.5;
            d_chip_mapper = &ppdu_chip_mapper_bc_impl::cck_5_5M_chips;
            d_psdu_symbol_num = 2;  // 2 symbols per byte
            d_preType = true;
            break;
        case 3: // LONG11M
            d_rateVal = 11.0;
            d_chip_mapper = &ppdu_chip_mapper_bc_impl::cck_11M_chips;
            d_psdu_symbol_num = 1;  // 1 symbol per byte
            d_preType = true;
            break;
        // ... similar for SHORT preambles (cases 4-6)
    }
}
```

### 4.4 Barker Code Spreading

**Barker-11 Code:** [1, -1, 1, 1, -1, 1, 1, 1, -1, -1, -1]

Used by 1M and 2M rates for spreading.

```cpp
static const float d_barker[11] = {
    1, -1, 1, 1, -1, 1, 1, 1, -1, -1, -1
};

// Example for 1M (DBPSK):
// Each bit becomes 11 chips (Barker code)
// 1 byte = 8 bits = 88 chips
```

**Barker Code Properties:**
- **Length:** 11 chips
- **Autocorrelation:** 11 at zero-lag, ≤1 at other lags
- **Processing Gain:** 10.4 dB (= 10 log10(11))
- **Peak-to-Average Ratio:** 1.0 (Good PAPR)

### 4.5 Modulation Schemes

#### 4.5.1 DBPSK - 1 Mbps

Binary differential PSK - each bit maps to a phase state.

```cpp
int dbpsk_1M_chips(gr_complex* out, uint8_t byte, bool even) {
    static const float d_dbpsk_phase[2] = {0, M_PI};
    
    // Each of 8 bits in byte becomes a separate symbol
    for (int i = 0; i < 8; ++i) {
        uint8_t tmp_bit = (byte >> i) & 0x01;
        
        // Differential: phase accumulates
        d_phase_acc += d_dbpsk_phase[tmp_bit];
        d_phase_acc = phase_wrap(d_phase_acc);
        
        // Create complex symbol
        gr_complex tmpChip = gr_expj(d_phase_acc);
        
        // Spread with Barker code (11 chips per symbol)
        for (int j = 0; j < 11; ++j) {
            out[i*11 + j] = tmpChip * d_barker[j];
        }
    }
    return 88;  // 8 bits × 11 chips = 88 chips
}
```

**Phase Mapping:**
- Bit 0 → Phase increment of 0 (no change)
- Bit 1 → Phase increment of π (180° rotation)

#### 4.5.2 DQPSK - 2 Mbps

Quadrature differential PSK - pairs of bits map to phase states.

```cpp
int dqpsk_2M_chips(gr_complex* out, uint8_t byte, bool even) {
    static const float d_dqpsk_phase[4] = {
        0,         // (0,0) bits
        1.5*M_PI,  // (0,1) bits
        0.5*M_PI,  // (1,0) bits
        M_PI       // (1,1) bits
    };
    
    // Each 2-bit pair becomes a separate symbol
    for (int i = 0; i < 4; ++i) {
        uint8_t tmp_bits = (byte >> (2*i)) & 0x03;  // Extract 2 bits
        
        d_phase_acc += d_dqpsk_phase[tmp_bits];
        d_phase_acc = phase_wrap(d_phase_acc);
        
        gr_complex tmpChip = gr_expj(d_phase_acc);
        
        // Spread with Barker code
        for (int j = 0; j < 11; ++j) {
            out[i*11 + j] = tmpChip * d_barker[j];
        }
    }
    return 44;  // 4 symbols × 11 chips = 44 chips
}
```

**Phase Mapping (2-bit symbols):**
- Bits (0,0) → 0°
- Bits (0,1) → 270°
- Bits (1,0) → 90°
- Bits (1,1) → 180°

#### 4.5.3 CCK - 5.5 Mbps

Complementary Code Keying - uses 6 phase terms to encode each pair of symbols.

```cpp
int cck_5_5M_chips(gr_complex* out, uint8_t byte, bool even) {
    // Byte layout for 5.5M: [b1 b0 b3 b2 | b7 b6 b5 b4]
    // Outputs 16 chips (8 + 8)
    
    float phase_00 = d_cck_dqpsk_phase[byte & 0x03][0];      // b1b0
    float phase_10 = d_cck_dqpsk_phase[(byte >> 4) & 0x03][1]; // b5b4
    float phase_b2 = ((byte >> 2) & 0x01) ? 1.5*M_PI : 0.5*M_PI;
    float phase_b4 = ((byte >> 3) & 0x01) ? M_PI : 0;
    float phase_q2 = ((byte >> 6) & 0x01) ? 1.5*M_PI : 0.5*M_PI;
    float phase_q4 = ((byte >> 7) & 0x01) ? M_PI : 0;
    
    // First half (8 chips)
    d_phase_acc += phase_00;
    d_phase_acc = phase_wrap(d_phase_acc);
    cck_gen(out, d_phase_acc, phase_b2, 0, phase_b4);
    
    // Second half (8 chips)
    d_phase_acc += phase_10;
    d_phase_acc = phase_wrap(d_phase_acc);
    cck_gen(out + 8, d_phase_acc, phase_q2, 0, phase_q4);
    
    return 16;
}

// CCK symbol generator - creates 8 complex chips
inline void cck_gen(gr_complex* out, float p1, float p2, float p3, float p4) {
    out[0] = gr_expj(phase_wrap(p1 + p2 + p3 + p4));
    out[1] = gr_expj(phase_wrap(p1 + p3 + p4));
    out[2] = gr_expj(phase_wrap(p1 + p2 + p4));
    out[3] = gr_expj(phase_wrap(p1 + p4 + M_PI));
    out[4] = gr_expj(phase_wrap(p1 + p2 + p3));
    out[5] = gr_expj(phase_wrap(p1 + p3));
    out[6] = gr_expj(phase_wrap(p1 + p2 + M_PI));
    out[7] = gr_expj(phase_wrap(p1));
}
```

**CCK Encoding Process:**
1. Split byte into bit pairs: [b1 b0], [b3 b2], [b5 b4], [b7 b6]
2. Map each pair to a phase offset
3. Generate 8 complex chips using four phase components

**Why CCK is Efficient:**
- Uses the entire bit sequence to encode complementary phase patterns
- 8 chips encode 4 bits (2 bits per symbol) at 5.5 Mbps
- Better error correction through code redundancy

#### 4.5.4 CCK - 11 Mbps

Even more efficient encoding - all 8 bits of a byte encoded as a single symbol.

```cpp
int cck_11M_chips(gr_complex* out, uint8_t byte, bool even) {
    // For 11M, process based on even/odd symbol position
    float phase_0;
    if (even) {
        // Even index: use first phase table entry
        phase_0 = d_cck_dqpsk_phase[byte & 0x03][0];
    } else {
        // Odd index: use second phase table entry
        phase_0 = d_cck_dqpsk_phase[byte & 0x03][1];
    }
    
    // Extract phase values for other bit pairs
    float other[3];
    for (int i = 0; i < 3; ++i) {
        uint8_t tmpBit = (byte >> ((i+1)*2)) & 0x03;
        other[i] = d_cck_qpsk[tmpBit];
    }
    
    // Generate 8 chips
    d_phase_acc += phase_0;
    d_phase_acc = phase_wrap(d_phase_acc);
    cck_gen(out, d_phase_acc, other[0], other[1], other[2]);
    
    return 8;  // Single symbol yields 8 chips
}
```

**11M Characteristics:**
- 8 bits per symbol (byte)
- 8 chips per symbol
- Symbol duration: 0.727 µs
- Chip duration: 90.9 ns
- Double the rate of 5.5M

### 4.6 Phase Accumulation

All modulations use differential encoding with phase accumulation:

```cpp
float d_phase_acc;  // Maintains phase continuity across symbols

// Before generating each symbol:
d_phase_acc += phase_increment;
d_phase_acc = phase_wrap(d_phase_acc);

// Phase wrap function ensures -2π < phase < 2π
inline float phase_wrap(float phase) {
    while (phase >= TWO_PI)
        phase -= TWO_PI;
    while (phase <= -TWO_PI)
        phase += TWO_PI;
    return phase;
}
```

**Why Phase Accumulation?**
- Maintains differential encoding requirement
- Ensures phase continuity (smooth modulation)
- Improves receiver symbol timing recovery

### 4.7 Message Processing Flow

```
Input Stream Processing:

Rate Tag Detection:
  Tag at byte 0 identifies rate (0-6)
      │
      ▼
  updateRate() called → sets d_chip_mapper function pointer
      │
      ▼
Preamble Processing:
  • Long preamble (18 bytes): Generate 1M chips (88 each)
  • Short preamble (9 bytes): Generate 1M chips (88 each)
      │
      ▼
Header Processing (6 bytes):
  • Long preamble: Generate 1M chips (88 each)
  • Short preamble: First 9 bytes with 1M chips
                   Next 6 bytes with 2M chips (44 each)
      │
      ▼
PSDU Processing:
  Use rate-specific mapper:
  • 1M: dbpsk_1M_chips() → 88 chips/byte
  • 2M: dqpsk_2M_chips() → 44 chips/byte
  • 5.5M: cck_5_5M_chips() → 16 chips/byte
  • 11M: cck_11M_chips() → 8 chips/byte
      │
      ▼
Appended Symbols:
  11 complex chips of Barker code appended at end
```

### 4.8 Tag Propagation

Length tag calculation based on total PPDU content:

```cpp
int update_tag(int total_bytes) {
    // Calculate output chip count with preamble overhead
    switch(d_rate) {
        case LONG1M:
            return total_bytes * 88;
        case LONG2M:
            return LONG_PREAMBLE_LENGTH * 88 + 
                   (total_bytes - LONG_PREAMBLE_LENGTH) * 44;
        case LONG5_5M:
            return LONG_PREAMBLE_LENGTH * 88 + 
                   (total_bytes - LONG_PREAMBLE_LENGTH) * 16;
        case LONG11M:
            return LONG_PREAMBLE_LENGTH * 88 + 
                   (total_bytes - LONG_PREAMBLE_LENGTH) * 8;
        case SHORT2M:
            return SHORT_PREAMBLE_LENGTH * 88 + 
                   (total_bytes - SHORT_PREAMBLE_LENGTH) * 44;
        case SHORT5_5M:
            return SHORT_PREAMBLE_LENGTH * 88 +
                   HEADER_BYTES * 44 +
                   (total_bytes - SHORT_PREAMBLE_LENGTH - HEADER_BYTES) * 16;
        case SHORT11M:
            return SHORT_PREAMBLE_LENGTH * 88 +
                   HEADER_BYTES * 44 +
                   (total_bytes - SHORT_PREAMBLE_LENGTH - HEADER_BYTES) * 8;
    }
}

// Output tag updated from packet_len to actual chip count
// This allows downstream blocks to know exact output size
```

---

## 5. Complete Message Flow Example

### 5.1 100-byte Packet Transmission at 11 Mbps (Long Preamble)

```
Input: 100-byte PSDU

Step 1: PPDU Prefixer
  Preamble:        18 bytes (1 long preamble)
  PLCP Header:     6 bytes (SIGNAL, SERVICE, LENGTH, CRC)
  PSDU:           100 bytes (user data)
  ──────────────────────────
  Total PPDU:     124 bytes
  Scrambled:      Yes
  Rate tag:       0x03 (LONG11M)
  
  Output Message:
  [0x03 | scrambled(preamble+header+psdu)]
  
  Tag: "packet_len" = 125 (124 bytes + 1 rate tag byte)

Step 2: PPDU Chip Mapper
  
  Rate Detection: 0x03 = LONG11M
    → d_chip_mapper = cck_11M_chips
    → d_psdu_symbol_num = 1 (1 symbol per byte)
  
  Preamble Processing (18 bytes):
    ├─ Each byte → dbpsk_1M_chips() → 88 chips
    └─ Total: 18 × 88 = 1,584 chips (144 µs @ 11 MHz)
  
  Header Processing (6 bytes):
    ├─ Each byte → dbpsk_1M_chips() → 88 chips
    └─ Total: 6 × 88 = 528 chips (48 µs @ 11 MHz)
  
  PSDU Processing (100 bytes):
    ├─ Each byte → cck_11M_chips() → 8 chips
    │  (with phase accumulation)
    ├─ Phase updates:
    │  byte 0: bits 1:0 phase + bits 3:2 phase + bits 5:4 phase + bits 7:6 phase
    │  byte 1: ...
    └─ Total: 100 × 8 = 800 chips (72.7 µs @ 11 MHz)
  
  Appended Symbols (11 chips):
    └─ Total: 11 chips
  
  Output:
    1,584 + 528 + 800 + 11 = 2,923 complex chips @ 11 MHz
    Duration: (2,923 / 11e6) ≈ 265.7 µs
    
  Tag Updated:
    "packet_len" → 2,923

Step 3: Transmission
  Complex baseband samples pass to DAC or USRP sink
  Modulation up to RF center frequency
```

### 5.2 Timing Analysis

| Component | Bytes | Chips | Duration (µs) | Rate |
|-----------|-------|-------|---------------|------|
| Long Preamble | 18 | 1,584 | 144.0 | 1M (DBPSK) |
| PLCP Header | 6 | 528 | 48.0 | 1M (DBPSK) |
| PSDU (100 bytes) | 100 | 800 | 72.7 | 11M (CCK) |
| Appended Symbols | - | 11 | 1.0 | - |
| **TOTAL** | **124** | **2,923** | **265.7** | - |

**Transmission Efficiency:**
- Net payload data: 100 bytes = 800 bits
- Total chips transmitted: 2,923
- Effective bits per chip: 800/2,923 = 0.274
- Efficiency: 27.4%
- Overhead: 72.6% (preamble + header + appended symbols)

---

## 6. Rate-Specific Characteristics

### 6.1 Chip Output Summary

| Rate | Modulation | Spreading | Chips/Byte | Byte Duration | Examples |
|------|------------|-----------|-----------|---|---|
| 1M | DBPSK | Barker-11 | 88 | 88 µs | Link setup, long range |
| 2M | DQPSK | Barker-11 | 44 | 44 µs | Baseline data |
| 5.5M | CCK | Implicit | 16 | 2.9 µs | Medium range, good SNR |
| 11M | CCK | Implicit | 8 | 1.45 µs | High throughput, near range |

### 6.2 Phase Accumulation Behavior

All rates use **differential phase encoding**:

```
Final Phase = Initial Phase + Σ(Phase Increments)

Example (DQPSK at 2M):
Input: 0xA5 = 10100101 (bits: d0=1, d1=0, d2=1, d3=0, d4=0, d5=1, d6=0, d7=1)
       2-bit symbols: (d0,d1)=01, (d2,d3)=01, (d4,d5)=10, (d6,d7)=01

Symbol 0: bits (0,1) → phase += 270° (1.5π)
Symbol 1: bits (0,1) → phase += 270° (1.5π)
Symbol 2: bits (1,0) → phase += 90° (0.5π)
Symbol 3: bits (0,1) → phase += 270° (1.5π)

Output: 4 complex symbols (44 chips via Barker spreading)
```

---

## 7. System Integration Points

### 7.1 Block Connectivity

```python
# From dsss_loopback.py example:

# Message connections (signal passing)
self.msg_connect((msg_source, 'out'), 
                (ppdu_prefixer, 'psdu_in'))
self.msg_connect((ppdu_prefixer, 'ppdu_out'), 
                (chip_mapper, 'in'))

# Stream connections (sample flowing)
self.connect((chip_mapper, 0),        # Complex output
            (noise_adder, 0))         # Add noise/channel
```

### 7.2 Rate Selection

Rates are selected **dynamically**:

```python
# Static configuration:
ppdu_prefixer = ieee80211.ppdu_prefixer(rate=3)  # 11M long

# Dynamic update possible:
ppdu_prefixer.update_rate(3)  # Switch to 11M long
ppdu_prefixer.update_rate(6)  # Switch to 11M short
```

### 7.3 Message Tag Propagation

1. **ppdu_prefixer** embeds rate in first byte of PPDU
2. **ppdu_chip_mapper_bc** reads rate tag from input stream
3. **ppdu_chip_mapper_bc** detects packet length tag from upstream
4. **ppdu_chip_mapper_bc** outputs new tag with total chip count

```
Tag Flow:
packet_len=125 bytes
    ↓
ppdu_chip_mapper_bc (detects rate=3, calculates output size)
    ↓
packet_len=2923 chips
```

---

## 8. Implementation Details

### 8.1 Data Structure Layout

**PPDU Buffer (ppdu_prefixer):**
```
unsigned char d_buf[8192];       // Raw PPDU construction area
unsigned char d_spread_buf[8192]; // Scrambled PPDU output
unsigned char d_init_state;      // LFSR state for scrambler
int d_ppdu_index;                // Current position in buffer
```

**Chip Mapper State (ppdu_chip_mapper_bc_impl):**
```
int d_count;                    // Total PPDU bytes
int d_copy;                     // Bytes processed
int d_psdu_symbol_count;        // For 11M (tracks even/odd)
int d_psdu_symbol_num;          // Symbols per byte (8,4,2,1)
float d_phase_acc;              // Phase accumulator
int d_rate;                     // Current rate (0-6)
int d_append;                   // Appended chips counter
bool d_preType;                 // true=long, false=short preamble
```

### 8.2 Critical Constants

```cpp
// Preamble lengths
#define LONG_PREAMBLE_LENGTH 18      // bytes
#define SHORT_PREAMBLE_LENGTH 9      // bytes

// PLCP header size
#define WIFI80211DSSS_PHYHDR_BYTES 6 // SIGNAL+SERVICE+LEN+CRC

// Output chips appended at end
#define APPENDED_CHIPS 11            // Barker code sequence

// Scrambler reserved byte (for rate tag)
#define SCRAMBLER_BYTE_RESERVED 1

// Phase wrapping threshold
#define TWO_PI (M_PI * 2.0f)
```

---

## 9. Signal Processing Summary

### 9.1 1 Mbps Path (DBPSK + Barker)

```
Byte Input
    │
    ├─ Extract 8 bits
    │
    ▼
DBPSK Modulation:
  Phase increment = 0 (bit=0) or π (bit=1)
  
    │
    ▼
  8 Complex Symbols @ current_phase
    │
    ▼
Barker Spreading (×11 chips each):
  Symbol × [1, -1, 1, 1, -1, 1, 1, 1, -1, -1, -1]
    │
    ▼
  88 Complex Chips
```

### 9.2 2 Mbps Path (DQPSK + Barker)

```
Byte Input
    │
    ├─ Extract 4 × 2-bit pairs
    │
    ▼
DQPSK Modulation:
  Phase increment = 0°, 90°, 270°, or 180°
  
    │
    ▼
  4 Complex Symbols @ accumulated_phase
    │
    ▼
Barker Spreading (×11 chips each):
    │
    ▼
  44 Complex Chips
```

### 9.3 5.5 Mbps Path (CCK)

```
Byte Input
    │
    ├─ Extract 8 bits: [b7 b6 | b5 b4 b3 b2 | b1 b0]
    │
    ▼
CCK Encoding:
  Phase_0 = map(b1, b0)
  Phase_1 = map(b5, b4)
  Phase_2 = b2
  Phase_3 = b3
  Phase_4 = b6
  Phase_5 = b7
    │
    ▼
  Generate 8 chips via cck_gen() using 4 phase components
  Repeat for second half: 8 more chips
    │
    ▼
  16 Complex Chips
```

### 9.4 11 Mbps Path (CCK Optimized)

```
Byte Input (8 bits encoding 1 symbol)
    │
    ├─ Extract 4 × 2-bit pairs
    │
    ▼
CCK Modulation:
  All 8 bits map to 4 phase components
  (Differential based on even/odd position)
    │
    ▼
  Generate 8 chips via cck_gen()
    │
    ▼
  8 Complex Chips
```

---

## 10. Receiver Side Coordination

While this analysis focuses on TX, the receiver uses:

1. **chip_sync_c** - Synchronizes to Barker code in preamble
2. Detects rate from SIGNAL field in PLCP header
3. **Adaptive demodulation** - Switches decoder based on rate
4. **Descrambling** - Reverses LFSR scrambling

---

## 11. Performance Characteristics

### 11.1 Processing Overhead

| Rate | Preamble Overhead | Header Overhead | Payload Efficiency |
|------|------------------|-----------------|-------------------|
| 1M | 18 bytes @ 1M | 6 bytes @ 1M | Max ~94% |
| 2M | 18 bytes @ 1M + 6 @ 2M | Included | Max ~95% |
| 5.5M | 18 bytes @ 1M + 6 @ 2M | Included | Max ~97% |
| 11M | 18 bytes @ 1M + 6 @ 2M | Included | Max ~97% |

**Example (100-byte packet at 11M):**
```
Overhead = (18 × 88 + 6 × 44 + 11) chips = 2,123 chips
Payload = 100 × 8 chips = 800 chips
Total = 2,923 chips
Efficiency = 800/2,923 = 27.4%
```

### 11.2 Modulation Properties

**DBPSK (1M & Preambles):**
- Phase states: 0, π
- Differential detection ensures constant envelope
- Good for low SNR (long range)
- Spectral efficiency: 1 bit/Hz (with spreading)

**DQPSK (2M & Header in short preamble):**
- Phase states: 0, π/2, π, 3π/2
- 2 bits per symbol
- Better spectrum efficiency than DBPSK
- Moderate SNR requirement

**CCK (5.5M & 11M):**
- Orthogonal sequences encode information
- Implicit spreading (not Barker code)
- Very efficient spectrum usage
- Higher SNR requirement

---

## 12. Code Listing Summary

### Key Functions

| File | Function | Purpose |
|------|----------|---------|
| ppdu_prefixer.cc | `psdu_in()` | Message handler for incoming PSDU |
| ppdu_prefixer.cc | `placeHeader()` | Constructs PLCP header with CRC |
| ppdu_prefixer.cc | `scrambler()` | Applies LFSR scrambling |
| ppdu_chip_mapper_bc_impl.cc | `general_work()` | Main processing loop |
| ppdu_chip_mapper_bc_impl.cc | `chipGen()` | Rate-specific chip generation |
| ppdu_chip_mapper_bc_impl.cc | `updateRate()` | Sets modulation based on rate tag |
| ppdu_chip_mapper_bc_impl.cc | `dbpsk_1M_chips()` | 1M modulation |
| ppdu_chip_mapper_bc_impl.cc | `dqpsk_2M_chips()` | 2M modulation |
| ppdu_chip_mapper_bc_impl.cc | `cck_5_5M_chips()` | 5.5M modulation |
| ppdu_chip_mapper_bc_impl.cc | `cck_11M_chips()` | 11M modulation |

---

## 13. Conclusion

The gr-ieee80211 DSSS/CCK transmitter implements a complete 802.11b physical layer encoder:

1. **PPDU Prefixer** handles preamble and header generation with scrambling
2. **Chip Mapper** implements four distinct modulation schemes
3. **Barker spreading** (1M, 2M) ensures timing recovery capability
4. **CCK encoding** (5.5M, 11M) maximizes spectral efficiency
5. **Phase accumulation** maintains differential encoding coherence
6. **Tag propagation** enables downstream rate/length awareness

The design demonstrates flexibility in supporting multiple rates through function pointers and maintains signal integrity through careful phase management.

