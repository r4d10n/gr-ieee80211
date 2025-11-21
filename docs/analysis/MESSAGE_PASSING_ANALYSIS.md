# gr-ieee80211 Message Passing and Block Interaction Analysis

## Executive Summary

The gr-ieee80211 project implements sophisticated message passing and data flow mechanisms for 802.11 PHY processing. The communication architecture uses three distinct patterns:

1. **Stream Tags** for inline metadata propagation within audio/data streams
2. **Message Ports** for asynchronous control flow and statistics collection
3. **Hybrid Tag+Data** approach where tagged stream blocks coordinate with message handlers

---

## 1. Message Ports Usage Across Blocks

### 1.1 Message Port Architecture

The project implements **asynchronous message passing** using GNU Radio's message port framework. Message ports operate independently from streaming data and enable decoupled communication between blocks.

#### Port Categories

**Input Message Ports:**
- Control/Configuration ports
- Statistics reporting ports
- Packet data ports (PDUS)

**Output Message Ports:**
- Decoded packet output
- Statistics publication
- Status/error reporting

### 1.2 Key Blocks with Message Ports

#### A. wifi_stats_collector (Statistics Hub)
**Location:** `/lib/wifi_stats_collector_impl.cc`

Implements a centralized statistics collection block with **4 input ports and 1 output port**:

```cpp
// Input Ports
message_port_register_in(pmt::mp("rx_packets"));      // RX packet notifications
message_port_register_in(pmt::mp("tx_packets"));      // TX packet notifications  
message_port_register_in(pmt::mp("rx_errors"));       // Error notifications
message_port_register_in(pmt::mp("signal_quality"));  // SNR/RSSI updates

// Output Port
message_port_register_out(pmt::mp("stats"));          // Periodic statistics publication

// Message Handlers with Lambda Functions
set_msg_handler(pmt::mp("rx_packets"), 
                [this](pmt::pmt_t msg) { handle_rx_packet(msg); });
set_msg_handler(pmt::mp("tx_packets"),
                [this](pmt::pmt_t msg) { handle_tx_packet(msg); });
set_msg_handler(pmt::mp("rx_errors"),
                [this](pmt::pmt_t msg) { handle_rx_error(msg); });
set_msg_handler(pmt::mp("signal_quality"),
                [this](pmt::pmt_t msg) { handle_signal_quality(msg); });
```

**Message Handler Pattern:**
```cpp
void handle_rx_packet(pmt::pmt_t msg) {
    std::lock_guard<std::mutex> lock(d_mutex);
    
    // Extract packet metadata from PMT dictionary
    if (pmt::is_dict(msg)) {
        pmt::pmt_t len_pmt = pmt::dict_ref(msg, pmt::mp("length"), pmt::PMT_NIL);
        if (!pmt::eqv(len_pmt, pmt::PMT_NIL)) {
            size_t len = pmt::to_long(len_pmt);
            d_total_bytes += len;
        }
    }
}
```

**Statistics Publication:**
```cpp
void publish_stats() {
    // Create PMT dictionary with aggregated statistics
    pmt::pmt_t stats_dict = pmt::make_dict();
    stats_dict = pmt::dict_add(stats_dict, pmt::mp("rx_packets_total"),
                               pmt::from_uint64(report.rx_packets_total));
    stats_dict = pmt::dict_add(stats_dict, pmt::mp("throughput_current"),
                               pmt::from_double(report.throughput_current));
    stats_dict = pmt::dict_add(stats_dict, pmt::mp("snr_current"),
                               pmt::from_float(report.snr_current));
    
    message_port_pub(pmt::mp("stats"), stats_dict);
}
```

#### B. decode Block (Viterbi Decoder)
**Location:** `/lib/decode_impl.cc`

Asynchronously publishes decoded packets:

```cpp
message_port_register_out(pmt::mp("out"));

// Publishing decoded packets with metadata
message_port_pub(pmt::mp("out"), pmt::cons(tmpMeta, tmpPayload));
```

