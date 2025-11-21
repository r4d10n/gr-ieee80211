# GR-IEEE80211 Repository Architecture Report

**Project:** GNU Radio IEEE 802.11 Wireless Transceiver  
**Repository:** gr-ieee80211  
**Version:** 1.0  
**Date:** November 21, 2025  
**Build System:** CMake  
**Primary Language:** C++ (11/17) with Python bindings  
**Target Platform:** Ubuntu 22.04 + GNU Radio 3.10  

---

## Executive Summary

**gr-ieee80211** is a comprehensive GNU Radio Out-of-Tree (OOT) module implementing a complete IEEE 802.11 WiFi physical layer transceiver supporting WiFi standards from 802.11b (1999) through 802.11ac (2013). The platform provides both C++ implementations for performance-critical components and Python tools for development and learning.

**Key Features:**
- Support for 802.11b/a/g/n/ac standards
- 1 Mbps to 866 Mbps data rates
- Single-user MIMO (SU-MIMO) and multi-user MIMO (MU-MIMO) support
- Unified GNU Radio 3.10 compatibility
- Comprehensive Python toolbox for packet generation and testing
- GRC (GNU Radio Companion) graphical flowgraph support

---

## 1. Top-Level Directory Structure

```
gr-ieee80211/
├── README.md                 # Main project documentation
├── MANIFEST.md              # Project metadata
├── CMakeLists.txt           # Root build configuration
├── .gitignore
├── .clang-format            # Code style configuration
├── .conda/                  # Conda recipe for package distribution
├── LICENSE                  # GPL-3.0-or-later license
│
├── include/gnuradio/ieee80211/    # Public C++ headers
│   ├── api.h                      # Symbol export macros
│   ├── *.h                        # Block interface definitions
│   └── dsss/                      # 802.11b specific headers
│
├── lib/                     # C++ implementation files
│   ├── *.cc                 # Block implementations
│   ├── cloud80211phy.*      # Core PHY engine
│   ├── dsss/                # 802.11b implementations
│   ├── CMakeLists.txt       # Library build configuration
│   └── utils.cc             # Utility functions
│
├── python/ieee80211/        # Python bindings and modules
│   ├── __init__.py          # Module initialization
│   ├── qa_*.py              # Unit tests (QA - Quality Assurance)
│   ├── bindings/            # Pybind11-generated bindings
│   │   ├── *_python.cc      # Binding implementations
│   │   ├── python_bindings.cc
│   │   ├── docstrings/      # Python documentation
│   │   └── CMakeLists.txt
│   └── CMakeLists.txt
│
├── grc/                     # GNU Radio Companion block definitions
│   ├── *.block.yml          # YAML block specifications
│   └── CMakeLists.txt
│
├── examples/                # Example flowgraphs and scripts
│   ├── *.grc                # GNU Radio Companion files
│   ├── wifi_evolution_demo.py      # Python demonstration
│   ├── dsss/                # 802.11b examples
│   │   ├── dsss_loopback.py
│   │   ├── dsss_loopback.grc
│   │   ├── multi_mode_transceiver.py
│   │   └── usrp_dsss_transceiver.grc
│   ├── cmu_v1/, cmu_v2/, cmu_v3/   # MIMO examples
│   └── beacon/              # Beacon transmission examples
│
├── tools/                   # Python utility toolkit
│   ├── phy80211.py          # PHY layer packet generator
│   ├── mac80211.py          # MAC layer utilities
│   ├── pktGenExample.py     # Packet generation examples
│   ├── macExampleGr*.py     # MAC-GR integration examples
│   ├── cmu_v*/              # MIMO test implementations
│   ├── performance/         # Performance testing tools
│   ├── README.md            # Tools documentation
│   └── *.py                 # Various utility scripts
│
├── cmake/Modules/           # CMake helper modules
│   ├── gnuradio-ieee80211Config.cmake
│   ├── targetConfig.cmake.in
│   └── Modules/             # GNU Radio CMake modules
│
├── docs/                    # Documentation
│   ├── dsss/                # 802.11b integration documentation
│   │   ├── FINAL_SUMMARY.md
│   │   ├── INTEGRATION_GUIDE.md
│   │   └── INTEGRATION_SUMMARY.md
│   ├── doxygen/             # Doxygen configuration
│   └── CMakeLists.txt
│
├── apps/                    # Command-line applications (reserved)
│   └── CMakeLists.txt
│
├── tmp/                     # Temporary output files (signal bins, etc.)
├── build/                   # Build output directory (generated)
└── update.sh               # Utility update script

```

