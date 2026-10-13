# Buddy 4 (Subsystem 4: Terrain & IMU) Status & Buddy 2 Integration Guide

**Owner:** Darrius (Student ID: 2500360)  
**Target:** Micro T-Kernel 3.0 SMP (Core 1, Priority 16)  
**Hardware:** GY-511 (ST LSM303DLHC 3-axis Accelerometer + 3-axis Magnetometer, Grove 3 / I2C0)  
**Primary Integration Partner:** Buddy 2 (Mahesha — Motion, Motors & Encoders)  
**Status Date:** 13 October 2026  

---

## 1. Executive Summary

Subsystem 4 owns the GY-511 IMU on I2C0 (`GP4` SDA / `GP5` SCL). It is responsible for **motion monitoring, tilt kinematics, hump detection, maximum peak height estimation, and collision detection**.

The baseline subsystem is **fully compiled, linked, and verified** under micro T-Kernel 3.0 SMP. It runs at a deterministic **50 Hz (20 ms period)** on Core 1 and publishes thread-safe snapshot copies via a priority-inheritance mutex.

This document summarizes:
1. What is **currently implemented and working** in Subsystem 4.
2. The **existing integration points** with Subsystem 2 (`s2_stop`).
3. What is **pending and needs to be completed in collaboration with Buddy 2** (encoder odometry fusion, turn-rate estimation, and motion event classification).

---

## 2. Currently Implemented Features (Working Baseline)

### 2.1 Hardware Register Driver (`drv_gy511.c` / `drv_gy511.h`)
* **I2C0 Peripheral Configuration:** Direct register-level driver operating at 400 kHz Fast Mode.
* **Dual Device Addresses:**
  * Accelerometer: `0x19` (Write `0x32`, Read `0x33`)
  * Magnetometer: `0x1E` (Write `0x3C`, Read `0x3D`)
* **Coherent Multi-Byte Reads:** Sub-address bit 7 (`0x80`) auto-increment enabled for atomic multi-register transfers.
* **Non-Blocking Bus Guard:** Every bus wait has a hard $5\text{ ms}$ timeout (`GY511_TIMEOUT_US = 5000`). If a bus fault or disconnection occurs, the driver aborts and returns `false` without locking up the RTOS.
* **Operating Modes:** Accel configured to $100\text{ Hz}$, high-resolution $\pm 2\text{g}$ ($1\text{ mg/LSB}$); Mag configured to $30\text{ Hz}$ continuous.

### 2.2 Calibration, Filtering & Kinematics (`terrain_algo.c` / `terrain_algo.h`)
* **Startup Calibration:** Takes 100 samples during a 2.0-second resting window at startup to calculate zero-bias offsets ($A_{x,\text{bias}}, A_{y,\text{bias}}, A_{z,\text{bias}} - 1.0\text{g}$).
* **Noise Suppression:** 1st-order IIR low-pass filter ($\alpha = 0.20$) filters out high-frequency chassis and motor PWM vibrations.
* **Attitude Kinematics:** Computes true body Pitch and Roll angles in degrees using gravity vector decomposition:
  $$\theta_{\text{pitch}} = \text{atan2}(A_y, \sqrt{A_x^2 + A_z^2}) \cdot \frac{180^\circ}{\pi}$$
  $$\phi_{\text{roll}} = \text{atan2}(-A_x, A_z) \cdot \frac{180^\circ}{\pi}$$
* **Calibrated Coordinate Export:** Exposes calibrated lateral ($A_x$), forward ($A_y$), and vertical ($A_z$) acceleration in units of $g$.

### 2.3 Hump Detection & Peak Measurement State Machine (FR4 / FR5)
* **5-State Topography Machine:**
  ```text
  [FLAT] ──(Pitch > +3.5°)──> [CLIMBING] ──(Pitch enters ±3.5°)──> [CRESTING]
    ▲                                                                 │
    │                                                          (Pitch < -3.5°)
    │                                                                 ▼
    └────────(Pitch enters ±2.0°)──────── [DESCENDING] ◄──────────────┘
  ```
* **Kinematic Height Integration:** While climbing, forward travel increments $\Delta d$ are integrated into vertical rise:
  $$\Delta h = \sum \sin(\theta_{\text{pitch}}) \cdot \Delta d \cdot K_{\text{geom}}$$
  *(Geometric calibration factor $K_{\text{geom}} = 1.15$ accounts for chassis wheelbase ramp entry delay).*
