# gr-ieee80211 Documentation

> **Comprehensive technical documentation for the IEEE 802.11 a/b/g/n/ac SDR transceiver**

---

## 📚 Complete Documentation Suite

All documentation has been organized into the **[docs/](docs/)** directory.

### 🚀 Quick Start

**New Users**: Start with **[docs/README.md](docs/README.md)** - the master documentation index

**Developers**: See **[docs/architecture/BUILD_SYSTEM_QUICK_REFERENCE.md](docs/architecture/BUILD_SYSTEM_QUICK_REFERENCE.md)**

**Researchers**: Begin with **[docs/architecture/ARCHITECTURE.md](docs/architecture/ARCHITECTURE.md)**

---

## 📂 Documentation Organization

### **[docs/architecture/](docs/architecture/)** - System Design (5 files, 82 KB)
- System architecture and design patterns
- CMake build system analysis
- Project structure and organization
- Dependencies and installation

**Key Files**:
- [ARCHITECTURE.md](docs/architecture/ARCHITECTURE.md) - **START HERE**
- [BUILD_SYSTEM_ANALYSIS.md](docs/architecture/BUILD_SYSTEM_ANALYSIS.md)
- [BUILD_SYSTEM_QUICK_REFERENCE.md](docs/architecture/BUILD_SYSTEM_QUICK_REFERENCE.md)

### **[docs/analysis/](docs/analysis/)** - Technical Deep Dives (18 files, 400+ KB)
- Complete OFDM TX/RX chain analysis
- Complete DSSS TX/RX chain analysis
- Utility libraries and helper functions
- Message passing and block interactions
- Python bindings and GRC blocks

**Key Files**:
- [ofdm_analysis.md](docs/analysis/ofdm_analysis.md) - OFDM transmission
- [OFDM_RECEPTION_CHAIN_ANALYSIS.md](docs/analysis/OFDM_RECEPTION_CHAIN_ANALYSIS.md) - OFDM reception
- [DSSS_TX_CHAIN_README.md](docs/analysis/DSSS_TX_CHAIN_README.md) - DSSS transmission
- [DSSS_CCK_RECEPTION_ANALYSIS.md](docs/analysis/DSSS_CCK_RECEPTION_ANALYSIS.md) - DSSS reception
- [UTILITIES_ANALYSIS.md](docs/analysis/UTILITIES_ANALYSIS.md) - Utility functions
- [MESSAGE_PASSING_ANALYSIS.md](docs/analysis/MESSAGE_PASSING_ANALYSIS.md) - Block communication
- [BINDING_ARCHITECTURE_ANALYSIS.md](docs/analysis/BINDING_ARCHITECTURE_ANALYSIS.md) - Python/GRC

### **[docs/reference/](docs/reference/)** - Quick References (10 files, 100+ KB)
- Block API reference
- Examples catalog and quick starts
- Lookup tables and constants
- Visual diagrams and flowcharts

**Key Files**:
- [BLOCKS_REFERENCE.md](docs/reference/BLOCKS_REFERENCE.md) - All 17 blocks
- [EXAMPLES_README.txt](docs/reference/EXAMPLES_README.txt) - Examples guide
- [EXAMPLES_QUICK_REFERENCE.md](docs/reference/EXAMPLES_QUICK_REFERENCE.md) - Quick start
- [DSSS_QUICK_REFERENCE.md](docs/reference/DSSS_QUICK_REFERENCE.md) - DSSS lookups

---

## 🎯 Documentation by Role

### 🆕 **I want to try WiFi examples**
1. Read [docs/reference/EXAMPLES_README.txt](docs/reference/EXAMPLES_README.txt)
2. Follow [docs/reference/EXAMPLES_QUICK_REFERENCE.md](docs/reference/EXAMPLES_QUICK_REFERENCE.md)
3. Check [examples/README_WIFI_EVOLUTION_DEMO.md](examples/README_WIFI_EVOLUTION_DEMO.md)

### 🔧 **I want to build from source**
1. Read [docs/architecture/BUILD_SYSTEM_QUICK_REFERENCE.md](docs/architecture/BUILD_SYSTEM_QUICK_REFERENCE.md)
2. Review dependencies in [docs/architecture/BUILD_SYSTEM_ANALYSIS.md](docs/architecture/BUILD_SYSTEM_ANALYSIS.md)
3. Follow standard build process:
   ```bash
   mkdir build && cd build
   cmake ..
   make -j4
   sudo make install
   sudo ldconfig
   ```

### 🔬 **I want to understand the PHY layer**
1. Start with [docs/architecture/ARCHITECTURE.md](docs/architecture/ARCHITECTURE.md)
2. For OFDM: [docs/analysis/ofdm_analysis.md](docs/analysis/ofdm_analysis.md)
3. For DSSS: [docs/analysis/DSSS_TX_CHAIN_README.md](docs/analysis/DSSS_TX_CHAIN_README.md)

