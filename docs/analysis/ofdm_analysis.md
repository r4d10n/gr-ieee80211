# OFDM Transmission Chain Analysis: gr-ieee80211 (802.11a/g/n/ac)

## Executive Summary

The gr-ieee80211 OFDM transmission chain implements a complete physical layer encoder for IEEE 802.11 standards (Legacy/802.11g, 802.11n/HT, and 802.11ac/VHT). The chain converts raw packet data into RF-ready OFDM waveforms through a series of digital signal processing blocks that handle forward error correction, modulation, and frequency domain mapping.

---

## 1. ENCODE Blocks Overview

### 1.1 Block Types

#### **ENCODE** Block (encode.h / encode_impl.h/cc)
- **Input**: uint8_t packet data (bits)
- **Output**: uint8_t chips (mapped to constellation points)
- **Format Support**: 802.11a/g/n/ac Single-User (1x1 SISO mode)
- **Processing States**:
  - `ENCODE_S_RDTAG`: Read metadata tags (format, MCS, NSS, packet length, sequence)
  - `ENCODE_S_RDPKT`: Buffer entire packet
  - `ENCODE_S_MOD`: Perform encoding (all signal processing)
  - `ENCODE_S_COPY`: Output chips to stream
  - `ENCODE_S_CLEAN`: Cleanup/padding

#### **ENCODE2** Block (encode2.h / encode2_impl.h/cc)
- **Input**: uint8_t packet data (bits)
- **Output**: uint8_t chips (two spatial streams for 2x2 MIMO)
- **Format Support**: 802.11n/ac with 2x2 MIMO and MU-MIMO
- **Additional States**: Handles dual spatial streams (stream 0 and stream 1)

### 1.2 Data Flow Within ENCODE Block

```
Input Tags (format, mcs, nss, len, seq)
    ↓
[Service Bits (16 bits) + Packet Data (PSx8) + Padding + Tail Bits]
    ↓
Scrambler (93-state LFSR)
    ↓
BCC Encoder (Constraint Length 7, Rate 1/2)
    ↓
Puncturer (Rate matching: 1/2, 2/3, 3/4, 5/6)
    ↓
Bit Interleaver (Symbol-by-symbol)
    ↓
Bits-to-Chips Mapper (QAM constellation)
    ↓
Output Tags + Chips
```

### 1.3 Key Encoding Functions

#### 1.3.1 Signal Field Generation

**Legacy Signal Field** (802.11a/g)
```c
legacySigBitsGen(uint8_t* sigbits, uint8_t* sigbitscoded, int mcs, int len)
```
- Encodes MCS, length, and parity bits
- 24 bits → 48 bits (1/2 rate convolutional code)

**HT Signal Fields** (802.11n)
```c
htSigBitsGen(uint8_t* sigbits, uint8_t* sigbitscoded, c8p_mod* mod)
```
- Two fields: HT-SIG1 (24 bits) and HT-SIG2 (24 bits)
- Each encoded at 1/2 rate
- Total: 96 bits after coding

**VHT Signal Fields** (802.11ac)
```c
vhtSigABitsGen(uint8_t* sigabits, uint8_t* sigabitscoded, c8p_mod* mod)
vhtSigB20BitsGenSU(uint8_t* sigbbits, uint8_t* sigbbitscoded, uint8_t* sigbbitscrc, c8p_mod* mod)
```
- VHT-SIG-A: 48 bits → 96 bits (1/2 rate)
- VHT-SIG-B: 26 bits → 52 bits (1/2 rate) with CRC

#### 1.3.2 Scrambling

**Pseudorandom Number Generator**
```c
void scramEncoder(uint8_t* inBits, uint8_t* outBits, int len, int init)
void scramEncoder2(uint8_t* inBits, int len, int init)
```
- 7-tap Linear Feedback Shift Register (LFSR)
- Polynomial: x^7 + x^4 + 1
- Initialization: Typically seed = 93 (0x5D)
- Operating on all data bits (not signal bits)
- In-place operation for version 2

#### 1.3.3 Binary Convolutional Coding (BCC)

```c
void bccEncoder(uint8_t* inBits, uint8_t* outBits, int len)
```

**Encoder Parameters:**
- Constraint length: K = 7
- Code rate: 1/2 (2 output bits per input bit)
- Generator polynomials:
  - G₁ = 0o133 (133 in octal) = 1011011 in binary
  - G₂ = 0o171 (171 in octal) = 1111001 in binary

**State Machine:**
- 64 possible states (2^6 for K-1 bit shift register)
- Shift register stores previous 6 bits
- Each input bit produces 2 output bits

```
Trellis: Current State → (Output Bits, Next State)
[Implemented with lookup tables in cloud80211phy.cc]
- SV_STATE_NEXT[64][2]: Next state for inputs 0 and 1
- SV_STATE_OUTPUT[64][2]: Output bits for inputs 0 and 1
```

#### 1.3.4 Puncturing (Rate Matching)

```c
void punctEncoder(uint8_t* inBits, uint8_t* outBits, int len, c8p_mod* mod)
```

**Puncturing Patterns** (defined in cloud80211phy.cc):

| Code Rate | Input Length | Output Length | Pattern | Bits Removed |
|-----------|--------------|---------------|---------|--------------|
| 1/2       | N            | N             | None    | 0            |
| 2/3       | N            | 2N/3          | [1,1,0]  | Every 3rd    |
| 3/4       | N            | 3N/4          | [1,1,0,1] | Pattern    |
| 5/6       | N            | 5N/6          | [1,1,0,1,1,0] | Pattern |

