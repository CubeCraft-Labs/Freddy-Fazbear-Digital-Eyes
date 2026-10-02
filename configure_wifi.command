#!/bin/zsh
# Double-click in Finder to configure this project's Wi-Fi without sharing a password in chat.
cd -- "${0:A:h}" || exit 1
python3 firmware/tools/configure_ota.py
result=$?
echo
if (( result == 0 )); then
  echo "Wi-Fi configuration saved. You may close this window."
else
  echo "Configuration was not saved. You may close this window and retry."
fi
read -k 1 "?Press any key to close..."
exit $result
