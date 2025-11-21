# Python Bindings and GRC Architecture - Quick Summary

## Repository Statistics

| Metric | Value |
|--------|-------|
| **Binding Files** | 18 C++ files (python/ieee80211/bindings/) |
| **Total Binding Code** | 1,101 lines |
| **GRC Block Definitions** | 17 YAML files (grc/) |
| **Total Blocks** | 17 primary blocks exposed to Python |
| **Binding Framework** | Pybind11 with GNU Radio bindtool |
| **Python Module** | gnuradio.ieee80211 |
| **Build System** | CMake with GR_PYBIND_MAKE_OOT macro |

---

## Architecture Overview

### Layers (Bottom to Top)

```
┌─────────────────────────────────────┐
│  GNU Radio Companion (GRC) UI       │
│  (Visual Flowgraph Designer)        │
└────────────┬────────────────────────┘
             │
┌────────────▼────────────────────────┐
│  GRC Block Definitions (YAML)       │
│  17 x *.block.yml files             │
└────────────┬────────────────────────┘
             │
┌────────────▼────────────────────────┐
│  Python Module                      │
│  gnuradio.ieee80211 (from .so)      │
└────────────┬────────────────────────┘
             │
┌────────────▼────────────────────────┐
│  Pybind11 Bindings                  │
│  18 x *_python.cc → .so library     │
└────────────┬────────────────────────┘
             │
┌────────────▼────────────────────────┐
│  C++ Block Implementations          │
│  (include/gnuradio/ieee80211/*.h)   │
└─────────────────────────────────────┘
```

---

## Key Binding Patterns

### Pattern 1: Simple Block (No Parameters)
```cpp
void bind_signal(py::module& m) {
    py::class_<signal, gr::block, gr::basic_block,
        std::shared_ptr<signal>>(m, "signal", D(signal))
        .def(py::init(&signal::make), D(signal,make));
}
```
**Used by:** trigger, signal, modulation, encode, decode, etc.

### Pattern 2: Parametrized Block
```cpp
void bind_demod(py::module& m) {
    py::class_<demod, gr::block, gr::basic_block,
        std::shared_ptr<demod>>(m, "demod", D(demod))
        .def(py::init(&demod::make),
           py::arg("mupos"),
           py::arg("mugid"),
           D(demod,make));
}
```
**Used by:** demod, modulation2, pad2, etc.

### Pattern 3: Complex Block with Methods
```cpp
py::class_<chip_sync_c, gr::block, gr::basic_block,
    std::shared_ptr<chip_sync_c>>(m, "chip_sync_c", "Documentation...")
    .def(py::init(&chip_sync_c::make),
        py::arg("long_preamble"),
        py::arg("threshold"), "Create chip synchronization block...")
    .def("set_preamble_type", &chip_sync_c::set_preamble_type,
        py::arg("islong"), "Set preamble type...");
```
**Used by:** chip_sync_c, ppdu_prefixer, ppdu_chip_mapper_bc

### Pattern 4: Tagged Stream Blocks
```cpp
py::class_<pktgen, gr::tagged_stream_block, gr::block, 
    gr::basic_block, std::shared_ptr<pktgen>>(m, "pktgen", D(pktgen))
    .def(py::init(&pktgen::make),
       py::arg("lengthtagname") = "packet_len",
       D(pktgen,make));
```
**Used by:** pktgen

---

## GRC Block Configuration Patterns

### Simple I/O Block
```yaml
id: ieee80211_signal
label: Signal
category: '[IEEE 802.11 GR-WiFi]'

templates:
  imports: from gnuradio import ieee80211
  make: ieee80211.signal()

inputs:
- label: sync
  domain: stream
  dtype: byte

outputs:
- label: sig
  domain: stream
  dtype: complex

file_format: 1
```

