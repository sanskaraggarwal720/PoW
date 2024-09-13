#!/bin/bash

# Function to check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Ensure the compiled binaries exist
if [ ! -f firewall/main ]; then
    echo "Error: Firewall binary not found. Please compile first."
    exit 1
fi

if [ ! -f algos/c/main ]; then
    echo "Error: C Analyzer binary not found. Please compile first."
    exit 1
fi

if [ ! -f algos/rust/target/release/DDoS-Algos ]; then
    echo "Error: Rust Analyzer binary not found. Please build first."
    exit 1
fi

open_terminal() {
    osascript -e "tell application \"Terminal\" to do script \"$1\""
}

current_dir=$(pwd)

# Run the Firewall
if command_exists gnome-terminal; then
    gnome-terminal --title="Firewall" -- ./firewall/main
elif command_exists xterm; then
    open_terminal "/$current_dir/firewall/main"
else
    echo "No suitable terminal emulator found. Please open terminals manually and run ./firewall/firewall."
fi

# Run the C Analyzer
if command_exists gnome-terminal; then
    gnome-terminal --title="C Analyzer" -- ./algos/c/main
elif command_exists xterm; then
open_terminal "/$current_dir/algos/c/main"
else
    echo "No suitable terminal emulator found. Please open terminals manually and run ./algos/c/algoc."
fi

# Run the Rust Analyzer
if command_exists gnome-terminal; then
    gnome-terminal --title="Rust Analyzer" -- ./algos/rust/target/release/DDoS-Algos
elif command_exists xterm; then
    open_terminal "/$current_dir/algos/rust/target/release/DDoS-Algos"
else
    echo "No suitable terminal emulator found. Please open terminals manually and run ./algos/rust/target/release/rust_analyzer."
fi