```c
const int SV_PUNC_12[2]   = {0};        // No puncturing
const int SV_PUNC_23[4]   = {1,1,0};    // Remove every 3rd bit
const int SV_PUNC_34[6]   = {1,1,0,1,0,0}; 
const int SV_PUNC_56[10]  = {1,1,0,1,1,0,1,1,0,0};
```

#### 1.3.5 Bit Interleaving

**Legacy (802.11a/g) Interleaver:**
```c
void procSymIntelL2(uint8_t* in, uint8_t* out, c8p_mod* mod)
```

Pattern for 48 subcarriers:
- First permutation: Rotate by CBPS/16 positions
- Second permutation: Rotate by 8 positions
- Formula: i_out = ((i_in * CBPS/16) % CBPS)

**Non-Legacy (802.11n/ac) Interleaver:**
```c
void procSymIntelNL2SS1(uint8_t* in, uint8_t* out, c8p_mod* mod)
void procSymIntelNL2SS2(uint8_t* in, uint8_t* out, c8p_mod* mod)
```

Parameters:
```
NBPSC = Number of Bits Per SubCarrier (depends on modulation)
NCBPS = Number of Coded Bits Per Symbol
```

Interleaving for 52 subcarriers:
- First stage: Rotate by NCBPS/13
- Second stage: Rotate by 8

#### 1.3.6 Bits-to-Chips Mapping

```c
void bitsToChips(uint8_t* inBits, uint8_t* outChips, c8p_mod* mod)
```

**Modulation Types:**
```
C8P_QAM_BPSK    (0): 1 bit → 1 chip
C8P_QAM_QBPSK   (1): 2 bits → 1 chip (QBPSK)
C8P_QAM_QPSK    (2): 2 bits → 1 chip
C8P_QAM_16QAM   (3): 4 bits → 1 chip
C8P_QAM_64QAM   (4): 6 bits → 1 chip
C8P_QAM_256QAM  (5): 8 bits → 1 chip
```

**Constellation Mapping:**
```
BPSK:  bits [b0] → complex{+1, -1}
QPSK:  bits [b0,b1] → 4 points on unit circle
16-QAM: bits [b0,b1,b2,b3] → 16 points
64-QAM: bits [b0..b5] → 64 points
256-QAM: bits [b0..b7] → 256 points
```

Gray coding applied for optimal error performance.

### 1.4 Configuration Management

```cpp
struct c8p_mod {
    int format;        // L (0), HT (1), VHT (2), VHT_MU (3)
    int sumu;          // SU (0) or MU (1)
    int nSym;          // Number of OFDM symbols
    int nSD;           // Data subcarriers (48 for 20MHz)
    int nSP;           // Pilot subcarriers (4 for 20MHz)
    int nSS;           // Spatial streams (1 or 2)
    int nLTF;          // Number of LTFs in preamble
    int mcs;           // Modulation/Coding Scheme
    int len;           // Packet length in bytes
    int mod;           // Modulation type (QAM_*)
    int cr;            // Coding rate
    int nBPSCS;        // Bits per subcarrier
    int nDBPS;         // Data bits per symbol
    int nCBPS;         // Coded bits per symbol
    int nCBPSS;        // Coded bits per symbol per spatial stream
    int nIntCol;       // Interleaver columns
    int nIntRow;       // Interleaver rows
    int nIntRot;       // Interleaver rotation
};
```

### 1.5 Internal State Machine

```
┌─────────────────────────────────────────────┐
│ ENCODE_S_RDTAG: Read Tags                   │
│ - Extract format, mcs, nss, len, seq        │
│ - Validate format support                   │
│ - Log packet info                           │
└─────────────┬───────────────────────────────┘
              │
              ↓
┌─────────────────────────────────────────────┐
│ ENCODE_S_RDPKT: Read Packet Data            │
│ - Buffer packet bytes until complete        │
│ - Format bits in proper order (LSB first)   │
└─────────────┬───────────────────────────────┘
              │
              ↓
┌─────────────────────────────────────────────┐
│ ENCODE_S_MOD: Encode Payload                │
│ 1. Generate signal fields                   │
│ 2. Interleave signal bits                   │
│ 3. Assemble data (service + packet + pad)   │
│ 4. Scramble data                            │
│ 5. BCC encode (1/2 rate)                    │
│ 6. Puncture to target rate                  │
│ 7. Symbol-level interleave                  │
│ 8. Map bits to chips (QAM)                  │
│ 9. Generate output tags                     │
└─────────────┬───────────────────────────────┘
              │
              ↓
┌─────────────────────────────────────────────┐
│ ENCODE_S_COPY: Stream Chips                 │
│ - Copy chips to output buffer               │
│ - Respects output buffer limits             │
│ - Padding after last symbol                 │
└─────────────┬───────────────────────────────┘
              │
              ↓
┌─────────────────────────────────────────────┐
│ ENCODE_S_CLEAN: Cleanup                     │
│ - Wait for GR_PAD (160) samples             │
│ - Reset for next packet                     │
└─────────────────────────────────────────────┘
```

---

## 2. MODULATION Blocks Overview

### 2.1 Block Types