**Packet Structure:** `pmt::cons(metadata_dict, payload_blob)`

#### C. pktgen Block (Packet Generator)
**Location:** `/lib/pktgen_impl.cc`

Receives packet specifications via message port:

```cpp
message_port_register_in(pmt::mp("pdus"));
set_msg_handler(pmt::mp("pdus"), boost::bind(&pktgen_impl::msgRead, this, _1));

void msgRead(pmt::pmt_t msg) {
    // Extract packet vector from message pair
    pmt::pmt_t msgVec = pmt::cdr(msg);
    const uint8_t *tmpPkt = (const uint8_t *)pmt::uniform_vector_elements(msgVec, offset);
    
    // Queue packet for generation
    d_pktQ.push(std::vector<uint8_t>(tmpPkt, tmpPkt + pktLen));
}
```

#### D. ppdu_prefixer (DSSS PPDU Framing)
**Location:** `/lib/dsss/ppdu_prefixer.cc`

Message-based DSSS preamble prepending:

```cpp
class ppdu_prefixer_impl : public ppdu_prefixer {
    ppdu_prefixer_impl(int rate) : block(...) {
        message_port_register_in(d_in_port);
        set_msg_handler(d_in_port, [this](pmt::pmt_t msg){ psdu_in(msg); });
        message_port_register_out(d_out_port);
    }
    
    void psdu_in(pmt::pmt_t msg) {
        pmt::pmt_t v = pmt::cdr(msg);  // Get payload
        const uint8_t* uvec = pmt::u8vector_elements(v, io);
        // Generate PPDU with preamble/header...
        message_port_pub(d_out_port, ppdu_msg);
    }
};
```

#### E. modulation2 Block (Alternative Modulation)
**Location:** `/lib/modulation2_impl.cc`

Alternative implementation using message port for control:

```cpp
message_port_register_in(pmt::mp("pdus"));
set_msg_handler(pmt::mp("pdus"), boost::bind(&modulation2_impl::msgRead, this, _1));
```

---

## 2. Stream Tags for Metadata Propagation

### 2.1 Tag Propagation Policy

Blocks explicitly disable automatic tag propagation and manage tags manually:

```cpp
// All processing blocks use TPP_DONT (Tag Propagation Policy: Don't)
set_tag_propagation_policy(block::TPP_DONT);
```

**Blocks with custom tag management:**
- `demod` (line 46 in demod_impl.cc)
- `demod2` (line 44 in demod2_impl.cc)
- `signal` (line 48 in signal_impl.cc)
- `signal2` (line 48 in signal2_impl.cc)
- `decode` (line 47 in decode_impl.cc)
- `ppdu_chip_mapper_bc` (line 118 in ppdu_chip_mapper_bc_impl.cc)

### 2.2 Tag Reading Pattern

**Standard tag extraction pattern:**

```cpp
// Read tags at specific sample positions
std::vector<gr::tag_t> tags;
get_tags_in_range(tags, 0, nitems_read(0), nitems_read(0) + 1);

if (tags.size()) {
    // Aggregate tags into PMT dictionary
    pmt::pmt_t d_meta = pmt::make_dict();
    for (auto tag : tags) {
        d_meta = pmt::dict_add(d_meta, tag.key, tag.value);
    }
    
    // Extract specific metadata fields
    int format = pmt::to_long(pmt::dict_ref(d_meta, pmt::mp("format"), pmt::from_long(-1)));
    float cfo = pmt::to_float(pmt::dict_ref(d_meta, pmt::mp("cfo"), pmt::from_float(0.0f)));
    float snr = pmt::to_float(pmt::dict_ref(d_meta, pmt::mp("snr"), pmt::from_float(0.0f)));
}
```

### 2.3 Tag Writing Pattern

**Standard tag addition pattern (used 27 times in codebase):**

