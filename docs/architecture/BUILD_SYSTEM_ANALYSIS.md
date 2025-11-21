# gr-ieee80211 Build System Architecture Analysis

## Project Overview

**gr-ieee80211** is a GNU Radio-based WiFi transceiver implementation supporting IEEE 802.11b/a/g/n/ac (WiFi 4/5) standards with SISO/MIMO capabilities. The build system is based on CMake and follows the standard GNU Radio out-of-tree (OOT) module structure.

**Key Characteristics:**
- CMake 3.8+ minimum requirement
- GNU Radio 3.10 required
- C++ 11 standard (CXX language)
- Python 3 bindings via pybind11
- DSSS/CCK support for 802.11b
- MIMO (SU-MIMO and MU-MIMO) support

---

## 1. Top-Level CMakeLists.txt Configuration

**Location:** `/home/user/gr-ieee80211/CMakeLists.txt`

### Project Setup
```cmake
cmake_minimum_required(VERSION 3.8)
project(gr-ieee80211 CXX C)
enable_testing()
```

- **CMake Version:** 3.8 (enables modern CMake features)
- **Languages:** CXX (C++) and C
- **Testing:** Enabled for unit tests

### Build Type Configuration
```cmake
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "")
```
- Default build type is **Release** with optimization flags
- Can be overridden with `-DCMAKE_BUILD_TYPE=`

### Version Management
```cmake
set(VERSION_MAJOR 1)
set(VERSION_API   0)
set(VERSION_ABI   0)
set(VERSION_PATCH 0)
```
- Current version: **1.0.0**

### Installation Prefix Handling
```cmake
if(DEFINED ENV{PYBOMBS_PREFIX})
    set(CMAKE_INSTALL_PREFIX $ENV{PYBOMBS_PREFIX})
endif()
```
- Supports **PyBOMBS** (Python Build-Oriented Meta-Build System)
- Automatically detects PyBOMBS installations for proper library location

### CMake Module Path
```cmake
list(INSERT CMAKE_MODULE_PATH 0 ${PROJECT_SOURCE_DIR}/cmake/Modules)
find_package(Gnuradio "3.10" REQUIRED)
```
- Local CMake modules take precedence
- Uses GNU Radio 3.10+ CMake utilities for OOT module helpers

### Compiler Settings
```cmake
include(GrCompilerSettings)
```
- Applies GNU Radio compiler standards (warnings, optimization flags)
- Uses GrMinReq macro for minimum version requirements

### Key Dependencies Found
```cmake
find_package(Gnuradio COMPONENTS fft)    # FFT component for signal processing
find_package(UHD "3.9.7")                # USRP Hardware Driver (version >= 3.9.7)
find_package(Doxygen)                    # Optional documentation generation
```

**Dependency Summary:**
| Dependency | Version | Purpose |
|-----------|---------|---------|
| GNU Radio | 3.10+ | Core DSP framework |
| GNU Radio FFT | 3.10+ | FFT blocks for OFDM |
| UHD | 3.9.7+ | USRP hardware control |
| Doxygen | Optional | API documentation |
| PkgConfig | - | Package discovery |

### Install Directory Structure
```cmake
set(GR_INCLUDE_DIR      include/gnuradio/ieee80211)
set(GR_CMAKE_DIR        ${CMAKE_MODULES_DIR}/gnuradio-ieee80211)
set(GR_PKG_DATA_DIR     ${GR_DATA_DIR}/${CMAKE_PROJECT_NAME})
set(GR_PKG_DOC_DIR      ${GR_DOC_DIR}/${CMAKE_PROJECT_NAME})
set(GR_PKG_CONF_DIR     ${GR_CONF_DIR}/${CMAKE_PROJECT_NAME}/conf.d)
set(GR_PKG_LIBEXEC_DIR  ${GR_LIBEXEC_DIR}/${CMAKE_PROJECT_NAME})
```

### Platform-Specific Configuration (macOS)
- Sets `CMAKE_INSTALL_NAME_DIR` for proper library linking
- Configures `CMAKE_INSTALL_RPATH` for runtime library discovery
- Enables `CMAKE_BUILD_WITH_INSTALL_RPATH` for consistent builds