### 💻 **I want to develop new blocks**
1. Understand architecture: [docs/architecture/ARCHITECTURE.md](docs/architecture/ARCHITECTURE.md)
2. Learn message passing: [docs/analysis/MESSAGE_PASSING_ANALYSIS.md](docs/analysis/MESSAGE_PASSING_ANALYSIS.md)
3. Study bindings: [docs/analysis/BINDING_ARCHITECTURE_ANALYSIS.md](docs/analysis/BINDING_ARCHITECTURE_ANALYSIS.md)
4. Reference existing blocks: [docs/reference/BLOCKS_REFERENCE.md](docs/reference/BLOCKS_REFERENCE.md)

### 📊 **I want implementation details**
- **OFDM Encoding**: [docs/analysis/ofdm_analysis.md](docs/analysis/ofdm_analysis.md)
- **OFDM Decoding**: [docs/analysis/OFDM_RECEPTION_CHAIN_ANALYSIS.md](docs/analysis/OFDM_RECEPTION_CHAIN_ANALYSIS.md)
- **DSSS Encoding**: [docs/analysis/DSSS_ANALYSIS.md](docs/analysis/DSSS_ANALYSIS.md)
- **DSSS Decoding**: [docs/analysis/DSSS_CCK_RECEPTION_ANALYSIS.md](docs/analysis/DSSS_CCK_RECEPTION_ANALYSIS.md)
- **Utilities**: [docs/analysis/UTILITIES_ANALYSIS.md](docs/analysis/UTILITIES_ANALYSIS.md)

---

## 📊 Coverage Summary

| Component | Documentation | Status |
|-----------|--------------|--------|
| Architecture & Design | 82 KB (5 files) | ✅ Complete |
| OFDM TX Chain | 63 KB | ✅ Complete |
| OFDM RX Chain | 56 KB | ✅ Complete |
| DSSS TX Chain | 89 KB | ✅ Complete |
| DSSS RX Chain | 39 KB | ✅ Complete |
| Utilities & Helpers | 29 KB | ✅ Complete |
| Message Passing | 21 KB | ✅ Complete |
| Python Bindings | 69 KB | ✅ Complete |
| Examples Catalog | 53 KB | ✅ Complete |
| Build System | 55 KB | ✅ Complete |
| **TOTAL** | **~600 KB, 30+ files** | **✅ 100%** |

---

## 🌟 Highlights

### Supported WiFi Standards

| Standard | Year | Max Rate | Documentation |
|----------|------|----------|--------------|
| 802.11b | 1999 | 11 Mbps | [DSSS Analysis](docs/analysis/DSSS_TX_CHAIN_README.md) |
| 802.11a/g | 1999-2003 | 54 Mbps | [OFDM Analysis](docs/analysis/ofdm_analysis.md) |
| 802.11n | 2009 | 150 Mbps | [OFDM RX](docs/analysis/OFDM_RECEPTION_CHAIN_ANALYSIS.md) |
| 802.11ac | 2013 | 866+ Mbps | [Architecture](docs/architecture/ARCHITECTURE.md) |

### Key Features Documented

- ✅ **17 GNU Radio Blocks** - Complete API reference
- ✅ **23 Example Applications** - Full catalog with usage
- ✅ **10+ Algorithms** - Detailed analysis (Viterbi, PLL, CCK, etc.)
- ✅ **50+ ASCII Diagrams** - Visual flowcharts and data flows
- ✅ **200+ Code Examples** - Practical implementation snippets
- ✅ **100+ Reference Tables** - Lookup values and constants

---

## 🔗 Quick Links

**Main Documentation Index**: [docs/README.md](docs/README.md)

**By Category**:
- [Architecture](docs/architecture/) - System design & build
- [Analysis](docs/analysis/) - Technical deep dives
- [Reference](docs/reference/) - Quick lookups & examples

**By Standard**:
- [802.11b DSSS](docs/analysis/DSSS_TX_CHAIN_README.md)
- [802.11a/g OFDM](docs/analysis/ofdm_analysis.md)

**Popular Pages**:
- [Build Guide](docs/architecture/BUILD_SYSTEM_QUICK_REFERENCE.md)
- [Block Reference](docs/reference/BLOCKS_REFERENCE.md)
- [Examples Guide](docs/reference/EXAMPLES_README.txt)
- [Utilities](docs/analysis/UTILITIES_ANALYSIS.md)

---

## 📚 External Resources

- **Original Repository**: https://github.com/bastibl/gr-ieee80211
- **IEEE 802.11 Standard**: https://standards.ieee.org/standard/802_11-2020.html
- **GNU Radio**: https://www.gnuradio.org/

---

## 📝 License

Documentation licensed under GNU GPL v3.0+ (same as gr-ieee80211 source code)

---

**[→ Start Reading: docs/README.md](docs/README.md)**
