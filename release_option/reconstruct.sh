#!/usr/bin/env bash
set -Eeuo pipefail
shopt -s nullglob

ARCHIVE="LicheeRV-Nano-Build.tar.zst"
CHECKSUM_FILE="SHA256SUMS"
DESTINATION="${1:-.}"

if [[ ! -d "$DESTINATION" ]]; then
    echo "ERROR: Destination directory does not exist: $DESTINATION" >&2
    echo "Create it first, or run this script without an argument to extract here." >&2
    exit 1
fi

parts=(LicheeRV-Nano-Build.tar.zst.part-*)

if (( ${#parts[@]} == 0 )); then
    echo "ERROR: No LicheeRV-Nano-Build.tar.zst.part-* files were found." >&2
    echo "Download every part from the same GitHub Release and place them here." >&2
    exit 1
fi

if [[ ! -f "$CHECKSUM_FILE" ]]; then
    echo "ERROR: $CHECKSUM_FILE is missing. Download it from the same release." >&2
    exit 1
fi

if [[ -e "$ARCHIVE" ]]; then
    echo "ERROR: $ARCHIVE already exists in the current directory." >&2
    echo "Move or remove it only if you are sure it is safe to do so, then retry." >&2
    exit 1
fi

if [[ -e "$DESTINATION/LicheeRV-Nano-Build" ]]; then
    echo "ERROR: $DESTINATION/LicheeRV-Nano-Build already exists." >&2
    echo "To avoid overwriting files, choose another destination or move the existing directory." >&2
    exit 1
fi

echo "Found ${#parts[@]} part file(s)."
echo "Joining the parts..."
cat "${parts[@]}" > "$ARCHIVE"

echo "Verifying SHA-256 checksum..."
if ! sha256sum -c "$CHECKSUM_FILE"; then
    echo "ERROR: Checksum verification failed. The archive will not be extracted." >&2
    echo "Check that all parts are present, complete, and from the same release." >&2
    exit 1
fi

echo "Extracting into: $DESTINATION"
tar --zstd -xf "$ARCHIVE" -C "$DESTINATION"

if [[ ! -d "$DESTINATION/LicheeRV-Nano-Build" ]]; then
    echo "ERROR: Extraction finished, but the expected LicheeRV-Nano-Build directory was not found." >&2
    echo "Check the archive's internal directory structure." >&2
    exit 1
fi

echo
echo "Success! The complete environment is available at:"
echo "$DESTINATION/LicheeRV-Nano-Build"
echo "The verified archive and downloaded parts have been kept."
