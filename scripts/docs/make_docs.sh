#!/bin/sh

cd "$(dirname "$0")/../.."

if [ -z "$BUILD_DIR" ]; then
    echo "!!! Set BUILD_DIR environment variable to cmake build directory."
    echo "!!! See userver/scripts/docs/README.md"
    exit 2
fi

if [ ! -f "$BUILD_DIR/compile_commands.json" ]; then
    echo "!!! Fully build userver before running this script."
    echo "!!! See userver/scripts/docs/README.md"
    exit 2
fi

download_and_extract_doxygen() {
    if [ ! -d "$BUILD_DIR/doxygen-1.13.2" ]; then
        echo "Doxygen not found in $BUILD_DIR or version is too low. Downloading..."
        wget --no-verbose https://github.com/doxygen/doxygen/releases/download/Release_1_13_2/doxygen-1.13.2.linux.bin.tar.gz -P "$BUILD_DIR"
        tar -xzf "$BUILD_DIR/doxygen-1.13.2.linux.bin.tar.gz" -C "$BUILD_DIR"
        export DOXYGEN="$BUILD_DIR/doxygen-1.13.2/bin/doxygen"
        echo "Doxygen has been successfully downloaded."
    else
        echo "Using already downloaded doxygen in $BUILD_DIR."
        export DOXYGEN="$BUILD_DIR/doxygen-1.13.2/bin/doxygen"
    fi
}

# Find doxygen, download doxygen if needed.
DOXYGEN="${DOXYGEN:-doxygen}"

if ! $DOXYGEN --version >/dev/null 2>&1; then
    download_and_extract_doxygen
fi

DOXYGEN_VERSION_MIN="1.10.0"
DOXYGEN_VERSION_CUR=$($DOXYGEN --version | awk -F " " '{print $1}')

if ! printf "%s\n%s\n" "$DOXYGEN_VERSION_MIN" "$DOXYGEN_VERSION_CUR" | sort -C; then
    download_and_extract_doxygen
fi

# Run userver codegen to avoid doxygen errors with invalid includes.
CMAKE_COMMAND=$(grep -oP 'CMAKE_COMMAND:INTERNAL=\K.*' "$BUILD_DIR/CMakeCache.txt")
CMAKE_VERSION=$("$CMAKE_COMMAND" --version | grep -oP '\d+\.\d+')

echo "Building target userver-codegen."
"$CMAKE_COMMAND" --build "$BUILD_DIR" --target userver-codegen

echo "Building target userver-gen-dynamic-configs-docs."
"$CMAKE_COMMAND" --build "$BUILD_DIR" --target userver-gen-dynamic-configs-docs
rm -f scripts/docs/en/dynamic_configs
ln -s $BUILD_DIR/docs-dynamic-configs scripts/docs/en/dynamic_configs

# Run doxygen.
rm -rf "$BUILD_DIR/docs" || :

# curl, jq presence check
if ! command -v curl >/dev/null 2>&1; then
    echo "Error: curl not installed. To install try: sudo apt install curl"
    exit 1
fi
if ! command -v jq >/dev/null 2>&1; then
    echo "Error: jq not installed. To install try: sudo apt install jq"
    exit 1
fi


# Sorting function
semver_compare() {
    ver1=$(echo "$1" | sed 's/^[vV]//')
    ver2=$(echo "$2" | sed 's/^[vV]//')

    IFS=. read -r a1 a2 a3 <<EOF
$1
EOF
    IFS=. read -r b1 b2 b3 <<EOF
$2
EOF

    a1=${a1:-0}; a2=${a2:-0}; a3=${a3:-0}
    b1=${b1:-0}; b2=${b2:-0}; b3=${b3:-0}

    if [ "$a1" -lt "$b1" ]; then echo -1; return; fi
    if [ "$a1" -gt "$b1" ]; then echo 1; return; fi

    if [ "$a2" -lt "$b2" ]; then echo -1; return; fi
    if [ "$a2" -gt "$b2" ]; then echo 1; return; fi

    if [ "$a3" -lt "$b3" ]; then echo -1; return; fi
    if [ "$a3" -gt "$b3" ]; then echo 1; return; fi

    echo 0
}


