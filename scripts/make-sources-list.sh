#!/bin/bash -e

LSB_DISTRIB_CODENAME_FILE="/etc/lsb-release"

if [ ! -e "/etc/lsb-release" ]; then
  echo "Cannot find lsb-release file, are you running ubuntu?"
  exit 1
fi
source /etc/lsb-release

OUTPUT_FILE="/etc/apt/sources.list.d/yandex.list"
TEE_ARGS=

echo "Going to generate ${OUTPUT_FILE} for ${DISTRIB_CODENAME}."

if [ -e "$OUTPUT_FILE" ]; then
  while true; do
    read -p "Existing ${OUTPUT_FILE} found, do you want to View it, Overwrite, Append or Quit? [v/o/a/Q] " ans
    case "$ans" in
      [Vv])
        echo "${OUTPUT_FILE}:"
        cat "${OUTPUT_FILE}"
        ;;
      [Oo])
        break
        ;;
      [Aa])
        TEE_ARGS="${TEE_ARGS} -a"
        break
        ;;
      [Qq]|"")
        exit
        ;;
    esac
  done
fi

echo "Writing ${OUTPUT_FILE} (sudo required)."

sudo tee ${TEE_ARGS} "${OUTPUT_FILE}" >/dev/null <<EOF
deb http://yandex-taxi-${DISTRIB_CODENAME}.dist.yandex.ru/yandex-taxi-${DISTRIB_CODENAME} stable/all/
deb http://yandex-taxi-${DISTRIB_CODENAME}.dist.yandex.ru/yandex-taxi-${DISTRIB_CODENAME} stable/\$(ARCH)/
deb http://yandex-taxi-${DISTRIB_CODENAME}.dist.yandex.ru/yandex-taxi-${DISTRIB_CODENAME} prestable/all/
deb http://yandex-taxi-${DISTRIB_CODENAME}.dist.yandex.ru/yandex-taxi-${DISTRIB_CODENAME} prestable/\$(ARCH)/
deb http://yandex-taxi-${DISTRIB_CODENAME}.dist.yandex.ru/yandex-taxi-${DISTRIB_CODENAME} testing/all/
deb http://yandex-taxi-${DISTRIB_CODENAME}.dist.yandex.ru/yandex-taxi-${DISTRIB_CODENAME} testing/\$(ARCH)/
deb http://yandex-taxi-${DISTRIB_CODENAME}.dist.yandex.ru/yandex-taxi-${DISTRIB_CODENAME} unstable/all/
deb http://yandex-taxi-${DISTRIB_CODENAME}.dist.yandex.ru/yandex-taxi-${DISTRIB_CODENAME} unstable/\$(ARCH)/

deb http://yandex-taxi-common.dist.yandex.ru/yandex-taxi-common stable/all/
deb http://yandex-taxi-common.dist.yandex.ru/yandex-taxi-common stable/\$(ARCH)/
deb http://yandex-taxi-common.dist.yandex.ru/yandex-taxi-common prestable/all/
deb http://yandex-taxi-common.dist.yandex.ru/yandex-taxi-common prestable/\$(ARCH)/
deb http://yandex-taxi-common.dist.yandex.ru/yandex-taxi-common testing/all/
deb http://yandex-taxi-common.dist.yandex.ru/yandex-taxi-common testing/\$(ARCH)/
deb http://yandex-taxi-common.dist.yandex.ru/yandex-taxi-common unstable/all/
deb http://yandex-taxi-common.dist.yandex.ru/yandex-taxi-common unstable/\$(ARCH)/

deb http://yandex-${DISTRIB_CODENAME}.dist.yandex.ru/yandex-${DISTRIB_CODENAME} stable/all/
deb http://yandex-${DISTRIB_CODENAME}.dist.yandex.ru/yandex-${DISTRIB_CODENAME} stable/\$(ARCH)/
deb http://yandex-${DISTRIB_CODENAME}.dist.yandex.ru/yandex-${DISTRIB_CODENAME} prestable/all/
deb http://yandex-${DISTRIB_CODENAME}.dist.yandex.ru/yandex-${DISTRIB_CODENAME} prestable/\$(ARCH)/
#deb http://yandex-${DISTRIB_CODENAME}.dist.yandex.ru/yandex-${DISTRIB_CODENAME} testing/all/
#deb http://yandex-${DISTRIB_CODENAME}.dist.yandex.ru/yandex-${DISTRIB_CODENAME} testing/\$(ARCH)/
#deb http://yandex-${DISTRIB_CODENAME}.dist.yandex.ru/yandex-${DISTRIB_CODENAME} unstable/all/
#deb http://yandex-${DISTRIB_CODENAME}.dist.yandex.ru/yandex-${DISTRIB_CODENAME} unstable/\$(ARCH)/

deb http://common.dist.yandex.ru/common stable/all/
deb http://common.dist.yandex.ru/common stable/\$(ARCH)/
deb http://common.dist.yandex.ru/common prestable/all/
deb http://common.dist.yandex.ru/common prestable/\$(ARCH)/
#deb http://common.dist.yandex.ru/common testing/all/
#deb http://common.dist.yandex.ru/common testing/\$(ARCH)/
#deb http://common.dist.yandex.ru/common unstable/all/
#deb http://common.dist.yandex.ru/common unstable/\$(ARCH)/
EOF

echo "All done."
