# gr-ieee80211 Utility Libraries and Helper Functions

## Overview

This document provides a comprehensive analysis of the utility libraries and helper functions in the gr-ieee80211 project. The utilities are organized into core categories: FCS/CRC/scrambler functions, WiFi rate definitions and metadata, OFDM PHY constants and processing, and rate adaptation/statistics collection.

---

## 1. Core Utilities (utils.h / utils.cc)

Location: `/include/gnuradio/ieee80211/utils.h` and `/lib/utils.cc`

### 1.1 FCS (Frame Check Sequence) Functions

**Purpose**: Implements IEEE 802.11 Frame Check Sequence (CRC-32) for MAC frame integrity validation.

#### Function: `calc_fcs()`
```cpp
IEEE80211_API uint32_t calc_fcs(const uint8_t* data, size_t len);
```
- **Algorithm**: CRC-32 with IEEE 802.11 polynomial
- **Polynomial**: G(x) = x^32 + x^26 + x^23 + x^22 + x^16 + x^12 + x^11 + x^10 + x^8 + x^7 + x^5 + x^4 + x^2 + x + 1
- **Initialization**: CRC register = 0xFFFFFFFF
- **Finalization**: Result inverted (~crc)
- **Implementation**: Uses precomputed 256-entry lookup table for performance
- **Use Case**: Calculate FCS for MAC frames before transmission

#### Function: `validate_fcs()`
```cpp
IEEE80211_API bool validate_fcs(const uint8_t* data, size_t len);
```
- **Operation**: Verifies received FCS by recalculating and comparing
- **Input**: Complete frame including 4-byte FCS appended
- **Return**: true if FCS matches, false otherwise
- **Requirement**: Frame length must be at least 4 bytes

#### Function: `extract_fcs()`
```cpp
IEEE80211_API uint32_t extract_fcs(const uint8_t* data, size_t len);
```
- **Purpose**: Extracts the 4-byte FCS from end of frame
- **Returns**: FCS value in host byte order
- **Safety**: Returns 0 if frame < 4 bytes

#### CRC-32 Lookup Table
- **Size**: 256 entries (one per byte value)
- **Purpose**: Accelerates CRC calculation using table-driven approach
- **Performance**: O(n) complexity with minimal computation per byte

### 1.2 PLCP (Physical Layer Convergence Procedure) CRC-16

**Purpose**: Implements CCITT CRC-16 for 802.11b PLCP header validation.

#### Function: `calc_plcp_crc16()`
```cpp
IEEE80211_API uint16_t calc_plcp_crc16(const uint8_t* header);
```
- **Polynomial**: x^16 + x^12 + x^5 + 1 (CCITT standard)
- **Input**: 4-byte PLCP header (SIGNAL, SERVICE, LENGTH)
- **Initialization**: CRC register = 0xFFFF
- **Algorithm**: Bit-by-bit processing with feedback masking
- **Use Case**: Protect DSSS PLCP header (different from MAC FCS)

#### Function: `validate_plcp_crc16()`
```cpp
IEEE80211_API bool validate_plcp_crc16(const uint8_t* header);
```
- **Input**: 6-byte PLCP header (4 data bytes + 2 CRC bytes)
- **Operation**: Recalculates CRC and compares with received value
- **Return**: true if valid, false otherwise

### 1.3 Scrambler Functions

**Purpose**: Implements IEEE 802.11b scrambler (7-bit LFSR) for DSSS/CCK data scrambling.

#### Function: `scramble()`
```cpp
IEEE80211_API void scramble(uint8_t* data, size_t len, uint8_t init = 0x1B);
```
- **Type**: 7-bit Linear Feedback Shift Register
- **Polynomial**: S(x) = x^7 + x^4 + 1
- **Initial States**:
  - `0x1B`: Long preamble (standard)
  - `0x6C`: Short preamble
- **Operation**: 
  - Processes input bit-by-bit
  - Combines input with feedback from positions 3 and 6
  - Updates 7-bit state register after each bit
- **Key Property**: Self-inverse operation (scrambling = descrambling)
- **Processing**: In-place modification of input buffer

#### Function: `descramble()`
```cpp
inline void descramble(uint8_t* data, size_t len, uint8_t init = 0x1B);
```
- **Implementation**: Simple wrapper around `scramble()`
- **Advantage**: Makes intent explicit in code

### 1.4 Miscellaneous Utility Functions

#### CRC-32 Byte Processing
```cpp
IEEE80211_API uint32_t crc32_byte(uint32_t crc, uint8_t byte);
```
- **Purpose**: Processes single byte in CRC-32 calculation
- **Use**: Runtime table generation or incremental CRC updates
- **Formula**: `crc32[byte_val ^ (crc & 0xFF)] ^ (crc >> 8)`

