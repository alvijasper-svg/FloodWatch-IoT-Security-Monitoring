# Blynk Datastream Mapping

The final system uses **11 virtual datastreams (V0-V10)**.

| Pin | Field | Meaning |
|---|---|---|
| V0 | Water Level | Scaled prototype depth in metres |
| V1 | Rain Intensity | Mapped rain-plate intensity (%) |
| V2 | Temperature | DHT22 temperature (°C) |
| V3 | Humidity | DHT22 relative humidity (%) |
| V4 | System Mode | SAFE, rain, flood, DATA TAMPER or SENSOR ISSUE state |
| V5 | Tampering | YES/NO diagnostic flag |
| V6 | Sensor Issue | YES/NO diagnostic flag |
| V7 | Water Raw | Original water ADC reading |
| V8 | Rain Raw | Original rain ADC reading |
| V9 | Tamper Reason | Explanation for the integrity flag |
| V10 | Sensor Issue Reason | Explanation for sensor-health fault |

Telemetry was documented at approximately **two-second intervals** for the cloud update path.

## Recommended dashboard layout

A useful portfolio/demo dashboard should show both interpreted and raw data:

- Water Level + Water Raw
- Rain Intensity + Rain Raw
- Temperature + Humidity
- System Mode
- Tampering + Tamper Reason
- Sensor Issue + Sensor Issue Reason
- Device ONLINE/OFFLINE state

This makes the final decision traceable instead of showing only a high-level warning.