### Parametrized Block with Validation
```yaml
id: ieee80211_demod
label: Demod

templates:
  make: ieee80211.demod(${mupos}, ${mugid})

parameters:
- id: mupos
  label: MU-MIMO User Pos
  dtype: int
  default: '0'
- id: mugid
  label: MU-MIMO Group ID
  dtype: int
  default: '2'

asserts:
- ${ mupos >= 0 && mupos <= 3 }
- ${ mugid >= 1 && mugid <= 62 }
```

### Enum and Message-Based Block
```yaml
id: ieee80211_ppdu_prefixer
label: 802.11b PPDU Prefixer

parameters:
- id: rate
  label: Rate
  dtype: enum
  default: '0'
  options: ['0', '1', '2', '3', '4', '5', '6']
  option_labels: ['1 Mbps Long', '2 Mbps Long', ..., '11 Mbps Short']

templates:
  make: ieee80211.ppdu_prefixer(${rate})
  callbacks:
  - set_preamble_type(${long_preamble})

inputs:
- domain: message
  id: psdu_in
  optional: true

outputs:
- domain: message
  id: ppdu_out
  optional: true
```

---

## Parameter Types Supported

| Type | YAML | Example | GRC UI |
|------|------|---------|--------|
| int | `dtype: int` | `0`, `5`, `-1` | Text input |
| float | `dtype: float` | `2.3`, `0.5` | Text input |
| string | `dtype: string` | `"packet_len"` | Text input |
| bool | `dtype: bool` | `True`, `False` | Checkbox |
| enum | `dtype: enum` | With options list | Dropdown |

---

## C++ to Python Flow Example: demod Block

**1. C++ Header** (`include/gnuradio/ieee80211/demod.h`)
```cpp
class demod : virtual public gr::block {
    static sptr make(int mupos, int mugid);
};
```

**2. Pybind11 Binding** (`python/ieee80211/bindings/demod_python.cc`)
```cpp
py::class_<demod, gr::block, gr::basic_block,
    std::shared_ptr<demod>>(m, "demod", D(demod))
    .def(py::init(&demod::make),
       py::arg("mupos"),
       py::arg("mugid"),
       D(demod,make));
```

**3. GRC Definition** (`grc/ieee80211_demod.block.yml`)
```yaml
id: ieee80211_demod
label: Demod

templates:
  make: ieee80211.demod(${mupos}, ${mugid})

parameters:
- id: mupos
  label: MU-MIMO User Pos
  dtype: int
  default: '0'
- id: mugid
  label: MU-MIMO Group ID
  dtype: int
  default: '2'
```

**4. Python API Usage**
```python
from gnuradio import ieee80211

# Direct Python API
block = ieee80211.demod(mupos=1, mugid=32)

# Help system (from Pybind11 documentation)
help(ieee80211.demod)
```

**5. GRC Generated Flowgraph**
```python
# From visual GRC design
demod_block = ieee80211.demod(1, 32)
tb.connect(source, demod_block)
tb.connect(demod_block, sink)
```

---

## Block Organization by Category

All blocks organized under: `[IEEE 802.11 GR-WiFi]`

### Original 802.11a/n Blocks (12)
- **Transmitter:** Trigger → Signal → Modulation → Pad
- **Receiver:** Sync → Demod → Decode
- **MU-MIMO Tx:** Signal2 → Modulation2 → Pad2
- **MU-MIMO Rx:** Demod2
- **Utilities:** Encode, Encode2

### New 802.11b DSSS Blocks (5)
- **Transmitter:** Pktgen → PPDU Prefixer → PPDU Chip Mapper
- **Receiver:** Chip Sync
- **Deprecated:** (was 4th utility, now 3)

---

## Build System Integration

### CMakeLists.txt: Binding Compilation
```cmake
include(GrPybind)

list(APPEND ieee80211_python_files
    trigger_python.cc
    sync_python.cc
    # ... [16 more files] ...
    ppdu_prefixer_python.cc
    python_bindings.cc)

GR_PYBIND_MAKE_OOT(ieee80211
   ../../..
   gr::ieee80211
   "${ieee80211_python_files}")

install(TARGETS ieee80211_python 
        DESTINATION ${GR_PYTHON_DIR}/gnuradio/ieee80211)
```