#### Power Conversion Functions
```cpp
IEEE80211_API float dbm_to_linear(float dbm);
IEEE80211_API float linear_to_dbm(float linear);
```
- **Purpose**: Convert between logarithmic (dBm) and linear power scales
- **Formula**:
  - dbm_to_linear: 10^(dBm/10)
  - linear_to_dbm: 10 * log10(linear)
- **Use Case**: Signal strength analysis and power level calculations

---

## 2. WiFi Rates and Modulation Info (wifi_rates.h / wifi_rates.cc)

Location: `/include/gnuradio/ieee80211/wifi_rates.h` and `/lib/wifi_rates.cc`

### 2.1 WiFi Rate Enumeration

**Purpose**: Unified rate representation across all WiFi standards (802.11b/a/g/n/ac).

#### 802.11b DSSS/CCK Rates
```cpp
enum class wifi_rate {
    // Long preamble (144 bits)
    DSSS_1M_LONG,      // 1 Mbps DBPSK
    DSSS_2M_LONG,      // 2 Mbps DQPSK
    DSSS_5_5M_LONG,    // 5.5 Mbps CCK
    DSSS_11M_LONG,     // 11 Mbps CCK
    
    // Short preamble (72 bits)
    DSSS_2M_SHORT,     // 2 Mbps DQPSK
    DSSS_5_5M_SHORT,   // 5.5 Mbps CCK
    DSSS_11M_SHORT,    // 11 Mbps CCK
    
    // 802.11a/g OFDM (8 rates, 20 MHz)
    OFDM_6M, OFDM_9M, OFDM_12M, OFDM_18M,
    OFDM_24M, OFDM_36M, OFDM_48M, OFDM_54M,
    
    // 802.11n HT (MCS 0-7, 20 MHz)
    HT_MCS0_20MHz,  // 6.5 Mbps
    HT_MCS1_20MHz,  // 13 Mbps
    // ... through HT_MCS7_20MHz (65 Mbps)
    
    // 802.11ac VHT (MCS 0-9, 20 MHz)
    VHT_MCS0_20MHz,  // 6.5 Mbps
    // ... through VHT_MCS9_20MHz (86.7 Mbps)
};
```

**Note**: Extensible for 40/80 MHz bandwidths and multiple spatial streams.

### 2.2 Modulation and Coding Enumerations

#### Modulation Types
```cpp
enum class modulation_type {
    DSSS,    // Direct Sequence Spread Spectrum (802.11b)
    CCK,     // Complementary Code Keying (802.11b)
    OFDM     // Orthogonal Frequency Division Multiplexing
};
```

#### Coding Types
```cpp
enum class coding_type {
    NONE,    // No coding (DSSS/CCK)
    BCC,     // Binary Convolutional Coding (802.11a/g/n/ac)
    LDPC     // Low-Density Parity Check (802.11n/ac)
};
```

### 2.3 Rate Information Structure

```cpp
struct rate_info {
    wifi_rate rate;              // Rate enumeration value
    float mbps;                  // Data rate in Mbps
    modulation_type mod;         // Modulation scheme
    coding_type coding;          // Channel coding scheme
    int bandwidth_mhz;           // Channel bandwidth in MHz
    std::string description;     // Human-readable description
    bool long_preamble;          // Long/short preamble (802.11b only)
};
```

### 2.4 Rate Information Table

**Static mapping** containing comprehensive metadata for all 31 rates:

| Rate | Mbps | Modulation | Coding | BW | Description |
|------|------|------------|--------|----|----|
| DSSS_1M_LONG | 1.0 | DSSS | NONE | 22 MHz | 1 Mbps DBPSK (long) |
| DSSS_11M_SHORT | 11.0 | CCK | NONE | 22 MHz | 11 Mbps CCK (short) |
| OFDM_6M | 6.0 | OFDM | BCC | 20 MHz | 6 Mbps BPSK 1/2 |
| OFDM_54M | 54.0 | OFDM | BCC | 20 MHz | 54 Mbps 64-QAM 3/4 |
| HT_MCS7_20MHz | 65.0 | OFDM | BCC | 20 MHz | HT 64-QAM 5/6 |
| VHT_MCS9_20MHz | 86.7 | OFDM | BCC | 20 MHz | VHT 256-QAM 5/6 |

### 2.5 Rate Lookup Functions

```cpp
// Get complete rate information structure
const rate_info& get_rate_info(wifi_rate r);

// Get human-readable rate name
std::string get_rate_name(wifi_rate r);

// Get data rate in Mbps
float get_rate_mbps(wifi_rate r);

// Check if rate is DSSS/CCK (802.11b)
bool is_dsss_rate(wifi_rate r);

// Check if rate is OFDM-based (802.11a/g/n/ac)
bool is_ofdm_rate(wifi_rate r);
```

