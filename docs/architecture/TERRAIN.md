# Terrain architecture

Owner: Darrius. Parent: [system architecture](../ARCHITECTURE.md). Wiring: [hardware](../HARDWARE.md). Review scope: FR4/FR5, Sections 3–6; trials T06/T07/T14/T15.

## Integrated target behavior

Terrain owns GY-511/I2C0 at proposed priority 2, nominal 20 ms fresh-data polling. Combine calibrated acceleration/magnetic readings with coherent measured motion context to classify stationary, accelerating, turning, climb/descent and impact. Publish timestamped `M_terrain`; collision/critical acquisition faults notify Safety independently of diagnostics.

GY-511 has no gyro. Turn rate may be derived from encoder motion and must be labelled as an estimate. Hump detection is distinct from physical height. Use a reference-validated pitch/travel or other justified method, with axis/bias calibration and rejection of slip/dynamic/incomplete windows; bare uncorrected double integration is not a valid established height result. Retain the greatest **valid** run peak; invalid estimates must not overwrite it.

## Current implementation and optional stationary bench

The active follower does not read GY-511; `src/subsystems/terrain.c` remains a mission stub. The optional stationary `src/sensor_bench.c` configures acceleration 0x19 at 50 Hz, ±2 g, high-resolution 12-bit/BDU and magnetometer 0x1E at 30 Hz continuous, ±1.3 gauss. Decode device X/Z/Y order into XYZ; overflow readings are invalid. Pitch/roll is gravity-derived in sensor axes only near 1 g; no calibrated heading, raw gyro or physical height is claimed.

I2C operations have finite 2 ms timeouts, device configuration retries every 2 s, data expires after 250 ms and the optional bench watchdog is 1 s. Mounted axis/offset and dynamic-motion validation remain pending. Test labelled traces and measured reference humps, including an invalid estimate after a valid maximum.

## GY-511 / LSM303DLHC technical reference

[OKY3231-2.pdf](../../../document/OKY3231-2.pdf) identifies a **GY-511 breakout carrying an LSM303DLHC** and advertises a 3–5 V module input. [PicoCar.pdf p. 2](../../../document/PicoCar.pdf) labels its breakout connections `DRDY`, `INT2`, `INT1`, `SDA`, `SCL`, `GND`, `3.3`, and `Vin`. The breakout's regulator/pull-up circuit is not specified, so the safe supply pin and I²C high level must be verified on the physical board. The **chip itself** is specified for Vdd 2.16–3.6 V and is not a 5 V IC. [lsm303dlhc.pdf pp. 1, 12–14](../../../document/lsm303dlhc.pdf)

The GY-511 product sheet additionally advertises 16-bit output, ±2/4/8/16 g accelerometer scales, ±1.3–8.1 gauss magnetometer scales, embedded temperature sensing, acceleration FIFO, low-power/power-down modes, orientation detection and programmable motion/free-fall interrupts. These describe chip capabilities; the breakout sheet does **not** specify a gyroscope, a circuit schematic, pull-up voltage, or the electrical meaning of both `Vin` and `3.3` headers. The pictured header order from one end is **DRDY, INT2, INT1, SDA, SCL, GND, 3.3, Vin**; verify board silkscreen orientation before wiring. [OKY3231-2 p. 1; PicoCar p. 2]

**Why only four GY-511 wires are proposed:** `Vin` plus `GND` powers the module, and `SDA` plus `SCL` lets the Pico configure and read all three acceleration axes and all three magnetic-field axes over I²C. The breakout's separate `3.3` pin is **not another axis or data connection**; the supplied breakout sheet does not establish whether it is regulator output or a bypass input. Power through the advertised 3–5 V module input (`Vin`) from Grove 3's 3V3 rail and leave `3.3` unconnected, rather than tying two power pins together. Verify that the installed board powers up and the I²C lines idle at no more than 3.3 V. `INT1` and `INT2` are optional, configurable accelerometer event outputs (for thresholds, motion/orientation, etc.); `DRDY` is an optional data-ready output. The firmware can poll the accelerometer status register and magnetic data-ready status over I²C before reading samples, so none of these three outputs is required for basic sensing. They can be added later if event-driven sampling is useful, with extra GPIO allocation and voltage checks. [lsm303dlhc pp. 9, 17, 23, 26–34, 39]

