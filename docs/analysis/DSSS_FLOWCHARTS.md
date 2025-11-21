# 802.11b DSSS/CCK Transmission Chain - Detailed Flow Diagrams

## Transmission Pipeline Diagrams

### 1. High-Level Packet Flow

```
┌─────────────────────────────────────────────────────────────────────┐
│                                                                     │
│                   PACKET TRANSMISSION PIPELINE                      │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘

User Application Layer
        │
        │ Packet (PSDU)
        │ payload_data[0..N-1]
        │
        ▼
┌─────────────────────────────────────────────────────────────────────┐
│                                                                     │
│         STAGE 1: PPDU PREFIXER (ppdu_prefixer.cc)                 │
│                                                                     │
│  Input:  PSDU message (u8vector via PMT message port)             │
│  Output: PPDU message with rate tag embedded                      │
│                                                                     │
│  Operations:                                                        │
│  ├─ Copy preamble bytes (18 or 9)                                │
│  ├─ Generate PLCP header (6 bytes)                               │
│  │  ├─ SIGNAL field (1 byte) - rate indicator                   │
│  │  ├─ SERVICE field (1 byte) - flags                           │
│  │  ├─ LENGTH field (2 bytes) - transmission time               │
│  │  └─ CRC16 (2 bytes) - CCITT polynomial check                 │
│  ├─ Append PSDU bytes                                            │
│  └─ Apply LFSR scrambling (7-bit, self-synchronizing)           │
│                                                                     │
│  Data Structure Evolution:                                         │
│  ┌──────────────────────────────────────────────────┐            │
│  │ Input PSDU (N bytes)                             │            │
│  └──────────────────────────────────────────────────┘            │
│                    │                                               │
│                    ▼                                               │
│  ┌──────────────────────────────────────────────────┐            │
│  │ Preamble (18 or 9 bytes)                        │            │
│  │ ├─ Sync (0xFF × 16 or 0x00 × 7)               │            │
│  │ └─ SFD (0xA0, 0xF3 or 0xCF, 0x05)            │            │
│  └──────────────────────────────────────────────────┘            │
│                    │                                               │
│                    ▼                                               │
│  ┌──────────────────────────────────────────────────┐            │
│  │ PLCP Header (6 bytes)                           │            │
│  │ ├─ Byte 0: SIGNAL (0x0A/0x14/0x37/0x6E)      │            │
│  │ ├─ Byte 1: SERVICE (flags)                    │            │
│  │ ├─ Bytes 2-3: LENGTH (microseconds)           │            │
│  │ └─ Bytes 4-5: CRC16                           │            │
│  └──────────────────────────────────────────────────┘            │
│                    │                                               │
│                    ▼                                               │
│  ┌──────────────────────────────────────────────────┐            │
│  │ PSDU (N bytes) - Scrambled                      │            │
│  └──────────────────────────────────────────────────┘            │
│                                                                     │
│  Output Message:                                                   │
│  ┌─────────────────────────────────────────────────────────┐     │
│  │ [rate_tag(1 byte) | PPDU_data(18+6+N bytes)]          │     │
│  │  rate_tag ∈ {0,1,2,3,4,5,6}                           │     │
│  │  - 0: LONG1M    - 1: LONG2M   - 2: LONG5_5M           │     │
│  │  - 3: LONG11M   - 4: SHORT2M  - 5: SHORT5_5M          │     │
│  │  - 6: SHORT11M                                         │     │
│  └─────────────────────────────────────────────────────────┘     │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
        │
        │ PMT Message: (pmt_nil, blob)
        │ blob = [rate_byte | scrambled_ppdu]
        │
        ▼
┌─────────────────────────────────────────────────────────────────────┐
│                                                                     │
│     STAGE 2: PPDU CHIP MAPPER (ppdu_chip_mapper_bc_impl.cc)       │
│                                                                     │
│  Input:  Byte stream from PPDU prefixer                           │
│  Output: Complex baseband samples (gr_complex)                    │
│                                                                     │
│  Key Operations:                                                    │
│  ├─ Extract rate tag from first byte                             │
│  ├─ Initialize rate-specific mapper function pointer             │
│  ├─ Process preamble (always at 1M: 88 chips/byte)              │
│  ├─ Process header:                                              │
│  │  ├─ Long preamble: 1M (88 chips/byte)                        │
│  │  └─ Short preamble: 1M then 2M                               │
│  ├─ Process PSDU at rate-selected modulation:                    │
│  │  ├─ 1M: DBPSK + Barker (88 chips/byte)                       │
│  │  ├─ 2M: DQPSK + Barker (44 chips/byte)                       │
│  │  ├─ 5.5M: CCK (16 chips/byte)                                │
│  │  └─ 11M: CCK optimized (8 chips/byte)                        │
│  └─ Append Barker code terminator (11 chips)                    │
│                                                                     │
│  State Machine:                                                    │
│  ┌────────────────────────────────────────────────────────┐      │
│  │ [IDLE] --rate_tag_detected--> [PREAMBLE_GEN]          │      │
│  │                                      │                 │      │
│  │                                      ▼                 │      │
│  │                            [HEADER_GEN]                │      │
│  │                                      │                 │      │
│  │                                      ▼                 │      │
│  │                            [PSDU_GEN]                  │      │
│  │                                      │                 │      │
│  │                                      ▼                 │      │
│  │                            [APPEND_BARKER]            │      │
│  │                                      │                 │      │
│  │                                      ▼                 │      │
│  │                                  [IDLE]                │      │
│  └────────────────────────────────────────────────────────┘      │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
        │
        │ Complex baseband samples @ 11 MHz
        │ Output[0..M-1] where M depends on rate:
        │   M = 18×88 + 6×88 + N×88 + 11     (1M)
        │   M = 18×88 + 6×44 + N×44 + 11     (2M)
        │   M = 18×88 + 6×44 + N×16 + 11     (5.5M)
        │   M = 18×88 + 6×44 + N×8 + 11      (11M)
        │
        ▼
┌─────────────────────────────────────────────────────────────────────┐
│                                                                     │
│           STAGE 3: CHANNEL & TRANSMISSION                         │
│                                                                     │
│  ├─ Apply gain/attenuation                                        │
│  ├─ Add noise (if simulation)                                     │
│  ├─ Send to USRP/DAC                                             │
│  └─ RF modulation to carrier frequency                            │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 2. PPDU Prefixer Detailed Flow

```
PPDU PREFIXER MESSAGE HANDLER
┌─────────────────────────────────────────────────────────────────────┐

