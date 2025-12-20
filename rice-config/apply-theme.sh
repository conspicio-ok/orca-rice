#!/bin/bash

CONFIG_DIR="$HOME/.config"
RICE_CONFIG="$CONFIG_DIR/rice-config/config.yaml"

# Fonction pour lire la config YAML (nécessite yq)
get_config() {
    yq eval ".$1" "$RICE_CONFIG"
}

# Fonction pour appliquer le thème
apply_theme() {
    local theme_name=$(get_config "theme.name")
    
    echo "🎨 Application du thème: $theme_name"
    
    # Symlinks pour les thèmes
    ln -sf "$CONFIG_DIR/hypr/themes/${theme_name}.conf" "$CONFIG_DIR/hypr/themes/current.conf"
    ln -sf "$CONFIG_DIR/waybar/themes/${theme_name}.css" "$CONFIG_DIR/waybar/themes/current.css"
    ln -sf "$CONFIG_DIR/rofi/themes/${theme_name}.rasi" "$CONFIG_DIR/rofi/themes/current.rasi"
    
    # Recharger les composants
    pkill waybar && waybar &
    hyprctl reload
    
    echo "✅ Thème appliqué avec succès !"
}

# Fonction pour basculer les widgets
toggle_widget() {
    local widget=$1
    local current_state=$(get_config "widgets.$widget")
    local new_state
    
    if [ "$current_state" = "true" ]; then
        new_state="false"
    else
        new_state="true"
    fi
    
    yq eval ".widgets.$widget = $new_state" -i "$RICE_CONFIG"
    
    echo "🔄 Widget $widget: $new_state"
    reload_waybar
}

# Fonction pour recharger waybar avec la nouvelle config
reload_waybar() {
    python3 "$CONFIG_DIR/rice-config/generate-waybar.py"
    pkill waybar && waybar &
}

# Menu interactif
case "$1" in
    "theme")
        apply_theme
        ;;
    "toggle")
        toggle_widget "$2"
        ;;
    "reload")
        apply_theme
        reload_waybar
        ;;
    "menu")
        # Menu rofi pour les paramètres
        rofi -dmenu -p "Rice Settings" << EOF | while read choice; do
            case "$choice" in
                "Changer thème") "$0" theme-menu ;;
                "Toggle widgets") "$0" widget-menu ;;
                "Recharger config") "$0" reload ;;
            esac
        done
EOF
        ;;
    *)
        echo "Usage: $0 {theme|toggle <widget>|reload|menu}"
        ;;
esac