### Build Features
- **Doxygen Support:** Conditional documentation generation
- **Uninstall Target:** Custom cmake_uninstall.cmake for clean removal
- **Compile Commands:** `CMAKE_EXPORT_COMPILE_COMMANDS ON` for IDE integration

### Subdirectories Management
```cmake
add_subdirectory(include/gnuradio/ieee80211)  # Headers
add_subdirectory(lib)                         # C++ library core
add_subdirectory(apps)                        # Standalone applications
add_subdirectory(docs)                        # Documentation
if(ENABLE_PYTHON)
  add_subdirectory(python/ieee80211)          # Python bindings
  add_subdirectory(grc)                       # GRC block definitions
endif()
```

---

## 2. Library Build Configuration (lib/CMakeLists.txt)

**Location:** `/home/user/gr-ieee80211/lib/CMakeLists.txt`

### Library Target Definition
```cmake
add_library(gnuradio-ieee80211 SHARED ${ieee80211_sources})
```
- **Library Name:** `gnuradio-ieee80211`
- **Type:** SHARED (dynamic library)
- **Target Name:** `gnuradio-ieee80211` or `gr::ieee80211`

### Source Files Organization
The library builds from the following C++ implementation files:

**Core WiFi Blocks (802.11a/g/n/ac):**
```
trigger_impl.cc              - Frame detection/trigger
sync_impl.cc                 - Synchronization
signal_impl.cc               - Signal field processing
modulation_impl.cc           - QAM modulation
demod_impl.cc                - OFDM demodulation
decode_impl.cc               - Viterbi decoding
encode_impl.cc               - BCC encoding
cloud80211phy.cc             - PHY layer core (legacy)
```

**Extended Blocks (v2 Implementation):**
```
signal2_impl.cc              - Signal field v2
demod2_impl.cc               - Demod v2 (MIMO-aware)
pktgen_impl.cc               - Packet generator
encode2_impl.cc              - Encoder v2
pad_impl.cc                  - Legacy preamble padding
modulation2_impl.cc          - Modulation v2
pad2_impl.cc                 - Pad v2
```

**Utility Modules:**
```
utils.cc                     - Helper functions
wifi_rates.cc                - WiFi rate tables and adaptation
wifi_stats_collector_impl.cc  - Statistics collection (NEW)
```

**DSSS/CCK Modules (802.11b Support - NEW):**
```
dsss/chip_sync_c_impl.cc           - Chip-level synchronization
dsss/ppdu_chip_mapper_bc_impl.cc   - Chip mapping for PPDU
dsss/ppdu_prefixer.cc              - DSSS preamble insertion
```

**Total Source Files:** ~34 files

### Dependency Linking
```cmake
target_link_libraries(gnuradio-ieee80211 
    gnuradio::gnuradio-runtime 
    gnuradio::gnuradio-fft 
    UHD::UHD)
```

**Linked Libraries:**
| Library | Target | Purpose |
|---------|--------|---------|
| GNU Radio Runtime | `gnuradio::gnuradio-runtime` | Core DSP blocks and stream processing |
| GNU Radio FFT | `gnuradio::gnuradio-fft` | OFDM FFT/IFFT operations |
| UHD | `UHD::UHD` | USRP hardware interface |

### Include Directories
```cmake
target_include_directories(gnuradio-ieee80211
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    PUBLIC $<INSTALL_INTERFACE:include>
    ${CUDA_INCLUDE_DIRS})
```

**Include Paths:**
- **Build Interface:** Local header directory relative to source
- **Install Interface:** Headers installed to system include paths
- **CUDA Support:** Optional CUDA includes for future GPU acceleration

### Library Installation
```cmake
include(GrMiscUtils)
GR_LIBRARY_FOO(gnuradio-ieee80211)
```

**Installation Details:**
- Uses GNU Radio's `GrMiscUtils` module for standard library installation
- Macro `GR_LIBRARY_FOO()` handles:
  - Library versioning
  - Symbol export
  - Target file generation
  - CMake config file installation

### Symbol Export Configuration
```cmake
set_target_properties(gnuradio-ieee80211 
    PROPERTIES DEFINE_SYMBOL "gnuradio_ieee80211_EXPORTS")
```
- Windows DLL symbol export control
- Enables proper visibility on different platforms

