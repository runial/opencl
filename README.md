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

## Build

```bash
# One-shot build (Ubuntu). Set REPO_URL to your fork.
REPO_URL=https://github.com/you/opencl.git \
REPO_DIR=${REPO_DIR:-$PWD/opencl}

sudo apt update
sudo apt install -y build-essential cmake git opencl-headers ocl-icd-opencl-dev clinfo

# Optional: Intel OpenCL runtime (skip by exporting INSTALL_INTEL_RUNTIME=0)
if [[ ${INSTALL_INTEL_RUNTIME:-1} == 1 ]]; then
  wget -qO- https://apt.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB \
    | gpg --dearmor | sudo tee /usr/share/keyrings/oneapi-archive-keyring.gpg >/dev/null
  echo "deb [signed-by=/usr/share/keyrings/oneapi-archive-keyring.gpg] https://apt.repos.intel.com/oneapi all main" \
    | sudo tee /etc/apt/sources.list.d/oneAPI.list >/dev/null
  sudo apt update
  sudo apt install -y intel-oneapi-runtime-opencl
fi

if [[ ! -d "$REPO_DIR/.git" ]]; then
  git clone "$REPO_URL" "$REPO_DIR"
else
  git -C "$REPO_DIR" pull --ff-only
fi

cmake -S "$REPO_DIR" -B "$REPO_DIR/build" -DCMAKE_BUILD_TYPE=${BUILD_TYPE:-Release}
cmake --build "$REPO_DIR/build" --parallel "$(nproc)"

# Optional: run the binary after build
if [[ ${RUN_BINARY:-0} == 1 ]]; then
  "$REPO_DIR/build/opencl_test"
fi
```

## Format

```Bash
find . -type f \( -name "*.c" -o -name "*.h" \) -exec clang-format -i {} +
```