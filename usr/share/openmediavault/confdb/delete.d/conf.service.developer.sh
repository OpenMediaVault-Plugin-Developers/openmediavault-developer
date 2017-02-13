#!/bin/sh

set -e

. /usr/share/openmediavault/scripts/helper-functions

if omv_config_exists "/config/services/developer"; then
    omv_config_delete "/config/services/developer"
fi

exit 0