### macOS-Specific Configuration
```cmake
if(APPLE)
    set_target_properties(gnuradio-ieee80211 PROPERTIES
        INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib")
endif(APPLE)
```

### Unit Testing
```cmake
include(GrTest)

list(APPEND test_ieee80211_sources
    dsss/qa_dsss.cc)

list(APPEND GR_TEST_TARGET_DEPS gnuradio-ieee80211)

foreach(qa_file ${test_ieee80211_sources})
    get_filename_component(qa_name ${qa_file} NAME_WE)
    GR_ADD_CPP_TEST("ieee80211_${qa_name}"
        ${CMAKE_CURRENT_SOURCE_DIR}/${qa_file})
endforeach()
```

**Test Configuration:**
- **Test Framework:** Boost.UTF (Unit Test Framework)
- **Current Tests:** `qa_dsss.cc` (DSSS module testing)
- **Test Naming:** `ieee80211_<module_name>`
- **Test Dependencies:** Links against built library

---

## 3. Header Installation (include/CMakeLists.txt)

**Location:** `/home/user/gr-ieee80211/include/gnuradio/ieee80211/CMakeLists.txt`

### Public Header Installation
```cmake
install(FILES
    api.h trigger.h sync.h signal.h modulation.h demod.h
    decode.h encode.h signal2.h demod2.h pktgen.h encode2.h
    pad.h modulation2.h pad2.h wifi_rates.h utils.h
    DESTINATION include/gnuradio/ieee80211)
```

**Main Headers (14 files):**
| Header | Purpose |
|--------|---------|
| `api.h` | Symbol export macros |
| `trigger.h` - `pad2.h` | Block interface declarations |
| `wifi_rates.h` | WiFi rate configuration |
| `utils.h` | Utility function declarations |

### DSSS Headers Installation
```cmake
install(FILES
    dsss/chip_sync_c.h
    dsss/ppdu_chip_mapper_bc.h
    dsss/ppdu_prefixer.h
    DESTINATION include/gnuradio/ieee80211/dsss)
```

**DSSS Headers (3 files):**
- Separated into `dsss/` subdirectory
- Modular design for DSSS/CCK functionality

### Installation Destination
```
${CMAKE_INSTALL_PREFIX}/include/gnuradio/ieee80211/
```

---

## 4. Python Module Installation (python/CMakeLists.txt)

**Location:** `/home/user/gr-ieee80211/python/ieee80211/CMakeLists.txt`

### Python Framework Integration
```cmake
include(GrPython)
if(NOT PYTHONINTERP_FOUND)
    return()
endif()
```
- Uses GNU Radio's Python CMake macros
- Gracefully skips Python components if not available

### Subdirectory Organization
```cmake
add_subdirectory(bindings)
```

### Python Module Installation
```cmake
GR_PYTHON_INSTALL(
    FILES __init__.py
    DESTINATION ${GR_PYTHON_DIR}/gnuradio/ieee80211)
```

**Python Package Structure:**
```
gnuradio/ieee80211/
├── __init__.py              # Package initialization
├── ieee80211_python.so      # Python bindings (built by pybind11)
├── qa_*.py                  # Unit test scripts (14 files)
└── bindings/               # Binding source code
```

### Test Module Creation
```cmake
add_custom_target(copy_module_for_tests ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory 
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${PROJECT_BINARY_DIR}/test_modules/gnuradio/ieee80211/)
```
- Creates isolated test module directory
- Enables tests to run before installation

