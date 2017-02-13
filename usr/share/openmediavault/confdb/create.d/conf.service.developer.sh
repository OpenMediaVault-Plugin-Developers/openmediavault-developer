#!/bin/sh

set -e

. /usr/share/openmediavault/scripts/helper-functions

SERVICE_XPATH_NAME="developer"
SERVICE_XPATH="/config/services/${SERVICE_XPATH_NAME}"

if ! omv_config_exists "${SERVICE_XPATH}"; then
    omv_config_add_node "/config/services" "${SERVICE_XPATH_NAME}"
    omv_config_add_key "${SERVICE_XPATH}" "sharedfolderref" ""
    omv_config_add_key "${SERVICE_XPATH}" "owner" ""
    omv_config_add_key "${SERVICE_XPATH}" "copylocation" ""
    omv_config_add_key "${SERVICE_XPATH}" "gitname" ""
    omv_config_add_key "${SERVICE_XPATH}" "gitemail" ""
    omv_config_add_key "${SERVICE_XPATH}" "txhostname" "https://www.transifex.com"
    omv_config_add_key "${SERVICE_XPATH}" "txpassword" ""
    omv_config_add_key "${SERVICE_XPATH}" "txtoken" ""
    omv_config_add_key "${SERVICE_XPATH}" "txusername" ""
    omv_config_add_key "${SERVICE_XPATH}" "btusername" ""
    omv_config_add_key "${SERVICE_XPATH}" "btapikey" ""
    omv_config_add_key "${SERVICE_XPATH}" "btgpgpass" ""
    omv_config_add_node "${SERVICE_XPATH}" "locations"
fi

exit 0