#### **MODULATION** Block (modulation.h / modulation_impl.h/cc)
- **Input**: uint8_t chips (QAM constellation points)
- **Output**: gr_complex samples (OFDM subcarriers)
- **Mode**: SISO (1x1)
- **Key Operations**:
  - Chips-to-QAM mapping
  - OFDM subcarrier mapping (including DC and guard bands)
  - Pilot subcarrier insertion
  - Null subcarrier masking

#### **MODULATION2** Block (modulation2.h / modulation2_impl.h/cc)
- **Input**: uint8_t chips (dual streams)
- **Output**: gr_complex samples (OFDM subcarriers for 2x2 MIMO)
- **Mode**: MIMO (2x2)

### 2.2 Data Flow Within MODULATION Block

```
Input Chips (uint8_t)
    ↓
[Read Packet Tags: format, mcs, nss, len, seq]
    ↓
Signal Field Processing:
  - Convert signal bits to QAM
  - Insert pilots
  - Generate complex-valued signal symbols
    ↓
Data Symbol Generation (per symbol):
  - Read nSD chips for subcarriers
  - Map chips to QAM constellation
  - Insert 4 pilot subcarriers
  - Zero DC and guard bands
  - Generate 64-point OFDM symbol
    ↓
Padding Symbols:
  - Generate 2 all-zeros symbols
  - For symbol timing
    ↓
Output 64-point Complex Samples
```

### 2.3 Key Modulation Functions

#### 2.3.1 Chips-to-QAM Mapping

**Legacy Subcarrier Mapping** (48 data + 4 pilots):
```c
void procChipsToQamNonShiftedScL(const uint8_t* inChips, gr_complex* outQam, int qamType)
```

**Non-Legacy Subcarrier Mapping** (52 data + 4 pilots):
```c
void procChipsToQamNonShiftedScNL(const uint8_t* inChips, gr_complex* outQam, int qamType)
```

**General Constellation Mapper:**
```c
void procChipsToQam(const uint8_t* inChips, gr_complex* outQam, int qamType, int len)
```

**QAM Constellation Reference:**
- BPSK:    {+1+0j, -1+0j}
- QPSK:    {+1+1j, +1-1j, -1+1j, -1-1j} / √2
- 16-QAM:  4×4 grid with Gray coding
- 64-QAM:  8×8 grid with Gray coding
- 256-QAM: 16×16 grid with Gray coding

#### 2.3.2 Pilot Insertion

```c
void procInsertPilots(gr_complex* sigIn, gr_complex* pilots)
```

**Pilot Subcarrier Positions:**
- Position -21, -7, +7, +21 (relative to DC)
- Absolute indices in 64-point FFT: 43, 57, 7, 21

**Pilot Values (Sequence):**
```c
const float PILOT_P[127];  // Pseudo-random sequence

// Legacy pilots (rotate by sequence)
const float PILOT_L[4] = {+1, +1, +1, -1};

// HT pilots (two offset sequences)
const float PILOT_HT_1[4];
const float PILOT_HT_2_1[4];
const float PILOT_HT_2_2[4];

// VHT pilots
const float PILOT_VHT[4];
```

**Pilot Pattern Rotation Per Symbol:**
```
For symbol n:
  PILOT_ROTATED[i] = PILOT_SEQUENCE[(n + offset) % 127]
```

#### 2.3.3 Subcarrier Mapping

**Legacy (802.11a/g) 64-point FFT:**
```
DC (index 0):           0 (NULL)
Negative frequencies:   -32 to -1 → indices 32 to 63
Positive frequencies:   +1 to +31 → indices 1 to 31

Guard bands:
  Indices 0, 32-63 (negative guard)
  Indices 32-39 (DC region)
  Indices 1-5 (positive guard)
  Indices 59-64 (positive guard)

Data/Pilot subcarriers:
  Indices 6-31, 33-59 (48 total)
  With 4 pilots at [43, 57, 7, 21]
  Leaving 44 data subcarriers
```

**Subcarrier Assignment (48 data):**
```
Standard FFT output ordering:
- Indices 0: DC (NULL)
- Indices 1-31: Positive frequencies (right half)
- Indices 32-63: Negative frequencies (left half, conjugate)
```

**Non-Legacy (802.11n/ac) Mapping:**
- 52 data subcarriers
- 4 pilot subcarriers
- Same FFT size (64 points)
- Similar guard band structure

#### 2.3.4 Non-Data Subcarrier Handling

```c
void procNonDataSc(gr_complex* sigIn, gr_complex* sigOut, int format)
```

**Processing:**
- Zero out DC subcarrier
- Zero out guard band subcarriers
- Conjugate mirror negative frequencies (for real-valued output after IFFT)

### 2.4 Signal Field Processing

The modulation block handles preamble signals separately from payload:

**Legacy Signal Field** (1 OFDM symbol):
- BPSK modulation (1 bit per subcarrier)
- All 48 data subcarriers used
- Interleaved bits from encode block

**HT/VHT Signal Fields** (1-3 OFDM symbols):
- HT-SIG: 2 symbols with BPSK
- VHT-SIG-A: 2 symbols with BPSK and QBPSK
- VHT-SIG-B: 1 symbol with BPSK

**Pilot Insertion:**
- Fixed sequence for signal fields
- Different from data pilots

### 2.5 Internal State Machine