### Python Unit Tests
```cmake
GR_ADD_TEST(qa_trigger ${PYTHON_EXECUTABLE} ...)
GR_ADD_TEST(qa_sync ${PYTHON_EXECUTABLE} ...)
GR_ADD_TEST(qa_signal ${PYTHON_EXECUTABLE} ...)
GR_ADD_TEST(qa_modulation ${PYTHON_EXECUTABLE} ...)
GR_ADD_TEST(qa_demod ${PYTHON_EXECUTABLE} ...)
GR_ADD_TEST(qa_decode ${PYTHON_EXECUTABLE} ...)
GR_ADD_TEST(qa_encode ${PYTHON_EXECUTABLE} ...)
GR_ADD_TEST(qa_signal2 ${PYTHON_EXECUTABLE} ...)
GR_ADD_TEST(qa_demod2 ${PYTHON_EXECUTABLE} ...)
GR_ADD_TEST(qa_pktgen ${PYTHON_EXECUTABLE} ...)
GR_ADD_TEST(qa_encode2 ${PYTHON_EXECUTABLE} ...)
GR_ADD_TEST(qa_pad ${PYTHON_EXECUTABLE} ...)
GR_ADD_TEST(qa_modulation2 ${PYTHON_EXECUTABLE} ...)
GR_ADD_TEST(qa_pad2 ${PYTHON_EXECUTABLE} ...)
```

**14 Python Test Scripts** covering all major blocks

---

## 5. Python Bindings Configuration (python/ieee80211/bindings/CMakeLists.txt)

**Location:** `/home/user/gr-ieee80211/python/ieee80211/bindings/CMakeLists.txt`

### Binding Prerequisites
```cmake
if(NOT ieee80211_sources)
    MESSAGE(STATUS "No C++ sources... skipping python bindings")
    return()
endif()

GR_PYTHON_CHECK_MODULE_RAW(
    "pygccxml" "import pygccxml"
    PYGCCXML_FOUND)
```
- Skips if no C++ sources available
- Checks for `pygccxml` (XML parsing for header binding)

### NumPy Integration
```cmake
execute_process(
    COMMAND python3 -c "import os; os.chdir('/tmp'); 
            import numpy; print(numpy.get_include())"
    OUTPUT_VARIABLE NUMPY_INCLUDE_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET)

if(NUMPY_INCLUDE_DIR)
    include_directories(${NUMPY_INCLUDE_DIR})
else()
    include_directories(/usr/lib/python3/dist-packages/numpy/core/include)
endif()
```
- Dynamically discovers NumPy include directory
- Falls back to standard Ubuntu/Debian location
- Required for pybind11 to handle NumPy arrays

### pybind11 Binding Generation
```cmake
include(GrPybind)

list(APPEND ieee80211_python_files
    trigger_python.cc       signal_python.cc       modulation_python.cc
    demod_python.cc         decode_python.cc       encode_python.cc
    signal2_python.cc       demod2_python.cc       pktgen_python.cc
    encode2_python.cc       pad_python.cc          modulation2_python.cc
    pad2_python.cc
    chip_sync_c_python.cc                # DSSS blocks
    ppdu_chip_mapper_bc_python.cc
    ppdu_prefixer_python.cc
    python_bindings.cc)                  # Main module

GR_PYBIND_MAKE_OOT(ieee80211
   ../../..
   gr::ieee80211
   "${ieee80211_python_files}")
```

**Binding Architecture:**
- **17 Python Binding Files** (one per C++ block)
- **Main Module:** `python_bindings.cc` (aggregates all bindings)
- **Method:** pybind11 (Modern Python/C++ binding)
- **Target Name:** `ieee80211_python` or `ieee80211_python.so`

### Binding File Pattern
Each block has a corresponding binding file:
```cpp
// trigger_python.cc - example
#include <pybind11/pybind11.h>
#include <gnuradio/ieee80211/trigger.h>
#include <trigger_pydoc.h>

void bind_trigger(py::module& m) {
    using trigger = gr::ieee80211::trigger;
    py::class_<trigger, gr::block, gr::basic_block,
        std::shared_ptr<trigger>>(m, "trigger", D(trigger))
        .def(py::init(&trigger::make), D(trigger,make))
        ...;
}
```

### Post-Build Actions
```cmake
add_custom_command(TARGET ieee80211_python POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:ieee80211_python>
    ${PROJECT_BINARY_DIR}/test_modules/gnuradio/ieee80211/)
```
- Copies built binding module to test directory
- Enables testing before system installation

### Installation
```cmake
install(TARGETS ieee80211_python 
    DESTINATION ${GR_PYTHON_DIR}/gnuradio/ieee80211 
    COMPONENT pythonapi)
```
- Installs to standard GNU Radio Python package directory
- Component tagged as `pythonapi` for selective installation

