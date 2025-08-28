#!/bin/bash
# Config launcher basé sur rofi + yq
# Dépendances : rofi, yq, alacritty, nvim (remplace si besoin)

CONFIGS="$HOME/.config/myconfigs.yaml"

# Vérifie l'existence du fichier YAML
if [ ! -f "$CONFIGS" ]; then
    notify-send "Config launcher" "Le fichier $CONFIGS est introuvable"
    exit 1
fi

# Liste les noms disponibles
CHOICE=$(yq '.[].name' $CONFIGS | rofi --dmenu --prompt "Edit config:")

# Quitte si rien n'a été choisi
[ -z "$CHOICE" ] && exit 0

# Récupère le chemin correspondant
FILE=$(yq ".[] | select(.name == \"$CHOICE\") | .path" "$CONFIGS")

# Expansion du tilde (~)
FILE=$(eval echo "$FILE")

# Ouvre avec ton éditeur favori
alacritty -e vim "$FILE"