**Exception Handling**: `get_rate_info()` throws `std::invalid_argument` for unknown rates.

---

## 3. OFDM PHY Constants and Processing (cloud80211phy.h / cloud80211phy.cc)

Location: `/lib/cloud80211phy.h` and `/lib/cloud80211phy.cc` (3161 lines)

### 3.1 Physical Layer Frame Format Definitions

```cpp
#define C8P_F_L       0    // Legacy (802.11a/g)
#define C8P_F_HT      1    // HT (802.11n)
#define C8P_F_VHT     2    // VHT (802.11ac)
#define C8P_F_VHT_MU  3    // VHT Multi-User
```

### 3.2 Bandwidth and Coding Rate Definitions

```cpp
#define C8P_BW_20     0    // 20 MHz bandwidth
#define C8P_BW_40     1    // 40 MHz bandwidth
#define C8P_BW_80     2    // 80 MHz bandwidth

#define C8P_CR_12     0    // Coding rate 1/2
#define C8P_CR_23     1    // Coding rate 2/3
#define C8P_CR_34     2    // Coding rate 3/4
#define C8P_CR_56     3    // Coding rate 5/6
```

### 3.3 Modulation Type Definitions

```cpp
#define C8P_QAM_BPSK      0    // BPSK (1 bit/symbol)
#define C8P_QAM_QBPSK     1    // QBPSK (orthogonal BPSK)
#define C8P_QAM_QPSK      2    // QPSK (2 bits/symbol)
#define C8P_QAM_16QAM     3    // 16-QAM (4 bits/symbol)
#define C8P_QAM_64QAM     4    // 64-QAM (6 bits/symbol)
#define C8P_QAM_256QAM    5    // 256-QAM (8 bits/symbol)
```

### 3.4 Modulation Parameters

```cpp
#define C8P_MAX_N_LTF    4        // Maximum number of Long Training Fields
#define C8P_MAX_N_SS     2        // Maximum spatial streams (2x2 MIMO)
#define C8P_MAX_N_CBPSS  416      // Max coded bits per symbol per SS (256QAM)

#define C8P_SYM_SAMP_SHIFT 8      // Symbol sample shift for timing alignment
```

### 3.5 Modulation Configuration Structure (c8p_mod)

**Purpose**: Complete modulation and transmission parameters for single-user and multi-user modes.

```cpp
class c8p_mod {
public:
    // Frame format
    int format;         // L (legacy), HT, VHT, VHT_MU
    int sumu;           // 0=SU, 1=MU (Multi-User)
    int ampdu;          // A-MPDU aggregation flag
    
    // Symbol parameters
    int nSym;           // Total number of symbols
    int nSymSamp;       // Samples per symbol
    
    // Subcarrier information
    int nSD;            // Number of data subcarriers (52 for 20MHz legacy)
    int nSP;            // Number of pilot subcarriers (4 for 20MHz legacy)
    int nSS;            // Number of spatial streams (1-2)
    int nLTF;           // Number of long training fields
    
    // Modulation and coding
    int mcs;            // Modulation and Coding Scheme
    int mod;            // Modulation type (BPSK, QPSK, etc.)
    int cr;             // Coding rate (1/2, 2/3, 3/4, 5/6)
    int nBPSCS;         // Bits per subcarrier per spatial stream
    int nDBPS;          // Data bits per symbol
    int nCBPS;          // Coded bits per symbol
    int nCBPSS;         // Coded bits per symbol per spatial stream
    
    // Interleaving parameters (HT/VHT specific)
    int nIntCol;        // Interleaving columns
    int nIntRow;        // Interleaving rows
    int nIntRot;        // Interleaving rotation
    
    // Packet length
    int len;            // PSDU length for legacy/HT, APEP length for VHT
    
    // Multi-User parameters
    int groupId;        // Group ID for MU transmission
    int mcsMu[4];       // MCS for each user (0-3)
    int lenMu[4];       // Packet length for each user
    int modMu[4];       // Modulation for each user
    int crMu[4];        // Coding rate for each user
    int nBPSCSMu[4];    // Bits per subcarrier for each user
    int nDBPSMu[4];     // Data bits per symbol for each user
    int nCBPSMu[4];     // Coded bits per symbol for each user
    int nCBPSSMu[4];    // Coded bits per symbol per SS for each user
    int nIntColMu[4];   // Interleaving columns for each user
    int nIntRowMu[4];   // Interleaving rows for each user
    int nIntRotMu[4];   // Interleaving rotation for each user
};
```

### 3.6 Signal Field Structures

