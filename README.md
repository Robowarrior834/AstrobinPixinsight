# AstroBin CSV Generator for PixInsight

> **Note:** This project was developed with the assistance of opencode.

A native PixInsight GUI script for generating AstroBin acquisition CSV files directly from FITS and XISF light frames.

## Features

- Reads FITS and XISF headers
- Extracts exposure, gain, filter, temperature, date, telescope, site and camera metadata
- Groups frames by session, filter, gain, exposure and binning
- Handles overnight imaging sessions
- Skips master frames automatically
- Generates AstroBin Bulk Import CSV files
- Saves settings between sessions

## Installation (Recommended - Update Repository)

The easiest way to install and keep the script updated is using PixInsight's built-in update repository system:

1. In PixInsight, go to **Resources → Updates → Manage Repositories**
2. Click **Add** and enter this URL:
   ```
   https://robowarrior834.github.io/AstrobinPixinsight/updates/
   ```
3. Click **OK** to close the dialog
4. Go to **Resources → Updates → Check for Updates**
5. Select **AstroBin CSV Generator** and click **Apply**
6. Restart PixInsight when prompted

The script will appear under **Scripts → Utilities → AstroBin CSV Generator**.

To update: Simply check for updates again — PixInsight will automatically download and install new versions.

## Installation (Manual) - Depercated

1. Extract `AstroBin.7z`.
2. Copy the `astrobin` folder into PixInsight's `src/scripts` folder.
3. Open **Scripts → Feature Scripts**.
4. Click **Add** and select the folder.
5. Run **Scripts → Utilities → AstroBin CSV Generator**.

## Signing the Script (Maintainers)

PixInsight requires scripts and update repositories to be signed so users can trust and install them. Signing uses a secure keys file (`.xssk`) that must be kept private and is excluded from git. The developer ID used by this project is `JamiesAstroPhotos`.

1. **Generate signing keys (one-time)** — In PixInsight, run **Scripts → Development → Signing Keys**. Check **Generate Signing Keys**, select **CPD** (or **Local Signing Identity**), choose an output path and a strong password, then run it. This creates your `.xssk` keys file (e.g. `Jamiesastrophotos_new.xssk`). Do **not** commit this file to the repository.

2. **Sign the script** — Run **Scripts → Development → Code Sign**. Add `AstroBinCSVGenerator.js` to the list, select your `.xssk` keys file, enter its password and run. This generates `AstroBinCSVGenerator.xsgn` in the same folder. Re-sign every time the script changes.

3. **Build the update package** — Run the build script:
   ```
   .\build-package.ps1
   ```
   This zips the script and its `.xsgn` signature into `updates\`, and updates the `sha1` and `releaseDate` attributes in `updates\updates.xri`.

4. **Re-sign `updates\updates.xri`** — Step 3 invalidates the repository signature, so run **Scripts → Development → Code Sign** again, add `updates\updates.xri`, select your `.xssk` keys file, enter the password and run. This appends a new `<Signature>` element to the XRI document.

5. **Commit and push** — Commit the `updates\` directory (the `.zip`, `updates.xri` and the root `AstroBinCSVGenerator.xsgn`) and push to GitHub. Users will then see the update when they check for updates.

## Contributing

This script is intended for educational purposes in the field of astrophotography. It is part of an open-source project and contributions or suggestions for improvements are welcome.

To contribute to this project, follow these steps:

1. Fork this repository.
2. Create a branch: `git checkout -b <branch_name>`.
3. Make your changes and commit them: `git commit -m '<commit_message>'`.
4. Push to the original branch: `git push origin <project_name>/<location>`.
5. Create the pull request.

Alternatively, see the GitHub documentation on [creating a pull request](https://docs.github.com/en/github/collaborating-with-issues-and-pull-requests/creating-a-pull-request).

## License

This project uses the following licence: [GNU General Public Licence v3.0](https://github.com/SteveGreaves/AstroBinUploader/blob/main/LICENSE).
