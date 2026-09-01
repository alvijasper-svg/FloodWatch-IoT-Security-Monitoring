# Security Notes

This repository intentionally excludes live Wi-Fi credentials and Blynk authentication tokens.

## Local setup

Copy:

`firmware/secrets.example.h` -> `firmware/secrets.h`

Then add the local device values to `secrets.h`. The `.gitignore` prevents that file from being committed.

## Credential rotation

Earlier private development source revisions contained deployment credentials directly in the sketch. If those credentials are still active, rotate the **Blynk device token** and **Wi-Fi password** before publishing any project history or old source file.

## Scope

The anomaly logic is a prototype integrity/reliability feature. A DATA TAMPER result means that a configured rule detected suspicious or inconsistent sensor behaviour; it is not cryptographic proof that an attacker modified the data.
