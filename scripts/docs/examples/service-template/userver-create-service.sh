userver-create-service() {
    REPO_URL="https://github.com/userver-framework/userver.git"
    BRANCH="develop"
    WORKDIR="/tmp/userver-create-service"
    if [ ! -d "$WORKDIR" ]; then
        mkdir -p "$WORKDIR"
        git clone -q --depth 1 --branch "$REPO_URL" "$BRANCH" "$WORKDIR"
    fi
    "$WORKDIR/scripts/userver-create-service" "$@"
}
