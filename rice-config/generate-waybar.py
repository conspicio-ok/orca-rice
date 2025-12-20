#!/usr/bin/env python3
import yaml
import json
import os

def load_rice_config():
    config_path = os.path.expanduser("~/.config/rice-config/config.yaml")
    with open(config_path, 'r') as f:
        return yaml.safe_load(f)

def generate_waybar_config(config):
    waybar_config = {
        "layer": "top",
        "position": config['components']['waybar']['position'],
        "height": config['components']['waybar']['height'],
        "modules-left": [],
        "modules-center": [],
        "modules-right": []
    }
    
    modules = config['components']['waybar']['modules']
    
    # Modules de gauche
    if modules.get('workspaces'):
        waybar_config["modules-left"].append("hyprland/workspaces")
    
    # Modules du centre
    if modules.get('window_title'):
        waybar_config["modules-center"].append("hyprland/window")
    if modules.get('clock'):
        waybar_config["modules-center"].append("clock")
    
    # Modules de droite
    if modules.get('network'):
        waybar_config["modules-right"].append("network")
    if modules.get('volume'):
        waybar_config["modules-right"].append("pulseaudio")
    if modules.get('battery'):
        waybar_config["modules-right"].append("battery")
    if modules.get('tray'):
        waybar_config["modules-right"].append("tray")
    
    # Ajouter les configurations des modules
    waybar_config.update({
        "hyprland/workspaces": {
            "format": "{id}",
            "on-click": "activate"
        },
        "clock": {
            "format": "{:%H:%M}",
            "format-alt": "{:%Y-%m-%d}"
        },
        "battery": {
            "format": "{capacity}% {icon}",
            "format-icons": ["", "", "", "", ""]
        }
        # ... autres modules
    })
    
    return waybar_config

def main():
    config = load_rice_config()
    waybar_config = generate_waybar_config(config)
    
    output_path = os.path.expanduser("~/.config/waybar/config")
    with open(output_path, 'w') as f:
        json.dump(waybar_config, f, indent=2)
    
    print("✅ Configuration waybar générée")

if __name__ == "__main__":
    main()
