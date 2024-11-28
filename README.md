
# Xvisor Final Project

This repository contains the implementation and tests related to a custom project using the Xvisor hypervisor. The project explores CPU detection, interrupt handling, and forked process management on the AArch64 architecture. Each test is implemented in C and is tailored for environments where Xvisor is the hypervisor.

---

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Getting Started](#getting-started)
  - [Requirements](#requirements)
  - [Setup](#setup)
- [Usage](#usage)
- [License](#license)

---

## Overview

Xvisor is a lightweight hypervisor designed for embedded systems. This project builds on Xvisor's capabilities by implementing various tests and features focused on:

1. **CPU Detection**: Testing the ability of the system to detect and identify CPU features.
2. **Process Forking and CPU Utilization**: Exploring the behavior of forked processes in the AArch64 environment.
3. **Interrupt Handling**: Evaluating the system's ability to handle interrupts in real-time and standard scenarios.

---

## Features

- **CPU Detection**:
  - Program to test and detect CPU properties in a virtualized environment.
  - File: `cpu-detection.c`
- **Fork CPU Tests**:
  - Tests forked processes under Xvisor.
  - Files: `aarm64_fork_test.c`, `fork_cpu_detection.c`
- **Interrupt Handling**:
  - Tests handling of interrupts in real-time and non-real-time scenarios.
  - Files: `interrupt1.c`, `interrupt_realtime.c`, `interrupt_catcher.c`

---

## Getting Started

### Requirements

- AArch64 platform (or emulator).
- Xvisor hypervisor installed and configured.
- GCC or any compatible C compiler for AArch64.
- Basic understanding of hypervisor operations.

### Setup

1. Clone this repository:
   \`\`\`bash
   git clone https://github.com/username/Xvisor-Final-Project.git
   cd Xvisor-Final-Project-main
   \`\`\`

2. Compile the test programs:
   \`\`\`bash
   gcc -o aarm64_cpu_test aarm64_cpu_test.c
   gcc -o aarm64_fork_cpu_test aarm64_fork_cpu_test.c
   gcc -o cpu_detection cpu-detection.c
   gcc -o interrupt1 interrupt1.c
   gcc -o interrupt_realtime interrupt_realtime.c
   gcc -o interrupt_catcher interrupt_catcher.c
   \`\`\`

3. Run the binaries in the Xvisor environment.

---

## Usage

### Running a Test

- Ensure that Xvisor is running and the environment is set up correctly.
- Use the following command format to execute a test:
  \`\`\`bash
  ./binary_name
  \`\`\`
  Example:
  \`\`\`bash
  ./cpu_detection
  \`\`\`

### Testing Scenarios

1. **CPU Detection**:
   - Binary: `cpu_detection`
   - Tests CPU identification under virtualization.

2. **Forked CPU Tests**:
   - Binary: `fork_cpu_detection`
   - Simulates and monitors forked processes.

3. **Interrupt Handling**:
   - Binary: `interrupt1`
   - Tests interrupt handling in a standard environment.

---

## License

This project is licensed under the terms of the license included in the `LICENSE` file.
