# I2C Puppet

## Checkout

    git clone https://github.com/solderparty/i2c_puppet
    cd i2c_puppet
    git submodule update --init
    cd 3rdparty/pico-sdk
    git submodule update --init

## Build

    mkdir build
    cd build
    cmake -DPICO_BOARD=bbq20kbd_breakout -DCMAKE_BUILD_TYPE=Debug ..
    make

## Add rules

    sudo cp tools/99-i2c_puppet.rules /lib/udev/rules.d/
    sudo udevadm control --reload
    sudo udevadm trigger