```
┌────────────────────────────────┐
│ MODUL_S_RD_TAG: Read Tags      │
│ - Extract format, mcs, nss     │
│ - Pre-compute signal QAM       │
│ - Check MIMO support           │
│ - Allocate symbol buffer       │
└────────────┬────────────────────┘
             │
             ↓
┌────────────────────────────────┐
│ MODUL_S_SIG: Output Signal     │
│ - Stream signal field samples  │
│ - All 64 complex samples       │
│ - Multiple symbols if needed   │
└────────────┬────────────────────┘
             │
             ↓
┌────────────────────────────────┐
│ MODUL_S_DATA: Output Data      │
│ For each symbol:               │
│  1. Read nSD chips from input  │
│  2. Map chips to constellation │
│  3. Insert pilots              │
│  4. Zero DC and guards         │
│  5. Output 64 complex samples  │
│                                │
│ After nSym symbols:            │
│  - Output MODUL_N_PADSYM (2)   │
│  - All-zero padding symbols    │
└────────────┬────────────────────┘
             │
             ↓
┌────────────────────────────────┐
│ MODUL_S_CLEAN: Cleanup         │
│ - Wait for GR_PAP (160) input  │
│ - Reset for next packet        │
└────────────────────────────────┘
```

---

## 3. Complete Message Flow: Application to RF

```
┌──────────────────────────────────────────────────────────────┐
│                        APPLICATION                           │
│                    (Packet Generator)                        │
│            Sends PDU: [format, mcs, nss, len, ...]           │
└────────────────────────┬─────────────────────────────────────┘
                         │
                    PKTGEN Block
                  (genpkt_impl.cc)
                         │
        ┌────────────────┴───────────────────┐
        │  Message Handler                   │
        │  - Queue PDU                       │
        │  - Extract parameters              │
        │  - Calculate output length         │
        └────────────┬──────────────────────┘
                     │
           ┌─────────↓──────────┐
           │  Output: uint8_t   │
           │ [len, mcs, nss, ...│
           │  + packet data]    │
           └─────────┬──────────┘
                     │
          ENCODE Block (or ENCODE2)
         (encode_impl.cc / encode2_impl.cc)
                     │
    ┌────────────────┴────────────────────┐
    │                                     │
    │  State: ENCODE_S_MOD                │
    │  Processing per packet:             │
    │  1. Signal bits generation          │
    │  2. PSDU assembly                   │
    │  3. Scrambling (93-state LFSR)      │
    │  4. BCC encoding (1/2)              │
    │  5. Puncturing (rate matching)      │
    │  6. Symbol interleaving             │
    │  7. Chips mapping (QAM)             │
    │  8. Tag generation                  │
    │                                     │
    │  Output tags:                       │
    │  - sigl, signl, sigb0 (signal)      │
    │  - format, mcs0, nss0, len0, seq    │
    │                                     │
    └────────────┬───────────────────────┘
                 │
      ┌──────────↓──────────┐
      │  Output: uint8_t    │
      │  Chips (QAM indices)│
      │  + Tags             │
      └──────────┬──────────┘
                 │
       MODULATION Block
     (modulation_impl.cc)
                 │
   ┌─────────────┴──────────────┐
   │                            │
   │ State: MODUL_S_DATA        │
   │ Per symbol:                │
   │ 1. Read nSD chips          │
   │ 2. Chips → QAM mapping     │
   │ 3. Pilot insertion         │
   │ 4. DC/guard zeroing        │
   │ 5. 64-point OFDM symbol    │
   │                            │
   │ + 2 padding symbols        │
   │                            │
   └─────────────┬──────────────┘
                 │
      ┌──────────↓───────────┐
      │ Output: gr_complex   │
      │ 64 samples/symbol    │
      │ (nSym + 2) symbols   │
      └──────────┬───────────┘
                 │
         PAD Block
      (pad_impl.cc)
                 │
   ┌─────────────┴──────────────┐
   │                            │
   │ Prepend preamble:          │
   │ 1. Legacy STF              │
   │    - 10 OFDM symbols       │
   │    - Normalized, scaled    │
   │                            │
   │ 2. Legacy LTF              │
   │    - 2 OFDM symbols        │
   │    - Normalized, scaled    │
   │                            │
   │ 3. Scale data symbols      │
   │    - Normalize by √52      │
   │                            │
   └─────────────┬──────────────┘
                 │
      ┌──────────↓────────────────┐
      │ Output: gr_complex         │
      │ Complete PPDU              │
      │ [STF][LTF][SIG][Data][Pad]│
      │                            │
      │ Ready for transmission!    │
      └────────────────────────────┘
```

### 3.1 Metadata Tag Propagation

**Tag Format:**
```python
{
    'format': int,      # 0=Legacy, 1=HT, 2=VHT
    'mcs0': int,        # MCS index 0-9
    'nss0': int,        # Spatial streams (1-2)
    'len0': int,        # Payload length in bytes
    'seq': int,         # Packet sequence number
    'sigl': uint8_t[],  # Legacy signal bits (48)
    'signl': uint8_t[], # Non-legacy signal bits (96)
    'sigb0': uint8_t[], # VHT SIG-B (52)
    'packet_len': int,  # Total PPDU length in symbols
    'nss': int          # Spatial streams
}
```

---

## 4. Key Algorithms and Signal Processing Steps

### 4.1 Scrambler (Pseudorandom Number Generator)

