#pragma once

#include "radio.hpp"
#include "volume.hpp"

#include <gtkmm/box.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/button.h>
#include <gtkmm/grid.h>
#include <gtkmm/label.h>
#include <gtkmm/scale.h>
#include <gtkmm/window.h>

#include <string>

// Panneau de controle rapide facon telephone : surface layer-shell ancree sous
// la waybar, tuiles carrees a bascule, se ferme sur Escape.
class SettingsWindow : public Gtk::Window {
public:
	SettingsWindow();

private:
	// Tuile reduite a son glyphe, cablee sur une Radio.
	class Tile : public Gtk::Button {
	public:
		Tile(const Glib::ustring& icon_on, const Glib::ustring& icon_off, Radio& radio,
		     const Glib::ustring& icon_class = {});

	private:
		void render(bool on);

		Radio& radio_;
		Glib::ustring icon_on_;
		Glib::ustring icon_off_;
		Gtk::Label icon_;
	};

	bool on_key_press_event(GdkEventKey* event) override;

	void init_layer_shell();
	void load_style();

	// Luminosite : lecture directe dans sysfs, ecriture par logind.
	void init_brightness();
	void on_brightness_changed();
	// Le slider est lu une fois a l'ouverture : les touches de luminosite
	// continuent d'agir pendant ce temps, il faut donc le resynchroniser.
	void refresh_brightness();

	// Volume : lecture et ecriture par libpulse, sur le sink par defaut.
	void init_volume();
	void on_volume_changed();
	void on_volume_state();
	void render_volume_icon();

	// Entete : distribution et etat de la batterie, lus au demarrage comme les
	// versions, le panneau etant relance a chaque ouverture.
	void init_status();
	// Rafraichissement periodique : la capacite et la duree bougent pendant que
	// le panneau reste ouvert.
	bool update_status();
	// Etat batterie via upower quand il tourne, sysfs sinon.
	bool status_from_upower();

	// Versions GTK : celle de la waybar est scannee au demarrage, celle du
	// panneau est figee a la compilation. Divergence = bulle orange.
	void init_versions();
	static std::string scan_waybar_gtk();

	// Assemble une ligne [glyphe] [barre] [glyphe] sur toute la largeur utile
	// du panneau : la barre absorbe l'espace, les glyphes non.
	static void pack_slider_row(Gtk::Box& row, Gtk::Widget& before,
	                            Gtk::Scale& scale, Gtk::Widget* after);

	Radio bluetooth_;
	Radio wifi_;
	Gtk::EventBox panel_;
	Gtk::Grid grid_;
	Gtk::Box tiles_row_;
	Tile bluetooth_tile_;
	Tile wifi_tile_;
	Gtk::Box brightness_row_;
	Gtk::Label brightness_low_;
	Gtk::Scale brightness_;
	Gtk::Label brightness_high_;
	Glib::RefPtr<Gio::DBus::Proxy> logind_;
	std::string backlight_name_;

	Volume volume_;
	Gtk::Box volume_row_;
	Gtk::Button volume_button_;
	Gtk::Label volume_icon_;
	Gtk::Label volume_high_;
	Gtk::Scale volume_scale_;
	Gtk::Box versions_box_;
	Gtk::Box versions_head_;
	Gtk::Box versions_row_;
	Gtk::Label head_waybar_;
	Gtk::Label head_settings_;
	Gtk::Label version_waybar_;
	Gtk::Label version_settings_;
	Gtk::Box status_box_;
	Gtk::Box status_head_;
	Gtk::Box status_row_;
	Gtk::Label head_distro_;
	Gtk::Label head_capacity_;
	Gtk::Label status_time_;
	Glib::RefPtr<Gio::DBus::Proxy> upower_;

	// Garde anti-boucle : une mise a jour venue de PulseAudio ne doit pas
	// reecrire le volume par le signal du slider.
	bool volume_guard_ = false;
	// Meme garde pour la luminosite : une valeur relue dans sysfs ne doit pas
	// repartir vers logind par le signal du slider.
	bool brightness_guard_ = false;
};