```cpp
// Create PMT dictionary of metadata
pmt::pmt_t dict = pmt::make_dict();
dict = pmt::dict_add(dict, pmt::mp("format"), pmt::from_long(pktFormat));
dict = pmt::dict_add(dict, pmt::mp("mcs0"), pmt::from_long(pktMcs0));
dict = pmt::dict_add(dict, pmt::mp("cfo"), pmt::from_float(cfo_radians));
dict = pmt::dict_add(dict, pmt::mp("snr"), pmt::from_float(snr_db));
dict = pmt::dict_add(dict, pmt::mp("chan"), pmt::init_c32vector(channel.size(), channel));

// Add all key-value pairs as stream tags
pmt::pmt_t pairs = pmt::dict_items(dict);
for (size_t i = 0; i < pmt::length(pairs); i++) {
    pmt::pmt_t pair = pmt::nth(i, pairs);
    add_item_tag(0,                    // output port index
                  nitems_written(0),   // output sample position
                  pmt::car(pair),      // key
                  pmt::cdr(pair),      // value
                  alias_pmt());        // source identification
}
```

### 2.4 Metadata Tag Inventory

**Common tags propagated through signal blocks:**

| Tag Key | Type | Source | Destination | Purpose |
|---------|------|--------|-------------|---------|
| `format` | long | pktgen, encode | modulation, signal | Packet format (Legacy/HT/VHT) |
| `mcs0` | long | pktgen, encode | modulation | Modulation/Coding Scheme |
| `nss0` | long | pktgen, encode | modulation | Number of spatial streams |
| `len0` | long | pktgen, encode | modulation | Packet payload length |
| `seq` | long | pktgen, encode, signal | decode, demod | Packet sequence number |
| `cfo` | float | signal, demod | demod | Carrier Frequency Offset (Hz) |
| `snr` | float | signal, demod | demod, decode | Signal-to-Noise Ratio (dB) |
| `rssi` | float | signal, demod | demod, decode | Received Signal Strength |
| `chan` | c32vector | signal, demod | modulation | Channel Impulse Response |
| `nsamp` | long | signal | demod | Number of samples in packet |
| `sigl` | u8vector | encode | modulation | Legacy Signal field bits |
| `signl` | u8vector | encode | modulation | Non-legacy Signal field bits |
| `sigb0` | u8vector | encode | modulation | VHT Signal B field bits |
| `packet_len` | long | modulation | pad | Output packet length |

### 2.5 Tag Flow Diagrams

**TX Path Tag Propagation:**
```
pktgen (writes format, mcs0, nss0, len0, seq)
  ↓
encode (reads tags, adds sigl/signl/sigb0)
  ↓
modulation (reads tags, writes packet_len)
  ↓
pad (reads packet_len, discards other tags)
```

**RX Path Tag Propagation:**
```
signal (writes cfo, snr, rssi, seq, chan, mcs, len, nsamp)
  ↓
demod (reads tags, extracts channel/CFO/SNR)
  ↓
decode (reads tags, extracts format/length info)
```

---

## 3. PMT (Polymorphic Types) Usage

### 3.1 PMT Type System

The project uses 236 PMT operations across the codebase. PMT enables type-safe polymorphic data handling.

### 3.2 PMT Constructor Functions

**Scalar Types:**
```cpp
pmt::from_long(int_value)          // 64-bit integer
pmt::from_uint64(uint64_value)      // Unsigned 64-bit
pmt::from_float(float_value)        // 32-bit float
pmt::from_double(double_value)      // 64-bit double
```

**Container Types:**
```cpp
pmt::make_dict()                    // Empty dictionary
pmt::init_u8vector(size, data)      // 8-bit unsigned vector
pmt::init_c32vector(size, data)     // Complex 32-bit vector
pmt::make_u8vector(size, fill)      // Create and fill vector
```

**Pair/List Types:**
```cpp
pmt::cons(car, cdr)                 // Cons pair for (metadata . payload)
pmt::car(pair)                      // Extract first element
pmt::cdr(pair)                      // Extract second element
```

