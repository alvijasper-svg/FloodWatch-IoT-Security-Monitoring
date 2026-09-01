# Data Provenance

The project documentation includes a **1,000-record structured V0-V10 analysis dataset**. It is important to describe that dataset accurately.

Complete high-resolution historical logging for every Blynk sensor datastream was **not enabled during the original demonstration**. The 1,000-record structured dataset was later reconstructed from the final calibration logic, recorded test behaviour, retained screenshots and session evidence so that charts and state coverage could be analysed consistently.

Therefore:

- do not describe the 1,000 rows as a raw field-telemetry export;
- keep genuine Blynk ONLINE/OFFLINE lifecycle records distinct from reconstructed analysis records;
- use the representative CSV in this repository only as an example of the dataset schema;
- for future testing, enable high-resolution or averaged history for V0-V10 before running the experiment.

This distinction makes the repository more defensible in an interview or academic review.