#### HT Signal Field (c8p_sigHt)
```cpp
class c8p_sigHt {
    int mcs;        // MCS index
    int len;        // Packet length
    int bw;         // Bandwidth (20/40 MHz)
    int smooth;     // Smoothing flag
    int noSound;    // No sounding flag
    int aggre;      // Aggregation flag
    int stbc;       // Space-Time Block Coding
    int coding;     // LDPC coding flag
    int shortGi;    // Short Guard Interval
    int nExtSs;     // Number of extension spatial streams
};
```

#### VHT Signal Field A (c8p_sigVhtA)
```cpp
class c8p_sigVhtA {
    int bw;                 // Bandwidth
    int stbc;               // STBC flag
    int groupId;            // Group ID (for MU)
    int su_nSTS;            // SU: Number of space-time streams
    int su_partialAID;      // SU: Partial AID
    int su_coding;          // SU: LDPC coding
    int su_mcs;             // SU: MCS
    int su_beamformed;      // SU: Beamformed flag
    int mu_coding[4];       // MU: LDPC coding per user
    int mu_nSTS[4];         // MU: Spatial streams per user
    int txoppsNot;          // TXOP PS Not Supported
    int shortGi;            // Short Guard Interval
    int shortGiNsymDis;     // Short GI NSYM Disambiguation
    int ldpcExtra;          // LDPC Extra OFDM Symbol
};
```

### 3.7 Training Field Constants

#### Legacy Long Training Field (LTF_L)
- **Pattern**: 64-point complex values (pilots)
- **Format**: `const gr_complex LTF_L_26_F_COMP[64]`
- **Use**: Channel estimation and synchronization for 802.11a/g

#### Non-Legacy LTF Patterns
```cpp
const float LTF_NL_28_F_FLOAT[64];           // HT/VHT LTF pattern
const float LTF_NL_28_F_FLOAT_VHT22[64];     // Variant for VHT 20/20
const float LTF_NL_28_F_FLOAT2[64];          // Scaled version
const float LTF_L_26_F_FLOAT[64];            // Float version for legacy
const float LTF_L_26_F_FLOAT2[64];           // Scaled version
```

#### Short Training Field (STF)
- **Pattern**: `const gr_complex C8P_STF_F[64]`
- **Purpose**: Automatic Gain Control (AGC) and timing synchronization
- **Energy**: Sparse pattern (only 8 filled subcarriers out of 64)

#### Pilot Patterns
```cpp
const float PILOT_P[127];              // Pseudo-random pilot sequence
const float PILOT_L[4];                // Legacy pilot sequence [1, 1, 1, -1]
const float PILOT_HT_1[4];             // HT pilot variant 1
const float PILOT_HT_2_1[4];           // HT spatial stream 1 pilots
const float PILOT_HT_2_2[4];           // HT spatial stream 2 pilots
const float PILOT_VHT[4];              // VHT pilot sequence
```

### 3.8 Subcarrier Mapping Constants

#### QAM Constellation Lookup Tables
```cpp
const gr_complex C8P_QAM_TAB_BPSK[2];      // 2 points
const gr_complex C8P_QAM_TAB_QBPSK[2];     // Orthogonal BPSK
const gr_complex C8P_QAM_TAB_QPSK[4];      // 4 points
const gr_complex C8P_QAM_TAB_16QAM[16];    // 16 points
const gr_complex C8P_QAM_TAB_64QAM[64];    // 64 points
const gr_complex C8P_QAM_TAB_256QAM[256];  // 256 points
```

#### Subcarrier Mapping
```cpp
const int MAP_QAM_TO_NONSHIFTED_SC_L[48];    // Legacy (26-subcarrier) data SC
const int MAP_QAM_TO_NONSHIFTED_SC_NL[52];   // HT/VHT (52-subcarrier) data SC

const int FFT_26_DEMAP[64];                  // 64-point to 26 SC legacy
const int FFT_26_SHIFT_DEMAP[128];           // Shifted variant for pilot/null
const int FFT_L_SIG_DEMAP[64];               // L-SIG field demapping
const int FFT_NL_SIG_DEMAP[128];             // HT/VHT signal demapping
```

#### Data/Pilot/Null Subcarrier Maps
```cpp
const int C8P_LEGACY_DP_SC[64];    // Legacy data/pilot/DC subcarrier flags
const int C8P_LEGACY_D_SC[64];     // Legacy data subcarrier mapping
```

#### Deinterleaving Maps (for QAM decode)
```cpp
const int mapDeintLegacyBpsk[48];
const int mapDeintLegacyQpsk[96];
const int mapDeintLegacy16Qam[192];
const int mapDeintLegacy64Qam[288];
const int mapDeintNonlegacyBpsk[52];
const int mapDeintNonlegacyQpsk[104];
const int mapDeintNonlegacy16Qam[208];
const int mapDeintNonlegacy64Qam[312];
const int mapDeintNonlegacy256Qam[416];
const int mapDeintVhtSigB20[52];
```

