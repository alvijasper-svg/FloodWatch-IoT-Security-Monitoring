# Security and Reliability Design

FloodWatch intentionally uses **lightweight, deterministic controls** rather than machine learning or a separate SOC/security platform.

## 1. Silent / complete-device failure visibility

The cloud connection lifecycle is used to distinguish a working controller from a device that has stopped communicating. During retained testing, the project recorded **38 genuine lifecycle transitions: 19 ONLINE and 19 OFFLINE**, representing 19 operating sessions.

This is useful because a flood monitor that simply stops transmitting should not continue to appear healthy.

## 2. Sensor-health separation

Repeated invalid DHT22 reads are treated as a **sensor issue**, not automatically as malicious tampering. The diagnostic path exposes:

- V6 = Sensor Issue YES/NO
- V10 = Sensor Issue Reason
- System Mode = SENSOR ISSUE when the fault is active

The documented logic uses an invalid-read counter and retains the last valid temperature/humidity values rather than overwriting them with an invalid sample.

## 3. Rule-based anomaly / tamper detection

The prototype includes simple consistency checks rather than claiming cryptographic protection of the sensor bus. Examples include:

- a latched **sudden water jump above 350 raw units**;
- configured startup inconsistencies between water and rain readings;
- a diagnostic reason exposed through V9.

These checks can identify suspicious or implausible behaviour, but they do **not prove an attack** and do not authenticate the physical sensor source.

## 4. Transition-aware state handling

The system maintains an explicit mode and reacts when that mode changes. This reduces repetitive alert behaviour compared with blindly re-triggering the same notification on every sensor loop.

## Important limitations

This is a prototype reliability/security layer, not an intrusion-prevention system. It does not provide:

- cryptographic sensor authentication;
- secure-element credential storage;
- protection against physical manipulation of analog lines;
- resistance to crafted values that remain within expected ranges;
- production-grade availability guarantees.

Those limitations are deliberately documented so the repository does not overstate the cybersecurity contribution.
