# Upload to GitHub

## Recommended repository name

`FloodWatch-IoT-Security-Monitoring`

## Browser method

1. Sign in to GitHub and choose **New repository**.
2. Name it `FloodWatch-IoT-Security-Monitoring`.
3. Set it **Public** if it is intended as a recruiter portfolio.
4. Do **not** initialize it with a README, `.gitignore`, or license because this package already contains the repository files.
5. Create the repository.
6. Extract `FloodWatch-IoT-GitHub-Ready.zip` on your computer.
7. Open the extracted folder and upload **the contents of the folder**, not the ZIP itself.
8. Commit the files with a message such as `Initial public FloodWatch portfolio release`.
9. Add repository topics such as `arduino`, `iot`, `cybersecurity`, `blynk`, `embedded-systems`, `flood-monitoring`, `sensor-monitoring`.
10. Pin the repository on your GitHub profile.

## Git command method

```bash
git init
git add .
git commit -m "Initial public FloodWatch portfolio release"
git branch -M main
git remote add origin https://github.com/YOUR-USERNAME/FloodWatch-IoT-Security-Monitoring.git
git push -u origin main
```

Before the first push, run the included pre-publish secret check:

```bash
python tools/pre_publish_check.py .
```

## Do not upload

- Wi-Fi passwords;
- Blynk device auth tokens;
- API keys;
- raw credentials or private contact details;
- draft reports that contain obsolete BME280/DHT11 implementation claims;
- screenshots that reveal private account identifiers or tokens.