---

## 2. Main Components and Their Organization

### 2.1 Core PHY Engine (`lib/cloud80211phy.*`)

The heart of the implementation - monolithic PHY processing engine (~9,400 lines).

**Purpose:** Implements fundamental WiFi physical layer algorithms
**Key Functions:**
- Signal detection and synchronization
- Channel estimation
- CFO (Carrier Frequency Offset) tracking
- OFDM modulation/demodulation
- Encoding/decoding with interleaving
- CRC calculation and validation

**Architecture Pattern:** Procedural/static methods with heavy optimization

---

### 2.2 GNU Radio Blocks (TX/RX Pipeline)

Modular blocks implementing the send/receive chain:

#### **Transmit (TX) Path:**

| Block | File | Purpose |
|-------|------|---------|
| `pktgen` | `pktgen_impl.*` | Packet source generation with MAC interface |
| `encode` / `encode2` | `encode*_impl.*` | Channel coding (BCC), scrambling, stream parsing |
| `modulation` / `modulation2` | `modulation*_impl.*` | QAM constellation mapping with pilot insertion |
| `pad` / `pad2` | `pad*_impl.*` | Legacy preamble addition, signal scaling |

#### **Receive (RX) Path:**

| Block | File | Purpose |
|-------|------|---------|
| `trigger` | `trigger_impl.*` | Auto-correlation based packet detection |
| `sync` | `sync_impl.*` | Timing synchronization using LTF correlation |
| `signal` / `signal2` | `signal*_impl.*` | CFO compensation, legacy channel estimation |
| `demod` / `demod2` | `demod*_impl.*` | OFDM demodulation, soft bit extraction |
| `decode` | `decode_impl.*` | Viterbi decoding with FCS verification |

**Naming Convention:** Primary and secondary implementations use numeric suffixes (_impl versions are the actual implementations, exposed through header interfaces).

---

### 2.3 802.11b DSSS/CCK Blocks (NEW)

Recent integration of legacy WiFi support:

| Block | File | Purpose | Key Feature |
|-------|------|---------|-------------|
| `ppdu_prefixer` | `dsss/ppdu_prefixer.cc` | PLCP header/preamble generation | Message-driven, rate-configurable |
| `ppdu_chip_mapper_bc` | `dsss/ppdu_chip_mapper_bc_impl.*` | DSSS/CCK modulation (bytes→chips) | Supports 1/2/5.5/11 Mbps |
| `chip_sync_c` | `dsss/chip_sync_c_impl.*` | DSSS/CCK demodulation (chips→packets) | Barker code correlation, PLL tracking |

**Supported Rates:**
- 1 Mbps: DBPSK + Barker code (11 chips)
- 2 Mbps: DQPSK + Barker code (11 chips)
- 5.5 Mbps: CCK-4 (4 code words)
- 11 Mbps: CCK-8 (64 code words)

**Preamble Support:**
- Long preamble: 144 bits (all rates)
- Short preamble: 72 bits (2/5.5/11 Mbps)

---

### 2.4 Utility Library

**Components:**

| File | Lines | Purpose |
|------|-------|---------|
| `utils.cc` | 176 | CRC-32 FCS, PLCP CRC-16, power conversions, scrambler |
| `wifi_rates.cc` | 98 | Rate table lookups and MCS conversion |
| `wifi_stats_collector_impl.*` | 288 | Real-time statistics collection |

