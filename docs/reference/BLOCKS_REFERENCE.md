# IEEE80211 Blocks Quick Reference

## Block Listing

### All 17 Blocks with Details

| ID | Label | Category | Inputs | Outputs | Parameters | Purpose |
|----|-------|----------|--------|---------|------------|---------|
| ieee80211_trigger | Trigger | [IEEE 802.11 GR-WiFi] | float | byte | - | Energy detection trigger |
| ieee80211_signal | Signal | [IEEE 802.11 GR-WiFi] | byte, complex | complex | - | SIGNAL field insertion (802.11a/n) |
| ieee80211_modulation | Mod | [IEEE 802.11 GR-WiFi] | byte | complex | - | OFDM modulation (802.11a/n) |
| ieee80211_pad | Padding | [IEEE 802.11 GR-WiFi] | complex | complex | - | Pilot subcarrier insertion (802.11a/n) |
| ieee80211_demod | Demod | [IEEE 802.11 GR-WiFi] | complex | float | mupos, mugid | OFDM demodulation (802.11a/n) |
| ieee80211_decode | Decode | [IEEE 802.11 GR-WiFi] | byte | byte | - | Viterbi convolutional decoding |
| ieee80211_encode | Encode | [IEEE 802.11 GR-WiFi] | byte | byte | - | Convolutional encoding |
| ieee80211_signal2 | Signal 2 | [IEEE 802.11 GR-WiFi] | byte, complex | complex | - | MU-MIMO SIGNAL field |
| ieee80211_modulation2 | Mod 2 | [IEEE 802.11 GR-WiFi] | byte | complex | - | MU-MIMO OFDM modulation |
| ieee80211_pad2 | Padding 2 | [IEEE 802.11 GR-WiFi] | complex, complex | complex, complex | - | MU-MIMO pilot insertion |
| ieee80211_demod2 | Demod 2 | [IEEE 802.11 GR-WiFi] | complex, complex | float | - | MU-MIMO demodulation |
| ieee80211_encode2 | Encode 2 | [IEEE 802.11 GR-WiFi] | byte | byte | - | MU-MIMO encoding |
| ieee80211_pktgen | Pkt Gen | [IEEE 802.11 GR-WiFi] | message | stream:byte | tag | Packet generation with tagging |
| ieee80211_chip_sync_c | 802.11b DSSS Sink | [IEEE 802.11 GR-WiFi] | stream:complex | message | long_preamble, threshold | Barker correlation & packet demod |
| ieee80211_ppdu_chip_mapper_bc | 802.11b Chip Mapper | [IEEE 802.11 GR-WiFi] | stream:byte | stream:complex | length_tag_name | DSSS/CCK chip mapping & modulation |
| ieee80211_ppdu_prefixer | 802.11b PPDU Prefixer | [IEEE 802.11 GR-WiFi] | message | message | rate | PLCP preamble & header prepender |

---

## Transmitter Chains

### 802.11a/n OFDM Transmitter
```
Source → Trigger → Signal → Modulation → Pad → [RF Frontend]
```
- **Trigger:** Detects transmission opportunity
- **Signal:** Adds SIGNAL field (rate, length info)
- **Modulation:** 64-QAM/16-QAM/QPSK/BPSK depending on rate
- **Pad:** Inserts pilot subcarriers on subcarriers {-21,-7,7,21}

### 802.11a/n MU-MIMO Transmitter
```
Source → Signal2 → Modulation2 → Pad2 → [RF Frontend]
```
- **Signal2:** Multi-user SIGNAL field
- **Modulation2:** Per-user OFDM modulation
- **Pad2:** Dual-stream pilot insertion

### 802.11b DSSS Transmitter
```
PDU Source → Pktgen → PPDU Prefixer → PPDU Chip Mapper → [RF Frontend]
```
- **Pktgen:** Converts messages to byte streams with length tags
- **PPDU Prefixer:** Adds PLCP preamble (144 or 72 bits) + SFD + SIGNAL + SERVICE + LENGTH + CRC
- **PPDU Chip Mapper:** Barker spreading (1-2 Mbps) or CCK encoding (5.5-11 Mbps)

---

## Receiver Chains

### 802.11a/n OFDM Receiver
```
[RF Frontend] → Sync → Demod → Decode → [MAC]
```
- **Sync:** Symbol timing synchronization
- **Demod:** OFDM demodulation to soft bits (LLR)
- **Decode:** Viterbi decoding to hard bits

