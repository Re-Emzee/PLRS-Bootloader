# BMS Firmware Overview

This page houses the firmware project responsible for communicating with the **Battery Management System (BMS)** and collecting battery cell information.  
The firmware runs on an **STM32U5xx** board using the **I²C** protocol for communication with the BMS.  
The BMS is controlled by the **BQ76920** chip (more information can be found on the [BMS page]).  

Diagnostics and events are handled by the firmware and reported via **CAN** to the Raspberry Pi.  
A component overview can be found on the **BMS Research** page.

> **Note:** This project is a fork of `bq769x0` tailored for PLRS.

---

## Events and Alert Sequence

The **BQ76920** features an alert signal driven by the `SYS_STAT` register.  
This signal is the logical OR of all bits in the register. These conditions are mapped as follows:

| Addr | D7        | D6        | D5            | D4           | D3  | D2  | D1  | D0  |
|------|------------|-----------|----------------|--------------|-----|-----|-----|-----|
| 0x00 | CC_READY   | RESERVED  | DEVICE_XREADY | OVRD_ALERT   | UV  | OV  | SCD | OCD |

To clear the **ALERT** signal, the source bit in the `SYS_STAT` register must first be cleared by writing a “1” to that bit.  
The ALERT pin clears automatically once all bits are cleared.  

The `SYS_STAT` register updates and triggers the alert pin under the following conditions:

---

### D7 — `CC_READY`

Triggered when a new current reading is available.  
Our configuration uses `CC_EN` in **ALWAYS ON** mode, gathering a new reading every **250 ms**.  
(See **Cell Balancing** section.)

- `0` – Fresh CC reading not yet available or bit cleared by host microcontroller.  
- `1` – Fresh CC reading is available. Remains latched high until cleared by host.

---

### D5 — `DEVICE_X`

Internal chip fault indicator. When this bit is set to `1`, it should be cleared by the host.  
May be set due to excessive system transients. This bit may only be cleared (not set) by the host.  
This event also clears all `CELLBAL` control bits and must be explicitly rewritten by the host.

- `0` – Device is OK.  
- `1` – Internal chip fault detected. Recommended: host clears this bit after a few seconds.  

---

### D4 — `OVRD_`

External pull-up on the ALERT pin indicator. Active only when ALERT pin is not already driven high by the AFE.

- `0` – No external override detected.  
- `1` – External override detected. Remains latched high until cleared by host.

---

### D3 — `UV` (Undervoltage)

> **2.5 V lower limit**

Undervoltage fault event indicator.

- `0` – No UV fault detected.  
- `1` – UV fault detected. Remains latched high until cleared by host.

**Behavior:**  
When UV is detected, discharging is disabled until the UV condition clears.  
A UV counter exists for SOH and debug implementations.

---

### D2 — `OV` (Overvoltage)

Overvoltage fault event indicator.

- `0` – No OV fault detected.  
- `1` – OV fault detected. Remains latched high until cleared by host.

**Behavior:**  
When OV is triggered, charging is disabled until the condition clears.  
An OV counter exists for SOH and debugging.  
If this occurs during discharge (`PackCurrent ≥ 0`) and SOH not recently updated, the firmware runs the SOH calculation.

---

### D1/D0 — `SCD` / `OCD`

Short-circuit or overcurrent in discharge fault indicator.

- `0` – No fault detected.  
- `1` – Fault detected. Remains latched high until cleared by host.

**Behavior:**  
On `SCD` or `OCD` trigger, discharging stops immediately.

---

## Cell Balancing

The PLRS battery pack consists of **4 cells** in a **4s2p** configuration.  
While there are 8 total cells, each pair is paralleled — so the BMS “sees” only 4.  
(See **Battery Configuration** for more info.)

| Pin | 0x0 | 0x1 | 0x2 | 0x3 | 0x4 | 0x5 |
|------|------|------|------|------|------|------|
| Cell | GND | Cell1 | Cell2 | Cell3 | Cell3 | Cell4 |

During charge/discharge cycles, slight differences in capacity, leakage, and self-discharge cause cell charge drift.

The **BQ76920** uses **passive internal balancing drivers** to equalize cell voltages at up to **50 mA**.  
Multiple cells may be balanced simultaneously, but **adjacent cells should not** be balanced together (to avoid exceeding pin limits).  
The total duty cycle for balancing is ~70 % per 250 ms.

**Balancing occurs only during charging** for efficiency and pack health.

To activate a balancing channel, set the corresponding bit in the `CELLBAL1`, `CELLBAL2`, or `CELLBAL3` registers.

All cell balancing bits in these registers are **automatically cleared** under these conditions:

- `DEVICE_XREADY` is set  
- Device enters **NORMAL** mode from **SHIP** mode  

These bits must be explicitly rewritten by the host after such events.

---

## Testing

The firmware testing process has two primary goals:

1. Test against a **simulated testbench** with known register values.  
2. Perform **quantitative testing** with physical hardware.

See **BMS Testing Procedures** for other BMS-related tests.  
Additional tools are available in the `dev` branch.

---

### Procedure

1. Connect the corresponding **I²C** pins (`SDA` and `SCL`).  
   - The BMS already includes **10 kΩ pull-up resistors**, so no external ones are required.
2. Connect the **common ground** between the BMS and STM board.  
   - Any ground pin on the STM is fine; they are internally connected.
3. Run the **BMS firmware** in **debug mode**.  
   - STM32CubeIDE does not easily print to STDOUT.  
   - Inspect variable values by pausing execution and hovering over variables.
4. Ensure the **BMS struct** is defined after initialization.  
   - Key fields to verify: `gain` and `offset`.

---

## STM32 and AD2

An **Analog Discovery 2 (AD2)** is used as a spy device to read I²C signals directly from the STM board.  
This ensures that the STM board correctly requests registers from the BMS (verified via the protocol viewer).

More information is available on the **AD2 website** and below.  
See the **Overview page** for related BMS resources and libraries.

---

## STM32 and Arduino

To verify the BMS behavior, the firmware was tested against an **Arduino** that emulates BMS register values.  
The setup was built for the **SAMD XIAO**, but should work with other Arduino-compatible devices.

We used the **Arduino IDE** for its serial monitor, though other toolchains are supported.  
(Refer to your microcontroller documentation.)

The Arduino testbench code can be found here:  
**[SAMD XIAO BMS](#)**

---