**Key Functions:**
```cpp
// CRC operations
uint32_t crc32(const uint8_t* data, size_t len);
bool verify_crc32(const uint8_t* data, size_t len);
uint16_t crc16_plcp(const uint8_t* data, size_t len);

// Rate information
struct wifi_rate_info { uint8_t mcs; uint8_t bw; uint8_t gi; /* ... */ };

// Scrambler
uint8_t scramble_byte(uint8_t data, int& state);
```

---

## 3. File Naming Conventions

### 3.1 C++ Implementation Pattern

```
Header Files (include/gnuradio/ieee80211/):
  ├── block_name.h              # Public interface (block class declaration)
  └── dsss/block_name.h         # DSSS-specific blocks

Implementation Files (lib/):
  ├── block_name_impl.h         # Implementation class declaration (private)
  ├── block_name_impl.cc        # Implementation (what gets called)
  └── dsss/block_name_impl.cc   # DSSS implementations
  └── dsss/block_name.cc        # Direct implementations (some DSSS)
```

**Pattern Logic:**
- Public `.h` in include: Interface visible to users
- `*_impl.h/cc` in lib: GNU Radio's pimpl pattern (Pointer to Implementation)
- Direct `.cc` files: When block doesn't need separate impl class

### 3.2 Python Test Files

```
python/ieee80211/qa_BLOCKNAME.py

Format: qa_ prefix = "Quality Assurance" (GNU Radio convention)
Contains: Unit tests for corresponding C++ block
```

### 3.3 GRC Block Definitions

```
grc/ieee80211_BLOCKNAME.block.yml

Format: YAML (modern GNU Radio 3.10+)
Content: Block parameters, ports, documentation
```

### 3.4 Python Bindings

```
python/ieee80211/bindings/
  ├── BLOCKNAME_python.cc       # Pybind11 binding wrapper
  ├── python_bindings.cc        # Main binding entry point
  └── docstrings/BLOCKNAME_pydoc_template.h  # Documentation

Format: Pybind11 (C++17 modern binding)
```

---

## 4. Dependencies and External Libraries

### 4.1 Build-Time Dependencies

**Required:**
```cmake
find_package(Gnuradio "3.10" REQUIRED)  # GNU Radio 3.10 runtime
find_package(Gnuradio COMPONENTS fft)   # FFT algorithms
find_package(UHD "3.9.7")               # USRP Hardware Driver
```

**Optional:**
```cmake
find_package(Doxygen)                   # Documentation generation
find_package(CUDA)                      # GPU acceleration (preparatory)
```

### 4.2 Runtime Dependencies

**Core Libraries:**
- `libgnuradio-runtime.so` - GNU Radio core runtime
- `libgnuradio-fft.so` - FFT processing (used for OFDM)
- `libuhd.so` - USRP/SDR hardware interface
- `libc++` (C++17 standard library)

**Python Runtime:**
- `numpy` - Numerical computing
- `scipy` - Signal processing utilities
- `matplotlib` - Visualization (tools and examples)
- `pybind11` - C++/Python binding library

### 4.3 System Dependencies

**Ubuntu 22.04:**
```bash
gnuradio-dev          # GNU Radio development libraries
libuhd-dev            # USRP development files
cmake                 # Build system
build-essential       # C++ compiler and tools
```

### 4.4 Link Targets

```cmake
target_link_libraries(gnuradio-ieee80211
    gnuradio::gnuradio-runtime  # Main GR runtime
    gnuradio::gnuradio-fft      # FFT support
    UHD::UHD                     # USRP driver
)
```

---

## 5. Overall Design Patterns Used

### 5.1 GNU Radio Block Pattern (pimpl)

**Pattern:** Pointer to Implementation (pimpl)

