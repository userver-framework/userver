# README for frontend developers who mysteriously got here

## Requirement 📋

The build was tested on Ubuntu 22.04 (and macOS Ventura 13.5 for development purposes)

❗️ Requires doxygen 1.10.0+

## Instruction 🧾

1. install dependencies:
   ```shell
   sudo apt install make graphviz
   ```

2. run cmake with options:
   ```shell
   -DUSERVER_BUILD_ALL_COMPONENTS=1 \
   -DUSERVER_BUILD_TESTS=1 \
   -DUSERVER_BUILD_SAMPLES=1 \
   -DCMAKE_CXX_STANDARD=20 \
   -DUSERVER_DEBUG_INFO_COMPRESSION=z \
   -DCMAKE_BUILD_TYPE=Debug \
   -DCMAKE_C_COMPILER=... \
   -DCMAKE_CXX_COMPILER=...
   ```
   (you might want to add some options depending on your environment)

3. in userver folder run:
   ```shell
   BUILD_DIR=/absolute/path/to/build_dir ./scripts/docs/make_docs.sh
   BUILD_DIR=/absolute/path/to/build_dir ./scripts/docs/upload_docs.sh
   ```
   or
   ```shell
   BUILD_DIR=/absolute/path/to/build_dir ../scripts/userver/docs/make_docs.sh
   BUILD_DIR=/absolute/path/to/build_dir ../scripts/userver/docs/upload_docs.sh
   ```

4. docs will appear in `$BUILD_DIR/docs`, warnings will be printed to `$BUILD_DIR/doxygen.err.log`

## How to develop? 🛠️

After building the documentation, you can add hard links to the files to be changed. For example, I'm going to change `customdoxygen.css`:

```
frontend@PC:~/Desktop/userver$ rm docs/html/customdoxygen.css
frontend@PC:~/Desktop/userver$ ln scripts/docs/customdoxygen.css docs/html/customdoxygen.css
```

Now it is possible to use tools like [LiveServer](https://marketplace.visualstudio.com/items?itemName=ritwickdey.LiveServer)
