# Testing and Results

## Verified project evidence

The retained final documentation reports:

- **14 system functions** evaluated across sensing, alerting, diagnostics and recovery;
- **11 Blynk datastreams (V0-V10)**;
- **15-sample median filtering** for water-level stability;
- approximately **2-second cloud telemetry**;
- **38 genuine ONLINE/OFFLINE lifecycle events** across **19 sessions**;
- one verified university recognition: **Technical Excellence Award at the Technovation 2.0 Showcase**.

## Water-level calibration examples

| Raw / range | Scaled depth | Final state | Interpretation |
|---:|---:|---|---|
| 5 | 0.00 m | SAFE | Dry reference |
| 0–199 | 0.00–≈0.25 m | SAFE | Below low-flood boundary |
| 200 | ≈0.25 m | LOW FLOOD begins | First warning boundary |
| 315 | 0.40 m | LOW FLOOD | Calibration reference |
| 390 | ≈0.53 m | MEDIUM FLOOD begins | Refined practical boundary |
| 420 | ≈0.58 m | MEDIUM FLOOD | Typical stabilized reading after an upward spike |
| 470 | 0.80 m | MEDIUM FLOOD | High-contact reference |
| 500 | ≈0.93 m | CRITICAL FLOOD begins | Critical boundary |
| 515 | 1.00 m | CRITICAL FLOOD | Full-scale prototype reference |

## DHT22 test observations

Normal temperature/humidity acquisition and stable repeated reads passed within the prototype scope. The invalid-read counter and last-valid-value handling were verified by logic inspection. The final report notes that complete timestamped DHT22 failure/recovery export evidence was limited, so those fault-evidence cases should not be overstated.

## Lifecycle statistics

The 19 retained operating sessions had:

- median duration: **7 min 58 s**;
- mean duration: **15 min 2 s**;
- shortest: **1 min 33 s**;
- longest: **1 h 10 min 39 s**.

These values describe test activity and repeated reconnections; they are **not** a long-term reliability or uptime rate.


## Physical implementation evidence

Photographs of the final prototype, installed sensors, live university demonstration and showcase recognition are available in [`../evidence/`](../evidence/README.md). These photographs support the physical implementation claim but should not be interpreted as raw telemetry logs.