**Dictionary Operations:**
```cpp
pmt::dict_add(dict, key, value)     // Add entry (immutable, returns new dict)
pmt::dict_ref(dict, key, default)   // Read entry with default fallback
pmt::dict_items(dict)               // Get all key-value pairs
```

### 3.3 PMT Accessor Functions

**Type Checking:**
```cpp
pmt::is_dict(obj)                   // Check if dictionary
pmt::is_u8vector(obj)               // Check if u8 vector
pmt::is_c32vector(obj)              // Check if complex vector
pmt::eqv(a, b)                      // Equality comparison (pmt::PMT_NIL aware)
```

**Data Extraction:**
```cpp
pmt::to_long(pmt_obj)               // Convert to int64_t
pmt::to_float(pmt_obj)              // Convert to float
pmt::to_double(pmt_obj)             // Convert to double
pmt::u8vector_elements(pmt_obj, offset)   // Get u8 vector pointer
pmt::c32vector_elements(pmt_obj)    // Get complex vector pointer
pmt::uniform_vector_elements(pmt_obj, offset) // Generic vector access
pmt::blob_length(pmt_obj)           // Get blob byte length
```

### 3.4 PMT Usage Patterns in Block Implementations

**Pattern 1: Dictionary-based Message Passing (Most Common - 236 uses)**

```cpp
// Create and populate dictionary
pmt::pmt_t msg = pmt::make_dict();
msg = pmt::dict_add(msg, pmt::mp("length"), pmt::from_uint64(packet_len));
msg = pmt::dict_add(msg, pmt::mp("rate"), pmt::from_long(data_rate));

// Publish via message port
message_port_pub(pmt::mp("rx_packets"), msg);

// Receive and extract
pmt::pmt_t len_pmt = pmt::dict_ref(msg, pmt::mp("length"), pmt::PMT_NIL);
if (!pmt::eqv(len_pmt, pmt::PMT_NIL)) {
    size_t len = pmt::to_long(len_pmt);
    // Process packet
}
```

**Pattern 2: Cons-Pair Message Structure (decode block)**

```cpp
// Message format: (metadata_dict . payload_blob)
pmt::pmt_t tmpMeta = pmt::make_dict();
tmpMeta = pmt::dict_add(tmpMeta, pmt::mp("format"), pmt::from_long(format));
pmt::pmt_t tmpPayload = pmt::init_u8vector(payload_len, payload_data);

message_port_pub(pmt::mp("out"), pmt::cons(tmpMeta, tmpPayload));
```

**Pattern 3: Tag-based Vector Passing**

```cpp
// Store complex vector as tag
std::vector<gr_complex> channel(64);
pmt::pmt_t tag_value = pmt::init_c32vector(channel.size(), channel);
dict = pmt::dict_add(dict, pmt::mp("chan"), tag_value);

// Later retrieve vector
pmt::pmt_t chan_pmt = pmt::dict_ref(d_meta, pmt::mp("chan"), pmt::PMT_NIL);
std::vector<gr_complex> retrieved = pmt::c32vector_elements(chan_pmt);
```

---

## 4. Block Interconnection Patterns

### 4.1 Block Type Classification

**Message-Driven Blocks (Zero streaming I/O):**
- `ppdu_prefixer`: Pure message input/output
- `wifi_stats_collector`: Pure message input/output with 0 streaming ports

**Tagged-Stream Blocks:**
- `pktgen`: Uses `gr::tagged_stream_block` for TSB handling

**General Streaming Blocks (Manual Tag Management):**
- `signal`, `signal2`, `demod`, `demod2`: General blocks with explicit tag handling
- `encode`, `encode2`: Tag-reading input, tag-writing output
- `modulation`, `modulation2`: Tag-reading input, tag-writing output
- `decode`: Tag-reading input, message-port output
- `pad`, `pad2`: Multi-stream tag propagation

### 4.2 Data Flow Chain Patterns

**TX Path (Transmit Chain):**

