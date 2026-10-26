#!/bin/bash
# Prints the functions and the disassembly around the addresses listed in a
# file (one or more per line, '#' starts a comment) as GitHub notices.
elf="$1"; list="$2"
bin="$DEVKITPPC/bin"
addrs=$(grep -v '^#' "$list" | tr -s ' ,\n' ' ')

notice() { # title, text
	printf '::notice title=%s::%s\n' "$1" "$(printf '%s' "$2" | sed -e 's/%/%25/g' -e 's/\r//g' | awk '{printf "%s%%0A", $0}')"
}

notice "Crash addresses" "$("$bin/powerpc-eabi-addr2line" -f -C -i -p -e "$elf" $addrs 2>&1)"

dis=""
for a in $addrs; do
	s=$(printf '0x%x' $(( a - 0x40 )))
	e=$(printf '0x%x' $(( a + 0x10 )))
	dis+="== $a"$'\n'
	dis+=$("$bin/powerpc-eabi-objdump" -d -C --no-show-raw-insn --start-address="$s" --stop-address="$e" "$elf" | grep -E '^ *8|>:')$'\n'
done
notice "Disassembly" "$dis"
