# ansible — orce-rice

## Prérequis

- Arch installé, démarrant, avec un réseau fonctionnel
- L'utilisateur cible existe et dispose de `sudo`
- `ansible` et `git` installés

## Usage

Pour cacher le dossier de conf :
```bash
git clone git@github.com:conspicio-ok/orca-rice.git ~/.ansible
cd .ansible
```

Sur desktop :
```bash
ansible-playbook -i inventory.ini public.yml -K --limit desktop
```
Sur laptop :
```bash
ansible-playbook -i inventory.ini public.yml -K --limit laptop
```

### Options interactives (`public.yml`)

Chaque groupe est proposé au lancement, défaut `no` :

| Prompt | Contenu |
|---|---|
| `install_dev` | neovim, tree-sitter-cli, plugins et parsers treesitter |
| `install_android_dev` | `android-tools`, Android Studio (AUR) |
| `install_games` | Steam, Lutris, Wine, gamemode, libs 32 bits — active `multilib` |
| `install_virt` | virt-manager, QEMU, dnsmasq — ajoute l'utilisateur à `libvirt` |
| `install_creative` | Blender, Krita, CUDA |
| `install_ts_web` | parsers treesitter svelte, css, js, ts (nécessite `install_dev`) |
| `install_ts_go` | parser treesitter Go (nécessite `install_dev`) |

## Structure

```
inventory.ini
public.yml
group_vars/
  all.yml
host_vars/
  desktop.yml
  laptop.yml
dotfiles/
  sway/                    config sway partagée + config.d/local (généré, ignoré)
  waybar/                  config waybar
  foot/                    config foot
  zsh/                     zshrc — symlinké vers ~/.zshrc
roles/
  yay packages dotfiles services shell neovim
```

## Inventaire et variables par machine

| Variable | Rôle |
|---|---|
| `pacman_packages_host` | paquets pacman propres à la machine |
| `aur_packages_host` | paquets AUR propres à la machine |
| `sway_keyboard_layout` | `us` (défaut, QWERTY) ou `fr` |
| `sway_touchpad` | booléen — rend un bloc `input type:touchpad` |
| `sway_output` | ligne `output …` brute ; absente, sway auto-détecte |
| `sway_key_repeat_delay` / `sway_key_repeat_rate` | défauts 300 / 50 |

## Dotfiles

Une config réelle préexistante est déplacée dans `~/.config.bak/<app>` avant la pose du
lien, arborescence conservée, jamais supprimée. Une config déjà symlinkée est laissée
telle quelle.

## Rôles

| Rôle | Effet |
|---|---|
| `yay` | installe l'AUR helper depuis les sources si absent |
| `packages` | paquets pacman/AUR — core, machine, groupes optionnels ; active `multilib` si gaming |
| `dotfiles` | symlinks des apps migrées, `~/.zshrc`, config SSH, fragment sway |
| `services` | active les services systemd système et utilisateur, `ly` |
| `shell` | zsh en shell par défaut, configs root, `/usr/local/bin/nv` |
| `neovim` | plugins (nvim-tree, plenary, gitgraph, treesitter), parsers, timer mensuel — sous `install_dev` |