**LFSR Configuration:**
```
Polynomial: 1 + x^4 + x^7
Taps:       [4, 7]
Length:     7 bits (state)
Output:     XOR of taps with LSB shift

     ┌──────────────────────────┐
     │    7-bit State Register   │
     │ [b6|b5|b4|b3|b2|b1|b0]   │
     └──────────────────────────┘
          ↑              ↓
          └──┬───────────┘
             │
         XOR Gate
             │
        Output Bit
```

**Seeding:**
- Initialization value: 93 (0x5D = 0b1011101)
- Standard for all packets

**Characteristics:**
- Period: 2^7 - 1 = 127 bits
- Whitens bit patterns
- Reduces spectral lines

### 4.2 Binary Convolutional Encoder (Rate 1/2)

**Encoder Structure:**
```
Input bit → [Shift Register (7 bits)] → Output bits (2)

        ┌─────────────────────────┐
Input──→│ S6 S5 S4 S3 S2 S1 S0 [←]├──┐
        └─────────────────────────┘  │
                 │ │  ↓              │
                 │ └──┤              │
                 │    ├─ XOR ─────→ Output 0 (G1)
                 │    │
                 │    └─ XOR ─────→ Output 1 (G2)

G1: S6 + S5 + S3 + S0      (0o133 = 1011011)
G2: S6 + S5 + S4 + S2 + S0 (0o171 = 1111001)
```

**Rate Matching After Encoding:**
- 1/2 rate: Keep all bits (no puncturing)
- 2/3 rate: Remove every 3rd bit
- 3/4 rate: Remove bits per pattern [1,1,0,1,0,0]
- 5/6 rate: Remove bits per pattern [1,1,0,1,1,0,1,1,0,0]

### 4.3 Bit Interleaver (Block Interleaver)

**Legacy (802.11a/g) for 48 Data Subcarriers:**

Stage 1 - Rotate:
```
i_after_stage1 = ((k * CBPS/16) mod CBPS)
where CBPS = number of coded bits per symbol
```

Stage 2 - Rotate:
```
i_out = (i_after_stage1 + (i_after_stage1 mod 8) * (CBPS/8)) mod CBPS
```

**Purpose:**
- Breaks up burst errors
- Distributes coded bits across subcarriers
- Improves performance over fading channels

### 4.4 OFDM Subcarrier Allocation

**Frequency Structure (20 MHz Bandwidth):**
```
        ← Negative Frequencies →  DC  ← Positive Frequencies →
    ┌─────────────────────────────┬──────────────────────────────┐
    │ 32-39: Guard              │0│ 1-5: Guard     6-31, 33-59: Data
    │ 40-53: Data/Pilots        │ │ 40-53: Pilots
    │ 54-63: Guard              │ │ 54-59: Data
    └─────────────────────────────┴──────────────────────────────┘
    
    -26 -20 -14 -8  -2   0   +2  +8 +14 +20 +26  (MHz)
```

**Subcarrier Count:**
- Legacy: 48 data + 4 pilots = 52 active (12 null)
- Non-Legacy: 52 data + 4 pilots = 56 active (8 null)

### 4.5 Pilot Subcarrier Sequence

**Frequency Domain Positions:**
```
FFT Index    Frequency    Pilot Type
   7        -21 MHz     Pilot 1
  21         +7 MHz     Pilot 2
  43        -21 MHz     Pilot 3
  57         +7 MHz     Pilot 4
```

**Time Domain Rotation:**
```
Pilot_symbol_n = Pilot_base * P[(n + offset) mod 127]

where P is a 127-length sequence defined in cloud80211phy.cc
```

**Purpose:**
- Phase tracking at receiver
- Channel estimation reference
- Timing synchronization

### 4.6 QAM Modulation Details

**BPSK (1 bit/subcarrier):**
```
bit=0 → +1+0j
bit=1 → -1+0j
```

**QPSK (2 bits/subcarrier):**
```
bits [b1 b0]:
  00 → (+1+1j)/√2
  01 → (-1+1j)/√2
  11 → (-1-1j)/√2
  10 → (+1-1j)/√2
```

**16-QAM (4 bits/subcarrier):**
```
4×4 rectangular grid, Gray coded
Real axis: {-3, -1, +1, +3}
Imag axis: {-3, -1, +1, +3}
Average power: 10
Scaling: 1/√10
```

**64-QAM (6 bits/subcarrier):**
```
8×8 rectangular grid, Gray coded
Real/Imag: {-7, -5, -3, -1, +1, +3, +5, +7}
Average power: 42
Scaling: 1/√42
```

**256-QAM (8 bits/subcarrier):**
```
16×16 rectangular grid, Gray coded
Real/Imag: {-15,...,-1,+1,...,+15}
Average power: 170
Scaling: 1/√170
```

---

## 5. Block Interconnections and Data Flow

### 5.1 Simplified Block Diagram

