git clone https://github.com/raspberrypi/pico-sdk.git
git clone https://github.com/raspberrypi/pico-extras.git
cd pico-sdk
git submodule update --init --recursive
cd ..
cp pico-sdk/external/pico_sdk_import.cmake .
cp pico-extras/external/pico_extras_import.cmake .
mkdir build
cat << EOF > build/gregmake.sh
make
picotool load test.uf2
picotool reboot
EOF
chmod +x build/gregmake.sh