```cpp
// include/gnuradio/ieee80211/decode.h (PUBLIC API)
class decode : virtual public gr::block {
    typedef std::shared_ptr<decode> sptr;
    static sptr make(/* params */);  // Factory method
};

// lib/decode_impl.h (PRIVATE IMPL)
class decode_impl : public decode {
    // Actual work() implementation
    void work(...) override;
    // Private member data
};
```

**Purpose:** Maintains binary compatibility across versions
**Benefit:** Users see stable interface; implementation can change

### 5.2 Factory Pattern

All blocks use static `make()` factory methods:
```cpp
auto block = ieee80211::ppdu_prefixer::make(rate);
```

**Benefit:** Consistent object creation, enables testing mocks

### 5.3 Message-Passing Architecture

**Two communication methods:**

1. **Stream I/O** (continuous data)
   - Used for high-bandwidth waveform processing
   - OFDM symbols, samples, chips

2. **Message Port** (discrete events)
   - Used for packet-level control
   - PPDU composition, rate updates
   - Event-driven synchronization

```cpp
// Example: ppdu_prefixer receives MAC packets via message
message_port_register_in(pmt::intern("psdu_in"));
set_msg_handler(pmt::intern("psdu_in"),
    [this](pmt::pmt_t msg) { /* handle */ });
```

### 5.4 State Machine Pattern

Used in receiver blocks for synchronization:

```cpp
enum class State {
    SEARCHING,      // Looking for packet start
    DETECTING,      // Found correlation peak
    SYNCHRONIZED,   // Locked onto timing
    DEMODULATING    // Processing packet
};
```

### 5.5 Pipeline Architecture

**TX Pipeline:**
```
[MAC Packets] → [Encode] → [Modulate] → [Pad] → [USRP/File]
   (messages)   (stream)   (stream)    (stream)
```

**RX Pipeline:**
```
[USRP/File] → [Trigger] → [Sync] → [Demod] → [Decode] → [MAC Packets]
  (stream)    (stream)  (stream) (stream)   (stream)    (messages)
```

### 5.6 Template Method Pattern

Core algorithms use procedural methods with heavy optimization:

```cpp
class cloud80211phy {
    // Core processing templates
    static void encode_data(...);      // Complex encoding
    static void modulate_ofdm(...);    // OFDM generation
    static void demodulate_ofdm(...);  // OFDM extraction
    // ~140 KB of optimized procedural code
};
```

### 5.7 Builder/Configuration Pattern

Blocks are highly configurable:

```python
# Python usage
prefixer = gr.ieee80211.ppdu_prefixer(rate=3)  # 11 Mbps
prefixer.update_rate(1)                        # Change dynamically
```

### 5.8 Inheritance Hierarchy

```
gr::block (GNU Radio base)
    ↓
ieee80211::decode (public interface)
    ↓
decode_impl (actual implementation)
```

### 5.9 C++17 Modern Features

**Used Throughout:**
- `std::shared_ptr` (smart pointers, replacing Boost)
- `std::mutex` (thread synchronization, replacing Boost)
- Lambda functions (callbacks, replacing Boost.bind)
- `std::string_view` (zero-copy strings)
- `auto` type deduction

**Rationale:** GNU Radio 3.10 standardized on C++17

---

## 6. Data Flow and Signal Processing

### 6.1 Transmit Data Flow

```
MAC PDU (bytes)
    ↓
[MAC Header Construction] ← mac80211.py
    ↓
PSDU (Payload Service Data Unit)
    ↓
[ppdu_prefixer] → Adds 802.11 PLCP header + preamble
    ↓
PPDU (Physical layer PDU)
    ↓
[encode/encode2] → Channel coding (BCC convolution code)
                 → Scrambling (7-bit LFSR)
                 → Stream parsing for MIMO
    ↓
Coded bits per subcarrier
    ↓
[modulation/modulation2] → Map to QAM constellation
                         → Insert pilot subcarriers
                         → Apply spatial mapping matrix Q (MU-MIMO)
    ↓
Complex symbols (per subcarrier)
    ↓
[FFT] → Inverse FFT for OFDM
    ↓
Time-domain samples (with cyclic prefix)
    ↓
[pad/pad2] → Add legacy preamble
           → Scale for transmit power
           → Add UHD tags for USRP timing
    ↓
Analog samples → USRP DAC → RF transmission
```

