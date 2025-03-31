# README for frontend developers who mysteriously got here

## Requirement 📋

The build was tested on Ubuntu 22.04 (and macOS Ventura 13.5 for development purposes)

❗️ Requires doxygen 1.10.0+

## Instruction 🧾

1. install dependencies:
   ```shell
   sudo apt install make graphviz
   ```

2. download doxygen 1.10.0+:
   ```shell
   wget https://github.com/doxygen/doxygen/releases/download/Release_1_10_0/doxygen-1.10.0.linux.bin.tar.gz && tar -xvzf doxygen-1.10.0.linux.bin.tar.gz
   ```

3. run cmake with options:
   ```shell
   -DUSERVER_BUILD_ALL_COMPONENTS=1 -DUSERVER_BUILD_TESTS=1 -DUSERVER_BUILD_SAMPLES=1 -DCMAKE_CXX_STANDARD=20 -DCMAKE_BUILD_TYPE=Debug
   ```

4. perform a full build of userver, run in build dir:
   ```shell
   cmake --build .
   ```

   * on cmake 3.31+, it's enough to run in build dir:
     ```shell
     cmake --build . --target codegen
     ```

5. in project folder run:
   ```shell
   make docs DOXYGEN=/PATH_TO/doxygen-1.10.0/bin/doxygen BUILD_DIR=/absolute/path/to/build_dir
   ```

6. docs will appear in `$BUILD_DIR/docs`, warnings will be printed to stdout

## How to develop? 🛠️

After building the documentation, you can add hard links to the files to be changed. For example, I'm going to change `customdoxygen.css`:

```
frontend@PC:~/Desktop/userver$ rm docs/html/customdoxygen.css
frontend@PC:~/Desktop/userver$ ln scripts/docs/customdoxygen.css docs/html/customdoxygen.css
```

Now it is possible to use tools like [LiveServer](https://marketplace.visualstudio.com/items?itemName=ritwickdey.LiveServer)
