set "REPO_URL=https://github.com/userver-framework/userver.git"
set "BRANCH=vMAJOR.MINOR"
set "WORKDIR=%TEMP%\userver-create-service\%BRANCH%"
if not exist "%WORKDIR%\" (
    mkdir "%WORKDIR%"
    git clone -q --depth 1 --branch "%BRANCH%" "%REPO_URL%" "%WORKDIR%"
)
doskey userver-create-service=py -3 "%WORKDIR%\scripts\userver-create-service.py" $*