### 3.9 Convolutional Code Parameters

**Viterbi Decoder State Tables**:
```cpp
extern const int SV_STATE_NEXT[64][2];      // Next state for input 0,1
extern const int SV_STATE_OUTPUT[64][2];    // Output bits for input 0,1

// Puncturing patterns for different code rates
extern const int SV_PUNC_12[2];             // 1/2 rate: 2 bits per 2 coded
extern const int SV_PUNC_23[4];             // 2/3 rate: 4 bits per 6 coded
extern const int SV_PUNC_34[6];             // 3/4 rate: 6 bits per 8 coded
extern const int SV_PUNC_56[10];            // 5/6 rate: 10 bits per 12 coded
```

### 3.10 Signal Field and CRC Definitions

```cpp
const uint8_t LEGACY_RATE_BITS[8][4];       // Legacy rate encoding (4 bits)
const uint8_t VHT_NDP_SIGB_20_BITS[26];     // VHT NDP SIG-B pattern
const uint8_t EOF_PAD_SUBFRAME[32];         // Subframe padding pattern
```

### 3.11 Signal Decoding Class (svSigDecoder)

**Purpose**: Viterbi decoder for legacy signal field decoding.

```cpp
class svSigDecoder {
private:
    int i, j, t;
    float accum_err_metric0[64];        // Error metrics for states
    float accum_err_metric1[64];
    int state_history[64][49];          // Trellis state history
    int state_sequence[49];             // Decoded state sequence
    
    // Viterbi decoder internal variables
    int op0, op1, next0, next1;
    float acc_tmp0, acc_tmp1, t0, t1;
    float tbl_t[4];

public:
    void decode(float* llrv, uint8_t* decoded_bits, int trellisLen);
};
```

### 3.12 PHY Processing Functions (exported)

#### QAM and Subcarrier Processing
```cpp
// Convert chips/bits to QAM constellation points
void procChipsToQam(const uint8_t* inChips, gr_complex* outQam, int qamType, int len);
void procChipsToQamNonShiftedScL(const uint8_t* inChips, gr_complex* outQam, int qamType);
void procChipsToQamNonShiftedScNL(const uint8_t* inChips, gr_complex* outQam, int qamType);

// Pilot and non-data subcarrier handling
void procInsertPilots(gr_complex* sigIn, gr_complex* pilots);
void procNonDataSc(gr_complex* sigIn, gr_complex* sigOut, int format);
```

#### QAM to LLR (Log-Likelihood Ratio) Conversion
```cpp
void procSymQamToLlr(gr_complex* inQam, float* outLlr, c8p_mod* mod);
```

#### Deinterleaving
```cpp
void procSymDeintL2(float* in, float* out, c8p_mod* mod);         // Legacy 2SS
void procSymDeintNL2SS1(float* in, float* out, c8p_mod* mod);    // Non-legacy SS1
void procSymDeintNL2SS2(float* in, float* out, c8p_mod* mod);    // Non-legacy SS2
```

#### Interleaving
```cpp
void procSymIntelL2(uint8_t* in, uint8_t* out, c8p_mod* mod);
void procSymIntelNL2SS1(uint8_t* in, uint8_t* out, c8p_mod* mod);
void procSymIntelNL2SS2(uint8_t* in, uint8_t* out, c8p_mod* mod);
void procIntelLegacyBpsk(uint8_t* inBits, uint8_t* outBits);
void procIntelVhtB20(uint8_t* inBits, uint8_t* outBits);
```

#### Deinterleaving Legacy Signals
```cpp
void procDeintLegacyBpsk(float* inBits, float* outBits);
```

#### Space-Time and Spatial Multiplexing
```cpp
void procSymDepasNL(float in[C8P_MAX_N_SS][C8P_MAX_N_CBPSS], float* out, c8p_mod* mod);
void procNss2SymBfQ(gr_complex* sig0, gr_complex* sig1, gr_complex* bfQ);
```

#### Cyclic Shift and Tone Scaling
```cpp
void procCSD(gr_complex* sig, int cycShift);            // Cyclic Shift Diversity
void procToneScaling(gr_complex* sig, int ntf, int nss, int len);
```

#### Coded/Uncoded Bit Conversion
```cpp
int nCodedToUncoded(int nCoded, c8p_mod* mod);
int nUncodedToCoded(int nUncoded, c8p_mod* mod);
```

