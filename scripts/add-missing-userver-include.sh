if [[ -z "${ENTITY}" ]] || [[ -z "${HEADER}" ]] ; then
    SCRIPT="$0"

    FOLDER='services'
    ENTITY='PeriodicTask'
    HEADER='userver/utils/periodic_task.hpp'

    echo "Error: missing arguments!"
    echo "Example usage:"
    echo "    ENTITY='${ENTITY}' HEADER='${HEADER}' ${SCRIPT}"
    echo "Or:"
    echo "    FOLDER='${FOLDER}' ENTITY='${ENTITY}' HEADER='${HEADER}' ${SCRIPT}"
    exit 1
fi

FOLDER=${FOLDER:-$(pwd)}

echo "Searching for '${ENTITY}' in '${FOLDER}' to add '${HEADER}' header"

USERVER_INCLUDE='#include <userver'
REPLACEMENT="s|${USERVER_INCLUDE}|#include <${HEADER}>\n${USERVER_INCLUDE}|"
FIRST_USERVER_INCLUDE="0,/${USERVER_INCLUDE}/"

grep -lr "${ENTITY}" --include='*pp' ${FOLDER} \
  | xargs grep -L "<${HEADER}>" \
  | xargs sed -i "${FIRST_USERVER_INCLUDE} ${REPLACEMENT}"
