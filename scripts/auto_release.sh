#!/usr/bin/env zsh
set -euo pipefail

# Auto build + tag + push / release helper for Matrix_16x10
# Usage:
# 1) Trigger CI release (push tag to origin to run your GitHub Actions workflow):
#    ./scripts/auto_release.sh --tag v1.2.3 --mode ci
# 2) Create GitHub release directly (requires `gh` CLI authenticated):
#    ./scripts/auto_release.sh --tag v1.2.3 --mode direct
# 3) If you don't provide --tag, a timestamp tag will be generated (vYYYYMMDD-HHMMSS)

PIO=${PLATFORMIO:-"$HOME/.platformio/penv/bin/platformio"}
ENV=${ENVIRONMENT:-esp32dev}
MODE="ci"
TAG=""

print_usage() {
  echo "Usage: $0 --tag <tag> [--mode ci|direct]"
  echo "  --tag    Tag name to create (e.g. v1.0.0). If omitted, a timestamp tag will be generated."
  echo "  --mode   'ci' to push tag and trigger workflow (default). 'direct' to create GH release with artifact (requires gh)."
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --tag)
      TAG="$2"; shift 2;;
    --mode)
      MODE="$2"; shift 2;;
    -h|--help)
      print_usage; exit 0;;
    *) echo "Unknown arg: $1"; print_usage; exit 1;;
  esac
done

if [[ -z "$TAG" ]]; then
  TAG="v$(date +%Y%m%d-%H%M%S)"
fi

echo "[release] Using tag: $TAG"

echo "[release] Building with PlatformIO ($PIO) environment=$ENV..."
"$PIO" run --environment "$ENV"

BIN_PATH=".pio/build/${ENV}/firmware.bin"
if [[ ! -f "$BIN_PATH" ]]; then
  echo "[release] ERROR: firmware not found at $BIN_PATH" >&2
  exit 2
fi

OUT_BIN="firmware_${TAG}.bin"
cp "$BIN_PATH" "$OUT_BIN"
echo "[release] Copied firmware to $OUT_BIN"

if [[ "$MODE" == "ci" ]]; then
  echo "[release] Creating and pushing tag to origin to trigger GitHub Actions..."
  git fetch origin
  git tag -a "$TAG" -m "Release $TAG"
  git push origin "$TAG"
  echo "[release] Tag pushed. Watch Actions -> My Release workflow for the build/release." 
  exit 0
fi

if [[ "$MODE" == "direct" ]]; then
  if ! command -v gh >/dev/null 2>&1; then
    echo "[release] ERROR: gh CLI not found. Install and authenticate: https://cli.github.com/" >&2
    exit 3
  fi

  echo "[release] Creating GitHub release $TAG and uploading artifact $OUT_BIN"
  gh release create "$TAG" "$OUT_BIN" --title "$TAG" --notes "Automated release $TAG"
  echo "[release] Release created. URL:" 
  gh release view "$TAG" --repo $(git remote get-url origin) --json url -q .url
  exit 0
fi

echo "[release] Unknown mode: $MODE" >&2
exit 4