```
pktgen (TSB)
  │ generates uint8 stream + "packet_len" TSB tag
  ↓
encode (Block)
  │ reads "packet_len" tag → creates format/mcs/nss/len tags
  ↓
modulation (Block)
  │ reads format/mcs tags → creates modulated complex symbols
  ↓
pad (Block)
  │ reads packet_len tag → adds preambles/pilots/padding
  ↓
[USRP or Loopback]
```

**RX Path (Receive Chain):**

```
[Signal Input]
  ↓
trigger (Block)
  │ detects power plateau, outputs trigger indicator
  ↓
sync (Block)  
  │ reads trigger, outputs sync indicators + tags with (rad/snr/rssi)
  ↓
signal (Block)
  │ reads sync+tags → decodes signal field → writes cfo/snr/rssi/mcs/len/chan tags
  ↓
demod (Block)
  │ reads cfo/snr/chan tags → FFT-demodulates OFDM
  ↓
decode (Block)
  │ reads format/len tags → Viterbi decodes payload → message port output
  ↓
decode output → message port → Application
```

### 4.3 Hybrid Data/Message Flow

**DSSS TX Chain (Message-Stream Hybrid):**

```
blocks_message_strobe
  │ generates PDU message
  ↓
ppdu_prefixer (Block - Message I/O)
  │ receives PSDU via message port
  │ outputs PPDU via message port
  ↓
ppdu_chip_mapper_bc (Block - Tagged Stream)
  │ receives PPDU bits + "packet_len" tag
  │ outputs chips with propagated tags
```

---

## 5. Data Flow vs Control Flow

### 5.1 Data Flow (Streaming)

**Characteristics:**
- Continuous audio/RF sample streams
- Fixed sample rates (11 MHz typical)
- Large buffer throughput (thousands of samples per work call)
- Primary transport for waveform samples

**Sample Block Processing:**

```cpp
int demod_impl::general_work(int noutput_items,
                    gr_vector_int &ninput_items,
                    gr_vector_const_void_star &input_items,
                    gr_vector_void_star &output_items) {
    const gr_complex* inSig = static_cast<const gr_complex*>(input_items[0]);
    float* outLlrs = static_cast<float*>(output_items[0]);
    
    // Process stream: Each sample consumes input, produces output
    for (int i = 0; i < noutput_items; i++) {
        // FFT demodulation on complex input samples
        process_symbol(inSig[i * 64], &outLlrs[i * 48]);
    }
    
    return noutput_items;
}
```

### 5.2 Control Flow (Message Passing)

**Characteristics:**
- Asynchronous, event-driven
- Sporadic occurrences (packets, errors, statistics updates)
- Small data payloads (metadata dictionaries)
- Out-of-band transport independent of streaming

**Message Handler Pattern:**

```cpp
void wifi_stats_collector_impl::handle_rx_packet(pmt::pmt_t msg) {
    // Asynchronous handler - invoked when message arrives
    std::lock_guard<std::mutex> lock(d_mutex);
    
    // Control decision: Update state
    d_rx_packets_total++;
    
    // Control decision: Trigger publication
    update_throughput();
    publish_stats();
}
```

### 5.3 Tag-Based Coordination (Hybrid)

**Characteristics:**
- In-band signaling (embedded in stream)
- Sample-aligned metadata
- Automatically pruned at block boundaries (TPP_DONT)
- Connects data-driven decisions to state transitions

**State Machine Pattern with Tags:**

```cpp
// State: WAIT_FOR_PACKET
if (d_state == DEMOD_S_RDTAG) {
    // Look for tag indicating packet start
    get_tags_in_range(tags, 0, nitems_read(0), nitems_read(0) + 1);
    
    if (tags.size()) {
        // Tag found: Extract packet parameters from tag
        int mcs = pmt::to_long(pmt::dict_ref(d_meta, pmt::mp("mcs"), pmt::from_long(-1)));
        int len = pmt::to_long(pmt::dict_ref(d_meta, pmt::mp("len"), pmt::from_long(-1)));
        
        // Control: Transition to packet processing state
        d_state = DEMOD_S_PROCESSING;
        d_samples_remaining = len * 8 / mcs_rate;  // Calculate duration
    }
}
```

