# Deterministic Window Scheduling and Pre/Post-Silicon Co-Verification for Preemptive RTOS Edge Nodes

## 1. Abstract
This specification addresses the microsecond-level concurrency bottleneck in power-constrained Internet of Things (IoT) edge devices where sub-millisecond task preemption conflicts with asynchronous single-wire bus protocols. Utilizing the asynchronous dht22 communication protocol as an evaluation baseline, logic states are encoded via strict pulse-width variations—specifically $26\ \mu\text{s}$ and $70\ \mu\text{s}$ high-level durations. When encapsulated within a preemptive Real-Time Operating System (RTOS) environment, the stochastic arrival of the $1\ \text{ms}$ kernel tick interrupt induces thread context switching during critical timing-loops, resulting in data frame corruption and elevated Bit Error Rates (BER). Traditional mitigations that mask hardware interrupts (`taskENTER_CRITICAL()`) degrade deterministic responsiveness and increase global interrupt latency.

This project implements a Zero-Interrupt-Latency, Microsecond-Safe Window Scheduling Algorithm. By probing the ARM Cortex-M4 internal hardware down-counter (`SysTick->VAL`), the firmware dynamically evaluates the residual time budget prior to the next scheduled kernel tick boundary. Real-time CPU execution is voluntarily yielded via deterministic back-off mechanisms if the computed window falls below the minimum transaction threshold ($150\ \mu\text{s}$), eliminating bus collisions without masking low-level interrupts. The architectural integrity is validated through a unified Pre/Post-Silicon Co-Verification framework, combining SystemVerilog Constrained Random Verification (CRV) for boundary emulation with automated host-side Python stress-testing for physical silicon characterization.

## 2. System Architecture and Verification Pipelines

The validation topology is bifurcated into dual verification domains to isolate logical correctness from physical non-idealities:

### 2.1 Pre-Silicon Functional Verification Domain (Track A)
The simulated testbench is constructed entirely within an IEEE 1800 SystemVerilog environment. The execution behavior of the preemptive RTOS scheduler, the single-wire asynchronous device, and multi-drop sub-nodes are modeled as non-deterministic concurrent processes. 
*   **Stimulus Generation**: Object-oriented transaction classes utilize constrained random variables to model the mathematical distribution of task activation offsets relative to the kernel tick.
*   **Multi-Drop Bus Modeling**: Leverages tri-state net configurations (`tri1`) to accurately emulate open-drain Wired-AND physical layer behaviors where concurrent slave transmissions create predictable electrical collisions.
*   **Property Checking**: SystemVerilog Assertions (SVA) monitor the sub-microsecond pulse-width intervals and dynamic protocol state switches to capture timing violations at boundary conditions (corner cases).

### 2.2 Post-Silicon Physical Validation Domain (Track B)
The physical implementation leverages an ARM Cortex-M4 microcontroller (STM32F411CEU6) interfacing with a physical sensor network via an open-drain GPIO configuration. 
*   **On-Chip Subsystems**: `vTask_Sensor` polls core registers (`SysTick->VAL`) to execute the deterministic scheduling algorithm, passing validated data structures asynchronously via thread-safe FreeRTOS kernel queues (`xQueue`) to `vTask_UART`.
*   **Host Telemetry Layer**: A host-side adaptive telemetry execution engine (`Phase61TelemetryBridge` inside `dv_bridge.py`) dynamically segregates ingested data packet types via pattern-matching, parsing both legacy DHT22 frames and 64-bit 1-Wire ROM IDs while cross-checking standard Maxim CRC-8 polynomials under peak interrupt load.

---

## 3. Implementation Framework and Theoretical Milestones

### Phase 1: Constrained Random Verification of Scheduling Boundaries
*   **Implementation**: Construct the complete SystemVerilog testbench infrastructure (`tb_bus_scheduler.sv`), including virtual interfaces, transactors, and synchronization drivers.
*   **Theoretical Foundations**: Discrete-event simulation mechanics, transaction-level modeling (TLM), and functional coverage closure techniques.

### Phase 2: Hardware-Software Interface Abstraction & Data Serialization
*   **Implementation**: Configure the STM32 hardware clock trees, establish register-level open-drain bi-directional GPIO controls, and deploy the core Python serialization script to interface with host subsystems.
*   **Theoretical Foundations**: Bi-directional open-drain bus mechanics, transmission line parasitic capacitance effects, and UART baud rate derivation parameters.

### Phase 3: Concurrency Degradation Profiling under Preemptive Kernels
*   **Implementation**: Deploy the FreeRTOS kernel with active preemption enabled. Quantify the exact correlation between task context switching frequency and the resulting data packet drop rate.
*   **Theoretical Foundations**: Rate-monotonic scheduling theory, critical section analysis, and priority-induced execution jitter.