### CMakeLists.txt: GRC Installation
```cmake
install(FILES
    ieee80211_trigger.block.yml
    ieee80211_sync.block.yml
    # ... [15 more files] ...
    ieee80211_ppdu_prefixer.block.yml
    DESTINATION share/gnuradio/grc/blocks)
```

---

## Installation Paths

```
/usr/lib/python3/dist-packages/gnuradio/ieee80211/
├── __init__.py                  # Module entry point
├── ieee80211_python.so          # Compiled pybind11 shared library
├── bindings/                    # Source (optional)
└── qa_*.py                      # Unit tests

/usr/share/gnuradio/grc/blocks/
├── ieee80211_*.block.yml        # GRC block definitions (17 files)
└── ...
```

---

## Key Architecture Strengths

| Aspect | Implementation |
|--------|----------------|
| **Code Reuse** | Single C++ codebase compiled to Python + GRC |
| **Type Safety** | Pybind11 enforces C++ type checking in Python |
| **Documentation** | Auto-generated docstrings from C++ |
| **Callbacks** | Dynamic parameter updates without restart |
| **Validation** | GRC assertions prevent invalid configurations |
| **Modularity** | Per-block binding files (18 files, 1 library) |
| **Extensibility** | Easy to add new blocks (bindtool + YAML template) |
| **Standards** | GNU Radio conventions + Pybind11 best practices |

---

## Documentation Access

### Python Help System
```python
from gnuradio import ieee80211

# Help on class
help(ieee80211.chip_sync_c)

# Help on method
help(ieee80211.chip_sync_c.set_preamble_type)

# Docstring access
print(ieee80211.chip_sync_c.__doc__)
```

### GRC Documentation
```yaml
documentation: |-
  802.11b DSSS/CCK Chip Synchronization...
  
  Supported data rates:
    - 1 Mbps (DBPSK with Barker spreading)
    - 2 Mbps (DQPSK with Barker spreading)
    - 5.5 Mbps (CCK with 4-bit encoding)
    - 11 Mbps (CCK with 8-bit encoding)
```

---

## Files Reference

### Essential Files

**Python Bindings:**
- `/home/user/gr-ieee80211/python/ieee80211/bindings/python_bindings.cc` (87 lines) - Main entry point
- `/home/user/gr-ieee80211/python/ieee80211/bindings/*_python.cc` (18 files, avg 59 lines each)
- `/home/user/gr-ieee80211/python/ieee80211/bindings/CMakeLists.txt` - Build configuration
- `/home/user/gr-ieee80211/python/ieee80211/__init__.py` - Module initialization

**GRC Blocks:**
- `/home/user/gr-ieee80211/grc/*.block.yml` (17 files, 282-1778 lines each)
- `/home/user/gr-ieee80211/grc/CMakeLists.txt` - Installation configuration

**C++ Blocks:**
- `/home/user/gr-ieee80211/include/gnuradio/ieee80211/*.h` - Block interfaces
- `/home/user/gr-ieee80211/lib/ieee80211/` - Implementation files

---

## Conclusion

The gr-ieee80211 binding architecture demonstrates professional software engineering:

1. **Separation of Concerns:** C++ → Pybind11 → Python/GRC
2. **Automation:** Bindtool reduces manual binding code
3. **Documentation:** Unified docstring system across all interfaces
4. **Maintainability:** Modular organization with clear file structure
5. **Standards:** Follows GNU Radio and Pybind11 conventions
6. **Extensibility:** Simple to add new blocks without major refactoring

The YAML-based GRC definitions provide a declarative approach to block configuration, while Pybind11 bindings ensure type-safe Python access with minimal boilerplate.

---

**For detailed technical analysis, see:** `BINDING_ARCHITECTURE_ANALYSIS.md` (1,304 lines)

