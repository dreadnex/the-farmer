#!/bin/bash

# Check if the script is being run as root
if [ "$(id -u)" -ne 0 ]; then
    echo "This script needs to be run as root."
    exit 1
fi

# Install libssl and libnotify development packages
echo "Installing libssl and libnotify development packages..."
apt-get update
apt-get install -y libssl-dev libnotify-dev
if [ $? -ne 0 ]; then
    echo "Failed to install required development packages."
    exit 1
fi

# Compile the code
gcc -o thefarmer thefarmer.c -lcrypto `pkg-config --cflags --libs libnotify`
if [ $? -ne 0 ]; then
    echo "Compilation failed."
    exit 1
fi

# Move the compiled program to a system directory
mv thefarmer /usr/local/bin/

# Set up an alias for the user
echo "alias thefarmer='/usr/local/bin/thefarmer'" >> ~/.bashrc
source ~/.bashrc

echo "Setup completed successfully. You can now use 'thefarmer' command."