---

## 6. GRC Block Definition (grc/CMakeLists.txt)

**Location:** `/home/user/gr-ieee80211/grc/CMakeLists.txt`

### Block Definition Installation
```cmake
install(FILES
    ieee80211_trigger.block.yml
    ieee80211_sync.block.yml
    ieee80211_signal.block.yml
    ieee80211_modulation.block.yml
    ieee80211_demod.block.yml
    ieee80211_decode.block.yml
    ieee80211_encode.block.yml
    ieee80211_signal2.block.yml
    ieee80211_demod2.block.yml
    ieee80211_pktgen.block.yml
    ieee80211_encode2.block.yml
    ieee80211_pad.block.yml
    ieee80211_modulation2.block.yml
    ieee80211_pad2.block.yml
    ieee80211_chip_sync_c.block.yml
    ieee80211_ppdu_chip_mapper_bc.block.yml
    ieee80211_ppdu_prefixer.block.yml
    DESTINATION share/gnuradio/grc/blocks)
```

**GRC Configuration:**
- **17 Block Definition Files** (YAML format)
- **Destination:** `${GR_DATA_DIR}/grc/blocks`
- **Purpose:** Defines block appearance and parameters for GNU Radio Companion GUI

**Installation Directory:**
```
/usr/share/gnuradio/grc/blocks/
/usr/local/share/gnuradio/grc/blocks/  (custom prefix)
```

---

## 7. Documentation Setup (docs/CMakeLists.txt)

**Location:** `/home/user/gr-ieee80211/docs/CMakeLists.txt`

### Doxygen Integration
```cmake
find_package(Doxygen)

if(ENABLE_DOXYGEN)
    add_subdirectory(doxygen)
endif()
```

**Documentation Configuration:**
- Optional feature controlled by `ENABLE_DOXYGEN` flag
- Delegates to `doxygen/CMakeLists.txt` for detailed configuration

### Doxygen CMakeLists Details (docs/doxygen/CMakeLists.txt)
```cmake
# Configure Doxyfile with project paths
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
    ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)

# Generate HTML and XML documentation
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/html 
           ${CMAKE_CURRENT_BINARY_DIR}/xml
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

add_custom_target(doxygen_target ALL DEPENDS ...)

# Install documentation
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html 
                  ${CMAKE_CURRENT_BINARY_DIR}/xml 
        DESTINATION ${GR_PKG_DOC_DIR})
```

**Documentation Features:**
- HTML generation enabled
- XML generation enabled (for Sphinx integration)
- LaTeX disabled (lighter documentation)
- MathJax disabled
- Graphviz (dot) support if available

---

## 8. Application/Tools Setup (apps/CMakeLists.txt)

**Location:** `/home/user/gr-ieee80211/apps/CMakeLists.txt`

### Python Application Installation
```cmake
include(GrPython)

GR_PYTHON_INSTALL(
    PROGRAMS
    DESTINATION bin)
```

**Purpose:**
- Installs Python scripts/applications to `${CMAKE_INSTALL_PREFIX}/bin`
- Currently placeholder (no programs listed)
- Used for command-line tools or example scripts

---

## 9. Dependency Management Summary

### Hard Requirements
```
GNU Radio              3.10+    - Core DSP framework
                               - CMake modules for OOT builds
                               - FFT blocks for signal processing

UHD                    3.9.7+   - USRP hardware driver
                               - Device interface library

Python                 3.6+     - Runtime for Python bindings
pybind11               -        - C++/Python binding generation
NumPy                  -        - Array handling in bindings
```

### Build Tools
```
CMake                  3.8+     - Build system
C++ Compiler           C++11+   - Modern C++ standard library
GNU Make               -        - Build execution
pkg-config             -        - Library discovery
```

### Optional Dependencies
```
Doxygen                -        - API documentation generation
Graphviz               -        - Diagram generation for docs
pygccxml               -        - XML header parsing for bindings
```