#### Signal Field Demodulation and Decoding
```cpp
void procLHSigDemodDeint(gr_complex *sym1, gr_complex *sym2, 
                         gr_complex *sig, std::vector<gr_complex> &h, float *llr);
void procNLSigDemodDeint(gr_complex *sym1, gr_complex *sym2, 
                         std::vector<gr_complex> h, float *llrht, float *llrvht);
void SV_Decode_Sig(float* llrv, uint8_t* decoded_bits, int trellisLen);
```

#### Signal Field Validation and Parsing
```cpp
bool signalCheckLegacy(uint8_t* inBits, int* mcs, int* len, int* nDBPS);
bool signalCheckHt(uint8_t* inBits);
bool signalCheckVhtA(uint8_t* inBits);

void signalParserL(int mcs, int len, c8p_mod* outMod);
void signalParserHt(uint8_t* inBits, c8p_mod* outMod, c8p_sigHt* outSigHt);
void signalParserVhtA(uint8_t* inBits, c8p_mod* outMod, c8p_sigVhtA* outSigVhtA);
void signalParserVhtB(uint8_t* inBits, c8p_mod* outMod);
```

#### Modulation Parameter Parsing
```cpp
void modParserHt(int mcs, c8p_mod* outMod);
void modParserVht(int mcs, c8p_mod* outMod);
void formatToModSu(c8p_mod* mod, int format, int mcs, int nss, int len);
void formatToModMu(c8p_mod* mod, int mcs0, int nSS0, int len0, 
                   int mcs1, int nSS1, int len1);
void vhtModMuToSu(c8p_mod* mod, int pos);
void vhtModSuToMu(c8p_mod* mod, int pos);
bool formatCheck(int format, int mcs, int nss);
```

#### Encoding Functions
```cpp
void genCrc8Bits(uint8_t* inBits, uint8_t* outBits, int len);
bool checkBitCrc8(uint8_t* inBits, int len, uint8_t* crcBits);
void bccEncoder(uint8_t* inBits, uint8_t* outBits, int len);
void scramEncoder(uint8_t* inBits, uint8_t* outBits, int len, int init);
void scramEncoder2(uint8_t* inBits, int len, int init);
void punctEncoder(uint8_t* inBits, uint8_t* outBits, int len, c8p_mod* mod);
void streamParser2(uint8_t* inBits, uint8_t* outBits1, uint8_t* outBits2, 
                   int len, c8p_mod* mod);
void bitsToChips(uint8_t* inBits, uint8_t* outChips, c8p_mod* mod);
```

#### Signal Field Generation
```cpp
void legacySigBitsGen(uint8_t* sigbits, uint8_t* sigbitscoded, int mcs, int len);
void vhtSigABitsGen(uint8_t* sigabits, uint8_t* sigabitscoded, c8p_mod* mod);
void vhtSigB20BitsGenSU(uint8_t* sigbbits, uint8_t* sigbbitscoded, 
                        uint8_t* sigbbitscrc, c8p_mod* mod);
void vhtSigB20BitsGenMU(uint8_t* sigbbits0, uint8_t* sigbbitscoded0, 
                        uint8_t* sigbbitscrc0, uint8_t* sigbbits1, 
                        uint8_t* sigbbitscoded1, uint8_t* sigbbitscrc1, c8p_mod* mod);
void htSigBitsGen(uint8_t* sigbits, uint8_t* sigbitscoded, c8p_mod* mod);
```

---

## 4. Rate Adaptation (wifi_rate_adapter.h)

Location: `/include/gnuradio/ieee80211/wifi_rate_adapter.h`

### 4.1 Purpose and Overview

**Purpose**: Automatic WiFi rate adaptation to optimize throughput based on channel conditions (SNR, PER).

### 4.2 Supported Algorithms

```cpp
enum class algorithm {
    MINSTREL,    // Minstrel rate adaptation (default)
    ARF,         // Auto Rate Fallback
    AARF,        // Adaptive Auto Rate Fallback
    SAMPLE_RATE, // SampleRate algorithm
    FIXED_RATE   // Fixed rate (no adaptation)
};
```

### 4.3 Rate Adapter Interface

```cpp
class wifi_rate_adapter : virtual public gr::block {
public:
    // Factory method
    static sptr make(algorithm algo = MINSTREL,
                     wifi_rate min_rate = wifi_rate::DSSS_1M_LONG,
                     wifi_rate max_rate = wifi_rate::VHT_MCS9_20MHz,
                     float target_per = 0.1);

    // State query
    virtual wifi_rate get_current_rate() = 0;

    // Configuration
    virtual void set_algorithm(algorithm algo) = 0;
    virtual void set_rate_limits(wifi_rate min_rate, wifi_rate max_rate) = 0;
    virtual void set_target_per(float per) = 0;  // Target Packet Error Rate
    virtual void set_enabled(bool enabled) = 0;
};
```

### 4.4 Default Parameters