```
┌──────────────────────────────────────────────────────────┐
│ GNU Radio IEEE 802.11 OFDM Transmission Chain           │
└──────────────────────────────────────────────────────────┘

Message Port
     │
     ↓
 ┌────────────┐
 │  PKTGEN    │◄──── PDU: [format, mcs, nss, len, data...]
 │ (Tagged    │
 │  Stream)   │
 └─────┬──────┘
       │ [uint8_t] ← Packet data
       ↓
┌──────────────────────────────────────────┐
│ ENCODE (or ENCODE2 for 2x2)              │
│ ─────────────────────────────────────    │
│ State Machine:                           │
│  RDTAG → RDPKT → MOD → COPY → CLEAN     │
│                                          │
│ Processing:                              │
│  - Signal generation                     │
│  - Scrambling                            │
│  - BCC encoding                          │
│  - Puncturing                            │
│  - Interleaving                          │
│  - QAM mapping to chips                  │
└────────┬───────────────────────────────┘
         │ [uint8_t] ← Chips
         │ [PMT Tags] ← Metadata
         ↓
┌──────────────────────────────────────────┐
│ MODULATION (or MODULATION2)              │
│ ─────────────────────────────────────    │
│ State Machine:                           │
│  RDTAG → SIG → DATA → CLEAN              │
│                                          │
│ Processing:                              │
│  - Chips to QAM conversion               │
│  - Subcarrier mapping                    │
│  - Pilot insertion                       │
│  - DC/Guard band zeroing                 │
│  - OFDM symbol generation (64 samples)   │
└────────┬───────────────────────────────┘
         │ [gr_complex] ← OFDM samples
         ↓
┌──────────────────────────────────────────┐
│ PAD (or PAD2)                            │
│ ─────────────────────────────────────    │
│ Processing:                              │
│  - Prepend legacy preamble (STF+LTF)     │
│  - Scale payload symbols                 │
│  - Add padding symbols                   │
└────────┬───────────────────────────────┘
         │ [gr_complex] ← Complete PPDU
         ↓
    [To Transmitter]
    [USRP or other RF device]
```

### 5.2 Data Type Transformations

```
Packet Bytes (uint8_t)
    ↓
Bits (uint8_t, LSB = bit 0, MSB = bit 7)
    ↓
Scrambled Bits (uint8_t)
    ↓
Convolutional Coded Bits (uint8_t, 2x length)
    ↓
Punctured Bits (uint8_t)
    ↓
Interleaved Bits (uint8_t)
    ↓
Chips/Constellation Indices (uint8_t, 0-255)
    ↓
QAM Complex Samples (gr_complex, 52-56 values/symbol)
    ↓
OFDM Samples (gr_complex, 64 samples/symbol via FFT)
    ↓
Baseband Waveform (gr_complex, to DA converter)
    ↓
RF Signal (to antenna)
```

### 5.3 Critical Timing and Buffer Management

**ENCODE Block Buffer Sizing:**
```
Input packet: d_pkt[4095]          (4095 bytes max)
Bit buffer: d_bits0[65728]         (8.2 KB per packet)
Coded bits: d_bitsCoded[65728]     (Convolutional output)
Punctured: d_bitsPunct[65728]      (Rate matched)
Interleaved: d_bitsInted0[65728]   (Per symbol)
Output chips: d_chips0[65728]      (Constellation mapped)

Total per-block allocation: ~400 KB
```

**MODULATION Block Buffers:**
```
Signal fields:
  Legacy: d_sigl[64]               (1 symbol)
  Non-Legacy: d_signl[384]         (up to 3 symbols)

Pilot sequences:
  d_pilotsL[1408][4]               (1408 symbols × 4 pilots)
  d_pilotsHT[1408][4]
  d_pilotsVHT[1408][4]

Total per-block: ~150 KB
```

### 5.4 Sample Rates and Timing

**Standard IEEE 802.11 20 MHz Channel:**
```
FFT Size:           64 points
Subcarrier Spacing: 312.5 kHz (20 MHz / 64)
Useful Symbol Time: 3.2 μs (1 / 312.5 kHz)
Guard Interval:     0.8 μs (1/4 of symbol time)
Total Symbol Time:  4.0 μs
Sampling Rate:      20 MHz
Samples/Symbol:     80 (64 FFT + 16 CP)
Samples/FFT:        64
```

**PPDU Structure (802.11a/g example, no short GI):**
```
L-STF:          10 OFDM symbols =  400 samples = 20.0 μs
L-LTF:           2 OFDM symbols =   80 samples =  4.0 μs
L-SIG:           1 OFDM symbol  =   64 samples =  3.2 μs
[Non-Legacy Preamble: HT-SIG or VHT-SIG-A: varies]
HT-LTF/VHT-LTF: 1-4 OFDM symbols = 64-256 samples = 3.2-12.8 μs
[VHT-SIG-B: 1 symbol = 64 samples = 3.2 μs]
Data Payload:   N OFDM symbols =  64*N samples = 3.2*N μs
Padding:        2 OFDM symbols =  128 samples =  6.4 μs
────────────────────────────────────────────────
Total:         (16 + N + 2) symbols
```

---

## 6. Configuration and Parameters

### 6.1 Supported MCS Indexes

**802.11a/g Legacy (0-7):**
```
MCS | Modulation | Coding | Bits/Symbol | Throughput (20MHz)
────┼────────────┼────────┼─────────────┼──────────────────
 0  | BPSK       | 1/2    | 24          | 6 Mbps
 1  | BPSK       | 3/4    | 36          | 9 Mbps
 2  | QPSK       | 1/2    | 48          | 12 Mbps
 3  | QPSK       | 3/4    | 72          | 18 Mbps
 4  | 16-QAM     | 1/2    | 96          | 24 Mbps
 5  | 16-QAM     | 3/4    | 144         | 36 Mbps
 6  | 64-QAM     | 2/3    | 192         | 48 Mbps
 7  | 64-QAM     | 3/4    | 216         | 54 Mbps
```