# Fetch versions
versions=$(curl -s -L -H "Accept: application/vnd.github+json" https://api.github.com/repos/userver-framework/docs/contents/ | jq -r '.[] | select(.type == "dir") | .name')
versionjs_path="scripts/docs/scripts/versions.js"

# Data check
if [ -z "$versions" ]; then
    echo "Error: versions not found in repository userver-framework/docs."
    echo "export const versions = [];" > $versionjs_path
    exit 1
fi

# Sorting versions
versions_sorted=$(printf "%s" "$versions" | awk '
{
  for(i=1;i<=NF;i++){
    orig = $i
    token = orig
    sub(/^[vV]/, "", token)  
    split(token, p, ".")
    major = (p[1] == "" ? 0 : p[1] + 0)
    minor = (p[2] == "" ? 0 : p[2] + 0)
    patch = (p[3] == "" ? 0 : p[3] + 0)
    key = sprintf("%08d.%08d.%08d", major, minor, patch)
    count[key]++
    arr[key, count[key]] = orig
  }
}
END {
  n = asorti(count, keys) 
  for(i=1;i<=n;i++){
    k = keys[i]
    for(j=1;j<=count[k];j++){
      printf "%s", arr[k, j]
      if (!(i==n && j==count[k])) printf " "
    }
  }
}')

# Writing versions to versions.js
printf "%s" "$versions_sorted" | jq -R -s -c 'split(" ") | map(select(length > 0))' | \
awk '{print "export const versions = " $0 ";"}' > "$versionjs_path"

echo "versions.js created successfully"


DOXYFILE_OVERRIDES="${DOXYFILE_OVERRIDES:-}"

echo "Running doxygen..."
(
    cat scripts/docs/doxygen.conf;
    echo "$DOXYFILE_OVERRIDES";
) | $DOXYGEN - 2>&1 | python3 scripts/docs/clean_doxygen_logs.py | tee "$BUILD_DIR/doxygen.err.log"
echo "A copy of doxygen logs is in: $BUILD_DIR/doxygen.err.log"

# Apply minor clean-ups to doxygen output.
if [ -z "$NO_DEFAULT_HTML_CLEANUP" ]; then
    echo "userver.tech" > "$BUILD_DIR/docs/html/CNAME"
    cp "$BUILD_DIR/docs/html/d8/dee/md_en_2userver_2404.html" "$BUILD_DIR/docs/html/404.html" || :
    sed -i 's|\.\./\.\./|/|g' "$BUILD_DIR/docs/html/404.html"
fi

# Add versions page
DOCS_DIR="$BUILD_DIR/docs/html"
INDEX_FILE="$DOCS_DIR/index.html"
OUT_FILE="$DOCS_DIR/versions.html"
NEW_TITLE="userver Versions"
TAB_TITLE="userver: Versions"

VERSIONS="$versions_sorted"

VERSIONS_HTML="<ul>"
for ver in $VERSIONS; do
    VERSIONS_HTML="$VERSIONS_HTML
  <li><a href=\"docs/$ver/index.html\">$ver</a></li>"
done
VERSIONS_HTML="$VERSIONS_HTML
</ul></div>"

awk -v new_title="$NEW_TITLE" -v tab_title="$TAB_TITLE" '
/<div class="contents">/ { exit }
{
  line=$0
  if (index(line, "<title>")) {
    start=index(line, "<title>")
    before=substr(line, 1, start + length("<title>") - 1)
    after=substr(line, index(line, "</title>"))
    line=before tab_title after
  }

  pos=index(line, "<div class=\"title\">")
  if (pos) {
    before = substr(line, 1, pos-1)
    rest   = substr(line, pos + length("<div class=\"title\">"))
    endpos = index(rest, "</div>")
    if (endpos) {
      after = substr(rest, endpos)
      line  = before "<div class=\"title\">" new_title after
    }
  }
  print line
}
' "$INDEX_FILE" > "$OUT_FILE"

# Add versions
echo "$VERSIONS_HTML" >> "$OUT_FILE"

# Add footer
awk 'f {print} /<\/div><!-- contents -->/ {f=1}' "$INDEX_FILE" >> "$OUT_FILE"

echo "$OUT_FILE created successfully"