The LSM303DLHC contains a **3-axis accelerometer and 3-axis magnetometer**, with a temperature output and motion/orientation interrupt functions. It does **not** contain a gyroscope. Its accelerometer ranges are ±2/4/8/16 g; magnetic ranges ±1.3 to ±8.1 gauss. The I²C bus supports 100 and 400 kHz; the accelerometer uses 7-bit address **0x19**, the magnetometer **0x1E**. The two cores have separate register maps on the same bus. External I²C pull-ups are required at chip level; check whether the breakout already has them and that they pull to 3.3 V. [lsm303dlhc.pdf pp. 1, 10, 13, 19–23](../../../document/lsm303dlhc.pdf)

**Electrical and data facts from the chip sheet:** chip Vdd operating range 2.16–3.6 V, Vdd_IO from 1.71 V up to approximately Vdd+0.1 V, operating temperature −40 to +85 °C; chip-level absolute maximum Vdd is 4.8 V, which is a damage limit and **not** a 5 V operating rating. I²C standard/fast clocks are up to 100/400 kHz with external pull-ups; the application example recommends **10 kΩ pull-ups** on SDA/SCL and local decoupling (4.7 µF, 0.22 µF, 100 nF and 10 µF capacitors at the bare chip). The assembled GY-511 may already contain relevant parts. High-current motor wiring can distort the magnetic field measurement; keep it away from the chip. [lsm303dlhc pp. 12–14, 17–18]

| Device | 7-bit I²C address | Corresponding 8-bit write/read address bytes | Practical note |
| --- | --- | --- | --- |
| Accelerometer | `0x19` | `0x32` / `0x33` | Set the register-subaddress high bit for multi-byte auto-increment reads. |
| Magnetometer | `0x1E` | `0x3C` / `0x3D` | Separate register map; data register order is X, Z, Y. |

Use **7-bit addresses** with Pico SDK I²C calls that expect 7-bit addresses; do not pass the 8-bit write byte in their place. [lsm303dlhc pp. 19–22]

| Accelerometer setting (`CTRL_REG1_A` ODR bits) | Normal/low-power output data rate |
| --- | ---: |
| `0000` | Power-down |
| `0001`, `0010`, `0011`, `0100` | 1, 10, 25, 50 Hz |
| `0101`, `0110`, `0111` | 100, 200, 400 Hz |
| `1000` | 1.620 kHz in low-power mode only |
| `1001` | 1.344 kHz normal or 5.376 kHz low-power |

`CTRL_REG1_A` defaults to `0x07`: X/Y/Z enable bits are set but ODR=`0000`, so configure a nonzero rate before expecting fresh acceleration samples. `CTRL_REG4_A` controls full scale, high-resolution mode and block-data-update (BDU); enabling BDU prevents high/low bytes from being mixed across an update. The datasheet lists nominal acceleration sensitivity of **1, 2, 4, 12 mg/LSB** for ±2, ±4, ±8, ±16 g respectively under its stated conditions. Data are signed two's complement; use the configured mode/scale when converting raw register values to g or m/s². [lsm303dlhc pp. 10, 23, 25, 27, 29]

| Magnetometer `CRA_REG_M` rate bits | Output rate |
| --- | ---: |
| `000`, `001`, `010`, `011` | 0.75, 1.5, 3, 7.5 Hz |
| `100`, `101`, `110`, `111` | 15, 30, 75, 220 Hz |

`CRB_REG_M` chooses the magnetic range/gain; `MR_REG_M` selects continuous, single-conversion or sleep mode. The register map lists `MR_REG_M` default `0x03` (sleep), so configure a measurement mode before expecting magnetic samples. At ±1.3 gauss, nominal gain is **1100 LSB/gauss for X/Y and 980 LSB/gauss for Z**; gains decrease at wider ranges. Temperature is enabled through `CRA_REG_M` and read from `TEMP_OUT_H_M`/`TEMP_OUT_L_M`; the sheet gives **8 LSB/°C** and a 12-bit two's-complement value, but no absolute-temperature accuracy claim suitable for hump measurements. [lsm303dlhc pp. 10, 24, 37–39]

| Needed operation | Register / fact | Source |
| --- | --- | --- |
| Enable acceleration axes and choose output rate | `CTRL_REG1_A` at 0x20 on 0x19; default axes enabled but **power-down rate** until configured. | [lsm303dlhc p. 25](../../../document/lsm303dlhc.pdf) |
| Set accelerometer scale and coherent read | `CTRL_REG4_A` at 0x23; full scale, high-resolution mode, and block-data-update options. | [lsm303dlhc p. 27](../../../document/lsm303dlhc.pdf) |
| Read acceleration | `OUT_X/Y/Z_L/H_A` at 0x28–0x2D; multi-byte accelerometer reads require the subaddress auto-increment bit. Convert signed data using the selected range/mode. | [lsm303dlhc pp. 21, 23](../../../document/lsm303dlhc.pdf) |
| Configure/read magnetic field | `CRA_REG_M` 0x00 rate, `CRB_REG_M` 0x01 gain, `MR_REG_M` 0x02 mode; data registers 0x03–0x08 are ordered **X, Z, Y** and high byte before low byte. | [lsm303dlhc pp. 24, 37–39](../../../document/lsm303dlhc.pdf) |
| Check freshness and temperature | Accelerometer status at 0x27, magnetic data-ready at 0x09; temperature output at 0x31/0x32 after enabling it. | [lsm303dlhc pp. 23–24, 28, 39](../../../document/lsm303dlhc.pdf) |