**802.11n HT (0-31):**
```
MCS  | Modulation | Coding | Spatial Streams | 20 MHz | 40 MHz
─────┼────────────┼────────┼─────────────────┼────────┼────────
0-7  | (varies)   | (varies) | 1              | (varies) | 2×
8-15 | (varies)   | (varies) | 2              | 2×MCS0-7 | 4×MCS0-7
```

**802.11ac VHT (0-9):**
```
MCS | Modulation | Coding | Throughput/SS (20MHz)
────┼────────────┼────────┼──────────────────────
 0  | BPSK       | 1/2    | 6.5 Mbps
 1  | QPSK       | 1/2    | 13 Mbps
 2  | QPSK       | 3/4    | 19.5 Mbps
 3  | 16-QAM     | 1/2    | 26 Mbps
 4  | 16-QAM     | 3/4    | 39 Mbps
 5  | 64-QAM     | 2/3    | 52 Mbps
 6  | 64-QAM     | 3/4    | 58.5 Mbps
 7  | 256-QAM    | 5/6    | 65 Mbps
 8  | 256-QAM    | 3/4    | 58.5 Mbps
 9  | 256-QAM    | 5/6    | 65 Mbps
```

### 6.2 Format Identifiers

```
C8P_F_L       = 0    // 802.11a/g Legacy
C8P_F_HT      = 1    // 802.11n High Throughput
C8P_F_VHT     = 2    // 802.11ac Very High Throughput
C8P_F_VHT_MU  = 3    // 802.11ac Multi-User
```

### 6.3 Bandwidth Support

```
C8P_BW_20     = 0    // 20 MHz (primary)
C8P_BW_40     = 1    // 40 MHz (secondary channel)
C8P_BW_80     = 2    // 80 MHz (VHT only)
```

---

## 7. Performance Characteristics

### 7.1 Error Correction Performance

**Convolutional Code (K=7, Rate 1/2):**
- Free distance: d_free = 10
- Coding gain: ~5.2 dB over uncoded
- Constraint length 7 provides good BER performance
- Trellis depth: typically 5×K = 35 states

**Interleaving:**
- Reduces burst error impact
- Distributes bit errors across subcarriers
- Improves performance in fading channels

### 7.2 Spectral Characteristics

**Spectral Mask (FCC):**
- Main lobe: ±10 MHz @ 20 dB
- Adjacent channels: ±20 MHz @ 30 dB
- Extended: ±30 MHz @ 40 dB

**Subcarrier Spacing:**
- 312.5 kHz (20 MHz / 64 FFT)
- Orthogonal within guard interval
- Pilot subcarriers every 7 subcarriers

### 7.3 Throughput Calculations

**Payload Data Rate:**
```
Data Rate = (N_subcarriers × bits_per_subcarrier × N_symbols)
            / (Total PPDU Time)

Example (802.11g MCS7, SISO):
  = (48 × 6 bits × N_payload_symbols) / (20 + 4 + 3.2 + 4 × N_payload_symbols) ms
  = 54 Mbps (at high N)
```

**Protocol Overhead:**
- Preamble: ~20 μs (Legacy STF + LTF + SIG)
- Non-Legacy preamble: +8 μs (HT) to +12 μs (VHT)
- Per-packet: header + CRC processing
- Minimum PPDU: ~24 μs (smallest payload)

---

## 8. Example Signal Processing Flow (MCS7 Packet, 100 bytes)

### Detailed Step-by-Step Walkthrough

**Input:** 100-byte payload packet

**ENCODE Block Processing:**

```
1. Signal Field Generation:
   - Legacy SIG (24 bits): MCS=7, Len=100
   - Encode to 48 bits @ 1/2 rate
   - Interleave: 48 → 48 bits

2. Packet Bit Assembly:
   - Service bits: 16 bits (0x00)
   - Payload: 100 bytes = 800 bits (LSB first)
   - Tail bits: 6 bits (0x00)
   - Padding: 0 bits (800 + 6 = 806, already fits)
   - Total: 16 + 800 + 6 = 822 bits

3. Scrambling:
   - LFSR seed: 93
   - 822 bits → whitened bits

4. Convolutional Coding:
   - Input: 822 bits
   - Output: 1644 bits (2× rate)
   - Trellis trace through 64 states

5. Puncturing (3/4 rate):
   - Input: 1644 bits
   - Output: 1233 bits (1644 × 3/4)
   - Pattern: [1,1,0,1,0,0] repeated

6. Interleaving:
   - NCBPS = 288 (48 subcarriers × 6 bits)
   - NBPSC = 6
   - 5 OFDM symbols needed (1233 / 288 ≈ 5)
   - Per-symbol interleaving × 5

7. QAM Mapping:
   - 6 bits → 64-QAM constellation point
   - Generate chips/indices: 1233 chips
   - Organized into 5 symbols × 288 chips
```

**MODULATION Block Processing:**

