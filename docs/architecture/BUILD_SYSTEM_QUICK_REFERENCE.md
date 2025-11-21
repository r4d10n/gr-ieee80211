# gr-ieee80211 Build System - Quick Reference Guide

## Installation Steps

```bash
# Install dependencies (Ubuntu 22.04)
sudo apt-get install git gnuradio-dev uhd-host libuhd-dev cmake build-essential

# Clone and build
cd ~/projects
git clone https://github.com/cloud9477/gr-ieee80211.git
cd gr-ieee80211
mkdir build && cd build

# Configure and build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local ../
make -j$(nproc)
make test

# Install
sudo make install
sudo ldconfig
```

## Build System Architecture

### Hierarchy
```
CMakeLists.txt (root)
├── include/gnuradio/ieee80211/CMakeLists.txt
├── lib/CMakeLists.txt
├── python/ieee80211/CMakeLists.txt
│   └── bindings/CMakeLists.txt
├── grc/CMakeLists.txt
├── docs/CMakeLists.txt
│   └── doxygen/CMakeLists.txt
└── apps/CMakeLists.txt
```

### Key Files Built

| Component | Source | Output | Destination |
|-----------|--------|--------|-------------|
| **C++ Library** | `lib/*.cc` (34 files) | `gnuradio-ieee80211.so` | `/usr/local/lib/` |
| **Python Bindings** | `python/ieee80211/bindings/*_python.cc` (17 files) | `ieee80211_python.so` | `{PYTHON_DIR}/gnuradio/ieee80211/` |
| **Headers** | `include/gnuradio/ieee80211/*.h` (17 files) | `.h` files | `/usr/local/include/gnuradio/ieee80211/` |
| **GRC Blocks** | `grc/*.block.yml` (17 files) | `.yml` files | `/usr/local/share/gnuradio/grc/blocks/` |
| **Documentation** | `docs/doxygen/` | HTML/XML | `/usr/local/share/doc/gnuradio-ieee80211/` |
| **Unit Tests** | `lib/dsss/qa_dsss.cc` | Test binary | `build/lib/` |

## Core Dependencies

### Required
```
GNU Radio               3.10+       ← Must have!
UHD                     3.9.7+      ← Must have!
C++ Compiler            C++11+      ← Must have!
CMake                   3.8+        ← Must have!
Python 3                (optional)
```

### Optional
```
Doxygen                             ← For API documentation
NumPy                               ← For Python bindings
pybind11                            ← For Python bindings
pygccxml                            ← For binding generation
```

## Build Configuration Options

### Common Commands

```bash
# Release build (default, optimized)
cmake ../

# Debug build with debugging symbols
cmake -DCMAKE_BUILD_TYPE=Debug ../

# Install to custom location
cmake -DCMAKE_INSTALL_PREFIX=$HOME/gr-ieee80211-install ../

# Disable Python bindings/GRC
cmake -DENABLE_PYTHON=OFF ../

# Skip documentation generation
cmake -DENABLE_DOXYGEN=OFF ../

# Verbose build output
make VERBOSE=1

# Parallel build with 8 cores
make -j8

# Clean build
rm -rf build && mkdir build && cd build && cmake ../
```

## What Each CMakeLists.txt Does

### `/CMakeLists.txt` (Root)
- Sets project metadata (version 1.0.0)
- Finds GNU Radio 3.10+
- Finds UHD 3.9.7+
- Defines install directory structure
- Controls Python/GRC build (ENABLE_PYTHON flag)
- Registers all subdirectories
- Configures CMake helper files

**Key Variables:**
- `CMAKE_BUILD_TYPE` - Release (default) or Debug
- `CMAKE_INSTALL_PREFIX` - Where to install
- `ENABLE_PYTHON` - Build Python bindings (default ON)
- `ENABLE_DOXYGEN` - Build documentation (default ON if found)

### `lib/CMakeLists.txt` (Core Library)
- Declares 34 C++ source files
- Creates `gnuradio-ieee80211.so` shared library
- Links to: `gnuradio-runtime`, `gnuradio-fft`, `UHD`
- Registers unit tests (C++ Boost.UTF)
- Configures symbol export (Windows compatibility)

**Key Targets:**
- Library: `gnuradio-ieee80211` (or `gr::ieee80211`)
- Test: `ieee80211_qa_dsss`

### `include/gnuradio/ieee80211/CMakeLists.txt` (Headers)
- Installs 14 main header files
- Installs 3 DSSS header files (to `dsss/` subdirectory)

