#!/bin/sh
set -eu
root=$(CDPATH= cd "$(dirname "$0")" && pwd)
cd "$root"
platform="${1:-linux}"
case "$platform" in
 linux|windows) targets="$platform" ;;
 all) targets="linux windows" ;;
 *) printf 'Usage: %s [linux|windows|all]\n' "$0" >&2; exit 2 ;;
esac
for target in $targets; do
 docker compose up --build --exit-code-from "$target" "$target"
 mkdir -p "$root/dist-$target"
 docker compose cp "$target:/out/$target/." "$root/dist-$target"
 printf 'Build and extraction complete: %s/dist-%s\n' "$root" "$target"
done
