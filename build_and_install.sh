#!/bin/bash

#rm -r build/
cmake . -B build/
cd  build
make
sudo cp -r src/*.driver /Library/Audio/Plug-Ins/HAL/
sudo launchctl kickstart -k system/com.apple.audio.coreaudiod 
cd ..