### Dependency Resolution Strategy
1. **CMake Module Path:** Local `cmake/Modules/` has priority
2. **GNU Radio Discovery:** Uses `find_package(Gnuradio)`
3. **UHD Discovery:** Uses `find_package(UHD)`
4. **NumPy Discovery:** Runtime Python discovery + fallback
5. **PkgConfig:** Fallback for library discovery

---

## 10. Build Process Flow

### Configuration Phase
```bash
cd build/
cmake ../
```
**Steps:**
1. Detect CMake version (min 3.8)
2. Load local CMake modules
3. Find GNU Radio 3.10
4. Find UHD 3.9.7+
5. Detect optional components (Doxygen, Python)
6. Configure install paths
7. Generate platform-specific makefiles

### Compilation Phase
```bash
make
```
**Steps:**
1. Compile C++ sources → `gnuradio-ieee80211.so`
2. Compile pybind11 bindings → `ieee80211_python.so`
3. Copy test modules
4. Build unit tests (if enabled)
5. Build documentation (if Doxygen enabled)

### Installation Phase
```bash
make install
```
**Steps:**
1. Install shared library → `${CMAKE_INSTALL_PREFIX}/lib`
2. Install headers → `${CMAKE_INSTALL_PREFIX}/include/gnuradio/ieee80211`
3. Install Python bindings → `${GR_PYTHON_DIR}/gnuradio/ieee80211`
4. Install GRC blocks → `share/gnuradio/grc/blocks`
5. Install documentation → `${GR_PKG_DOC_DIR}`
6. Install CMake config files → `lib/cmake/gnuradio-ieee80211`

### Test Execution
```bash
make test
# or
ctest
```
**Tests Run:**
- C++ unit tests (Boost.UTF): `ieee80211_qa_dsss`
- Python tests (14 scripts): `qa_trigger`, `qa_sync`, etc.

---

## 11. Installation Paths Reference

### Linux/Unix Default Paths
```
CMAKE_INSTALL_PREFIX:    /usr/local
Library:                 /usr/local/lib/
Headers:                 /usr/local/include/gnuradio/ieee80211/
Python:                  /usr/local/lib/pythonX.Y/dist-packages/gnuradio/ieee80211/
GRC Blocks:              /usr/local/share/gnuradio/grc/blocks/
CMake Config:            /usr/local/lib/cmake/gnuradio-ieee80211/
Documentation:           /usr/local/share/doc/gnuradio-ieee80211/
```

### With System Package Manager
```
CMAKE_INSTALL_PREFIX:    /usr
Library:                 /usr/lib/
Headers:                 /usr/include/gnuradio/ieee80211/
Python:                  /usr/lib/pythonX.Y/dist-packages/gnuradio/ieee80211/
GRC Blocks:              /usr/share/gnuradio/grc/blocks/
CMake Config:            /usr/lib/cmake/gnuradio-ieee80211/
Documentation:           /usr/share/doc/gnuradio-ieee80211/
```

### PyBOMBS Integration
```
CMAKE_INSTALL_PREFIX:    $PYBOMBS_PREFIX (e.g., /home/user/pybombs)
All paths:               Rooted at PYBOMBS_PREFIX
```

---

## 12. CMake Modules Architecture

### Local Modules
```
cmake/
├── Modules/
│   ├── gnuradio-ieee80211Config.cmake    - Package discovery config
│   ├── targetConfig.cmake.in             - Target dependency template
│   └── CMakeParseArgumentsCopy.cmake     - Utility macros
└── cmake_uninstall.cmake.in              - Uninstall script generator
```

### GNU Radio Inherited Modules
The project inherits CMake modules from GNU Radio installation:
- `GrVersion` - Version management
- `GrPlatform` - Platform detection and LIB_SUFFIX
- `GrMinReq` - Minimum version checks
- `GrCompilerSettings` - Compiler flags
- `GrPython` - Python integration
- `GrPybind` - pybind11 wrapper macros
- `GrMiscUtils` - Library installation helpers
- `GrTest` - Unit test registration

---

## 13. Build Customization Options

### CMake Cache Variables
```bash
# Build type
-DCMAKE_BUILD_TYPE=Release|Debug|RelWithDebInfo

# Installation prefix
-DCMAKE_INSTALL_PREFIX=/usr/local

# Python support
-DENABLE_PYTHON=ON|OFF

# Documentation
-DENABLE_DOXYGEN=ON|OFF

# Compilation options
-DCMAKE_CXX_FLAGS="-march=native"
-DCMAKE_CXX_STANDARD=11
```