Input PMT Message (PSDU):
  msg = (pmt_nil, u8vector[psdu_data])

                    │
                    ▼
        ┌──────────────────────────┐
        │ Extract PSDU from PMT    │
        │ psdu_len = vector.size() │
        └──────────────────────────┘
                    │
                    ▼
        ┌──────────────────────────────────────────┐
        │ Get current rate setting:                │
        │ d_rate ∈ {0,1,2,3,4,5,6}                │
        │ d_long_pre ∈ {true, false}              │
        └──────────────────────────────────────────┘
                    │
                    ▼
        ┌──────────────────────────────────────────┐
        │ SELECT PREAMBLE                          │
        │                                          │
        │ if (d_long_pre):                        │
        │   Copy d_long_preamble[18]              │
        │   = [0xFF×16, 0xA0, 0xF3]              │
        │   Length = 18 bytes                      │
        │ else:                                    │
        │   Copy d_short_preamble[9]              │
        │   = [0x00×7, 0xCF, 0x05]               │
        │   Length = 9 bytes                      │
        │                                          │
        │ d_ppdu_index += preamble_len            │
        └──────────────────────────────────────────┘
                    │
                    ▼
        ┌──────────────────────────────────────────┐
        │ GENERATE PLCP HEADER (6 bytes)           │
        │                                          │
        │ 1. Calculate LENGTH field:              │
        │    switch(d_rate):                      │
        │    ├─ 1M:   tmpLen = psdu_len × 8      │
        │    ├─ 2M:   tmpLen = psdu_len × 4      │
        │    ├─ 5.5M: tmpLen = psdu_len × 8/5.5 │
        │    └─ 11M:  tmpLen = psdu_len × 8/11   │
        │                                          │
        │ 2. Write SIGNAL byte:                   │
        │    d_buf[0] = d_sig[rate_index]        │
        │    (0x0A=1M, 0x14=2M, 0x37=5.5M,      │
        │     0x6E=11M)                           │
        │                                          │
        │ 3. Write SERVICE byte:                  │
        │    d_buf[1] = 0x04 or 0x84             │
        │    (bit 7 for 11M padding flag)        │
        │                                          │
        │ 4. Write LENGTH (little-endian):       │
        │    d_buf[2] = u16Len & 0xFF            │
        │    d_buf[3] = (u16Len >> 8) & 0xFF    │
        │                                          │
        │ 5. Calculate and write CRC-16:         │
        │    - Load CRC register: 0xFFFF        │
        │    - Process 32 bits (SIGNAL+SERVICE+  │
        │      LENGTH) with polynomial XOR      │
        │    - Output inverted CRC               │
        │    d_buf[4] = crc & 0xFF              │
        │    d_buf[5] = (crc >> 8) & 0xFF      │
        │                                          │
        │ d_ppdu_index += 6                       │
        └──────────────────────────────────────────┘
                    │
                    ▼
        ┌──────────────────────────────────────────┐
        │ COPY PSDU BYTES                          │
        │                                          │
        │ memcpy(d_buf + d_ppdu_index,            │
        │        uvec, psdu_len)                   │
        │ d_ppdu_index += psdu_len                │
        │                                          │
        │ Total PPDU length = preamble + 6 + psdu │
        └──────────────────────────────────────────┘
                    │
                    ▼
        ┌──────────────────────────────────────────┐
        │ APPLY SCRAMBLER                          │
        │                                          │
        │ Initialize LFSR state:                  │
        │   if (d_long_pre):                      │
        │     d_init_state = 0x1B                 │
        │   else:                                 │
        │     d_init_state = 0x6C                 │
        │                                          │
        │ For each byte in PPDU:                  │
        │   tmp_byte = 0x00                       │
        │   for each bit (j=0 to 7):             │
        │     input_bit = (byte >> j) & 0x01    │
        │     feedback = bit3 XOR bit6           │
        │     output = input XOR feedback         │
        │     tmp_byte |= (output << j)          │
        │     shift state left, insert output     │
        │                                          │
        │ d_spread_buf[RESERVE + i] = scrambled  │
        └──────────────────────────────────────────┘
                    │
                    ▼
        ┌──────────────────────────────────────────┐
        │ CREATE OUTPUT MESSAGE                    │
        │                                          │
        │ d_spread_buf[0] = d_rate               │
        │ (embeds rate as first byte)             │
        │                                          │
        │ blob = make_blob(d_spread_buf,         │
        │                  d_ppdu_index + 1)     │
        │                                          │
        │ msg = cons(pmt_nil, blob)              │
        │                                          │
        │ message_port_pub("ppdu_out", msg)      │
        └──────────────────────────────────────────┘
                    │
                    ▼
        Output: [rate_byte | scrambled_ppdu...]
                Rate byte: 0-6
                Total length: (preamble + 6 + psdu) + 1

