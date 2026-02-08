# Project Context

## Virtual Environment
Path: `/mnt/raid0/Code/venvs/.astrovenv`

## Usage
Always use the Python executable from the virtual environment:
`/mnt/raid0/Code/venvs/.astrovenv/bin/python3`

# Workspace Standards

- **Detailed Docstrings:** Every function, class, and module must include comprehensive docstrings and comments.
- **Inline Comments:** Logic must be accompanied by inline comments that explain the 'why' behind the implementation.

## Synchronization & Safety (RAID 0 Protection)
- **Change Detection:** Before concluding any task or before I exit the session, check `git status` for uncommitted changes.
- **Proactive Prompt:** If changes are detected, ask: "I see changes in [Folder Name]. Since we are on RAID 0, would you like me to sync these to GitHub for you?"
- **Sync Logic:** If I say "yes," perform:
    1. `git add .`
    2. Generate a concise, descriptive commit message based on the code edits.
    3. `git push origin main`
    4. **Confirmation:** Report back once the push is successful.

## Golden Tests
Always run these four tests after any code changes to ensure stability and verify against reference files in `golden_tests/references/`.

1. **Michael Test:**
   ```bash
   /mnt/raid0/Code/venvs/.astrovenv/bin/python3 AstroBinUpload.py "/home/steve/Downloads/Jason Astrobin Data" --test "Modified_headers_Michael.csv"
   ```
2. **Flame Test:**
   ```bash
   /mnt/raid0/Code/venvs/.astrovenv/bin/python3 AstroBinUpload.py "/home/steve/Downloads/Jason Astrobin Data" --test "flame_modified.csv"
   ```
3. **Alpha Test:**
   ```bash
   /mnt/raid0/Code/venvs/.astrovenv/bin/python3 AstroBinUpload.py "/home/steve/Downloads/Jason Astrobin Data" --test "flame_modified_Alpha_Zhang.csv"
   ```
4. **LBN 548 Test (Directory Scan):**
   ```bash
   /mnt/raid0/Code/venvs/.astrovenv/bin/python3 AstroBinUpload.py "/home/steve/Desktop/Pixinsight/LBN 548" "/mnt/raid0/AstroImaging/Preselected/Calibration data/31st May 2025"
   ```

**Verification:** Compare the generated `.csv` and `.txt` files in the respective `AstroBinUploadInfo` directories with the baseline references in `golden_tests/references/`.
