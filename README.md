# vvector
A simple vector library for C99, targeting Linux.

## Disclaimer
This library was written for fun and as a learning exercise.
I advise against using this in any critical codebases.
Do not trust code written by people you do not know.

## Usage
### First, clone this repo
```git clone https://github.com/lewieW/vvector```

```cd vvector```

### To install the library
```make build && make install #Prompts sudo```

You should now be able to write ```#include <vvector.h>``` in your C files and use the ```-lvvector-0.1.0``` flag to compile.

Alternatively, compile this library into a .o file and use it as you please! See the ```make demo``` command.

## Create the demo
Enter the downloaded vvector directory and execute 
```make demo```

## Uninstall
Enter the downloaded vvector directory and execute
```make uninstall #Prompts sudo```
