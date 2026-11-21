#!/usr/bin/env bash
# Fork-only helper: screenshot nito-qt with every network theme, light and dark.
# Usage: shoot.sh <path to nito-qt> <output folder>
set -u
QT=$1; OUT=$2; HERE=$(cd "$(dirname "$0")" && pwd)
mkdir -p "$OUT"; OUT=$(cd "$OUT" && pwd)
win() { if command -v cygpath >/dev/null 2>&1; then cygpath -w "$1"; else echo "$1"; fi; }
for net in main test signet regtest; do
  for th in light dark; do
    D=$(mktemp -d); tar -xzf "$HERE/regtest.tar.gz" -C "$D"
    rm -f "$OUT/go" "$OUT/done"
    NITO_SCREENSHOT_CHAIN=$net NITO_SCREENSHOT_DIR="$(win "$OUT")" NITO_SCREENSHOT_PREFIX="$net-$th-" \
      "$QT" -datadir="$(win "$D")" -guitheme=$th -splash=0 -resetguisettings -wallet=Main -qtkeepwalletsclosed=0 \
      > "$OUT/$net-$th.log" 2>&1 &
    PID=$!
    for i in $(seq 120); do grep -q "Done loading" "$D/regtest/debug.log" 2>/dev/null && break; sleep 1; done
    sleep 10; touch "$OUT/go"
    for i in $(seq 120); do [ -f "$OUT/done" ] && break; sleep 1; done
    kill $PID 2>/dev/null || true
    command -v taskkill >/dev/null 2>&1 && taskkill //F //IM nito-qt.exe >/dev/null 2>&1
    wait $PID 2>/dev/null
    cp "$D/regtest/debug.log" "$OUT/$net-$th-debug.log" 2>/dev/null || true
    echo "$net $th: $(ls "$OUT" | grep -c "^$net-$th-.*png") screenshots"
  done
done
