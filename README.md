## 0. Install npm, zlib, unzip, cmake, gcc, nodejs wget (skip this step if you machine has these libs)

```
sudo apt-get install zlib1g-dev unzip cmake gcc g++ libtinfo5 nodejs wget libncurses5-dev
```

## 1. Install SVF from source
follow the [guide](https://github.com/svf-tools/SVF/wiki/Setup-Guide#getting-started) for using cmake:
```
sudo apt install cmake gcc g++ libtinfo5 libz-dev libzstd-dev zip wget libncurses5-dev ##(If running on Ubuntu 20.04)
                                                                                       ##(had to install libncurses6-dev, I think. Follow the prompts apt gives with its version errors)

git clone https://github.com/SVF-tools/SVF.git
cd SVF
source ./build.sh
```

## 2. Clone repository
```
git clone git@github.com:rekap-ncsu/SVF-taint.git
```

## 3. Setup SVF environment and build your project 
modify [`env.sh`](./env.sh) to point `SVF_DIR` to the SVF root folder

```
source ./env.sh
```
cmake the project (`cmake -DCMAKE_BUILD_TYPE=Debug .` for debug build)
```
cmake . && make
```

The executable will be saved in `$repo/src/`
## 4. Analyze a bc file using svf-ex executable
```
clang -S -c -g -fno-discard-value-names -emit-llvm example.c -o example.ll
./bin/svf-ex example.ll
```
