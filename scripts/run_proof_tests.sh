#!/bin/sh

failed=0

discover_packages() {
  roots="$*"
  discovered_packages=

  for root in $roots; do
    for manifest in "$root"/*/package.edn; do
      [ -f "$manifest" ] || continue

      package=${manifest%/package.edn}
      if [ -d "$package/test" ] && grep -q ':config .*proof' "$manifest"; then
        discovered_packages="$discovered_packages $package"
      fi
    done
  done

  echo "$discovered_packages"
}

if [ "$1" = "--root" ]; then
  shift
  packages=$(discover_packages "$@")
  set -- $packages
elif [ "$#" -eq 0 ]; then
  packages=$(discover_packages lib examples)
  set -- $packages
fi

if [ "$#" -eq 0 ]; then
  echo "No proof test packages found." >&2
  exit 2
fi

for package in "$@"; do
  echo "==> $package"

  output=$(cd "$package" && lisple proof)
  status=$?
  printf '%s\n' "$output"

  if [ "$status" -ne 0 ]; then
    failed=1
    continue
  fi

  if ! printf '%s\n' "$output" |
    grep -Eq '^proof: [0-9]+ passed, 0 failed, [0-9]+ total$'; then
    failed=1
  fi
done

exit "$failed"
