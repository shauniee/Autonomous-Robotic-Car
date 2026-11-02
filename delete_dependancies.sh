#!/bin/sh
# Removes what ./install_dependancies.sh added: the tools folder and only the
# Homebrew packages it installed. Packages you already had stay.
#   ./delete_dependancies.sh            asks, then removes
#   ./delete_dependancies.sh --dry-run  only lists what would go
set -eu

tools=${ROBOT_TOOLS:-$HOME/.robot-car-tools}
list=$tools/brew-installed.txt
formulas=$( [ -f "$list" ] && sort -u "$list" | sed '/^$/d' | tr '\n' ' ' || true )

if [ ! -d "$tools" ]; then
    echo "Nothing to remove: $tools does not exist."
    exit 0
fi
echo "Folder:   $tools"
echo "Homebrew: ${formulas:-none}"
[ "${1:-}" != --dry-run ] || exit 0

printf 'Remove these? [y/N] '
read -r answer
[ "$answer" = y ] || [ "$answer" = Y ] || { echo "Nothing removed."; exit 0; }

# shellcheck disable=SC2086 # the list is one formula name per word
[ -z "$formulas" ] || brew uninstall $formulas
rm -rf "$tools"
echo "Removed. Xcode Command Line Tools and Homebrew itself were left in place."