### 6.2 Receive Data Flow

```
RF samples → USRP ADC
    ↓
[Trigger] → Auto-correlation of STF (Short Training Field)
          → Detect packet onset
          → Coarse CFO estimation
    ↓
Triggered samples with sync point
    ↓
[Sync] → Auto-correlation of LTF (Long Training Field)
       → Fine timing synchronization
       → Accurate CFO re-estimation
    ↓
Time-synchronized samples
    ↓
[Signal] → CFO compensation (complex rotation)
         → Legacy channel estimation
         → Extract Legacy SIG field
         → Detect HT-SIG and VHT-SIG-A
    ↓
Channel state info + OFDM symbols
    ↓
[FFT] → Forward FFT for OFDM demodulation
    ↓
Subcarrier symbols
    ↓
[Demod/Demod2] → Extract data subcarriers (remove pilots)
               → Soft-bit LLR (Log Likelihood Ratio) calculation
               → Spatial demultiplexing (MIMO)
    ↓
Soft bits per OFDM symbol
    ↓
[Decode] → Viterbi decoder (max-likelihood path finding)
         → FCS-32 verification
         → Packet assembly
    ↓
MAC PDU (bytes) → Message output
```

### 6.3 DSSS/802.11b Specific Flow

**TX:**
```
MAC PDU → [ppdu_prefixer] → Add long/short preamble
                          → Add PLCP header with CRC-16
       → [ppdu_chip_mapper_bc] → Barker/CCK spreading
                               → BPSK/QPSK modulation
       → 11 Msps samples (1/2/5.5/11 Mbps rate variants)
```

**RX:**
```
11 Msps samples → [chip_sync_c] → Barker code correlation
                                → Packet detection
                                → PLL carrier tracking
                                → Soft-bit demodulation
                                → Rate auto-detection
               → [Viterbi decoder] → FCS validation
               → MAC PDU output
```

---

## 7. Module Architecture

### 7.1 Namespace Organization

```cpp
namespace gr {
    namespace ieee80211 {
        // Legacy blocks (802.11a/g/n/ac)
        class decode;
        class demod;
        class modulation;
        
        // DSSS blocks (802.11b)
        namespace dsss {  // Logical grouping only
            class ppdu_prefixer;
            class ppdu_chip_mapper_bc;
            class chip_sync_c;
        }
    }
}
```

### 7.2 Python Module Structure

```python
import gnuradio.ieee80211 as gr_ieee80211

# Access blocks
gr_ieee80211.decode()
gr_ieee80211.ppdu_prefixer()

# Tools modules
from tools import phy80211      # PHY packet generation
from tools import mac80211      # MAC utilities
from tools import phy80211header # Header definitions
```

---

## 8. Build System Organization

### 8.1 CMake Hierarchy

```
CMakeLists.txt (root)
    ├─→ include/gnuradio/ieee80211/CMakeLists.txt
    │       └─ Installs 22 header files
    │
    ├─→ lib/CMakeLists.txt
    │       └─ Builds libgnuradio-ieee80211.so
    │           Links: gnuradio-runtime, gnuradio-fft, UHD
    │           Includes DSSS blocks
    │           Registers unit tests
    │
    ├─→ python/ieee80211/CMakeLists.txt
    │       └─ Generates Python module
    │
    ├─→ python/ieee80211/bindings/CMakeLists.txt
    │       └─ Pybind11 binding compilation
    │           Generates ieee80211_python.so
    │
    ├─→ grc/CMakeLists.txt
    │       └─ Installs YAML block definitions
    │
    ├─→ docs/CMakeLists.txt
    │       └─ Doxygen documentation
    │
    └─→ apps/CMakeLists.txt
            └─ Command-line tools (reserved)
```

