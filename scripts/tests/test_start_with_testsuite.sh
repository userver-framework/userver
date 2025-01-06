#!/bin/bash

###
### Makes sure that 'make start-*' or 'ninja start-*' works
### Accepts a BUILD_SYSTEM parameter.
###

# Exit on any error and treat unset variables as errors, print all commands
set -euox pipefail

if [ -z "$BUILD_SYSTEM" ]; then
    echo "Provide a BUILD_SYSTEM parameter"
    exit 1
fi

${BUILD_SYSTEM} start-userver-samples-postgres_service &
PG_SERVICE_PID=$!
${BUILD_SYSTEM} start-userver-samples-hello_service &
HELLO_SERVICE_PID=$!

# Disable exiting on error
set +e

while ! curl --max-time 1 'http://localhost:8080/hello'; do
    echo "Failed to ping hello service"
    sleep 1
done
echo "SUCCESS to ping hello service"

while RESPONSE_CODE=$(curl -o /dev/null -s -w "%{http_code}\n" -X POST 'http://localhost:8087/v1/key-value?key=new'); \
    [[ "$RESPONSE_CODE" != "201" && "$RESPONSE_CODE" != "200" ]]
do
    echo "Failed to curl postgres service, HTTP code is $RESPONSE_CODE"
    sleep 1
done
echo "SUCCESS to curl postgres service"

kill -9 $PG_SERVICE_PID $HELLO_SERVICE_PID
wait $PG_SERVICE_PID $HELLO_SERVICE_PID || : # OK if jobs finish faster or with error
