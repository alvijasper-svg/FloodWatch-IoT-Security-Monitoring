# Hardware and Wiring

## Final documented hardware

| Component | Arduino connection | Purpose |
|---|---|---|
| Analog water-level sensor | A0, 5 V, GND | Raw water-contact reading and flood classification |
| LM393 rain module / plate | A1, 5 V, GND | Rain surface wetness and mapped rain intensity |
| DHT22 / AM2302 | D2, 5 V, GND | Temperature and relative humidity |
| Status LED | D3 through 220 Ω resistor, GND | Visual alert patterns |
| Active buzzer | D8, GND | Audible alert patterns |
| 16×2 LCD with I²C backpack | SDA, SCL, 5 V, GND | Local readings, mode and diagnostic information |
| Arduino UNO R4 WiFi | onboard wireless interface | Wi-Fi/Blynk communication |

## Water-level calibration

The final documented classification boundaries are evaluated from the **raw ADC reading**:

| Raw reading | State |
|---:|---|
| 0–199 | SAFE / no flood |
| 200–389 | LOW FLOOD |
| 390–499 | MEDIUM FLOOD |
| 500+ | CRITICAL FLOOD |

Representative piecewise calibration points used by the firmware:

| Raw | Displayed prototype depth |
|---:|---:|
| 5 | 0.00 m |
| 315 | 0.40 m |
| 430 | 0.60 m |
| 470 | 0.80 m |
| 515 | 1.00 m |

The metre value is a project-defined scaled representation for the prototype. It should not be interpreted as a field-certified hydrological gauge.