### 8.2 Build Outputs

```
build/
├── lib/                           # Compiled libraries
│   └── libgnuradio-ieee80211.so*
├── python/
│   └── ieee80211_python.*.so      # Python bindings
├── test_modules/                  # Unit test binaries
└── Testing/                       # CTest results
```

### 8.3 Installation Structure

```
$(CMAKE_INSTALL_PREFIX)/
├── lib/
│   └── libgnuradio-ieee80211.so
├── include/gnuradio/
│   └── ieee80211/                 # 22 public headers
├── share/gnuradio/ieee80211/
│   └── grc_blocks/               # YAML block defs
├── lib/python*/dist-packages/gnuradio/
│   └── ieee80211/                # Python module
└── share/doc/gnuradio-ieee80211/ # Doxygen docs
```

---

## 9. Code Metrics

### 9.1 Lines of Code

| Component | Files | Lines | Type |
|-----------|-------|-------|------|
| Core Implementation | 15 | ~5,500 | C++ |
| DSSS Implementation | 5 | ~1,650 | C++ |
| Utilities | 3 | ~500 | C++ |
| Headers | 22 | ~1,000 | C++ |
| Python Bindings | 5 | ~350 | Pybind11 |
| Python Tests | 17 | ~600 | Python |
| Python Tools | 30+ | ~3,500 | Python |
| GRC Blocks | 18 | ~209 | YAML |
| Build System | 8 | ~650 | CMake |
| Documentation | 5 | ~2,500 | Markdown |
| **TOTAL** | **130+** | **~16,000+** | Mixed |

### 9.2 Largest Components

```
1. cloud80211phy.cc      9,400 lines  - Core PHY engine
2. demod2_impl.cc        29,485 lines - (Large due to LUT tables)
3. modulation2_impl.cc   20,239 lines - (Large due to constellation)
4. encode2_impl.cc       17,343 lines - (Large due to tables)
5. decode_impl.cc        17,374 lines - Viterbi decoder
```

---

## 10. Key Design Decisions

### 10.1 Monolithic vs. Modular

**Choice:** Hybrid approach
- **Monolithic:** Core PHY engine (`cloud80211phy`) uses procedural code with heavy optimization
- **Modular:** GNU Radio blocks provide loose coupling interface
- **Rationale:** Real-time processing requires optimization; blocks provide usability

### 10.2 C++ vs. Python

**Choice:** C++ for performance, Python for development
- **C++:** Physics layer, signal processing, critical loops
- **Python:** MAC layer, packet generation, testing, examples
- **Rationale:** WiFi PHY demands real-time performance (~ms latency)

### 10.3 Pybind11 vs. SWIG

**Choice:** Pybind11 for bindings
- **Why:** C++17 native support, simpler syntax, smaller overhead
- **Previous:** SWIG (legacy, now deprecated in GNU Radio 3.10+)

### 10.4 YAML vs. XML for GRC

**Choice:** YAML block definitions
- **Why:** Modern standard in GNU Radio 3.10+, cleaner syntax
- **Previous:** XML (legacy GNU Radio 3.9)

### 10.5 Message vs. Stream Ports

**Choice:** Dual-port architecture
- **Streams:** Continuous waveform data (samples, symbols, chips)
- **Messages:** Packet events (MAC PDUs, control commands)
- **Rationale:** Proper abstraction for network protocols

---

## 11. Extension Points

### 11.1 Adding New Blocks

**Process:**
1. Create header in `include/gnuradio/ieee80211/myblock.h`
2. Create impl in `lib/myblock_impl.h` and `lib/myblock_impl.cc`
3. Add to `lib/CMakeLists.txt` source list
4. Create Python binding in `python/ieee80211/bindings/myblock_python.cc`
5. Create GRC definition in `grc/ieee80211_myblock.block.yml`
6. Add unit tests in `python/ieee80211/qa_myblock.py`

### 11.2 Adding Standards Support

