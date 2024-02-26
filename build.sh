#!/bin/bash

# Just in case..
rm -rf GoXLRDevice.driver

# Update Submodules..
git submodule update --init --recursive

# Before we do anything, we need to create the 'goxlr-audio-helper' binaries..
cd goxlr-audio-helper || exit
cargo build --release --target=x86_64-apple-darwin
cargo build --release --target=aarch64-apple-darwin

# Create a 'Fat' universal binary..
lipo -create -output ../Resources/goxlr-audio-helper target/x86_64-apple-darwin/release/goxlr-audio-helper target/aarch64-apple-darwin/release/goxlr-audio-helper
cd .. || exit

# Create the Build Directory..
mkdir -p build

# Perform the libASPL BootStrap process..
cd build; cmake -DBOOTSTRAP=ON -B . ..; make; cd ..

# Now Build the Driver..
cd build; cmake -DBOOTSTRAP=OFF -B . ..; make; cd ..

mv build/src/*.driver .
tar zcvf GoXLRDevice.tgz GoXLRDevice.driver

# Remove the Driver
rm -rf GoXLRDevice.driver