### Common Build Commands
```bash
# Standard Release build
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local ../
make -j$(nproc)
make test
sudo make install

# Debug build with verbose output
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_VERBOSE_MAKEFILE=ON ../
make VERBOSE=1

# Custom Python location
cmake -DPYTHON_EXECUTABLE=/usr/bin/python3.9 ../

# Disable Python/GRC
cmake -DENABLE_PYTHON=OFF ../
```

---

## 14. Key Architectural Decisions

### Modular Block Design
- **One source file per DSP block** (trigger, sync, signal, etc.)
- **Corresponding header file** for interface definition
- **Python binding file** for each block
- **GRC block definition** (YAML) for GUI integration

### Version 2 Architecture
- Extended blocks (v2) for MIMO support
- Backward compatibility with v1 blocks
- Parallel implementations for evolution

### DSSS Integration
- Separate `dsss/` subdirectory for 802.11b support
- Modular design allows independent compilation
- Own unit test suite (`qa_dsss.cc`)

### Binding Strategy
- **pybind11** for modern Python integration
- **NumPy array support** for efficient signal handling
- **Post-build copy** for testing before installation
- **Separate test module** for pre-installation testing

### Platform Independence
- CMake handles cross-platform builds
- GNU Radio abstracts DSP framework
- UHD abstracts hardware interface
- Supports Linux, macOS, Windows

---

## 15. Build System Strengths and Design Patterns

### Strengths
1. **Standards Compliance:** Follows GNU Radio OOT module conventions
2. **Modular Design:** Easy to add new blocks without rebuild overhead
3. **Testing Integration:** Unit tests for both C++ and Python
4. **Documentation:** Automatic Doxygen integration
5. **Platform Support:** Handles Windows, macOS, Linux variations
6. **Component Isolation:** Python/GRC optional; C++ core always builds

### Design Patterns
- **Generator Pattern:** CMake macros (GR_ADD_TEST, GR_PYBIND_MAKE_OOT)
- **Configuration Pattern:** CMakeLists.txt at each level
- **Dependency Injection:** include_directories for modular includes
- **Template Method:** Doxyfile.in configured at build time
- **Factory Pattern:** Block creation via `make()` methods (Python binding)

---

## 16. Typical Build Output Structure

```
build/
├── CMakeFiles/                          # CMake internal files
├── lib/
│   ├── gnuradio-ieee80211.so           # Main shared library
│   ├── gnuradio-ieee80211.so.0         # Symlink (versioned)
│   └── cmake/gnuradio-ieee80211/       # CMake config files
├── python/ieee80211/
│   ├── ieee80211_python.so             # Python binding module
│   └── __pycache__/                    # Python cache
├── test_modules/                       # Isolated test environment
├── docs/doxygen/
│   ├── html/                           # HTML documentation
│   └── xml/                            # XML documentation
├── CMakeCache.txt                      # CMake configuration cache
├── Makefile                            # Generated build rules
└── compile_commands.json               # IDE integration
```

---

## Summary: Build Architecture Overview

The gr-ieee80211 build system implements a **hierarchical, modular CMake architecture** that:

1. **Core Library** (`gnuradio-ieee80211.so`) - C++ DSP blocks linked to GNU Radio and UHD
2. **Python Bindings** (`ieee80211_python.so`) - pybind11-generated interface to C++ blocks
3. **Python Package** (`gnuradio.ieee80211`) - Pure Python utilities and tests
4. **GRC Integration** (17 block definitions) - YAML files for GNU Radio Companion
5. **Documentation** (Doxygen) - Optional API reference generation
6. **Unit Tests** (C++ and Python) - Comprehensive testing framework

**Key Dependencies:**
- GNU Radio 3.10+ (runtime and CMake utilities)
- UHD 3.9.7+ (hardware interface)
- Python 3 + pybind11 (bindings)
- NumPy (array support)

**Installation:** Single `make install` command handles all components with platform-specific paths and optional features.