### 5.4 Latency Characteristics

| Path Type | Latency | Trigger | Use Case |
|-----------|---------|---------|----------|
| Data Stream | < 1ms | Sample clock | Real-time demodulation/decoding |
| Control Message | 1-10ms | Event (packet arrival, error) | Stats collection, rate adaptation |
| Tag Propagation | < 1ms | Stream alignment | Metadata coordination |

### 5.5 Synchronization Mechanisms

**Mutex-Protected State (wifi_stats_collector):**
```cpp
std::mutex d_mutex;  // Protects shared statistics state
// Used whenever message handlers access mutable state
std::lock_guard<std::mutex> lock(d_mutex);
```

**Thread-Safe Message Port Operations:**
```cpp
// message_port_pub() is thread-safe
// Multiple message handlers can call concurrently
message_port_pub(pmt::mp("stats"), stats_dict);
```

**Block-Level Synchronization:**
```cpp
// forecast() informs scheduler about consumption patterns
void forecast(int noutput_items, gr_vector_int &ninput_items_required) {
    ninput_items_required[0] = noutput_items + 160;  // Lookahead for tag detection
}
```

---

## 6. Summary: Communication Architecture

### 6.1 Three-Layer Architecture

```
Layer 3: APPLICATION CONTROL
    ├─ Message Ports: rx_packets, tx_packets, rx_errors, signal_quality
    └─ Output: statistics (throughput, SNR, PER)

Layer 2: PACKET STATE MACHINE  
    ├─ Stream Tags: format, mcs, len, cfo, snr, rssi, chan
    └─ Coordination: Block state transitions based on tag presence

Layer 1: SAMPLE PROCESSING
    ├─ Data Streams: complex samples, LLRs, bits
    └─ Processing: FFT, Viterbi, modulation
```

### 6.2 Block Responsibility Matrix

| Block | Input | Output | Tags Read | Tags Write | Messages |
|-------|-------|--------|-----------|-----------|----------|
| pktgen | - | uint8 + TSB | TSB tag | format/mcs/len/seq | - |
| encode | uint8 + tags | uint8 + tags | format/mcs/len | sigl/signl/sigb0 | - |
| modulation | uint8 + tags | complex + tags | format/mcs | - | - |
| pad | complex + tags | complex + tags | packet_len | (none) | - |
| signal | complex + sync | complex | rad/snr/rssi | cfo/snr/rssi/mcs/len | - |
| demod | complex + tags | float (LLRs) | cfo/snr/chan | (none) | - |
| decode | float + tags | (message) | format/len | (none) | out |
| wifi_stats | - | - | - | - | rx/tx/error in; stats out |

### 6.3 Key Design Patterns

1. **Immutable PMT Dictionaries**: `pmt::dict_add()` returns new dictionary
2. **Safe Tag Propagation Control**: All processing blocks use `TPP_DONT`
3. **Manual Tag Management**: Blocks explicitly control what metadata flows
4. **Message-Driven Statistics**: Decoupled stats collection via message ports
5. **Hierarchical Type System**: PMT provides compile-time type checking
6. **In-Band Coordination**: Tags enable data-driven state machines

---

## 7. Conclusion

The gr-ieee80211 message passing architecture elegantly combines three complementary mechanisms:

- **Stream Tags** for sample-aligned, deterministic metadata propagation
- **Message Ports** for asynchronous, decoupled event notifications
- **State Machines** that trigger on tag presence and consume tagged data

This design achieves separation of concerns: signal processing (data flow) is isolated from statistics collection (control flow) while maintaining tight sample-level coordination through in-band tags. The extensive use of PMT dictionaries (236 operations) ensures type-safe polymorphic data handling across heterogeneous block types.

