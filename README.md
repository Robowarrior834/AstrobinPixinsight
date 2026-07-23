# AstroBin CSV Generator for PixInsight

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

## Installation (Manual)

1. Extract `AstroBin.7z`.
2. Copy the `astrobin` folder into PixInsight's `src/scripts` folder.
3. Open **Scripts → Feature Scripts**.
4. Click **Add** and select the folder.
5. Run **Scripts → Utilities → AstroBin CSV Generator**.

## Contributing

This script is intended for educational purposes in the field of astrophotography. It is part of an open-source project and contributions or suggestions for improvements are welcome.

To contribute to this project, follow these steps:

1. Fork this repository.
2. Create a branch: `git checkout -b <branch_name>`.
3. Make your changes and commit them: `git commit -m '<commit_message>'`.
4. Push to the original branch: `git push origin <project_name>/<location>`.
5. Create the pull request.

Alternatively, see the GitHub documentation on [creating a pull request](https://docs.github.com/en/github/collaborating-with-issues-and-pull-requests/creating-a-pull-request).

## Contact

If you want to contact me, you can reach me at sgreaves139@gmail.com.

## License

This project uses the following licence: [GNU General Public Licence v3.0](https://github.com/SteveGreaves/AstroBinUploader/blob/main/LICENSE).