### 802.11a/n MU-MIMO Receiver
```
[RF Frontend] → Demod2 → [MAC]
```
- **Demod2:** Multi-user demodulation (extracts both users)

### 802.11b DSSS Receiver
```
[RF Frontend] → Chip Sync → [Packet Messages to MAC]
```
- **Chip Sync:** Barker correlation detection, auto-rate detection, packet demodulation, CRC check

---

## Parameter Reference

### Simple Parameters (No Options)

#### Integer
```yaml
parameters:
- id: mupos
  label: MU-MIMO User Pos
  dtype: int
  default: '0'
```
**Used by:** demod (mupos: 0-3), demod (mugid: 1-62)

#### Float
```yaml
parameters:
- id: threshold
  label: Correlation Threshold
  dtype: float
  default: '2.3'
```
**Used by:** chip_sync_c (0.0-11.0 typical)

#### String
```yaml
parameters:
- id: length_tag_name
  label: Length Tag Name
  dtype: string
  default: packet_len
```
**Used by:** pktgen, ppdu_chip_mapper_bc

#### Boolean
```yaml
parameters:
- id: long_preamble
  label: Long Preamble
  dtype: bool
  default: 'True'
```
**Used by:** chip_sync_c (True=144 bits, False=72 bits)

### Enumerated Parameters

```yaml
parameters:
- id: rate
  label: Rate
  dtype: enum
  default: '0'
  options: ['0', '1', '2', '3', '4', '5', '6']
  option_labels: ['1 Mbps Long', '2 Mbps Long', '5.5 Mbps Long', 
                  '11 Mbps Long', '2 Mbps Short', '5.5 Mbps Short',
                  '11 Mbps Short']
```
**Used by:** ppdu_prefixer (7 rate/preamble combinations)

---

## Data Types

### I/O Port Data Types

| Type | Size | Range | Usage |
|------|------|-------|-------|
| byte | 1 byte | 0-255 | Bit/symbol stream |
| int | 4 bytes | -2^31 to 2^31-1 | Integer streams |
| float | 4 bytes | IEEE 754 | Soft bits (LLR), energy |
| complex | 8 bytes | Complex | IQ samples, constellation |

### Domain Types

| Domain | Purpose | Example Blocks |
|--------|---------|-----------------|
| stream | Continuous data | All modulation/demod, encode/decode |
| message | Discrete packets | Pktgen, chip_sync_c, PPDU prefixer |

---

## Configuration Examples

### Example 1: Configure demod Block
```python
from gnuradio import ieee80211

# Python API
demod = ieee80211.demod(mupos=1, mugid=32)

# GRC YAML equivalent
# parameters:
# - id: mupos
#   default: '1'
# - id: mugid
#   default: '32'
```

### Example 2: Configure chip_sync_c Block
```python
# Python API
chip_sync = ieee80211.chip_sync_c(long_preamble=False, threshold=1.5)

# GRC YAML equivalent
# parameters:
# - id: long_preamble
#   default: 'False'
# - id: threshold
#   default: '1.5'
```

### Example 3: Configure ppdu_prefixer Block
```python
# Python API (rate = 0: 1 Mbps long preamble)
prefixer = ieee80211.ppdu_prefixer(rate=0)

# Or enumerate options
# 0 = 1 Mbps long preamble
# 1 = 2 Mbps long preamble
# 2 = 5.5 Mbps long preamble
# 3 = 11 Mbps long preamble
# 4 = 2 Mbps short preamble
# 5 = 5.5 Mbps short preamble
# 6 = 11 Mbps short preamble
```

---

## Block I/O Summary

### Multi-Input Blocks
| Block | Input 1 | Input 2 | Input 3 |
|-------|---------|---------|---------|
| signal | byte (bits) | complex (IQ) | - |
| signal2 | byte (bits) | complex (IQ) | - |
| demod2 | complex (IQ) | complex (IQ) | - |
| pad2 | complex (IQ) | complex (IQ) | - |

### Multi-Output Blocks
| Block | Output 1 | Output 2 |
|-------|----------|----------|
| pad2 | complex (IQ) | complex (IQ) |

### Message-Based Blocks
| Block | Input Type | Output Type | Purpose |
|-------|-----------|------------|---------|
| pktgen | message (PDU) | stream:byte | Tag packets |
| chip_sync_c | stream:complex | message (PSDU) | Demod to packets |
| ppdu_prefixer | message (PSDU) | message (PPDU) | Add PLCP header |

---

