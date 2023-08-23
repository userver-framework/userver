# README for frontend developers who mysteriously got here

## Requirement 📋

The build was tested on Ubuntu 22.04 (and Mac OS Ventura 13.5 for development purposes)

❗️ Requires doxygen 1.9.7+

## Instruction 🧾

1. install dependencies: `sudo apt install make graphviz`
2. install doxygen 1.9.7+: `wget https://www.doxygen.nl/files/doxygen-1.9.7.linux.bin.tar.gz && tar -xvzf doxygen-1.9.7.linux.bin.tar.gz && cd doxygen-1.9.7/ && sudo make install`
3. in project folder run: `make docs`

P.S. Do not be afraid of the huge number of errors at the beginning of the build 🙃

## How to develop? 🛠️

After building the documentation, you can add hard links to the files to be changed. For example, I'm going to change `customdoxygen.css`:

```
frontend@PC:~/Desktop/userver$ rm docs/html/customdoxygen.css
frontend@PC:~/Desktop/userver$ ln scripts/docs/customdoxygen.css docs/html/customdoxygen.css
```

Now it is possible to use tools like [LiveServer](https://marketplace.visualstudio.com/items?itemName=ritwickdey.LiveServer)