**Installation Path:**
```
${CMAKE_INSTALL_PREFIX}/include/gnuradio/ieee80211/
```

### `python/ieee80211/CMakeLists.txt` (Python Module)
- Installs `__init__.py`
- Creates isolated test module directory
- Registers 14 Python unit tests

**Test Scripts:**
```
qa_trigger.py, qa_sync.py, qa_signal.py, qa_modulation.py,
qa_demod.py, qa_decode.py, qa_encode.py, qa_signal2.py,
qa_demod2.py, qa_pktgen.py, qa_encode2.py, qa_pad.py,
qa_modulation2.py, qa_pad2.py
```

### `python/ieee80211/bindings/CMakeLists.txt` (Python Bindings)
- Generates pybind11 binding code
- Creates `ieee80211_python.so` (Python module)
- Discovers NumPy include directory
- Copies built module to test directory

**Key Macro:**
```cmake
GR_PYBIND_MAKE_OOT(ieee80211 ../../.. gr::ieee80211 "${files}")
```

### `grc/CMakeLists.txt` (GUI Integration)
- Installs 17 GRC block definition files (YAML)
- Makes blocks available in GNU Radio Companion

**Installation Path:**
```
${CMAKE_INSTALL_PREFIX}/share/gnuradio/grc/blocks/
```

### `docs/CMakeLists.txt` (Documentation)
- Conditionally includes Doxygen configuration
- Generates API documentation
- Creates HTML and XML output

### `apps/CMakeLists.txt` (Applications)
- Placeholder for standalone tools/scripts
- Currently empty (no installed programs)

## Understanding the Source Organization

### C++ Source Files (34 total)

**WiFi Block Implementations:**
- `trigger_impl.cc` - Detects frame start (auto-correlation of STF)
- `sync_impl.cc` - Synchronizes timing (LTF auto-correlation)
- `signal_impl.cc` - Processes signal field (CFO compensation)
- `modulation_impl.cc` - Maps bits to QAM symbols
- `demod_impl.cc` - OFDM demodulation
- `encode_impl.cc` - BCC encoding (convolutional coding)
- `decode_impl.cc` - Viterbi decoding

**v2 Extensions (MIMO Support):**
- `signal2_impl.cc`, `demod2_impl.cc`, `encode2_impl.cc`
- `modulation2_impl.cc`, `pad2_impl.cc`

**Generators & Utilities:**
- `pktgen_impl.cc` - Packet generator
- `pad_impl.cc` - Legacy preamble padding
- `utils.cc` - Helper functions
- `wifi_rates.cc` - WiFi rate tables and adaptation
- `wifi_stats_collector_impl.cc` - Statistics collection

**DSSS/CCK (802.11b - NEW):**
- `dsss/chip_sync_c_impl.cc` - Chip synchronization
- `dsss/ppdu_chip_mapper_bc_impl.cc` - Chip to symbol mapping
- `dsss/ppdu_prefixer.cc` - DSSS preamble insertion

### Python Binding Files (17 total)
One binding file per block + aggregator:
- `{block}_python.cc` - Generated pybind11 wrapper for each block
- `python_bindings.cc` - Main module that aggregates all bindings

### Test Infrastructure

**C++ Tests:**
- `lib/dsss/qa_dsss.cc` - DSSS module unit tests (Boost.UTF)

**Python Tests:**
- 14 test scripts in `python/ieee80211/` directory
- Test coverage for all major blocks
- Located in isolated module (pre-install testing)

## Installation Destinations

### Default (/usr/local)
```
Headers:    /usr/local/include/gnuradio/ieee80211/
Library:    /usr/local/lib/libgnuradio-ieee80211.so
Python:     /usr/local/lib/pythonX.Y/dist-packages/gnuradio/ieee80211/
GRC:        /usr/local/share/gnuradio/grc/blocks/
CMake:      /usr/local/lib/cmake/gnuradio-ieee80211/
Docs:       /usr/local/share/doc/gnuradio-ieee80211/
```

### System (/usr)
```
Headers:    /usr/include/gnuradio/ieee80211/
Library:    /usr/lib/libgnuradio-ieee80211.so
Python:     /usr/lib/pythonX.Y/dist-packages/gnuradio/ieee80211/
GRC:        /usr/share/gnuradio/grc/blocks/
CMake:      /usr/lib/cmake/gnuradio-ieee80211/
Docs:       /usr/share/doc/gnuradio-ieee80211/
```