* **Peak Retention Policy:** Only a fully completed climb-crest-descent cycle validates height ($> 5\text{ mm}$). Retains the **greatest valid peak** (`max_hump_peak_m`) across the entire run; incomplete humps or dynamic noise cannot overwrite it.

### 2.4 Emergency Collision Detection (FR11)
* **Threshold:** If total horizontal deceleration $\sqrt{A_x^2 + A_y^2} > 2.5\text{g}$ while flat, the subsystem latches into `TERRAIN_STATE_IMPACT`.
* **Buddy 2 Action:** Immediately calls `s2_stop(S2_EMERGENCY_OBSTACLE)` to trigger an emergency brake on both motors.

### 2.5 micro T-Kernel 3.0 Architecture (`task_terrain.c` / `subsystem4.c`)
* **Task Allocation:** Priority 16, pinned to **Core 1** (`TP_PRC2`), stack size $2048\text{ B}$.
* **Deterministic Period:** Runs every $20\text{ ms}$ ($50\text{ Hz}$) using `tk_dly_tsk(20)`.
* **Thread-Safe Snapshot:** Protected by a Priority-Inheritance Mutex (`TA_TFIFO | TA_INHERIT`) with a finite $5\text{ ms}$ timeout to prevent priority inversion with higher-priority tasks.

### 2.6 Live Telemetry & MQTT Test Bench (`tools/simple_mqtt_server.py`)
* Emits periodic $5\text{ Hz}$ telemetry strings (`[MQTT_XYZ]`).
* Standalone zero-dependency Python MQTT broker & live ASCII terminal dashboard for bench monitoring over USB serial (`--serial COMx`) or WiFi.

---

## 3. Public Interfaces Already Provided by Buddy 4

Any subsystem can include `subsystem4/subsystem4.h` to consume these interfaces:

```c
/* Thread-safe reader for the latest terrain snapshot */
ER s4_get_terrain(TerrainSnapshot *out);
```

### The `TerrainSnapshot` Data Structure:
```c
typedef struct {
    uint32_t     timestamp_ms;        /* Kernel acquisition timestamp */
    bool         terrain_valid;       /* False if I2C bus timed out */
    float        accel_x_g;           /* Calibrated lateral acceleration (g) */
    float        accel_y_g;           /* Calibrated forward acceleration (g) */
    float        accel_z_g;           /* Calibrated vertical acceleration (g) */
    float        pitch_deg;           /* Body pitch angle (+ = nose up) */
    float        roll_deg;            /* Body roll angle (+ = tilt right) */
    TerrainState state;               /* FLAT, CLIMBING, CRESTING, DESCENDING, IMPACT */
    bool         height_valid;        /* True if last hump validated */
    float        last_hump_height_m;  /* Height of most recent completed hump */
    float        max_hump_peak_m;     /* Greatest valid hump peak in this run */
} TerrainSnapshot;
```

---

## 4. What Is Left to Be Integrated with Buddy 2 (Action Items)

The following 4 items require collaboration and code integration with Buddy 2 (Mahesha):

### 1. Dynamic Odometry Distance Fusion ($\Delta d$)
* **Current Status:** Subsystem 4 currently estimates $\Delta d = 0.004\text{ m}$ (assuming constant $0.20\text{ m/s}$ forward cruise speed during each $20\text{ ms}$ tick).
* **The Problem:** If the car slows down, accelerates, or pauses on the ramp, hardcoded $\Delta d$ will cause calculated hump height to drift.
* **Integration Needed from Buddy 2:**
  Buddy 2's wheel encoders measure true incremental travel:
  $$\Delta d = \frac{\Delta d_{\text{left}} + \Delta d_{\text{right}}}{2}$$
* **Action:** Subsystem 4 will provide an odometry ingestion function:
  ```c
  void s4_update_odometry(float delta_dist_m);
  ```
  Buddy 2 (or Mission) will pass the measured wheel distance each cycle. If encoder data is unavailable or zero, Subsystem 4 falls back to the safe nominal default.

---

### 2. Turn-Rate Monitoring (Addressing "No Gyroscope")
* **Requirement:** The course brief specifically mandates:
  > *"Report turn rate and motion events. Label turn-rate estimates as encoder-derived, not raw gyro readings."*