└─────────────────────────────────────────────────────────────────────┘
```

---

## 3. Chip Mapper Modulation Selection

```
CHIP MAPPER RATE SELECTION & PROCESSING
┌─────────────────────────────────────────────────────────────────────┐

Input: PPDU bytes with rate tag at position 0

                    │
                    ▼
        ┌──────────────────────────────────────┐
        │ READ RATE TAG (First Byte)          │
        │ raw = in[0] ∈ {0,1,2,3,4,5,6}      │
        └──────────────────────────────────────┘
                    │
                    ▼
        ┌──────────────────────────────────────┐
        │ CALL updateRate(raw)                 │
        │                                      │
        │ Sets function pointer:               │
        │ d_chip_mapper = function_ptr         │
        │                                      │
        │ Also sets:                           │
        │ ├─ d_rateVal (1.0, 2.0, 5.5, 11.0) │
        │ ├─ d_preType (true/false)           │
        │ ├─ d_psdu_symbol_num (8,4,2,1)     │
        │ └─ d_rate_tag (PMT string)          │
        └──────────────────────────────────────┘
                    │
        ┌───────────┴──────────┬──────────────┬──────────────┐
        │                      │              │              │
        ▼                      ▼              ▼              ▼
    rate=0,3,4,6          rate=1            rate=2         rate=5
    (1M or 11M)           (2M)           (5.5M)          (5.5M)
        │                  │              │              │
        ▼                  ▼              ▼              ▼
    ┌────────┐         ┌────────┐     ┌────────┐    ┌────────┐
    │ 1M or  │         │ 2M     │     │ 5.5M   │    │ 5.5M   │
    │ CCK    │         │ DQPSK  │     │ CCK    │    │ CCK    │
    │        │         │        │     │        │    │        │
    │        │         │        │     │        │    │        │
    └────────┘         └────────┘     └────────┘    └────────┘

        ┌───────────────┬──────────────┬──────────────┬──────────────┐
        │               │              │              │              │
        ▼               ▼              ▼              ▼              ▼
    ┌─────────┐    ┌─────────┐   ┌──────────┐   ┌──────────┐   ┌──────────┐
    │dbpsk_   │    │dqpsk_   │   │cck_      │   │cck_      │   │cck_      │
    │1M_chips │    │2M_chips │   │5_5M_    │   │5_5M_    │   │11M_     │
    │         │    │         │   │chips    │   │chips    │   │chips    │
    │         │    │         │   │         │   │         │   │         │
    │FUNCTION │    │FUNCTION │   │FUNCTION │   │FUNCTION │   │FUNCTION │
    │POINTER  │    │POINTER  │   │POINTER  │   │POINTER  │   │POINTER  │
    └─────────┘    └─────────┘   └──────────┘   └──────────┘   └──────────┘

