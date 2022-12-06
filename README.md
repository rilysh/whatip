## whatip
Retrives information about an IPv4 address

## Build
```sh
git clone https://github.com/rilysh/whatip
cd whatip && make
```

## Usage
```
whatip

Usage:
whatip -ip [IPv4 address]
whatip -c -ip [IPv4 address]
```

## Note
I've removed Windows Network API support from "http.h" header file. To get them back, use [this](https://github.com/markusfisch/libhttp) header instead.