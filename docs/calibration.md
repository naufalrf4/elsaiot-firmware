# ELSA IoT Calibration Guide

## Table of Contents
1. [pH Sensor Calibration](#1-ph-sensor-calibration)
2. [TDS Sensor Calibration](#2-tds-sensor-calibration)
3. [DO Sensor Calibration](#3-do-sensor-calibration)
4. [Calibration Verification](#4-calibration-verification)
5. [Troubleshooting](#5-troubleshooting)

---

## 1. pH Sensor Calibration

### Method: 3-Point Linear Regression
**Equation:**  
`pH = (m × Voltage) + c`

**Requirements:**
- pH 4.01 buffer solution
- pH 7.00 buffer solution
- pH 10.01 buffer solution
- 250ml glass beakers

### Step-by-Step Process:
1. **Prepare Solutions**
   ```
   graph LR
   A[Buffer 4.01] -->|Fresh| B(Beaker 1)
   C[Buffer 7.00] -->|Fresh| D(Beaker 2)
   E[Buffer 10.01] -->|Fresh| F(Beaker 3)
   ```

2. **Web Interface Workflow**
   1. Navigate to `Calibration → pH`
   2. Select "Multi-Point Calibration"
   3. Follow prompts for each buffer solution

3. **Device Interaction**
   ```
   # Example MQTT Payload Sent
   {
     "ph": {
       "m": 1.025,
       "c": -0.12
     }
   }
   ```
   - Stored in NVS as `ph_m` and `ph_c`

### Expected Results:
| Buffer | Voltage Range | Calibrated pH |
|--------|---------------|---------------|
| 4.01   | 3.10-3.30V    | 4.00±0.05     |
| 7.00   | 2.40-2.60V    | 7.00±0.02     |
| 10.01  | 1.90-2.10V    | 10.00±0.05    |

---

## 2. TDS Sensor Calibration

### Method: K-Value Adjustment
**Equation:**  
`TDS = (Raw Value × Temp Compensation) × K`

**Requirements:**
- 342ppm NaCl calibration solution
- Temperature-stable environment (25°C ideal)

### Calibration Process:
1. **Immersion Protocol**
   - Submerge probe 3cm, wait 2 minutes for stabilization
   - Ensure no air bubbles on sensor surface

2. **Digital Calibration**
   ```
   User->Dashboard: Initiate TDS Calibration
   Dashboard->Device: Request raw reading
   Device->Dashboard: Send {"tds": {"raw": 712.3, "voltage": 1.23}}
   User->Dashboard: Enter 342ppm
   Dashboard->Device: Calculate & send K-value
   Device->NVS: Store tds_k_value=0.985
   ```

3. **MQTT Command Example**
   ```
   {
     "tds": {
       "m": 0.985,
       "c": 0.0
     }
   }
   ```

### Validation Criteria:
- 342ppm solution should read 335-349ppm after calibration
- K-value range: 0.500-1.500

---

## 3. DO Sensor Calibration

### Calibration Types:
| Type          | Requirements                | Accuracy |
|---------------|-----------------------------|----------|
| Single-Point  | Air-saturated water         | ±0.5mg/L |
| Two-Point     | 0% solution (Na₂SO₃) + 100% | ±0.2mg/L |

### Detailed Procedure:

#### A. Single-Point Calibration
1. **Preparation**
   - Fill container with water, aerate for 15 minutes
   - Measure temperature (record in dashboard)

2. **Web Interface Steps**
   ```
   graph TB
   Start -->|Select Single-Point| EnterTemp -->|Input 25°C| Confirm -->|Send to Device| Done
   ```

3. **Device Storage**
   ```
   // Stored in NVS
   do_cal1_v = 1.60  // Voltage at 25°C
   do_cal1_t = 25.0
   ```

#### B. Two-Point Calibration
1. **Chemical Preparation**
   - 0% Solution: 5g Na₂SO₃ per 100ml H₂O
   - 100% Solution: Aerate water for 30 minutes

2. **Calibration Sequence**
   ```
   # First point (0%)
   {
     "do": {
       "cal1_v": 0.02,
       "cal1_t": 25.0
     }
   }

   # Second point (100%)
   {
     "do": {
       "cal2_v": 1.60,
       "cal2_t": 25.0
     }
   }
   ```

3. **Validity Check**
   - ΔV between points ≥ 1.40V
   - Temperature difference ≤ 2°C

---

## 4. Calibration Verification

### Post-Calibration Tests
1. **pH Validation**
   - Test with 7.00 buffer: must read 6.95-7.05
   - Slope (m) should be 0.95-1.05

2. **TDS Check**
   - Distilled water: 0-5ppm
   - 500μS/cm → ~250ppm

3. **DO Validation**
   - Aerated tap water: 8-9mg/L at 20°C
   - Zero solution: 0.0-0.3mg/L

---

## 5. Troubleshooting

| Symptom                | Possible Cause            | Solution                   |
|------------------------|---------------------------|----------------------------|
| pH drifts >0.1/day     | Drying electrode bulb     | Soak in 3M KCl overnight    |
| TDS reads 0            | Probe not submerged       | Check immersion depth       |
| DO unstable readings   | Membrane contamination    | Replace DO cap              |
| Calibration not saved  | NVS write failure          | Power cycle & retry         |

**Safety Note:**  
Always wear PPE when handling calibration chemicals. Dispose of solutions according to local regulations.

[← Back to Main Documentation](../README.md)