Processing Order:
1. Preamble bytes:          Always use dbpsk_1M_chips()
   ├─ 18 bytes (long)       → 18 × 88 chips
   └─ 9 bytes (short)       → 9 × 88 chips

2. Header bytes:            Depends on preamble type
   ├─ Long preamble:
   │  6 bytes                → 6 × 88 chips (1M DBPSK)
   └─ Short preamble:
      6 bytes                → 6 × 44 chips (2M DQPSK)

3. PSDU bytes:              Use selected modulation function
   ├─ LONG1M/SHORT rates:   Use dbpsk_1M_chips or selected
   └─ Rate-specific mapper  → 88, 44, 16, or 8 chips/byte

4. Appended symbols:        11 fixed Barker code chips

└─────────────────────────────────────────────────────────────────────┘
```

---

## 4. Phase Accumulation in Differential Modulation

```
PHASE ACCUMULATION STATE MACHINE
┌─────────────────────────────────────────────────────────────────────┐

For each rate, modulation is differential:
  final_phase = Σ(phase_increments) mod 2π

Example: DQPSK 2M (4 symbols per byte)

Input byte: 0xB4 = 10110100 (binary)
            bits: [d0 d1 d2 d3 d4 d5 d6 d7]
                = [0  0  1  0  1  1  0  1]

Extract 2-bit pairs:
  pair0 = (d1, d0) = (0, 0) → index 0
  pair1 = (d3, d2) = (0, 1) → index 1
  pair2 = (d5, d4) = (1, 1) → index 3
  pair3 = (d7, d6) = (1, 0) → index 2

Phase lookup table:
  index 0 → 0       (bits 00)
  index 1 → 1.5π    (bits 01)
  index 2 → 0.5π    (bits 10)
  index 3 → π       (bits 11)

Processing:
                    Initial: d_phase_acc = 0

Symbol 0 (pair0 = 00):
  phase_inc = 0
  d_phase_acc = (0 + 0) mod 2π = 0
  Output: exp(j×0) = 1+0j
  Spread with Barker: [1+0j, -1+0j, ...] (11 chips)

Symbol 1 (pair1 = 01):
  phase_inc = 1.5π
  d_phase_acc = (0 + 1.5π) mod 2π = 1.5π
  Output: exp(j×1.5π) = 0-1j
  Spread with Barker: [0-1j, 0+1j, ...] (11 chips)

Symbol 2 (pair2 = 11):
  phase_inc = π
  d_phase_acc = (1.5π + π) mod 2π = 0.5π
  Output: exp(j×0.5π) = 0+1j
  Spread with Barker: [0+1j, 0-1j, ...] (11 chips)

Symbol 3 (pair3 = 10):
  phase_inc = 0.5π
  d_phase_acc = (0.5π + 0.5π) mod 2π = π
  Output: exp(j×π) = -1+0j
  Spread with Barker: [-1+0j, 1+0j, ...] (11 chips)

Total output: 44 complex chips @ 11 MHz

Key property: Phase difference between adjacent symbols contains
              the modulation information, not absolute phase.
              This allows receiver to track phase drift.

└─────────────────────────────────────────────────────────────────────┘
```

---

## 5. CCK Encoding Flow (5.5M)

```
CCK 5.5M ENCODING
┌─────────────────────────────────────────────────────────────────────┐

Input: Single byte (8 bits)
Byte layout: [b7 b6 | b5 b4 b3 b2 | b1 b0]
            └──┬──┘  └─────┬────┘  └─┬─┘
             (Q2,Q4)  (Q,I_other) (I0)

Output: 16 complex chips

Processing Steps:

1. Extract phase components from byte bits:
   
   phase_00 = lookup[byte & 0x03][0]        // bits b1b0
   phase_10 = lookup[(byte>>4)&0x03][1]     // bits b5b4
   phase_b2 = (bit2 ? 1.5π : 0.5π)         // bit b2
   phase_b4 = (bit3 ? π : 0)                // bit b3
   phase_q2 = (bit6 ? 1.5π : 0.5π)        // bit b6
   phase_q4 = (bit7 ? π : 0)               // bit b7

2. Accumulate phase for first symbol:
   
   d_phase_acc += phase_00
   d_phase_acc = wrap(d_phase_acc)
   
   Call: cck_gen(out[0..7], 
                 d_phase_acc,   // base phase
                 phase_b2,      // phase1
                 0,             // phase2
                 phase_b4)      // phase3

