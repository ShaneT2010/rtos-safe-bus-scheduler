# Deterministic Window Scheduling and Pre/Post-Silicon Co-Verification for Preemptive RTOS Edge Nodes

## 1. Abstract
This specification addresses the microsecond-level concurrency bottleneck in power-constrained Internet of Things (IoT) edge devices where sub-millisecond task preemption conflicts with asynchronous single-wire bus protocols. Utilizing the asynchronous dht22 communication protocol as an evaluation baseline, logic states are encoded via strict pulse-width variations—specifically $26\,\mu\text{s}$ and $70\,\mu\text{s}$ high-level durations. When encapsulated within a preemptive Real-Time Operating System (RTOS) environment, the stochastic arrival of the $1\,\text{ms}$ kernel tick interrupt induces thread context switching during critical timing-loops, resulting in data frame corruption and elevated Bit Error Rates (BER). Traditional mitigations that mask hardware interrupts (`taskENTER_CRITICAL()`) degrade deterministic responsiveness and increase global interrupt latency.

This project implements a Zero-Interrupt-Latency, Microsecond-Safe Window Scheduling Algorithm. By probing the ARM Cortex-M4 internal hardware down-counter (`SysTick->VAL`), the firmware dynamically evaluates the residual time budget prior to the next scheduled kernel tick boundary. Real-time CPU execution is voluntarily yielded via deterministic back-off mechanisms if the computed window falls below the minimum transaction threshold ($150\,\mu\text{s}$), eliminating bus collisions without masking low-level interrupts. The architectural integrity is validated through a unified Pre/Post-Silicon Co-Verification framework, combining SystemVerilog Constrained Random Verification (CRV) for boundary emulation with automated host-side Python stress-testing for physical silicon characterization.

## 2. System Architecture and Verification Pipelines

The validation topology is bifurcated into dual verification domains to isolate logical correctness from physical non-idealities:

### 2.1 Pre-Silicon Functional Verification Domain
The simulated testbench is constructed entirely within an IEEE 1800 SystemVerilog environment. The execution behavior of the preemptive RTOS scheduler and the single-wire asynchronous device is modeled as non-deterministic concurrent processes. 
*   **Stimulus Generation**: Object-oriented transaction classes utilize constrained random variables to model the mathematical distribution of task activation offsets relative to the kernel tick.
*   **Property Checking**: SystemVerilog Assertions (SVA) monitor the sub-microsecond pulse-width intervals to capture timing violations at boundary conditions (corner cases).

### 2.2 Post-Silicon Physical Validation Domain
The physical implementation leverages an ARM Cortex-M4 microcontroller (STM32F411CEU6) interfacing with a physical sensor node via an open-drain GPIO configuration. 
*   **On-Chip Subsystems**: `vTask_Sensor` polls core registers (`SysTick->VAL`) to execute the deterministic scheduling algorithm, passing validated data structures asynchronously via thread-safe FreeRTOS kernel queues (`xQueue`) to `vTask_UART`.
*   **Host Telemetry Layer**: A host-side automated execution engine (`DV_Bridge.py`) samples the hardware serialization stream, injecting high-frequency asynchronous dummy payloads via hardware UART interrupts to stress-test the MCU under peak interrupt load.

*   ## 3. Implementation Framework and Theoretical Milestones

Development and verification are categorized into five sequential execution phases, charting theoretical principles from pre-silicon validation to physical instrumentation.

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

## 4. Hardware and Software Environment Specs
*   **Target Core**: ARM Cortex-M4 (STM32F411CEU6)
*   **Toolchain**: STM32CubeIDE (GNU Embedded Toolchain for Arm)
*   **Simulation Suite**: QuestaSim / ModelSim (IEEE 1800 Compliant)
*   **Analysis Layer**: Python 3.10 (PySerial, Matplotlib)
*   **Physical Instrumentation**: VC9205 Digital Multimeter
