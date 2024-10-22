# Yahaha Texture Compression
## Main Features
- Texture Compression

## Setup
### Windows building
For visual studio 2019:
```bash
cmake -S . -B build -G "Visual Studio 16 2019" -A x64
```

### Mac building
For xcode:
```bash
cmake -S . -B build -G Xcode
```

## Optional Flags

- `-mipmap`: 
  - Generates mipmaps for the texture.
- `-background`: 
  - Optimizes the process for background performance by using a maximum of 5 threads or the minimum available CPU cores.
- `-threads N`: 
  - Specifies the exact number of threads (where N is the number) to use for compression.

## Example
```bash
cd bin
YaTCompress astc 6x6 input.png output.astc
YaTCompress bc 7 input.png output.dds
YaTCompress bc 3 input.png output.dds -mipmap
YaTCompress astc 6x6 input.png output.astc -mipmap -background
YaTCompress astc 6x6 input.png output.astc -mipmap -threads 8
```