3. CCK generator creates 8 chips:
   
   out[0] = exp(j(p1 + p2 + 0 + p4))
   out[1] = exp(j(p1 + 0 + 0 + p4))
   out[2] = exp(j(p1 + p2 + 0 + p4))  [note: duplicate of [0]]
   out[3] = exp(j(p1 + 0 + 0 + π+p4)) [note: pi offset]
   out[4] = exp(j(p1 + p2 + 0))
   out[5] = exp(j(p1 + 0))
   out[6] = exp(j(p1 + p2 + π))
   out[7] = exp(j(p1))

4. Accumulate phase for second symbol:
   
   d_phase_acc += phase_10
   d_phase_acc = wrap(d_phase_acc)
   
   Call: cck_gen(out[8..15],
                 d_phase_acc,   // base phase
                 phase_q2,      // phase1
                 0,             // phase2
                 phase_q4)      // phase3

5. Generate second set of 8 chips (same as step 3)

Output: out[0..15] = 16 complex baseband chips
        ready for next byte or EOF

Key insight: CCK uses phase information in multiple combinations
             to encode the 8 input bits into 16 output chips.
             The receiver correlates against known CCK sequences.

└─────────────────────────────────────────────────────────────────────┘
```

---

## 6. Timing Diagram: 100-byte Packet at 11M

```
TIMELINE: 100-byte Packet Transmission @ 11 Mbps (Long Preamble)
═════════════════════════════════════════════════════════════════════

Sample Rate: 11 MHz
Chip Duration: 90.9 ns

Start of Packet (SOP):
┌─────────────────────────────────────────────────────────────────────┐

Time    Event                  Bytes  Chips   Cumulative  Duration
(µs)                                          Chips       (µs)

0       Start                    -      -         0         -

        PREAMBLE (LONG)
0-1.6   Byte 0-15             16      1408     1408      128.0
16-18   Sync Field Bytes       18      1584     1584      144.0
18-19   SFD Bytes             (2)      (176)    1584       16.0

        PLCP HEADER
20-21   Byte 0 (SIGNAL)         1       88      1672      8.0
21-22   Byte 1 (SERVICE)        1       88      1760      8.0
22-23   Bytes 2-3 (LENGTH)      2      176      1936      16.0
24-25   Bytes 4-5 (CRC16)       2      176      2112      16.0

        PSDU (SCRAMBLED)
25-98   Bytes 0-99            100      800      2912      72.7

        APPENDED SYMBOLS
98-99   Barker terminator        -       11      2923      1.0

99      End of Packet            -       -      2923      266.0

═════════════════════════════════════════════════════════════════════

Chip Distribution:
├─ Preamble:     1,584 chips  (54.1%)
├─ Header:         528 chips  (18.1%)
├─ PSDU:           800 chips  (27.4%)
└─ Append:          11 chips  (0.4%)
                 ─────────────────
    Total:       2,923 chips (266.0 µs)

Overhead Analysis:
├─ Non-payload:  2,123 chips (72.6% overhead)
│  ├─ Preamble:  1,584 chips (72.3% of overhead)
│  ├─ Header:      528 chips (24.9% of overhead)
│  └─ Append:       11 chips (0.5% of overhead)
└─ Payload:        800 chips (27.4% efficiency)

Performance Metrics:
├─ Gross data rate:   3,000 bits / 266 µs = 11.28 Mbps
├─ Net data rate:     800 bits / 266 µs = 3.01 Mbps
│                    (100 bytes × 8 bits = 800 bits)
├─ Spectral efficiency: 11 Mbps / 22 MHz = 0.5 (bps/Hz)
└─ Link efficiency: 27.4% (payload / total)

Latency Contribution:
├─ Serialization: 266 µs (time to send 2,923 chips @ 11 MHz)
└─ Access overhead: 72.6% additional time due to preamble/header

└─────────────────────────────────────────────────────────────────────┘
```

---

## 7. PLCP Header CRC-16 Calculation

```
PLCP HEADER CRC-16 CALCULATION
┌─────────────────────────────────────────────────────────────────────┐

Polynomial: x^16 + x^12 + x^5 + 1 (CCITT polynomial)
Reflected:  No (normal order)
Initial:    0xFFFF
Final XOR:  0x0000 (implicit in inversion)

Input: 4 bytes [SIGNAL, SERVICE, LENGTH_LO, LENGTH_HI]

Example: SIGNAL=0x0A, SERVICE=0x04, LENGTH=0x0064
         Bytes: [0x0A, 0x04, 0x64, 0x00]

Register representation: 32-bit accumulator
┌─────────────────────────────────────────────────────────────────────┐

Step 1: Initialize CRC register to 0xFFFF
        crc_reg = 0xFFFF

