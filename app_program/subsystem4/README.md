# Subsystem 4 — Terrain & IMU Monitoring (Buddy 4)

**Owner:** Darrius (2500360)  
**RTOS Task:** Priority 16, Core 1 (`TP_PRC2`), 50 Hz (20 ms period)  
**Hardware:** GY-511 (ST LSM303DLHC, Grove 3: GP4 SDA, GP5 SCL)  

---

## File Directory

* **`subsystem4.h` / `subsystem4.c`**: Subsystem lifecycle (`init`, `start`), priority-inheritance mutex, and public snapshot reader (`s4_get_terrain`).
* **`task_terrain.h` / `task_terrain.c`**: micro T-Kernel 50 Hz periodic task, calibration settling guard, collision emergency stop trigger (`s2_stop`), and telemetry emission.
* **`terrain_algo.h` / `terrain_algo.c`**: 1st-order IIR filter ($\alpha=0.20$), trigonometric pitch & roll math, 5-state hump machine, kinematic height integration ($\sum \sin\theta \cdot \Delta d$), and impact detection.
* **`drv_gy511.h` / `drv_gy511.c`**: Low-level I2C0 driver for LSM303DLHC with 5 ms non-blocking bus timeouts.
* **`subsystem4_test.c`**: On-target automated bench self-tests (`make TEST=1`).

---

## Integration Guide

For the full architectural specification and integration roadmap with **Buddy 2 (Motion / Encoders)**, see:
[`../../docs/BUDDY4_INTEGRATION_GUIDE.md`](../../docs/BUDDY4_INTEGRATION_GUIDE.md).