**Process for new WiFi standard:**
1. Implement algorithms in `lib/std_name/` subdirectory
2. Create modular blocks (encode, modulate, demod, decode)
3. Pipeline blocks in GRC flowgraphs
4. Add unit tests and examples

### 11.3 Performance Optimization

**Hot paths identified:**
- `cloud80211phy` - Core algorithms (use intrinsics, vectorization)
- `demod_impl.cc` - Symbol detection (large LUT)
- `modulation_impl.cc` - Constellation mapping (large LUT)

---

## 12. Quality Assurance

### 12.1 Unit Tests

- 17 Python QA modules (one per major block)
- C++ unit tests in `lib/dsss/qa_dsss.cc`
- Integrated with CTest framework

### 12.2 Build Verification

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
make test                    # Run all unit tests
sudo make install
sudo ldconfig
```

### 12.3 Examples

- **GRC Flowgraphs:** 15+ example flowgraphs
- **Python Scripts:** Standalone examples in `examples/`
- **DSSS Examples:** 4 new 802.11b-specific examples

---

## 13. Documentation Structure

### 13.1 Documentation Locations

| Type | Location | Format |
|------|----------|--------|
| API Documentation | `docs/doxygen/` | Doxygen (C++) |
| Architecture | `docs/dsss/` | Markdown |
| Integration | `docs/dsss/INTEGRATION_*.md` | Markdown |
| Block Usage | `grc/*.block.yml` | YAML |
| Tool Usage | `tools/README.md` | Markdown |
| Examples | `examples/README*.md` | Markdown |

### 13.2 DSSS Documentation

- `FINAL_SUMMARY.md` - Complete implementation overview
- `INTEGRATION_GUIDE.md` - Step-by-step integration guide
- `INTEGRATION_SUMMARY.md` - Technical summary

---

## 14. GNU Radio Integration Points

### 14.1 GRC (GNU Radio Companion)

- Each block has YAML definition in `grc/`
- Parameters, ports, and documentation
- Auto-generates C++/Python flowgraphs

### 14.2 Python Binding Entry Point

```cpp
// python/ieee80211/bindings/python_bindings.cc
PYBIND11_MODULE(ieee80211_python, m) {
    // Exports all blocks to Python
    bind_decode(m);
    bind_demod(m);
    // ... etc
    bind_ppdu_prefixer(m);  // DSSS
}
```

### 14.3 Message PDU Format

Uses GNU Radio's standard PMT (PDU Message Type):
```cpp
pmt::cons(pmt::PMT_NIL, pmt::make_u8vector(data, length))
```

---

## 15. Performance Characteristics

### 15.1 Real-Time Processing

**Target:** Process one 802.11ac packet (~1-10 ms) in real-time

**Optimization Points:**
1. Monolithic PHY engine for signal processing
2. Block processing (not sample-by-sample)
3. LUT-based constellation mapping
4. Optimized Viterbi decoder

### 15.2 Throughput Capability

- **802.11b:** 11 Mbps theoretical
- **802.11a/g:** 54 Mbps
- **802.11n (2x2):** 300 Mbps
- **802.11ac (2x2):** 866 Mbps

(Actual depends on hardware and CPU)

---

## 16. Summary: Architecture Strengths

1. **Modular Design** - Blocks decouple functionality
2. **Modern C++** - C++17 standards compliance
3. **Real-Time Performance** - Optimized core engine
4. **Complete WiFi Coverage** - 802.11b through 802.11ac
5. **Dual Interface** - C++ and Python access
6. **Standards Compliance** - Follows IEEE 802.11-2020
7. **Extensible** - Clear patterns for additions
8. **Well-Documented** - Code, examples, guides
9. **Testing** - Unit tests and QA modules
10. **Active Maintenance** - Recent DSSS integration (2025)

---

**End of Report**

Generated: 2025-11-21  
Repository: gr-ieee80211  
Branch: claude/integrate-dsss-support-01VK7BT3wJt62TUfYDwpgn8t