Step 2: Process 32 input bits (bit-by-bit)
        
        For bit_index = 0 to 31:
          newBit = (accumulator >> bit_index) & 0x01
          nlsb = (crc_reg >> 15) ^ newBit
          
          if (nlsb):
            crc_reg ^= 0x0810  // feedback polynomial
          
          crc_reg = (crc_reg << 1) | nlsb

        Feedback mask: 0x0810
        Bits:         x^12 XOR x^15 (normal polynomial)

Step 3: Invert final CRC
        crc16_final = ~crc_reg

Step 4: Output as little-endian bytes
        byte[0] = crc16_final & 0xFF
        byte[1] = (crc16_final >> 8) & 0xFF

Receiver Validation:
  Receiver calculates CRC on received [SIGNAL, SERVICE, LENGTH]
  Compares with received CRC bytes
  If mismatch: Frame corrupted, discard

┌─────────────────────────────────────────────────────────────────────┐
│ Protection Scope:                                                   │
│                                                                     │
│ Protected: SIGNAL field (rate indicator)                           │
│           SERVICE field (flags)                                     │
│           LENGTH field (transmission duration)                      │
│                                                                     │
│ Not Protected: PSDU data (protected by MAC-layer FCS)              │
│                                                                     │
│ Error Detection: Detects up to 2-bit errors (with high prob.)      │
│                  Detects all single-bit errors                      │
│                  Detects all odd-parity errors                      │
└─────────────────────────────────────────────────────────────────────┘

└─────────────────────────────────────────────────────────────────────┘
```

---

## 8. Tag Propagation Through Message Chain

```
TAG PROPAGATION IN MESSAGE CHAIN
┌─────────────────────────────────────────────────────────────────────┐

Source: pkt_gen (generates PMT messages with packet_len tag)

    Initial Packet Length Tag:
    ┌─────────────────────┐
    │ Tag Name: packet_len│
    │ Tag Value: 100      │ (100-byte PSDU)
    └─────────────────────┘
           │
           ▼
    ┌──────────────────────────┐
    │  ppdu_prefixer           │
    │  Message-based block     │
    └──────────────────────────┘
           │
           │ Processing:
           │ ├─ Receives PSDU (100 bytes)
           │ ├─ Adds preamble (18 bytes)
           │ ├─ Adds header (6 bytes)
           │ ├─ Adds scrambler reserve (1 byte)
           │ └─ Total: 125 bytes
           │
           ▼
    Output: PPDU Message
    ┌──────────────────────────────────────┐
    │ Blob: [rate_tag | 124 byte PPDU]    │
    │ (no explicit tag, embedded rate)     │
    └──────────────────────────────────────┘
           │
           ▼
    ┌──────────────────────────────────────┐
    │  ppdu_chip_mapper_bc                 │
    │  Stream-based block (byte in/chip out)
    │                                      │
    │  Input processing:                   │
    │  └─ Read "packet_len" tag from input │
    │     Value: 125 bytes                 │
    │  └─ Extract rate_tag: raw = in[0]   │
    │  └─ Call updateRate(raw)            │
    │  └─ Calculate output chip count:    │
    │     out_chips = 18×88 + 6×88 +      │
    │                 100×8 + 11          │
    │     = 2,923 chips                    │
    └──────────────────────────────────────┘
           │
           ▼
    Output: Complex samples with updated tag
    ┌──────────────────────────────────────┐
    │ Tag Name: packet_len                │
    │ Tag Value: 2,923 (CHIP COUNT)       │
    │ Tag Offset: 0 (start of packet)      │
    │                                      │
    │ Samples: complex[0..2922]           │
    └──────────────────────────────────────┘
           │
           ▼
    ┌──────────────────────────────────────┐
    │  Downstream blocks (optional)        │
    │  ├─ Frequency offset correction      │
    │  ├─ Pulse shaping filter             │
    │  ├─ Amplitude/phase correction       │
    │  └─ USRP sink or file sink           │
    │                                      │
    │  Tag propagation:                    │
    │  └─ "packet_len" tag flows through   │
    │     with value 2,923                 │
    │  └─ Helps receiver know frame length │
    └──────────────────────────────────────┘

Key Points:
├─ ppdu_prefixer: Message port (no streaming tag)
├─ ppdu_chip_mapper_bc: Stream port (propagates tags)
├─ Tag semantics change:
│  ├─ Input: "packet_len" = PSDU bytes
│  └─ Output: "packet_len" = Output samples
└─ Receiver uses tag to synchronize demodulation window

└─────────────────────────────────────────────────────────────────────┘
```

---

## 9. Barker Code Spreading Visualization

```
BARKER CODE SPREADING (1M and 2M rates)
┌─────────────────────────────────────────────────────────────────────┐