### Custom
```bash
cmake -DCMAKE_INSTALL_PREFIX=/custom/path ../
# All files go under /custom/path/
```

## Troubleshooting Build Issues

### Issue: "Could not find GNU Radio"
**Solution:**
```bash
# Ensure GNU Radio is installed
apt-get install gnuradio-dev

# Or specify path
cmake -DGnuradio_DIR=/path/to/gnuradio/lib/cmake/gnuradio ../
```

### Issue: "Could not find UHD"
**Solution:**
```bash
# Install UHD development files
apt-get install libuhd-dev

# Or specify path
cmake -DUHD_DIR=/path/to/uhd/lib/cmake/uhd ../
```

### Issue: Python bindings not generated
**Solution:**
```bash
# Check Python is available
python3 --version

# Check pybind11 is installed
python3 -c "import pybind11; print(pybind11.get_cmake_dir())"

# Try rebuilding
make clean
cmake ../
make
```

### Issue: Tests fail with "Module not found"
**Solution:**
```bash
# Run tests from build directory
cd build
make test

# Or ensure PYTHONPATH is set
export PYTHONPATH="${CMAKE_BINARY_DIR}/test_modules:$PYTHONPATH"
ctest
```

## Useful CMake Cache Variables

```bash
# View all cache variables
cmake -LA ../

# View specific cache variable
cmake -DCMAKE_BUILD_TYPE=Debug -LA ../

# Edit cache interactively (requires ccmake)
ccmake ../

# Reset cache (remove CMakeCache.txt)
rm CMakeCache.txt
cmake ../
```

## Environment Variables

```bash
# PyBOMBS prefix (automatically detected)
export PYBOMBS_PREFIX=~/pybombs
cmake ../  # Will use PYBOMBS_PREFIX as install prefix

# GNU Radio prefix (fallback)
export Gnuradio_DIR=/path/to/gnuradio/lib/cmake/gnuradio

# UHD prefix (fallback)
export UHD_DIR=/path/to/uhd/lib/cmake/uhd

# Python path (for testing)
export PYTHONPATH="${BUILD_DIR}/test_modules:$PYTHONPATH"
```

## Development Workflow

### Adding a New Block

1. **Create C++ implementation files:**
   - `lib/newblock_impl.h` - Header with interface
   - `lib/newblock_impl.cc` - Implementation

2. **Create public header:**
   - `include/gnuradio/ieee80211/newblock.h`
   - Add to `include/gnuradio/ieee80211/CMakeLists.txt`

3. **Add to library build:**
   - Add to `ieee80211_sources` list in `lib/CMakeLists.txt`

4. **Create Python binding:**
   - `python/ieee80211/bindings/newblock_python.cc`
   - Add to `ieee80211_python_files` list in bindings/CMakeLists.txt

5. **Create GRC block definition:**
   - `grc/ieee80211_newblock.block.yml`
   - Add to install list in `grc/CMakeLists.txt`

6. **Rebuild:**
   ```bash
   cd build
   cmake ../
   make
   make test
   ```

## Verifying Installation

```bash
# Check library is installed
ldconfig -p | grep gnuradio-ieee80211

# Check headers
ls /usr/local/include/gnuradio/ieee80211/

# Check Python module
python3 -c "from gnuradio import ieee80211; print(ieee80211)"

# Check GRC blocks
ls /usr/local/share/gnuradio/grc/blocks/ieee80211*.block.yml

# Use in GNU Radio Companion
gnuradio-companion
# Should see ieee80211 blocks in Tools panel
```

## Performance Build Tips

```bash
# Optimize for current CPU
cmake -DCMAKE_CXX_FLAGS="-march=native -O3" ../

# Use all CPU cores
make -j$(nproc)

# Link-time optimization (slower build, faster runtime)
cmake -DCMAKE_CXX_FLAGS="-march=native -flto" ../

# Parallel linking (requires GNU binutils)
make -j$(nproc) LDFLAGS="-Wl,--parallel=4"
```

## Cleaning Up

```bash
# Clean build artifacts
make clean

# Remove everything (rebuild from scratch)
rm -rf build
mkdir build && cd build
cmake ../
make

# Uninstall
sudo make uninstall
# or
sudo cmake -P cmake_uninstall.cmake

# Remove build directory
cd ..
rm -rf build
```

---

**Last Updated:** 2025-11-21  
**gr-ieee80211 Version:** 1.0.0  
**Build System:** CMake 3.8+  
**Platform Support:** Linux, macOS, Windows
