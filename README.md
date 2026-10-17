# fclang / fclang++
> [Clang](https://clang.llvm.org/) wrapper for Kindle C/C++ program creation

fclang / fclang++ is a C++ based CLI wrapper for [Clang](https://clang.llvm.org/) to make jailbroken Kindle program development easier.

## License
fclang / fclang++ is licensed under the [MPLv2](./LICENSE).

## Usage
### Requirements
- An external device with [Clang](https://clang.llvm.org/), (LLD)[https://lld.llvm.org/] and [LLVM](https://llvm.org/) on it.
- A Linux (any), OpenBSD / NetBSD / FreeBSD or MacOS system. (Not Windows)
- [GNU make](https://www.gnu.org/software/make/)
- A Kindle device.
- A USB connector cable.
- A method to connect your kindle as a drive to your device. (Preferably mounted to `/mnt/us/` as shown in the example)
> [!WARNING]
> On Debian / Ubuntu latest and most other systems, fclang / fclang++ will fail to build without the right architectures installed. To fix these issues, try installing the correct packages:
```bash
# For 'old' / 'kindle-old' (Kindle Keyboard, Touch, 4-5)
sudo dpkg --add-architecture armel
sudo apt update
sudo apt install libc6-dev:armel libstdc++-12-dev:armel

# For 'legacy' / 'kindle-legacy' & 'modern' / 'kindle-modern' (Paperwhite 1-2, Voyage, Oasis 1, Paperwhite 4-5, 10-11)
sudo dpkg --add-architecture armel
sudo apt update
sudo apt install libc6-dev:armel libstdc++-12-dev:armel

# For 'pw3' / 'kindle-pw3' (Kindle Paperwhite 3, Oasis 1 (i.MX6))
sudo dpkg --add-architecture armhf
sudo apt update
sudo apt install libc6-dev:armhf libstdc++-12-dev:armhf

# For 'scribe' / 'kindle-scribe' (Kindle Scribe, Colorsoft)
sudo dpkg --add-architecture arm64
sudo apt update
sudo apt install libc6-dev:arm64 libstdc++-12-dev:arm64
```

### Example
```bash
# Write the file
cat << 'EOF' > file.c
#include <stdio.h>
int main(void) { printf("Hello, World!\n"); return 0; }
EOF

# Use fclang to compile your C code to an executable
fclang --target=legacy file.c -o hello

# Transfer the compiled binary to your mounted Kindle device drive
cp hello /mnt/us/hello

# Sync and unmount the Kindle device drive
sync
sudo umount /mnt/us
```

Then unplug your Kindle device, and go to a terminal inside of your Kindle device. (I recommend [kterm](https://github.com/bfabiszewski/kterm)) Then type this to test:

```bash
# Allow write access to the internals
mntroot rw

# Move the hello executable to an executable space
mv /mnt/us/hello /usr/local/bin/hello

# Run the hello executable
source /etc/profile
hello
```

## Supported Kindle devices
fclang / fclang++ supports a lot of targets to make cross-compiling for different Kindle hardware generations effortless:

| Custom alias `--target=...` | Target Kindle device                   | Underlying LLVM triple          |
|-----------------------------|----------------------------------------|---------------------------------|
| `legacy` or `kindle-legacy` | Kindle Paperwhite 1-2, Voyage, Oasis 1 | `armv7-unknown-linux-gnueabi`   |
| `pw3` or `kindle-pw3`       | Kindle Paperwhite 3, Oasis 1 (i.MX6)   | `armv7-unknown-linux-gnueabihf` |
| `modern` or `kindle-modern` | Kindle Paperwhite 4-5, 10-11           | `armv7-unknown-linux-gnueabi`   |
| `scribe` or `kindle-scribe` | Kindle Scribe (2022/2024), Colorsoft   | `aarch64-unknown-linux-gnu`     |
| `old` or `kindle-old`       | Kindle Keyboard, Touch, 4-5            | `arm-unknown-linux-gnueabi`     |

## Building & Installation
Clone or download the repository containing `fclang.cpp` and the `GNUmakefile`, then run:
```bash
# Build both fclang and fclang++ in parallel using GNU make
make -j

# Install system-wide (defaults to /usr/local/bin)
sudo make install
```
To uninstall (remove the binaries from your system):
```bash
sudo make uninstall
```

## Environment variables
- `NO_COLOR`: Fully adheres to the [`NO_COLOR` standard](https://no-color.org). Setting `NO_COLOR` to any value automatically disables colored compiler diagnostics.