Barker-11 code: [1, -1, 1, 1, -1, 1, 1, 1, -1, -1, -1]

Spreading example for DBPSK 1M:

Input byte: 0x5C = 01011100 (binary: [bit0, bit1, ..., bit7])
                           = [0, 0, 1, 1, 1, 0, 1, 0]

Each bit becomes a DQPSK symbol, then spreads with Barker:

┌─────────────┬────────────────┬─────────────────────────────────┐
│ Bit  │ Value│ DBPSK Phase    │ Spread with Barker              │
├─────────────┼────────────────┼─────────────────────────────────┤
│ 0    │ 0    │ phase+0 = 0    │ [1, -1, 1, 1, -1, 1, 1, 1, -1, │
│      │      │ exp(j×0)=1+0j  │  -1, -1] × (1+0j)              │
│      │      │                │ = [1, -1, 1, 1, -1, 1, 1, 1,   │
│      │      │                │   -1, -1, -1]                  │
├─────────────┼────────────────┼─────────────────────────────────┤
│ 1    │ 0    │ phase+0 = 0    │ [1, -1, 1, 1, -1, 1, 1, 1, -1, │
│      │      │ exp(j×0)=1+0j  │  -1, -1]                       │
├─────────────┼────────────────┼─────────────────────────────────┤
│ 2    │ 1    │ phase+π = π    │ [-1, 1, -1, -1, 1, -1, -1, -1, │
│      │      │ exp(j×π)=-1+0j │  1, 1, 1]                      │
├─────────────┼────────────────┼─────────────────────────────────┤
│ ...  │ ...  │ ...            │ ...                             │
├─────────────┼────────────────┼─────────────────────────────────┤
│ 7    │ 0    │ phase+0        │ [1, -1, 1, 1, -1, 1, 1, 1, -1, │
│      │      │ exp(j×phase)   │  -1, -1] × exp(j×phase)        │
└─────────────┴────────────────┴─────────────────────────────────┘

Total output: 8 bits × 11 chips/bit = 88 complex samples

Receiver Detection:
  Each Barker-11 sequence correlates with itself:
  
  Autocorrelation(lag=0) = 11 (max)
  Autocorrelation(lag≠0) ≤ 1 (low)
  
  This provides:
  ├─ Strong synchronization peak
  ├─ Timing recovery capability
  ├─ Processing gain: 10 log₁₀(11) = 10.4 dB
  └─ Sidelobe rejection: 11:1 ratio

Spectral Characteristics:
├─ Main lobe width: 2 × chip_rate / spread_factor
│  = 2 × 11 MHz / 11 = 2 MHz
├─ Bandwidth: ~22 MHz (FCC limit for 802.11b)
└─ Power spectral density: Well-behaved (low PAPR)

└─────────────────────────────────────────────────────────────────────┘
```

---

## 10. Complete Symbol Timeline for 5.5 Mbps

```
SYMBOL TIMELINE: 5.5 Mbps CCK Encoding
┌─────────────────────────────────────────────────────────────────────┐

Rate: 5.5 Mbps
Chip rate: 11 MHz
Chips per symbol: 16 (2 symbols per byte)
Symbol duration: 16 chips / 11 MHz = 1.45 µs
Byte duration: 2 × 1.45 = 2.91 µs

Input: Byte sequence (preamble, header, PSDU)

PREAMBLE (18 bytes):
├─ Modulation: DBPSK 1M (even though rate=5.5M)
├─ Each byte → 88 chips
├─ Total: 18 × 88 = 1,584 chips
└─ Duration: 144 µs

HEADER (6 bytes):
├─ Modulation: 2M DQPSK (for short preamble) or 1M (for long)
│  (For 5.5M long preamble: DQPSK 2M is NOT used in header)
├─ Each byte → 44 or 88 chips
└─ Duration: depends on preamble type

PSDU (N bytes):
├─ Modulation: CCK 5.5M
├─ Symbol stream:
│
│  Byte 0:
│  ├─ Extract bits [b7 b6 | b5 b4 b3 b2 | b1 b0]
│  ├─ Symbol 0 (I): bits b1b0, b3b2, b4 → 8 chips
│  │  Phase(b1b0) + Phase(b3b2) + 0 + Phase(b4)
│  ├─ Symbol 1 (Q): bits b5b4, b7b6, b6 → 8 chips
│  │  Phase(b5b4) + Phase(b7b6) + 0 + Phase(b6)
│  └─ Total: 16 chips
│
│  Byte 1:
│  ├─ Process similar to Byte 0
│  └─ Add phase accumulation from Byte 0
│
│  Byte N:
│  └─ Continue with differential phase tracking
│
├─ Total: N × 16 chips
└─ Duration: N × 1.45 µs

