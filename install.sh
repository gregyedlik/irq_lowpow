git clone https://github.com/raspberrypi/pico-sdk.git
cp pico-sdk/external/pico_sdk_import.cmake .
mkdir build
cat << EOF > build/gregmake.sh
make
# Check if 'make' was successful
if [ $? -ne 0 ]; then
  echo "Build failed. Exiting."
  exit 1
fi
picotool load test.uf2
picotool reboot
EOF
chmod +x build/gregmake.sh