## Python Usage Examples

### Basic Block Usage
```python
from gnuradio import ieee80211, gr

# Create flowgraph
tb = gr.top_block()

# Create simple block
signal_block = ieee80211.signal()

# Create parametrized block
demod_block = ieee80211.demod(mupos=0, mugid=2)

# Create block with methods
chip_sync = ieee80211.chip_sync_c(long_preamble=True, threshold=2.3)

# Change parameter at runtime
chip_sync.set_preamble_type(False)  # Switch to short preamble

# Get help
help(ieee80211.demod)
help(ieee80211.chip_sync_c.set_preamble_type)
```

### GRC Flowgraph (Generated Code)
```python
from gnuradio import ieee80211, gr, blocks

class top_block(gr.top_block):
    def __init__(self):
        gr.top_block.__init__(self, "Top Block")
        
        # Create blocks
        self.ieee80211_signal_0 = ieee80211.signal()
        self.ieee80211_demod_0 = ieee80211.demod(0, 2)
        self.ieee80211_chip_sync_c_0 = ieee80211.chip_sync_c(True, 2.3)
        
        # Connect blocks
        self.connect((source, 0), (self.ieee80211_signal_0, 0))
        self.connect((self.ieee80211_signal_0, 0), (sink, 0))

if __name__ == '__main__':
    tb = top_block()
    tb.start()
    tb.wait()
```

---

## Documentation Access

### Block Documentation in Python
```python
from gnuradio import ieee80211

# Full documentation
print(ieee80211.chip_sync_c.__doc__)

# Interactive help
help(ieee80211.chip_sync_c)

# Method documentation
help(ieee80211.chip_sync_c.set_preamble_type)
```

### Block Documentation in GRC
- Right-click block → "Help" button
- Full documentation in popup window
- Parameter descriptions inline

---

## Building and Installation

### Build from Source
```bash
mkdir build && cd build
cmake /home/user/gr-ieee80211
make -j4
sudo make install
```

### Verify Installation
```bash
python3 -c "from gnuradio import ieee80211; print(dir(ieee80211))"
```

### GRC Block Discovery
- Launch GNU Radio Companion
- Blocks appear under "IEEE 802.11 GR-WiFi" category
- Automatic discovery on startup

---

## Common Issues and Solutions

### Issue: ImportError: No module named ieee80211
**Solution:** Ensure bindings are installed:
```bash
sudo make install
python3 -c "from gnuradio import ieee80211"
```

### Issue: GRC blocks not appearing
**Solution:** Ensure YAML files installed:
```bash
ls /usr/share/gnuradio/grc/blocks/ | grep ieee80211
```

### Issue: Parameter validation fails
**Solution:** Check assertions in YAML:
```yaml
asserts:
- ${ mupos >= 0 }
- ${ mupos <= 3 }
```

---

## File Locations Reference

### Python Bindings
```
/home/user/gr-ieee80211/python/ieee80211/bindings/
├── python_bindings.cc              # Main module entry
├── signal_python.cc                # Individual block bindings
├── demod_python.cc
├── chip_sync_c_python.cc
├── ppdu_prefixer_python.cc
└── CMakeLists.txt
```

### GRC Blocks
```
/home/user/gr-ieee80211/grc/
├── ieee80211_signal.block.yml
├── ieee80211_demod.block.yml
├── ieee80211_chip_sync_c.block.yml
├── ieee80211_ppdu_prefixer.block.yml
└── CMakeLists.txt
```

### C++ Sources
```
/home/user/gr-ieee80211/include/gnuradio/ieee80211/
├── signal.h
├── demod.h
├── dsss/
│   ├── chip_sync_c.h
│   ├── ppdu_prefixer.h
│   └── ppdu_chip_mapper_bc.h
```

### Installation Paths
```
/usr/lib/python3/dist-packages/gnuradio/ieee80211/
├── __init__.py
├── ieee80211_python.so          # Compiled bindings
└── qa_*.py                       # Unit tests

/usr/share/gnuradio/grc/blocks/
├── ieee80211_*.block.yml         # GRC definitions
```

---

## See Also

- **Detailed Analysis:** `BINDING_ARCHITECTURE_ANALYSIS.md`
- **Summary:** `BINDING_ANALYSIS_SUMMARY.md`
- **IEEE 802.11 Specification:** 802.11-2020 standard
- **GNU Radio Documentation:** https://www.gnuradio.org/
- **Pybind11 Docs:** https://pybind11.readthedocs.io/

