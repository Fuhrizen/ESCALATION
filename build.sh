#!/bin/sh
# Native rootless Podman build + extraction. Nothing is mounted from the host.
set -eu
root=$(CDPATH= cd "$(dirname "$0")" && pwd)
engine="${CONTAINER_ENGINE:-podman}"
platform="${1:-linux}"
case "$platform" in
 linux|windows) targets="$platform" ;;
 all) targets="linux windows" ;;
 *) printf 'Usage: %s [linux|windows|all]\n' "$0" >&2; exit 2 ;;
esac
command -v "$engine" >/dev/null 2>&1 || {
 printf '%s is unavailable. On Bazzite, run this script as your normal user with Podman.\n' "$engine" >&2
 exit 127
}
jobs="${JOBS:-2}"
case "$jobs" in ''|*[!0-9]*) printf 'JOBS must be a positive integer.\n' >&2; exit 2 ;; esac
if ! [ "$jobs" -gt 0 ]; then printf 'JOBS must be a positive integer.\n' >&2; exit 2; fi
container=''
staging=''
cleanup() {
 if [ -n "$container" ]; then "$engine" rm "$container" >/dev/null 2>&1 || true; fi
 # Retain an incomplete staging directory for inspection; never remove prior outputs.
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM
for target in $targets; do
 image="localhost/escalation-build:$target"
 printf 'Building %s with %s (jobs=%s)...\n' "$target" "$engine" "$jobs"
 if [ "${CLEAN_BUILD:-0}" = 1 ]; then
  "$engine" build --no-cache --platform linux/amd64 --target "$target-artifacts" \
   --build-arg "JOBS=$jobs" -f "$root/Containerfile" -t "$image" "$root"
 else
  "$engine" build --platform linux/amd64 --target "$target-artifacts" \
   --build-arg "JOBS=$jobs" -f "$root/Containerfile" -t "$image" "$root"
 fi
 container=$("$engine" create --network none "$image")
 staging=$(mktemp -d "$root/.package-$target.XXXXXX")
 "$engine" cp "$container:/artifacts/." "$staging"
 executable='escalation'
 headless='escalation_headless'
 if [ "$target" = windows ]; then executable='escalation.exe'; headless='escalation_headless.exe'; fi
 for item in "$executable" "$headless" data/world.geojson data/histories.json README.md THIRD_PARTY_NOTICES.md; do
  if [ ! -f "$staging/$item" ]; then
   printf 'Incomplete package: %s missing. Previous output has been preserved. Staging: %s\n' "$item" "$staging" >&2
   exit 1
  fi
 done
 [ "$target" != linux ] || chmod u+x "$staging/escalation" "$staging/escalation_headless"
 "$engine" rm "$container" >/dev/null
 container=''
 destination="$root/dist-$target"
 if [ -e "$destination" ]; then
  backup="$root/dist-$target.previous-$(date +%s)-$$"
  mv "$destination" "$backup"
  if ! mv "$staging" "$destination"; then
   mv "$backup" "$destination"
   printf 'Could not publish package; previous output restored.\n' >&2
   exit 1
  fi
  printf 'Previous package retained: %s\n' "$backup"
 else
  mv "$staging" "$destination"
 fi
 staging=''
 printf 'Ready: %s/%s\n' "$destination" "$executable"
done