* **The Problem:** The GY-511 sensor has **no gyroscope**. Gyro turn-rate cannot be read from hardware.
* **Integration Needed from Buddy 2:**
  Buddy 2's differential wheel speeds give the true geometric yaw rate:
  $$\omega_{\text{turn}} = \frac{v_{\text{right}} - v_{\text{left}}}{W} \quad (\text{where } W \approx 0.13\text{ m is wheel track width})$$
* **Action:** Subsystem 4 will add `float turn_rate_dps;` and `bool turn_rate_is_estimate;` to `TerrainSnapshot`. Buddy 2 publishes wheel speeds via `M_motion_feedback`, and Subsystem 4 consumes it to report valid turn rates.

---

### 3. Comprehensive Motion Event Classification
* **Requirement:** Classify `Stationary`, `Accelerating`, `Turning`, `Climbing`, `Descending`, and `Impact`.
* **Integration Needed:**
  * When on flat ground (`TERRAIN_STATE_FLAT`), Subsystem 4 will classify:
    * `MOTION_STATIONARY`: when forward acceleration $|A_y| < 0.08\text{g}$ and wheel speed $\approx 0$.
    * `MOTION_ACCELERATING`: when $A_y > +0.15\text{g}$.
    * `MOTION_BRAKING`: when $A_y < -0.15\text{g}$.
    * `MOTION_TURNING`: when turn rate $> 10^\circ/\text{s}$ or centripetal acceleration $|A_x| > 0.15\text{g}$.
* **Action:** Combine IMU $A_x, A_y$ with Buddy 2's motion feedback to publish consistent motion classification states for the Mission controller.

---

### 4. Ramp Speed Advisory / Torque Boost (Optional Cooperative Feature)
* **Opportunity:** When small robotic cars hit a $15^\circ$ steep ramp, static motor PWM duty can cause the car to stall on the incline.
* **Integration Idea:**
  When Subsystem 4 transitions to `TERRAIN_STATE_CLIMBING`, Buddy 2 can read this state from `s4_get_terrain()` and apply an adaptive speed/torque boost ($+15\%$ PWM) to ensure the car powers cleanly over the hump without stalling.

---

## 5. Summary of Proposed Integration Interface

To finalize integration once Buddy 2's encoder driver is ready, Subsystem 4 will expose:

```c
/* -------------------------------------------------------------
 *  Subsystem 4 <-> Subsystem 2 Proposed Integration Contract
 * ------------------------------------------------------------- */

/* Buddy 4 provides to Buddy 2 / Mission: */
ER s4_get_terrain(TerrainSnapshot *out);

/* Buddy 2 provides to Buddy 4 (called each 20 ms motion cycle): */
void s4_update_motion_feedback(float avg_speed_mps, float delta_dist_m, float turn_rate_dps);

/* Buddy 4 calls on Buddy 2 during collision shock: */
ER s2_stop(void);
```

---

## 6. Joint Bench Testing Checklist

When testing together on the physical robot car:

- [ ] **Step 1: Quiescent Test (Stationary):**
  Car resting on flat surface. Confirm pitch $\approx 0.0^\circ$, roll $\approx 0.0^\circ$, motion state = `STATIONARY`.
- [ ] **Step 2: Straight Drive & Odometry:**
  Drive forward at constant speed. Confirm pitch remains $\approx 0.0^\circ$, $\Delta d$ matches wheel revolutions, state = `ACCELERATING` then `FLAT`.
- [ ] **Step 3: Turning Test:**
  Perform in-place or sweeping turn. Confirm `turn_rate_dps` reflects the turn direction and state flags `TURNING`.
- [ ] **Step 4: Hump Roll-Over:**
  Manually guide or drive car over standard obstacle ramp ($6\text{ cm}$). Confirm sequence: `FLAT` $\rightarrow$ `CLIMB` $\rightarrow$ `CREST` $\rightarrow$ `DESCENT` $\rightarrow$ `FLAT`. Verify reported peak is $60\text{ mm} \pm 15\text{ mm}$.
- [ ] **Step 5: Impact Safety Test:**
  Tap front bumper sharply while wheels are turning. Confirm `s2_stop()` is triggered and motors immediately brake.
