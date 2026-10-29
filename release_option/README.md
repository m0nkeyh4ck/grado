# Option 1: Download the Complete Build Environment from a Release

This option is for users who want the complete `LicheeRV-Nano-Build` directory without manually downloading dependencies or rebuilding the environment.

The complete environment is distributed as one compressed archive split into multiple parts because a single file may exceed the per-file upload limit for GitHub Releases. The parts are not separate archives: they must all be downloaded and joined in the correct order before extraction.

## Files to download

Open the project's **Releases** page and download every file named:

- `LicheeRV-Nano-Build.tar.zst.part-000`
- `LicheeRV-Nano-Build.tar.zst.part-001`
- `LicheeRV-Nano-Build.tar.zst.part-002`
- Continue until the last numbered part in that release.
- `SHA256SUMS`
- `reconstruct.sh`

Download every part from the same release. Do not rename the files, omit a part, or mix files from different releases.

## Requirements

On Ubuntu or Debian, install the required utilities:

```bash
sudo apt update
sudo apt install -y tar zstd coreutils
```

## Step 1: Put the downloaded files in one directory

For example, create a directory in your Downloads folder:

```bash
mkdir -p "$HOME/Downloads/licherv-release"
```

Move all downloaded `.part-*` files, `SHA256SUMS`, and `reconstruct.sh` into that directory. Then enter it:

```bash
cd "$HOME/Downloads/licherv-release"
```

The folder should contain all parts and both support files.

## Step 2: Reconstruct and extract the complete directory

To extract `LicheeRV-Nano-Build/` inside the current download directory, run:

```bash
chmod +x reconstruct.sh
./reconstruct.sh
```

The script will:
1. Check that the part files and checksum file exist.
2. Join all parts into `LicheeRV-Nano-Build.tar.zst`.
3. Verify the reconstructed archive with SHA-256.
4. Stop if verification fails.
5. Extract the archive into the current directory.

After successful extraction, the directory should look like:

```text
$HOME/Downloads/licherv-release/
├── LicheeRV-Nano-Build/
├── LicheeRV-Nano-Build.tar.zst
├── LicheeRV-Nano-Build.tar.zst.part-000
├── LicheeRV-Nano-Build.tar.zst.part-001
├── ... remaining part files ...
├── SHA256SUMS
└── reconstruct.sh
```

The actual number of parts depends on the archive size.

## Step 3: Extract into a different location (optional)

Pass an existing destination directory as the script argument. For example:

```bash
mkdir -p "$HOME/Projects"
./reconstruct.sh "$HOME/Projects"
```

The complete directory will be extracted to:

```text
$HOME/Projects/LicheeRV-Nano-Build
```

The script will not overwrite an existing `LicheeRV-Nano-Build` directory. If that directory already exists, choose another destination or carefully move/rename the existing directory first.

## Troubleshooting

- **Checksum verification fails:** make sure every part was downloaded from the same release and that no part is missing or incomplete. Download the affected files again.
- **No parts found:** run the script from the directory containing all `LicheeRV-Nano-Build.tar.zst.part-*` files.
- **Not enough disk space:** keep enough free space for the downloaded parts, the reconstructed archive, and the extracted directory. The extracted directory may be much larger than the compressed archive.
- **Extraction fails:** ensure `tar` and `zstd` are installed and check that the archive passed SHA-256 verification.

Do not delete the downloaded parts or reconstructed archive until you have confirmed that the extracted environment is complete and works as expected.
