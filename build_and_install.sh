#!/bin/bash

# Update Submodules..
git submodule update --init --recursive

# Create the Build Directory..
mkdir -p build

# Perform the libASPL BootStrap process..
cd build; cmake -DBOOTSTRAP=ON -B . ..; make; cd ..

# Now Build the Driver..
cd build; cmake -DBOOTSTRAP=OFF -B . ..; make; cd ..

mv -r build/src/*.driver .

