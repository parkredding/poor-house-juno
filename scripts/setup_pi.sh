#!/bin/bash
# Setup script for Raspberry Pi environment

set -e

echo "========================================="
echo "Poor House Juno - Raspberry Pi Setup"
echo "========================================="
echo ""
echo "This script will install dependencies on Raspberry Pi"
echo ""

# Update package list
echo "Updating package list..."
sudo apt-get update

# Install build dependencies
echo "Installing build dependencies..."
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    libasound2-dev \
    pkg-config

# Optional: Install JACK for pro audio
read -p "Install JACK audio server? (y/n) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    sudo apt-get install -y jackd2 qjackctl
fi

echo ""
echo "========================================="
echo "Setup complete!"
echo "========================================="
echo ""
echo "You can now build Poor House Juno:"
echo "  ./scripts/build_pi.sh"
