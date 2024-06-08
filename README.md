# redstone


## Building

```fish
git clone https://github.com/rondubi/redstone.git --recursive
cd redstone
mkdir build
cmake -B build .
cd build
make
```

## Running on a Program

Redstone accepts simulation configuration files that specify things like fault
likelihoods and program paths. For examples of use, see the demo_*.toml files in the
root directory.

```fish
./build/redstone demo_ff.toml
```