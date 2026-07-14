#!/usr/bin/env bash
set -euo pipefail

VERSION="1.0.0"
ZIP_NAME="VocalOne-macOS-${VERSION}.zip"
INSTALLER_DIR="installer"

echo ">> Locating built plugins under build/ ..."
VST3=$(find build -name "*.vst3" -type d | head -1 || true)
AU=$(find build -name "*.component" -type d | head -1 || true)

echo "   VST3 : ${VST3:-<not found>}"
echo "   AU   : ${AU:-<not found>}"

rm -rf "${INSTALLER_DIR}"
mkdir -p "${INSTALLER_DIR}"

[ -n "${VST3}" ] && cp -R "${VST3}" "${INSTALLER_DIR}/"
[ -n "${AU}"   ] && cp -R "${AU}"   "${INSTALLER_DIR}/"

# Scrivi INSTALLA.command - include rimozione quarantena Gatekeeper
INSTALL_CMD="${INSTALLER_DIR}/INSTALLA.command"
: > "${INSTALL_CMD}"

printf '%s\n' '#!/bin/bash'                                                                                       >> "${INSTALL_CMD}"
printf '%s\n' 'DIR="$(cd "$(dirname "$0")" && pwd)"'                                                              >> "${INSTALL_CMD}"
printf '%s\n' 'mkdir -p "$HOME/Library/Audio/Plug-Ins/VST3"'                                                      >> "${INSTALL_CMD}"
printf '%s\n' 'mkdir -p "$HOME/Library/Audio/Plug-Ins/Components"'                                                >> "${INSTALL_CMD}"
printf '%s\n' ''                                                                                                  >> "${INSTALL_CMD}"
printf '%s\n' 'VST3=$(find "$DIR" -maxdepth 1 -name "*.vst3" | head -1)'                                          >> "${INSTALL_CMD}"
printf '%s\n' 'AU=$(find "$DIR" -maxdepth 1 -name "*.component" | head -1)'                                       >> "${INSTALL_CMD}"
printf '%s\n' ''                                                                                                  >> "${INSTALL_CMD}"
printf '%s\n' 'if [ -n "$VST3" ]; then'                                                                           >> "${INSTALL_CMD}"
printf '%s\n' '  cp -R "$VST3" "$HOME/Library/Audio/Plug-Ins/VST3/"'                                             >> "${INSTALL_CMD}"
printf '%s\n' '  xattr -cr "$HOME/Library/Audio/Plug-Ins/VST3/$(basename $VST3)"'                                >> "${INSTALL_CMD}"
printf '%s\n' '  echo "✅ VST3 installato!"'                                                                      >> "${INSTALL_CMD}"
printf '%s\n' 'fi'                                                                                                >> "${INSTALL_CMD}"
printf '%s\n' ''                                                                                                  >> "${INSTALL_CMD}"
printf '%s\n' 'if [ -n "$AU" ]; then'                                                                             >> "${INSTALL_CMD}"
printf '%s\n' '  cp -R "$AU" "$HOME/Library/Audio/Plug-Ins/Components/"'                                         >> "${INSTALL_CMD}"
printf '%s\n' '  xattr -cr "$HOME/Library/Audio/Plug-Ins/Components/$(basename $AU)"'                            >> "${INSTALL_CMD}"
printf '%s\n' '  echo "✅ Audio Unit installato!"'                                                                 >> "${INSTALL_CMD}"
printf '%s\n' 'fi'                                                                                                >> "${INSTALL_CMD}"
printf '%s\n' ''                                                                                                  >> "${INSTALL_CMD}"
printf '%s\n' '# Rimuovi quarantena Gatekeeper anche dai plugin appena copiati'                                   >> "${INSTALL_CMD}"
printf '%s\n' 'sudo xattr -cr /Library/Audio/Plug-Ins/VST3/ 2>/dev/null || true'                                 >> "${INSTALL_CMD}"
printf '%s\n' 'sudo xattr -cr /Library/Audio/Plug-Ins/Components/ 2>/dev/null || true'                           >> "${INSTALL_CMD}"
printf '%s\n' ''                                                                                                  >> "${INSTALL_CMD}"
printf '%s\n' 'echo ""'                                                                                           >> "${INSTALL_CMD}"
printf '%s\n' 'echo "🎤 VocalOne installato con successo!"'                                                       >> "${INSTALL_CMD}"
printf '%s\n' 'echo "Riavvia il tuo DAW (Logic Pro, Ableton, Reaper...)"'                                        >> "${INSTALL_CMD}"
printf '%s\n' 'echo "Premi INVIO per chiudere..."'                                                                >> "${INSTALL_CMD}"
printf '%s\n' 'read'                                                                                              >> "${INSTALL_CMD}"

chmod +x "${INSTALL_CMD}"

rm -f "${ZIP_NAME}"
( cd "${INSTALLER_DIR}" && zip -r "../${ZIP_NAME}" . )

echo "=== ZIP ===" && ls -lh "${ZIP_NAME}"