```
1. Signal Symbol:
   - Legacy signal: 48 BPSK
   - Interleaved bits → chips
   - Map to constellation: {+1, -1}
   - Insert pilots at [43, 57, 7, 21]
   - 64-point OFDM symbol (padded with zeros)

2. Data Symbols (5 total):
   For each of 5 symbols:
     a. Read 288 chips (48 subcarriers × 6 bits each)
     b. Map to 64-QAM constellation (normalized)
     c. Place in FFT bins 1-31 and 33-59 (avoiding DC and guards)
     d. Insert 4 pilots at [43, 57, 7, 21]
     e. Zero DC (bin 0) and all guard bins
     f. OFDM symbol complete: 64 complex samples

3. Padding Symbols (2):
   - All zeros: 64 × 2 = 128 samples
   - For timing/symbol boundary

Total output: (1 signal + 5 data + 2 padding) × 64 = 512 samples
```

**PAD Block Processing:**

```
1. Prepend Legacy Preamble:
   - STF: 10 × 64 = 640 samples (scaled)
   - LTF: 2 × 64 = 128 samples (scaled)

2. Scale Symbols:
   - Signal: × (1/√52)
   - Data: × (1/√52)
   - Padding: × 0 (already zeros)

3. Final Output:
   - STF: 640 samples
   - LTF: 128 samples
   - SIG: 64 samples
   - Data: 320 samples (5 symbols)
   - Pad: 128 samples (2 symbols)
   ─────────────────────────
   Total: 1280 samples @ 20 MHz = 64 μs
```

---

## 9. Implementation Notes and Optimization

### 9.1 Memory-Efficient Buffering

The encode block uses in-place operations where possible:
```c
// Input packet buffer (4095 bytes)
uint8_t d_pkt[4095];

// Intermediate processing buffers (reused)
uint8_t d_bits0[65728];      // Bits + padding
uint8_t d_bitsCoded[65728];  // After BCC (2× expansion)
uint8_t d_bitsPunct[65728];  // After puncturing (rate matched)
uint8_t d_bitsInted0[65728]; // After interleaving
uint8_t d_chips0[65728];     // Final chips/constellation indices
```

### 9.2 Lookup Table Usage

Modulation/coding parameters pre-computed per format/MCS:
```c
// Defined in cloud80211phy.cc
const int MAP_QAM_TO_NONSHIFTED_SC_L[48];
const int MAP_QAM_TO_NONSHIFTED_SC_NL[52];

// Viterbi trellis tables
const int SV_STATE_NEXT[64][2];    // Next state lookup
const int SV_STATE_OUTPUT[64][2];  // Output bits lookup

// Deinterleaver patterns
const int mapDeintLegacyBpsk[48];
const int mapDeintLegacyQpsk[96];
const int mapDeintLegacy16Qam[192];
const int mapDeintLegacy64Qam[288];
const int mapDeintNonlegacyBpsk[52];
const int mapDeintNonlegacyQpsk[104];
// ... etc
```

### 9.3 FFT/IFFT Integration

The PAD block uses FFTW for preamble generation:
```c
gr::fft::fft_complex d_ofdm_fft(64, 1);  // Size 64, forward direction

// Pre-computed frequency domain pilots/training sequences
memcpy(d_ofdm_fft.get_inbuf(), &C8P_STF_F[32], sizeof(gr_complex)*32);
memcpy(d_ofdm_fft.get_inbuf()+32, &C8P_STF_F[0], sizeof(gr_complex)*32);
d_ofdm_fft.execute();  // IFFT to time domain
```

### 9.4 Tag Propagation

Uses GNU Radio tag system for packet metadata:
```c
pmt::pmt_t dict = pmt::make_dict();
dict = pmt::dict_add(dict, pmt::mp("format"), pmt::from_long(format_val));
// ... add more fields
add_item_tag(0, nitems_written(0), key, value, alias_pmt());
```

---

## 10. Data Format Specifications

### 10.1 Packet Format

**Input to PKTGEN (PDU Message):**
```
Byte 0:    Format (0=L, 1=HT, 2=VHT, 3=VHT_MU)
Byte 1:    MCS Index (0-9 for VHT, 0-7 for Legacy, etc.)
Byte 2:    NSS (Number of Spatial Streams: 1-4)
Bytes 3-4: Packet Length (Little-endian 16-bit)
Bytes 5+:  Payload Data (variable length)
```

### 10.2 OFDM Symbol Structure

**Time Domain (after IFFT):**
```
[Guard Interval (16)] [Useful Data (64)]
        ↓                     ↓
    0.8 μs               3.2 μs
    ← Total Symbol Time: 4.0 μs →
```

**Frequency Domain (64-point FFT):**
```
Index Range    Content              # Subcarriers
───────────────────────────────────────────────
0              DC (NULL)            1
1-31           Positive freq        31
32-39          Guard/DC region      8
40-63          Negative freq        24
(Total active: 48-52)
```

---

## Conclusion

The gr-ieee80211 OFDM transmission chain implements a comprehensive physical layer encoder that converts packet data into standards-compliant OFDM waveforms. The modular block architecture separates concerns:

1. **PKTGEN**: Protocol interface and timing
2. **ENCODE**: Baseband signal processing (FEC + modulation)
3. **MODULATION**: Frequency domain mapping and OFDM
4. **PAD**: Preamble and symbol alignment

Each block operates as a state machine, enabling streaming and pipelined processing suitable for real-time RF transmission. The use of standard algorithms (BCC, QAM, OFDM), coupled with lookup tables and efficient buffering, achieves the data rates and spectral efficiency required for modern wireless standards.