### Phase 4: Register-Level Window Verification and Fault Mitigation
*   **Implementation**: Embed the memory-mapped `SysTick->VAL` safe-window algorithm within the firmware execution path. Execute the automated Python noise-injection test to verify the suppression of packet drop rates to 0%.
*   **Theoretical Foundations**: ARM Cortex-M4 core architecture peripherals, pipeline latency optimization, and zero-interrupt-latency embedded programming paradigms.

### Phase 5: Power State Optimization and Micro-Ampere Instrumentation
*   **Implementation**: Enable FreeRTOS Tickless Idle mode, offloading kernel wake-up operations to internal Low-Power Timers. Physically measure and isolate static versus dynamic power consumption transitions using an inline shunted digital multimeter.
*   **Theoretical Foundations**: CMOS power dissipation physics ($P = C \cdot V^2 \cdot f + I_{\text{leak}} \cdot V$), low-power sleep state vector tables, and precision low-current physical instrumentation constraints.

---

### Track B: Post-Silicon Hardware Physical Validation (HW Phase 1-4)
*   **HW Phase 1: Physical Firmware Flashing & On-Chip Debugging (`trackB-hw-flash`)**
    *   *Execution*: Deploy compiled binaries via ST-Link V2 hardware programmers. Interrogate Cortex-M4 memory-mapped structures using GDB register-level inspection to guarantee that the `SysTick` down-counter runs at full CPU core frequency ($100\ \text{MHz}$).
*   **HW Phase 2: Signal Line Impedance & Coupling Characterization (`trackB-hw-wire`)**
    *   *Execution*: Interface the physical sensor array onto the MCU GPIO pins. Utilize a mixed-signal oscilloscope to analyze transmission line characteristics, tuning the pull-up resistor constraints ($4.7\ \text{k}\Omega$) to eliminate rise-time distortion ($T_r$) induced by parasitic bus capacitance ($C_{\text{bus}}$).
*   **HW Phase 3: Logic Analyzer Microsecond Protocol Profiling (`trackB-hw-logic`)**
    *   *Execution*: Hook up an external 16-channel logic analyzer to the active single-wire bus. Capture raw pulse-width profiles at $100\ \text{MS/s}$ sampling rates to cross-examine hardware bit timings against the predefined Pre-Silicon SVA protocol parameters.
*   **HW Phase 4: Telemetry Ground Station Hardware Integration (`trackB-hw-serial`)**
    *   *Execution*: Bridge the STM32 hardware UART peripheral with the host PC via a USB-to-TTL serial converter. Ingest asynchronous frames into the real-time telemetry bridge, tracking Packet Drop Rates (PDR) under active interrupt load.

---

### Phase 6: Multi-Node Topology Expansion, Predictive Windowing & Self-Healing
*   **Phase 6.1: Time-Division Multi-Sensor Chain & Co-Design Isolation (Current Milestone)**
    *   *Execution*: Implemented a software-defined TDMA sequential polling engine inside `firmware/sensor_array_scheduler.c` to handle multiple DHT22 channels on a shared system architecture. Refactored the SystemVerilog testbench top with `tri1` nets to verify multi-node electrical loading configurations, and updated the host Python telemetry layer to track PDR across distinct node profiles.
*   **Phase 6.2: Machine Learning Adaptive Windowing (`extension-ml-window`)**
    *   *Execution*: Incorporate an on-chip, lightweight linear regression predictor that dynamically analyzes the moving variance of historical RTOS task execution times, scaling the execution window threshold beyond the static $150\ \mu\text{s}$ baseline based on dynamic system loading.
*   **Phase 6.3: Fault-Tolerant Bus Crash Recovery (`extension-recovery`)**
    *   *Execution*: Implement an automated watchdog and reset routine inside the core hardware state machine. If an unresolvable line deadlock or an extended dominant-low state ($>960\ \mu\text{s}$) is captured by SVA or the firmware timer, the MCU triggers an immediate GPIO bit-bang pull-high reset sequence to clear the bus deadlock without power-cycling the system.

---

## 4. Hardware and Software Environment Specs
*   **Target Core**: ARM Cortex-M4 (STM32F411CEU6)
*   **Toolchain**: STM32CubeIDE (GNU Embedded Toolchain for Arm)
*   **Simulation Suite**: QuestaSim / ModelSim / Synopsys VCS (IEEE 1800 SystemVerilog Compliant)
*   **Analysis Layer**: Python 3.10+ (PySerial, Matplotlib)
*   **Physical Instrumentation**: VC9205 Digital Multimeter, 16-Ch 100MS/s Logic Analyzer, Mixed-Signal Oscilloscope
