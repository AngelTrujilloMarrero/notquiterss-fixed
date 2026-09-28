#!/bin/bash
# Lanzador de NotQuiteRSS con auto-actualización desde GitHub.
#
# Al abrir el programa:
#   1. Muestra "Comprobando actualizaciones…".
#   2. Si tu fork tiene cambios nuevos, los descarga, compila
#      y arranca el programa ya actualizado (reinicio con lo nuevo).
#   3. Si no hay cambios, arranca directamente.
# Sin red o sin cambios: abre la versión instalada sin esperas extra.
#
# La instalación es local (~/.local, sin sudo) para que la
# actualización funcione desatendida.

set -u

REPO="$HOME/notquiterss-fixed"
BRANCH="feature/i18n-es"   # TODO: cambiar a "master" cuando se fusione la rama
BUILD_DIR="$HOME/notquiterss-build-local"
PREFIX="$HOME/.local"
BIN="$PREFIX/bin/notquiterss"
FALLBACK_BIN="/usr/bin/notquiterss"
STAMP="$BUILD_DIR/.built-commit"
LOG="$HOME/.cache/NotQuiteRSS/update.log"

export MINIAUDIO_INCLUDE_DIR="$HOME/miniaudio-snapshot/include"
export QMAKEFEATURES="$HOME/app-deps-qt5/install/features"

say() {
  notify-send "NotQuiteRSS" "$1" 2>/dev/null || true
  echo "$1" >>"$LOG"
}

progress_start() {
  # Diálogo "trabajando…" que se cierra solo al terminar la compilación.
  zenity --progress --pulsate --no-cancel \
    --title="NotQuiteRSS" --text="$1" 2>/dev/null &
  PROGRESS_PID=$!
}

progress_stop() {
  kill "$PROGRESS_PID" 2>/dev/null || true
  wait "$PROGRESS_PID" 2>/dev/null || true
}

CHECK_ONLY=0
if [ "${1:-}" = "--check-only" ]; then
  CHECK_ONLY=1
  shift
fi

mkdir -p "$(dirname "$LOG")" "$BUILD_DIR"
echo "=== $(date -Is) ===" >>"$LOG"

launch() {
  if [ "$CHECK_ONLY" = "1" ]; then
    echo "CHECK-ONLY: no se abre el programa"
    return 0
  fi
  if [ -x "$BIN" ]; then
    exec "$BIN" "$@"
  elif [ -x "$FALLBACK_BIN" ]; then
    exec "$FALLBACK_BIN" "$@"
  else
    zenity --error --title="NotQuiteRSS" \
      --text="No hay ningún ejecutable instalado." 2>/dev/null || true
    exit 1
  fi
}

if [ ! -d "$REPO/.git" ]; then
  say "Sin repo local; abriendo versión instalada."
  launch "$@"
fi

cd "$REPO" || launch "$@"

say "Comprobando actualizaciones…"

# Traer lo nuevo sin tocar el árbol todavía. Sin red: se abre lo instalado.
if ! git fetch -q origin "$BRANCH" >>"$LOG" 2>&1; then
  say "Sin conexión; abriendo versión instalada."
  launch "$@"
fi

LOCAL="$(git rev-parse HEAD)"
REMOTE="$(git rev-parse "origin/$BRANCH")"

if [ "$REMOTE" != "$LOCAL" ]; then
  say "Actualización encontrada. Descargando y compilando…"
  if ! git pull -q --ff-only origin "$BRANCH" >>"$LOG" 2>&1; then
    say "No se pudo descargar; abriendo versión instalada."
    launch "$@"
  fi
  # Si el propio lanzador cambió en GitHub, re-ejecutarse con lo nuevo
  # (solo una vez por arranque para evitar bucles).
  if [ -z "${_NQR_UPDATED:-}" ]; then
    export _NQR_UPDATED=1
    if [ "$CHECK_ONLY" = "1" ]; then
      exec "$REPO/scripts/launch-with-update.sh" --check-only "$@"
    else
      exec "$REPO/scripts/launch-with-update.sh" "$@"
    fi
  fi
  LOCAL="$(git rev-parse HEAD)"
fi

BUILT="none"
[ -f "$STAMP" ] && BUILT="$(cat "$STAMP")"

if [ ! -x "$BIN" ] || [ "$BUILT" != "$LOCAL" ]; then
  say "Compilando nueva versión…"
  progress_start "Compilando la nueva versión de NotQuiteRSS…"
  BUILD_OK=1
  {
    cd "$BUILD_DIR" || exit 1
    qmake "$REPO/app.pro" CONFIG+=release CONFIG-=debug_and_release \
      "PREFIX=$PREFIX" || exit 1
    make -j"$(nproc)" || exit 1
    make install || exit 1
  } >>"$LOG" 2>&1 || BUILD_OK=0
  progress_stop
  if [ "$BUILD_OK" = "1" ]; then
    echo "$LOCAL" >"$STAMP"
    say "Programa actualizado. Abriendo nueva versión."
  else
    say "Falló la compilación (ver $LOG); abriendo versión anterior."
    zenity --error --title="NotQuiteRSS" \
      --text="Falló la compilación. Se abre la versión anterior.\nDetalle en $LOG" \
      2>/dev/null || true
  fi
fi

launch "$@"