**Implementation register map** (suffix `_A` belongs to I²C `0x19`; suffix `_M` belongs to I²C `0x1E`): [lsm303dlhc pp. 23–39]

| Address | Accelerometer register and role |
| --- | --- |
| `0x20` | `CTRL_REG1_A`: ODR, low-power enable and X/Y/Z enable. |
| `0x21` | `CTRL_REG2_A`: high-pass filter mode/cutoff and filtered-data selection. |
| `0x22` | `CTRL_REG3_A`: route click, interrupt and data-ready events to INT1. |
| `0x23` | `CTRL_REG4_A`: BDU, byte order, full-scale ±2/4/8/16 g and high-resolution mode. |
| `0x24` | `CTRL_REG5_A`: reboot memory content, FIFO enable and latched-interrupt options. |
| `0x25` | `CTRL_REG6_A`: route interrupt functions to second interrupt pin, polarity. |
| `0x26` | `REFERENCE_A`: high-pass/interrupt reference value. |
| `0x27` | `STATUS_REG_A`: new-data and overrun bits for axes. |
| `0x28–0x2D` | X low/high, Y low/high, Z low/high signed acceleration data. |
| `0x2E–0x2F` | FIFO control/source; bypass, FIFO, stream and trigger modes. |
| `0x30–0x33` | INT1 configuration/source/threshold/duration. |
| `0x34–0x37` | INT2 configuration/source/threshold/duration. |
| `0x38–0x3D` | Click/double-click configuration, source, threshold and timing. |

| Address | Magnetometer register and role |
| --- | --- |
| `0x00` | `CRA_REG_M`: output rate and temperature enable. |
| `0x01` | `CRB_REG_M`: magnetic full scale/gain. |
| `0x02` | `MR_REG_M`: continuous conversion, single conversion or sleep. |
| `0x03–0x08` | X high/low, **Z high/low**, **Y high/low** signed field data. |
| `0x09` | `SR_REG_M`: data ready and lock flags. |
| `0x0A–0x0C` | Identification registers; documented byte patterns `0x48`, `0x34`, `0x33`. |
| `0x31–0x32` | Temperature high/low, two's-complement data, when enabled. |

**Magnetic gain table** (configured range and nominal register counts per gauss): [lsm303dlhc pp. 10, 38]

| Range | X/Y LSB per gauss | Z LSB per gauss |
| ---: | ---: | ---: |
| ±1.3 gauss | 1100 | 980 |
| ±1.9 gauss | 855 | 760 |
| ±2.5 gauss | 670 | 600 |
| ±4.0 gauss | 450 | 400 |
| ±4.7 gauss | 400 | 355 |
| ±5.6 gauss | 330 | 295 |
| ±8.1 gauss | 230 | 205 |

Never write registers marked **reserved** or overwrite factory-calibration/trim contents. If using the chip's INT1/INT2 and FIFO later, configure threshold and duration relative to the chosen ODR and read the interrupt source register to clear a latched event. For a first driver, polling timestamped acceleration at a chosen ODR is simpler; the optional DRDY/INT wires are not required in the initial pinout. [lsm303dlhc pp. 23–24, 27–34]

**What the sensor can support:** pitch/roll from gravity when stationary or moving slowly; acceleration transients; a magnetic heading after calibration and separation from motor currents. It can help identify a climb/descent signature and sudden impacts. The source datasheet warns nearby high currents disturb magnetic readings. Mount it rigidly, mark its axes relative to forward/left/up, log stationary offsets, and test with motors running. [lsm303dlhc pp. 9, 15, 18]

**What it cannot directly report:** angular velocity or yaw rate, because there is **no gyro**. A heading difference divided by time is a derived magnetic estimate and can be poor beside motors or steel obstacles; encoder-based turning may be more reliable. Hump **height in metres** is also not a direct output. Pitch plus travelled distance could yield an estimate only if slope geometry, wheel contact, mounting, and slip are modeled and validated against known hump heights. Raw vertical acceleration double integration without drift control is not a dependable height measurement. Keep “detected hump” and “validated height estimate” as separate outputs.