- **Algorithm**: Minstrel (proven to be effective in practice)
- **Minimum Rate**: DSSS_1M_LONG (1 Mbps)
- **Maximum Rate**: VHT_MCS9_20MHz (86.7 Mbps)
- **Target PER**: 0.1 (10% acceptable error rate)

---

## 5. Statistics Collection (wifi_stats_collector.h)

Location: `/include/gnuradio/ieee80211/wifi_stats_collector.h`

### 5.1 Purpose and Overview

**Purpose**: Collects, aggregates, and reports comprehensive WiFi transmission statistics for performance monitoring and analysis.

### 5.2 Statistics Report Structure

```cpp
struct stats_report {
    // Packet counters
    uint64_t rx_packets_total;      // Total received packets
    uint64_t rx_packets_success;    // Successfully decoded packets
    uint64_t rx_packets_error;      // Packets with errors
    uint64_t tx_packets_total;      // Total transmitted packets

    // Throughput metrics (bytes per second)
    double throughput_current;      // Instantaneous throughput
    double throughput_average;      // Average over reporting period
    double throughput_peak;         // Peak throughput observed

    // Signal quality metrics
    float snr_current;              // Current Signal-to-Noise Ratio
    float snr_average;              // Average SNR
    float rssi_current;             // Current Received Signal Strength
    float rssi_average;             // Average RSSI

    // Error statistics
    double per_current;             // Current Packet Error Rate
    double per_average;             // Average PER

    // Rate information
    wifi_rate current_rate;         // Current transmission rate
    modulation_type current_modulation;  // Current modulation

    // Timing
    double elapsed_time;            // Total elapsed time (seconds)
    uint64_t total_bytes;           // Total bytes transmitted/received
};
```

### 5.3 Statistics Collector Interface

```cpp
class wifi_stats_collector : virtual public gr::block {
public:
    // Factory method
    static sptr make(int update_interval_ms = 1000);

    // Statistics retrieval
    virtual stats_report get_stats() = 0;

    // Management
    virtual void reset_stats() = 0;
    virtual void set_update_interval(int interval_ms) = 0;
};
```

### 5.4 Default Parameters

- **Update Interval**: 1000 ms (1 second)
- **Message Port**: Subscribes to packet events from other blocks
- **Publication**: Periodic statistics reports via message port

---

## 6. Shared Data Structures and Constants

### 6.1 API Export Macro

```cpp
#define IEEE80211_API
```

Used to mark public API functions and classes for library export/visibility.

### 6.2 Standard Includes

**Common to all utility headers:**
```cpp
#include <gnuradio/ieee80211/api.h>
#include <cstdint>
#include <cstddef>
#include <string>
#include <iostream>
#include <map>
#include <stdexcept>
```

### 6.3 Type Definitions

**Integer types**:
- `uint8_t`: Single byte values
- `uint16_t`: 16-bit CRC values
- `uint32_t`: 32-bit FCS and CRC values
- `size_t`: Buffer lengths and offsets

**Floating-point types**:
- `float`: Power values (dBm, linear), SNR, RSSI

**Complex values**:
- `gr_complex`: Complex modulation symbols (from GNU Radio)

---

## 7. Helper Algorithms Across Blocks

### 7.1 CRC/FCS Validation Flow

```
Input Frame
    |
    v
+-------------------+
| Calculate CRC-32  |  (using 256-entry lookup table)
| over frame data   |
+-------------------+
    |
    v
+-------------------+
| Append/Extract    |  (4-byte FCS)
| from frame        |
+-------------------+
    |
    v
+-------------------+
| Compare received  |  (RX) or validate (TX)
| vs calculated    |
+-------------------+
    |
    v
Valid/Invalid FCS
```

### 7.2 Scrambler/Descrambler Flow

```
Input Data (Byte Stream)
    |
    v
+-------------------+
| Initialize LFSR   |  (0x1B or 0x6C)
| state (7 bits)    |
+-------------------+
    |
    v
For Each Byte:
+-------------------+
| Process 8 bits    |  (bit-by-bit XOR with feedback)
| LSB to MSB        |
+-------------------+
    |
    v
| Shift LFSR        |  (positions 3 and 6)
| Update state      |
+-------------------+
    |
    v
Output Data (in-place modified)
```

### 7.3 Rate Information Lookup

```
WiFi Rate Enum
    |
    v
+-------------------+
| lookup in map     |  (static rate_table)
+-------------------+
    |
    v
Rate Info (mbps, modulation, coding, bw, description)
    |
    +-> Query specific field (get_rate_name, get_rate_mbps, etc.)
```

### 7.4 Signal Processing Pipeline (OFDM)

