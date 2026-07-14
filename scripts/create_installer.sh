#!/usr/bin/env bash
#
# create_installer.sh — Package the built VocalOne plugins into a macOS
# installer zip. Called from the GitHub Actions workflow so that no heredoc
# is required inside the YAML `run:` blocks (which broke YAML parsing).
#
# It:
#   1. Locates the built VST3 and AU (.component) bundles under build/
#   2. Creates the installer/ directory and copies the plugins into it
#   3. Writes installer/INSTALLA.command (built with printf, no heredoc)
#   4. Produces VocalOne-macOS-1.0.0.zip in the repo root
#
set -euo pipefail

VERSION="1.0.0"
ZIP_NAME="VocalOne-macOS-${VERSION}.zip"
INSTALLER_DIR="installer"

echo ">> Locating built plugins under build/ ..."
VST3=$(find build -name "*.vst3" -type d | head -1 || true)
AU=$(find build -name "*.component" -type d | head -1 || true)

echo "   VST3 : ${VST3:-<not found>}"
echo "   AU   : ${AU:-<not found>}"

# ---------------------------------------------------------------------------
# Prepare installer directory
# ---------------------------------------------------------------------------
rm -rf "${INSTALLER_DIR}"
mkdir -p "${INSTALLER_DIR}"

if [ -n "${VST3}" ]; then
    cp -R "${VST3}" "${INSTALLER_DIR}/"
    echo "   Copied VST3 into ${INSTALLER_DIR}/"
fi

if [ -n "${AU}" ]; then
    cp -R "${AU}" "${INSTALLER_DIR}/"
    echo "   Copied AU into ${INSTALLER_DIR}/"
fi

# ---------------------------------------------------------------------------
# Write installer/INSTALLA.command (no heredoc — one printf per line)
# ---------------------------------------------------------------------------
INSTALL_CMD="${INSTALLER_DIR}/INSTALLA.command"
: > "${INSTALL_CMD}"

printf '%s\n' '#!/bin/bash'                                                                                 >> "${INSTALL_CMD}"
printf '%s\n' 'DIR="$(cd "$(dirname "$0")" && pwd)"'                                                        >> "${INSTALL_CMD}"
printf '%s\n' 'mkdir -p "$HOME/Library/Audio/Plug-Ins/VST3"'                                                >> "${INSTALL_CMD}"
printf '%s\n' 'mkdir -p "$HOME/Library/Audio/Plug-Ins/Components"'                                          >> "${INSTALL_CMD}"
printf '%s\n' ''                                                                                            >> "${INSTALL_CMD}"
printf '%s\n' 'VST3=$(find "$DIR" -maxdepth 1 -name "*.vst3" | head -1)'                                    >> "${INSTALL_CMD}"
printf '%s\n' 'AU=$(find "$DIR" -maxdepth 1 -name "*.component" | head -1)'                                 >> "${INSTALL_CMD}"
printf '%s\n' ''                                                                                            >> "${INSTALL_CMD}"
printf '%s\n' '[ -n "$VST3" ] && cp -R "$VST3" "$HOME/Library/Audio/Plug-Ins/VST3/" && echo "VST3 installato!"'          >> "${INSTALL_CMD}"
printf '%s\n' '[ -n "$AU" ] && cp -R "$AU" "$HOME/Library/Audio/Plug-Ins/Components/" && echo "Audio Unit installato!"'  >> "${INSTALL_CMD}"
printf '%s\n' ''                                                                                            >> "${INSTALL_CMD}"
printf '%s\n' 'echo ""'                                                                                     >> "${INSTALL_CMD}"
printf '%s\n' 'echo "VocalOne installato! Riavvia il tuo DAW."'                                             >> "${INSTALL_CMD}"
printf '%s\n' 'echo "Premi INVIO per chiudere..."'                                                          >> "${INSTALL_CMD}"
printf '%s\n' 'read'                                                                                        >> "${INSTALL_CMD}"

chmod +x "${INSTALL_CMD}"
echo ">> Wrote ${INSTALL_CMD}"

# ---------------------------------------------------------------------------
# Create the zip in the repo root
# ---------------------------------------------------------------------------
rm -f "${ZIP_NAME}"
( cd "${INSTALLER_DIR}" && zip -r "../${ZIP_NAME}" . )

echo "=== ZIP ==="
ls -lh "${ZIP_NAME}"
