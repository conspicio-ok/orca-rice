#!/bin/bash

set -e

bluetoothctl power on &>/dev/null

while true; do
    bluetooth_enabled=$(bluetoothctl show | grep "Powered: yes")

    # Menu principal
    choice=$(printf "󰂯 Activer/Désactiver Bluetooth\n Scanner et connecter\n Quitter" | wofi --dmenu --no-markup -p "Bluetooth")

    case "$choice" in
        "󰂯 Activer/Désactiver Bluetooth")
            if [ -n "$bluetooth_enabled" ]; then
                bluetoothctl power off
                notify-send "Bluetooth désactivé"
            else
                bluetoothctl power on
                notify-send "Bluetooth activé"
            fi
            ;;

        " Scanner et connecter")
            bluetoothctl power on &>/dev/null
            bluetoothctl scan on &>/dev/null
            sleep 3
            bluetoothctl scan off &>/dev/null

            mapfile -t devices < <(bluetoothctl devices | awk '{$1=""; print substr($0,2)}')
            if [ ${#devices[@]} -eq 0 ]; then
                notify-send "Bluetooth" "Aucun périphérique trouvé"
            else
                selection=$(printf '%s\n' "${devices[@]}" | wofi --dmenu --no-markup -p "Choisir un appareil")
                [ -z "$selection" ] && continue
                mac=$(bluetoothctl devices | grep "$selection" | awk '{print $2}')
                bluetoothctl connect "$mac" > /dev/null && notify-send "Bluetooth" "Connecté à $selection"
            fi
            ;;

        " Quitter")
            exit 0
            ;;
    esac
done