```
Received Signal (frequency domain)
    |
    v
+-------------------+
| Extract subcarrier |  (using demap constants)
| from 64-point FFT |
+-------------------+
    |
    v
+-------------------+
| Equalize with     |  (from training fields)
| channel estimate  |
+-------------------+
    |
    v
+-------------------+
| Convert QAM to    |  (using constellation LUTs)
| log-likelihood    |
| ratio (LLR)       |
+-------------------+
    |
    v
+-------------------+
| Deinterleave      |  (using mapDeint tables)
| based on MCS      |
+-------------------+
    |
    v
+-------------------+
| Viterbi decode    |  (using SV_STATE tables)
| (convolutional)   |
+-------------------+
    |
    v
+-------------------+
| Descramble        |  (using LFSR)
| data bytes        |
+-------------------+
    |
    v
+-------------------+
| Validate FCS      |  (CRC-32)
+-------------------+
    |
    v
Decoded MAC Frame
```

---

## 8. Summary Table: All Utility Functions

| Category | Function | Purpose | Location |
|----------|----------|---------|----------|
| **FCS** | `calc_fcs()` | Compute CRC-32 FCS | utils.cc |
| | `validate_fcs()` | Verify frame FCS | utils.cc |
| | `extract_fcs()` | Extract 4-byte FCS | utils.cc |
| **PLCP** | `calc_plcp_crc16()` | DSSS PLCP CRC-16 | utils.cc |
| | `validate_plcp_crc16()` | Verify PLCP CRC | utils.cc |
| **Scrambler** | `scramble()` | 7-bit LFSR scrambling | utils.cc |
| | `descramble()` | Wrapper for clarity | utils.h |
| | `crc32_byte()` | Single-byte CRC | utils.cc |
| **Power** | `dbm_to_linear()` | dBm to watts | utils.cc |
| | `linear_to_dbm()` | Watts to dBm | utils.cc |
| **Rates** | `get_rate_info()` | Rate metadata | wifi_rates.cc |
| | `get_rate_name()` | Rate description | wifi_rates.cc |
| | `get_rate_mbps()` | Rate in Mbps | wifi_rates.cc |
| | `is_dsss_rate()` | Check 802.11b | wifi_rates.cc |
| | `is_ofdm_rate()` | Check OFDM | wifi_rates.cc |
| **Modulation** | `procChipsToQam()` | Chips to symbols | cloud80211phy.cc |
| | `procSymQamToLlr()` | QAM to LLR | cloud80211phy.cc |
| | `procSymDeintL2()` | Legacy deinterleave | cloud80211phy.cc |
| | `procSymDeintNL2SS1/SS2()` | HT/VHT deinterleave | cloud80211phy.cc |
| **Signal** | `signalCheckLegacy()` | Validate L-SIG | cloud80211phy.cc |
| | `signalCheckHt()` | Validate HT-SIG | cloud80211phy.cc |
| | `signalCheckVhtA()` | Validate VHT-SIG-A | cloud80211phy.cc |
| | `signalParserL()` | Parse legacy signal | cloud80211phy.cc |
| | `signalParserHt()` | Parse HT signal | cloud80211phy.cc |
| | `signalParserVhtA()` | Parse VHT-SIG-A | cloud80211phy.cc |
| **Encoding** | `bccEncoder()` | Conv. code encode | cloud80211phy.cc |
| | `punctEncoder()` | Puncturing | cloud80211phy.cc |
| | `scramEncoder()` | Scramble data | cloud80211phy.cc |
| | `bitsToChips()` | Bits to chips | cloud80211phy.cc |
| **Decoding** | `SV_Decode_Sig()` | Viterbi decode signal | cloud80211phy.cc |

---

## 9. Key Design Patterns

### 9.1 Lookup Tables for Performance

All CRC calculations use precomputed 256-entry lookup tables to achieve O(n) performance without bit-level operations for each byte.

### 9.2 Static Configuration

Rate information, training field patterns, and subcarrier maps are all statically defined for zero initialization overhead.

### 9.3 In-Place Modifications

Scrambler and encoding operations modify data in-place to minimize memory allocations.

### 9.4 Comprehensive Metadata

All rates are paired with complete modulation/coding/bandwidth information, eliminating the need for separate lookups.

### 9.5 Enum-Based Type Safety

Heavy use of `enum class` for format, modulation, and coding types prevents invalid combinations.

---

## 10. Integration Points

These utilities are used across multiple blocks:
- **Encoder blocks**: Use FCS calculation and scrambler
- **Decoder blocks**: Use FCS validation and descrambler
- **Modulation blocks**: Use rate information and QAM tables
- **Signal processing blocks**: Use OFDM constants and signal parsers
- **Rate adaptation**: Uses rate enumeration and statistics
- **Statistics**: Aggregates all transmission metrics