APPENDED SYMBOLS (Barker terminator):
├─ Fixed 11 chips
└─ Duration: 1 µs

Total transmission duration:
T = (preamble_chips + header_chips + psdu_chips + 11) / 11 MHz

Example (100-byte PSDU, long preamble):
T = (1,584 + 528 + 100×16 + 11) / 11 MHz
  = (1,584 + 528 + 1,600 + 11) / 11 MHz
  = 3,723 / 11 MHz
  = 338.5 µs

└─────────────────────────────────────────────────────────────────────┘
```

---

## 11. State Transitions in Chip Mapper

```
CHIP MAPPER STATE TRANSITIONS
┌─────────────────────────────────────────────────────────────────────┐

State Variables:
├─ d_count: Total PPDU bytes to process
├─ d_copy: Bytes processed so far
├─ d_preType: Long (true) or short (false) preamble
├─ d_rate: Rate code (0-6)
├─ d_psdu_symbol_count: Symbol counter for 11M (tracks even/odd)
└─ d_append: Appended chips counter

State Transitions:

┌──────────┐
│   IDLE   │ (waiting for input)
│ d_count=0│
│ d_copy=0 │
└────┬─────┘
     │ Input buffer with "packet_len" tag arrives
     ▼
┌──────────────────────────────────┐
│ TAG_CHECK                        │
│ ├─ Extract packet_len from tag   │
│ ├─ Extract rate from input[0]    │
│ ├─ Call updateRate(rate)         │
│ ├─ Set d_count = packet_len - 1  │
│ ├─ Set d_copy = 0                │
│ └─ Initialize d_phase_acc = 0    │
└────┬─────────────────────────────┘
     │
     ▼
┌──────────────────────────────────┐
│ PREAMBLE_GEN                     │
│ while d_copy < preamble_len:     │
│ ├─ Generate dbpsk_1M_chips()     │
│ ├─ Accumulate 88 chips           │
│ ├─ Increment d_copy              │
│ └─ If output buffer full: return │
└────┬─────────────────────────────┘
     │ (preamble complete)
     ▼
┌──────────────────────────────────┐
│ HEADER_GEN                       │
│ if d_preType:                    │
│ │ Generate dbpsk_1M_chips()      │
│ └─ 88 chips/byte                 │
│ else:                            │
│ ├─ First 6 bytes: dbpsk_1M_chips│
│ │  (88 chips/byte)               │
│ └─ Handled by conditional block  │
└────┬─────────────────────────────┘
     │ (header complete)
     ▼
┌──────────────────────────────────┐
│ PSDU_GEN                         │
│ while d_copy < d_count:          │
│ ├─ Call (*d_chip_mapper)()       │
│ │ ├─ dbpsk_1M_chips() → 88      │
│ │ ├─ dqpsk_2M_chips() → 44      │
│ │ ├─ cck_5_5M_chips() → 16      │
│ │ └─ cck_11M_chips() → 8        │
│ ├─ Accumulate chips              │
│ ├─ Increment d_psdu_symbol_count │
│ ├─ Increment d_copy              │
│ └─ If output buffer full: return │
└────┬─────────────────────────────┘
     │ (all PSDU bytes processed)
     ▼
┌──────────────────────────────────┐
│ APPEND_BARKER                    │
│ if d_append < 11:                │
│ ├─ Copy d_append_symbols[d_append│
│ ├─ Increment d_append            │
│ └─ Output 1 chip per call        │
└────┬─────────────────────────────┘
     │ (appended symbols complete)
     ▼
┌──────────────────────────────────┐
│ PACKET_COMPLETE                  │
│ ├─ Reset d_count = 0             │
│ ├─ Reset d_copy = 0              │
│ ├─ Reset d_append = 11           │
│ └─ Ready for next packet         │
└────┬─────────────────────────────┘
     │
     ▼
┌──────────┐
│   IDLE   │ (waiting for next packet)
└──────────┘

Conditional Branches:

PREAMBLE/HEADER Processing:
  ├─ If d_preType (long preamble):
  │  ├─ Preamble: bytes 0-17 → dbpsk_1M_chips
  │  └─ Header: bytes 18-23 → dbpsk_1M_chips
  │
  └─ Else (short preamble):
     ├─ Preamble: bytes 0-8 → dbpsk_1M_chips
     └─ Header: bytes 9-14 → dqpsk_2M_chips (special case!)

Chip Generation Loop:
  Accumulates output chips until:
  ├─ Output buffer full → return chips
  └─ All input bytes consumed → advance state

Buffer Overflow Handling:
  if (nout + nout_check() > noutput_items):
      return current_output
  else:
      continue processing

└─────────────────────────────────────────────────────────────────────┘
```

