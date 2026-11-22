#!/usr/bin/env bash
# Compares the wgpu* symbols two wgpu-native static libraries export: the source build (with webgpu-sdk's patches)
# and the upstream release of the same tag. The patches change behaviour, not the C API, so the sets must match.
# usage: compare_wgpu_symbols.sh <built library> <upstream library>
set -euo pipefail

nm_tool=nm
if command -v llvm-nm > /dev/null; then
	nm_tool=llvm-nm
fi

exports() {
	# Defined, external code and data symbols whose name starts with wgpu (an underscore precedes it on macOS).
	"$nm_tool" --defined-only --extern-only "$1" 2> /dev/null |
		awk 'NF >= 3 && $2 ~ /^[TDRB]$/ { print $3 }' |
		sed 's/^_//' | grep -E '^wgpu' | sort -u
}

built=$(exports "$1")
upstream=$(exports "$2")
echo "built: $(echo "$built" | grep -c .) wgpu symbols; upstream: $(echo "$upstream" | grep -c .)"
if [ -z "$upstream" ]; then
	echo "no wgpu symbols found in $2" >&2
	exit 1
fi
if ! diff <(echo "$upstream") <(echo "$built"); then
	echo "the source build exports different wgpu symbols from the upstream release (< upstream, > built)" >&2
	exit 1
fi
echo "exported wgpu symbols match"
