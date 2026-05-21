# Engineering Workflow and Branch Management Specification

This document details the operational Standard Operating Procedure (SOP) for branch transitions, testing paradigms, continuous localization, and multi-domain integration throughout the project lifecycle.

---

## 1. Standard Branch Transition Protocol
Every development milestone must be executed within an isolated feature branch derived from the latest upstream `main` branch. Direct commits to `main` are strictly prohibited to preserve structural integrity and maintain a clean production history.

### Git Command Sequence for Feature Initialization

    # Step 1: Sync local master topology with upstream repository
    git checkout main
    git pull origin main

    # Step 2: Initialize isolated parallel development branch
    git checkout -b feature/phase1-sv-env

### Git Command Sequence for Remote Synchronization

    # Step 3: Stage localized operational updates
    git status
    git add .

    # Step 4: Execute structured transaction logging (Commit)
    # Format standard: <type>: <clear technical description>
    git commit -m "feat: implement memory-mapped abstraction layer for phase 1"

    # Step 5: Push local pipeline state to remote origin
    git push -u origin feature/phase1-sv-env

---

## 2. Pull Request (PR) and Merge Gates
Upon pushing the feature branch, an upstream Pull Request (PR) must be initiated via the GitHub web interface.

1. **Title Alignment**: Must strictly reflect the engineering objective (e.g., `feat: Integrate SystemVerilog Assertion Engine for Boundary Tracking`).
2. **Code Review Checklist**: Inspect `Files changed` for redundant variables, timing alignment issues, or non-deterministic loops.
3. **Merge Action**: Execute `Merge pull request` and `Confirm merge` only after verifying zero syntax errors.
4. **Local Infrastructure Cleanup**:

    git checkout main
    git pull origin main

---

## 3. Five-Phase Sequential Execution Roadmap

### Phase 1: Pre-Silicon Functional Verification Domain [Active]
* **Scope**: IEEE 1800 SystemVerilog Verification Environment setup.
* **Core Components**: Interface definition, constrained-random transaction generation, and hardware assertion (SVA) engine tracking microsecond-level timing anomalies.

### Phase 2: Hardware-Software Interface Abstraction & Data Serialization [Pending]
* **Scope**: STM32 Hardware Abstraction Layer (HAL) firmware and data ingestion pipeline.
* **Core Components**: Bare-metal single-wire protocol implementation, direct register reading, and data synchronization interface with Python-based analysis tools.

### Phase 3: Concurrency Degradation Profiling under Preemptive Kernels [Pending]
* **Scope**: FreeRTOS Preemptive Scheduling Impact Evaluation.
* **Core Components**: Interrupt Service Routine (ISR) latency analysis, context switching timing measurements, and multi-task resource contention stress testing.

### Phase 4: Register-Level Window Verification and Fault Mitigation [Pending]
* **Scope**: Timing-error correction algorithms.
* **Core Components**: Hardware microsecond-counter tracking (`SysTick->VAL`), runtime jitter calculation, and software-fallback packet recovery mechanisms.

### Phase 5: Power State Optimization and Micro-Ampere Instrumentation [Pending]
* **Scope**: Energy-efficiency analysis and physical testing.
* **CoreComponents**: STM32 low-power sleep state configuration, sensor reactivation timing budget, and multi-meter current consumption profiling.
