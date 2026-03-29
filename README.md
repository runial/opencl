# opencl testing

## Setup (Ubuntu + Intel)

```Bash
sudo apt update
sudo apt install -y opencl-headers ocl-icd-opencl-dev clinfo

wget -O- https://apt.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB \
  | gpg --dearmor | sudo tee /usr/share/keyrings/oneapi-archive-keyring.gpg > /dev/null
echo "deb [signed-by=/usr/share/keyrings/oneapi-archive-keyring.gpg] https://apt.repos.intel.com/oneapi all main" \
  | sudo tee /etc/apt/sources.list.d/oneAPI.list
sudo apt install -y intel-oneapi-runtime-opencl
```

## Format

```Bash
find . -type f \( -name "*.c" -o -name "*.h" \) -exec clang-format -i {} +
```

## Check

```Bash
scan-build cmake --build .
```