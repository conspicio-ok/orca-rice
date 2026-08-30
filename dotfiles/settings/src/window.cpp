#include "window.hpp"

#include <gtk-layer-shell.h>
#include <gtkmm/cssprovider.h>
#include <gtkmm/stylecontext.h>

#include <glibmm/main.h>
#include <glibmm/variant.h>

#include <cstdio>
#include <filesystem>
#include <fstream>

namespace {

// Reprend colors.css de la waybar : meme fond translucide, memes gris.
const char* STYLE = R"CSS(
window {
	background-color: transparent;
}

.panel {
	background-color: rgba(30, 30, 30, 0.88);
	border-radius: 16px;
}

grid {
	margin: 14px;
	/* Les barres occupent toute la largeur utile : sans plancher explicite le
	   panneau se reduirait a la rangee de tuiles. */
	min-width: 200px;
}

button {
	background-image: none;
	background-color: rgba(255, 255, 255, 0.06);
	border: none;
	border-radius: 19px;
	box-shadow: none;
	color: #808080;
	min-width: 38px;
	min-height: 38px;
	padding: 0;
	transition: background-color 150ms ease, color 150ms ease;
}

button:hover {
	background-color: rgba(255, 255, 255, 0.12);
}

button.active {
	background-color: #cfcfcf;
	color: #1e1e1e;
}

button:disabled {
	background-color: rgba(255, 255, 255, 0.03);
	color: rgba(128, 128, 128, 0.4);
}

button label {
	padding: 0;
	margin: 0;
}

label.icon {
	font-family: "JetBrainsMono Nerd Font", "Symbols Nerd Font", monospace;
	font-size: 16px;
}

/* Ecart mesure sur capture entre le centre d'encre du glyphe et le centre du
   disque : le wifi tombait 2px a droite et 1px au-dessus du bluetooth, qui sert
   de reference. Le label etant en ALIGN_FILL avec x/yalign 0.5, il se recentre
   dans la boite moins le padding : un padding de N deplace donc de N/2. */
label.icon.wifi {
	padding-top: 2px;
	padding-right: 4px;
}

/* Glyphes d'encadrement des barres : lisibles a distance, donc un cran
   au-dessus des glyphes de tuiles malgre leur role secondaire. */
label.icon.level {
	font-size: 18px;
	color: #cfcfcf;
}

/* Le glyphe de volume est cliquable, mais ne doit pas ressembler aux tuiles
   rondes : ni fond, ni bordure, ni reaction au survol. */
button.plain,
button.plain:hover,
button.plain:active,
button.plain:checked {
	background-color: transparent;
	background-image: none;
	border: none;
	box-shadow: none;
	color: #cfcfcf;
	min-width: 0;
	min-height: 0;
	padding: 0;
}

button.plain:disabled {
	background-color: transparent;
	color: rgba(207, 207, 207, 0.4);
}

scale {
	margin: 0;
	min-width: 20px;
	min-height: 16px;
	padding: 0;
}

scale trough {
	background-color: rgba(255, 255, 255, 0.12);
	border: none;
	border-radius: 5px;
	min-height: 9px;
	min-width: 20px;
}

scale highlight {
	background-color: #cfcfcf;
	border: none;
	border-radius: 5px;
	min-height: 9px;
}

/* Pied de panneau : les versions sont une information de maintenance, elles ne
   doivent pas concurrencer les controles. */
/* Meme police que la waybar : "Fira Sans Semibold" est une famille a part
   entiere, la graisse est donc portee par le nom et non par font-weight. */
label.meta {
	font-family: "Fira Sans Semibold", "JetBrainsMono Nerd Font", sans-serif;
	font-size: 11px;
	color: #808080;
}

label.meta.head {
	font-size: 13px;
	color: #cfcfcf;
}

/* Marge negative compensee par le padding : la bulle deborde de 6px de chaque
   cote tandis que le texte, lui, reste aligne sous son titre. */
.versions {
	margin: 0 -6px;
	padding: 2px 6px;
	border-radius: 8px;
}

/* Seul cas ou le panneau porte une couleur : la waybar et le panneau ne
   tournent plus sur le meme GTK, il faut recompiler contre le bon. */
.versions.mismatch {
	background-color: rgba(224, 142, 60, 0.22);
	box-shadow: 0 0 4px rgba(224, 142, 60, 0.20);
}

.versions.mismatch label.meta {
	color: #e08e3c;
}

scale slider {
	background-color: #cfcfcf;
	border: none;
	border-radius: 50%;
	box-shadow: none;
	min-width: 13px;
	min-height: 13px;
	margin: -2px;
}
)CSS";

// Jeu Material Design Icons de la Nerd Font : les variantes barrees
// n'existent pas dans Font Awesome Free.
const char* ICON_BLUETOOTH_ON = "󰂯";
const char* ICON_BLUETOOTH_OFF = "󰂲";
const char* ICON_WIFI_ON = "󰖩";
const char* ICON_WIFI_OFF = "󰖪";
const char* ICON_BRIGHTNESS_LOW = "󰃞";
const char* ICON_BRIGHTNESS_HIGH = "󰃠";
const char* ICON_VOLUME_MUTED = "󰖁";
const char* ICON_VOLUME_LOW = "󰕿";
const char* ICON_VOLUME_MEDIUM = "󰖀";
const char* ICON_VOLUME_HIGH = "󰕾";

// Version GTK contre laquelle ce panneau est compile : figee dans le binaire,
// elle ne bouge qu'a la recompilation.
const std::string SETTINGS_GTK =
	std::to_string(GTK_MAJOR_VERSION) + "." +
	std::to_string(GTK_MINOR_VERSION) + "." +
	std::to_string(GTK_MICRO_VERSION);

namespace fs = std::filesystem;

// Premiere entree de /sys/class/backlight : le nom du controleur varie selon
// la machine (intel_backlight, amdgpu_bl0...), on ne le code pas en dur.
std::string first_backlight()
{
	std::error_code ec;
	fs::directory_iterator it("/sys/class/backlight", ec);
	if (ec || it == fs::directory_iterator())
		return {};
	return it->path().filename().string();
}

// Entier lu sur la premiere ligne d'un attribut sysfs, ou -1 si illisible.
long read_number(const fs::path& p)
{
	std::ifstream in(p);
	long v = -1;
	if (!(in >> v))
		return -1;
	return v;
}

// /etc/os-release : NAME= donne la forme courte ("Arch Linux"), la seule
// stable d'une distribution a l'autre.
std::string distro_name()
{
	std::ifstream in("/etc/os-release");
	std::string line;
	while (std::getline(in, line)) {
		if (line.rfind("NAME=", 0) != 0)
			continue;
		std::string value = line.substr(5);
		if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
			value = value.substr(1, value.size() - 2);
		return value;
	}
	return "linux";
}

// AC et batterie cohabitent sous /sys/class/power_supply et les noms varient
// (BAT0, BAT1, macsmc-battery) : on filtre sur l'attribut type.
fs::path first_battery()
{
	std::error_code ec;
	fs::directory_iterator it("/sys/class/power_supply", ec);
	if (ec)
		return {};
	for (const auto& entry : it) {
		std::ifstream in(entry.path() / "type");
		std::string type;
		if (in >> type && type == "Battery")
			return entry.path();
	}
	return {};
}

// Premier mot d'un attribut sysfs, ou chaine vide si illisible.
std::string read_word(const fs::path& p)
{
	std::ifstream in(p);
	std::string word;
	if (!(in >> word))
		return {};
	return word;
}

// Duree lisible a partir d'un nombre de secondes.
std::string format_duration(long seconds)
{
	const long minutes = seconds / 60;
	return std::to_string(minutes / 60) + " h " +
		(minutes % 60 < 10 ? "0" : "") + std::to_string(minutes % 60);
}

// charge_*/current_* sont en uAh et uA, energy_*/power_* en uWh et uW : le
// rapport est homogene dans les deux cas, seul le couple expose par le pilote
// change d'une machine a l'autre.
std::string battery_time(const fs::path& battery, const std::string& status)
{
	long now = read_number(battery / "charge_now");
	long full = read_number(battery / "charge_full");
	long rate = read_number(battery / "current_now");
	if (now < 0 || full <= 0 || rate <= 0) {
		now = read_number(battery / "energy_now");
		full = read_number(battery / "energy_full");
		rate = read_number(battery / "power_now");
	}
	if (now < 0 || full <= 0 || rate <= 0)
		return {};

	// "Full" et "Not charging" ne vont ni vers 0 ni vers 100 : il n'y a rien a
	// projeter, et le debit residuel donnerait une duree absurde.
	if (status != "Charging" && status != "Discharging")
		return {};

	const long remaining = status == "Charging" ? full - now : now;
	if (remaining <= 0)
		return {};

	// Debit instantane : sans historique, cette projection saute avec la
	// charge CPU. C'est le repli quand upower est absent.
	const std::string duration = format_duration(remaining * 3600 / rate);
	return status == "Charging" ? duration + " avant 100 %"
	                            : duration + " restantes";
}

void add_class(Gtk::Widget& w, const Glib::ustring& name)
{
	w.get_style_context()->add_class(name);
}

} // namespace

SettingsWindow::Tile::Tile(const Glib::ustring& icon_on,
                           const Glib::ustring& icon_off,
                           Radio& radio,
                           const Glib::ustring& icon_class)
	: radio_(radio)
	, icon_on_(icon_on)
	, icon_off_(icon_off)
{
	add_class(icon_, "icon");
	if (!icon_class.empty())
		add_class(icon_, icon_class);

	// ALIGN_FILL donne au label toute la boite du bouton ; x/yalign centrent
	// le glyphe dedans, ce que ALIGN_CENTER ne fait pas (taille naturelle,
	// donc decale par l'ascendante/descendante de la fonte).
	icon_.set_halign(Gtk::ALIGN_FILL);
	icon_.set_valign(Gtk::ALIGN_FILL);
	icon_.set_xalign(0.5f);
	icon_.set_yalign(0.5f);
	add(icon_);

	// Sans cela le Grid etire la tuile et le cercle devient un ovale.
	set_halign(Gtk::ALIGN_CENTER);
	set_valign(Gtk::ALIGN_CENTER);

	set_sensitive(radio_.available());
	render(radio_.powered());

	// Un bouton, pas un interrupteur : le clic bascule, pas besoin de garde
	// anti-boucle puisque l'affichage ne depend que du signal D-Bus.
	signal_clicked().connect([this] { radio_.set_powered(!radio_.powered()); });
	radio_.signal_changed().connect(sigc::mem_fun(*this, &Tile::render));
}

// L'etat se lit au glyphe (barre quand eteint) et a la couleur : classe CSS
// active, ou button:disabled quand la radio est absente.
void SettingsWindow::Tile::render(bool on)
{
	icon_.set_text(on ? icon_on_ : icon_off_);

	auto ctx = get_style_context();
	if (on)
		ctx->add_class("active");
	else
		ctx->remove_class("active");
}

SettingsWindow::SettingsWindow()
	: bluetooth_("org.bluez", Radio::bluez_adapter_path(), "org.bluez.Adapter1")
	, wifi_("net.connman.iwd", Radio::iwd_device_path(), "net.connman.iwd.Device")
	, tiles_row_(Gtk::ORIENTATION_HORIZONTAL, 8)
	, bluetooth_tile_(ICON_BLUETOOTH_ON, ICON_BLUETOOTH_OFF, bluetooth_)
	, wifi_tile_(ICON_WIFI_ON, ICON_WIFI_OFF, wifi_, "wifi")
	, brightness_row_(Gtk::ORIENTATION_HORIZONTAL, 4)
	, brightness_(Gtk::ORIENTATION_HORIZONTAL)
	, volume_row_(Gtk::ORIENTATION_HORIZONTAL, 4)
	, volume_scale_(Gtk::ORIENTATION_HORIZONTAL)
	, versions_box_(Gtk::ORIENTATION_VERTICAL)
	, versions_head_(Gtk::ORIENTATION_HORIZONTAL)
	, versions_row_(Gtk::ORIENTATION_HORIZONTAL)
	, status_box_(Gtk::ORIENTATION_VERTICAL)
	, status_head_(Gtk::ORIENTATION_HORIZONTAL)
	, status_row_(Gtk::ORIENTATION_HORIZONTAL)
{
	set_title("Settings");
	set_app_paintable(true);

	// Fond translucide : sans visuel RGBA, le border-radius laisse des coins noirs.
	if (auto visual = get_screen()->get_rgba_visual())
		gtk_widget_set_visual(GTK_WIDGET(gobj()), visual->gobj());

	grid_.set_row_spacing(8);
	grid_.set_column_spacing(8);
	// Les tuiles vivent dans leur propre boite alignee a gauche : la grille
	// n'a plus qu'une colonne, sinon les lignes de barres calqueraient leur
	// decoupe, et le bord gauche du premier cercle tombe sur celui des glyphes
	// des barres en dessous.
	tiles_row_.set_halign(Gtk::ALIGN_START);
	tiles_row_.pack_start(bluetooth_tile_, Gtk::PACK_SHRINK);
	tiles_row_.pack_start(wifi_tile_, Gtk::PACK_SHRINK);
	init_status();
	grid_.attach(status_box_, 0, 0, 1, 1);
	grid_.attach(tiles_row_, 0, 1, 1, 1);
	init_brightness();
	grid_.attach(brightness_row_, 0, 2, 1, 1);
	init_volume();
	grid_.attach(volume_row_, 0, 3, 1, 1);
	init_versions();
	grid_.attach(versions_box_, 0, 4, 1, 1);

	// La fenetre reste transparente : c'est le panneau qui porte le fond plein
	// et le border-radius, sous les tuiles.
	add_class(panel_, "panel");
	panel_.add(grid_);
	add(panel_);

	load_style();
	init_layer_shell();
	show_all_children();
}

// La ligne prend toute la largeur utile du panneau : les glyphes gardent leur
// taille naturelle et la barre absorbe tout le reste.
void SettingsWindow::pack_slider_row(Gtk::Box& row, Gtk::Widget& before,
                                     Gtk::Scale& scale, Gtk::Widget* after)
{
	row.set_halign(Gtk::ALIGN_FILL);
	row.set_hexpand(true);
	// Gouttiere entre les glyphes d'encadrement et la barre : suit la taille
	// des glyphes, sinon ils touchent presque le rail.
	row.set_spacing(12);

	scale.set_draw_value(false);
	scale.set_hexpand(true);

	before.set_valign(Gtk::ALIGN_CENTER);
	row.pack_start(before, Gtk::PACK_SHRINK);
	row.pack_start(scale, Gtk::PACK_EXPAND_WIDGET);
	if (after) {
		after->set_valign(Gtk::ALIGN_CENTER);
		row.pack_start(*after, Gtk::PACK_SHRINK);
	}
}

// L'echelle porte des valeurs brutes du controleur : pas de conversion en
// pourcentage, donc pas d'arrondi a recoller a chaque ecriture.
void SettingsWindow::init_brightness()
{
	backlight_name_ = first_backlight();
	const long max = backlight_name_.empty()
		? -1
		: read_number(fs::path("/sys/class/backlight") / backlight_name_ / "max_brightness");
	const long cur = max <= 0
		? -1
		: read_number(fs::path("/sys/class/backlight") / backlight_name_ / "brightness");

	add_class(brightness_low_, "icon");
	add_class(brightness_low_, "level");
	add_class(brightness_high_, "icon");
	add_class(brightness_high_, "level");
	brightness_low_.set_text(ICON_BRIGHTNESS_LOW);
	brightness_high_.set_text(ICON_BRIGHTNESS_HIGH);
	pack_slider_row(brightness_row_, brightness_low_, brightness_, &brightness_high_);

	if (max <= 0 || cur < 0) {
		brightness_row_.set_sensitive(false);
		return;
	}

	// Plancher a 1 % : a zero l'ecran est noir et plus rien n'est manipulable.
	const double floor_value = max / 100.0;
	brightness_.set_range(floor_value, static_cast<double>(max));
	brightness_.set_increments(max / 100.0, max / 10.0);
	brightness_.set_value(cur < floor_value ? floor_value : static_cast<double>(cur));

	try {
		logind_ = Gio::DBus::Proxy::create_for_bus_sync(
			Gio::DBus::BUS_TYPE_SYSTEM,
			"org.freedesktop.login1",
			"/org/freedesktop/login1/session/auto",
			"org.freedesktop.login1.Session");
	} catch (const Glib::Error&) {
		brightness_row_.set_sensitive(false);
		return;
	}

	brightness_.signal_value_changed().connect(
		sigc::mem_fun(*this, &SettingsWindow::on_brightness_changed));
}

// Ecriture par logind : sysfs n'est inscriptible que par root, et polkit
// autorise deja la session locale active pour SetBrightness.
void SettingsWindow::on_brightness_changed()
{
	if (!logind_ || brightness_guard_)
		return;

	const auto args = Glib::VariantContainerBase::create_tuple({
		Glib::Variant<Glib::ustring>::create("backlight"),
		Glib::Variant<Glib::ustring>::create(backlight_name_),
		Glib::Variant<guint32>::create(
			static_cast<guint32>(brightness_.get_value())),
	});

	logind_->call("SetBrightness", args);
}

// Le contexte PulseAudio n'est pret qu'apres coup : la ligne demarre grisee
// et c'est le premier signal d'etat qui l'active.
void SettingsWindow::init_volume()
{
	add_class(volume_icon_, "icon");
	add_class(volume_icon_, "level");
	volume_icon_.set_xalign(0.5f);
	volume_icon_.set_yalign(0.5f);

	add_class(volume_button_, "plain");
	volume_button_.set_relief(Gtk::RELIEF_NONE);
	volume_button_.set_can_focus(false);
	volume_button_.add(volume_icon_);

	// Le slider est en pourcentage : PulseAudio parle en pa_volume_t, la
	// conversion est faite par Volume.
	volume_scale_.set_range(0.0, 100.0);
	volume_scale_.set_increments(1.0, 10.0);

	// Borne haute fixe, simple label : la ligne du son est encadree comme
	// celle de la luminosite, seul le glyphe de gauche est cliquable.
	add_class(volume_high_, "icon");
	add_class(volume_high_, "level");
	volume_high_.set_text(ICON_VOLUME_HIGH);
	pack_slider_row(volume_row_, volume_button_, volume_scale_, &volume_high_);

	volume_row_.set_sensitive(false);
	render_volume_icon();

	volume_.signal_changed().connect(
		sigc::mem_fun(*this, &SettingsWindow::on_volume_state));
	volume_scale_.signal_value_changed().connect(
		sigc::mem_fun(*this, &SettingsWindow::on_volume_changed));
	volume_button_.signal_clicked().connect([this] {
		if (volume_.available())
			volume_.set_muted(!volume_.muted());
	});
}

// Etat pousse par PulseAudio : on repositionne le slider sous garde, sinon
// set_value rappellerait on_volume_changed et reecrirait ce qu'on vient de lire.
void SettingsWindow::on_volume_state()
{
	volume_row_.set_sensitive(volume_.available());
	if (!volume_.available())
		return;

	volume_guard_ = true;
	volume_scale_.set_value(volume_.level() * 100.0);
	volume_guard_ = false;

	render_volume_icon();
}

void SettingsWindow::on_volume_changed()
{
	if (volume_guard_)
		return;

	// Chemin garde par volume_guard_ : on n'arrive ici que sur une action
	// utilisateur, jamais sur une synchronisation venue de PulseAudio. Bouger
	// la barre pendant une sourdine la leve, comme sur les panneaux usuels.
	if (volume_.muted())
		volume_.set_muted(false);

	volume_.set_level(volume_scale_.get_value() / 100.0);
	render_volume_icon();
}

// Rampe a trois marches plus l'etat mute : le glyphe seul dit le niveau.
void SettingsWindow::render_volume_icon()
{
	if (volume_.muted()) {
		volume_icon_.set_text(ICON_VOLUME_MUTED);
		return;
	}

	const double level = volume_scale_.get_value();
	if (level < 34.0)
		volume_icon_.set_text(ICON_VOLUME_LOW);
	else if (level < 67.0)
		volume_icon_.set_text(ICON_VOLUME_MEDIUM);
	else
		volume_icon_.set_text(ICON_VOLUME_HIGH);
}

// Le binaire de la waybar ne dit pas sa version de GTK : on remonte a la
// bibliotheque qu'il charge reellement, puis au paquet qui la fournit. Passer
// par la soname (libgtk-3.so.0.2420.32) ne donnerait pas la version amont.
std::string SettingsWindow::scan_waybar_gtk()
{
	FILE* pipe = popen(
		"p=$(command -v waybar) || exit 1; "
		"l=$(ldd \"$p\" 2>/dev/null | awk '/libgtk-[34]\\.so/{print $3; exit}'); "
		"[ -n \"$l\" ] || exit 1; "
		"pacman -Qo \"$l\" 2>/dev/null | awk '{print $NF}' | "
		"sed 's/^[0-9]*://; s/-[^-]*$//'",
		"r");
	if (!pipe)
		return {};

	char buffer[64] = {0};
	const bool ok = fgets(buffer, sizeof(buffer), pipe) != nullptr;
	pclose(pipe);
	if (!ok)
		return {};

	std::string version(buffer);
	while (!version.empty() && (version.back() == '\n' || version.back() == ' '))
		version.pop_back();
	return version;
}

// Structure figee au demarrage : seuls les textes sont rafraichis ensuite.
void SettingsWindow::init_status()
{
	head_distro_.set_text(distro_name());

	for (Gtk::Label* l : {&head_distro_, &head_capacity_}) {
		add_class(*l, "meta");
		add_class(*l, "head");
	}
	add_class(status_time_, "meta");

	status_head_.set_halign(Gtk::ALIGN_FILL);
	status_head_.set_hexpand(true);
	status_head_.pack_start(head_distro_, Gtk::PACK_SHRINK);
	status_head_.pack_end(head_capacity_, Gtk::PACK_SHRINK);

	// La duree se lit sous le pourcentage qu'elle qualifie, donc a droite.
	status_row_.set_halign(Gtk::ALIGN_FILL);
	status_row_.set_hexpand(true);
	status_row_.pack_end(status_time_, Gtk::PACK_SHRINK);

	// Sans cela show_all_children() rendrait visible une duree qu'on veut
	// pouvoir masquer quand il n'y a rien a projeter.
	status_time_.set_no_show_all(true);

	status_box_.pack_start(status_head_, Gtk::PACK_SHRINK);
	status_box_.pack_start(status_row_, Gtk::PACK_SHRINK);

	// upower tourne en continu et lisse sa propre estimation ; le panneau, lui,
	// nait a chaque ouverture et n'a aucun historique a moyenner. On lui delegue
	// donc la duree, sysfs ne servant que de repli.
	try {
		upower_ = Gio::DBus::Proxy::create_for_bus_sync(
			Gio::DBus::BUS_TYPE_SYSTEM,
			"org.freedesktop.UPower",
			"/org/freedesktop/UPower/devices/DisplayDevice",
			"org.freedesktop.UPower.Device");
	} catch (const Glib::Error&) {
		upower_ = {};
	}

	update_status();
	// 1 s : le panneau ne vit que le temps d'une manipulation, les valeurs
	// affichees doivent y coller. Le cout se limite a quelques lectures sysfs.
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &SettingsWindow::update_status), 1000);
}

// DisplayDevice agrege les batteries de la machine et expose directement les
// deux durees, deja lissees. Rend false si upower est absent ou muet.
bool SettingsWindow::status_from_upower()
{
	if (!upower_)
		return false;

	Glib::Variant<guint32> state;
	Glib::Variant<double> percentage;
	upower_->get_cached_property(state, "State");
	upower_->get_cached_property(percentage, "Percentage");
	if (!state.gobj() || !percentage.gobj())
		return false;

	head_capacity_.set_text(
		std::to_string(static_cast<long>(percentage.get() + 0.5)) + " %");

	// 1 = charge, 2 = decharge ; les autres etats ne convergent ni vers 0 ni
	// vers 100 et n'ont donc pas de duree.
	const bool charging = state.get() == 1;
	if (!charging && state.get() != 2) {
		status_time_.set_text("");
		return true;
	}

	Glib::Variant<gint64> seconds;
	upower_->get_cached_property(seconds, charging ? "TimeToFull" : "TimeToEmpty");
	if (!seconds.gobj() || seconds.get() <= 0) {
		status_time_.set_text("");
		return true;
	}

	status_time_.set_text(format_duration(seconds.get()) +
	                      (charging ? " avant 100 %" : " restantes"));
	return true;
}

// Rend true pour rester branche sur le timer.
bool SettingsWindow::update_status()
{
	if (status_from_upower()) {
		if (status_time_.get_text().empty())
			status_time_.hide();
		else
			status_time_.show();
		refresh_brightness();
		return true;
	}

	const fs::path battery = first_battery();
	const long capacity = battery.empty() ? -1 : read_number(battery / "capacity");
	head_capacity_.set_text(capacity < 0 ? "-" : std::to_string(capacity) + " %");

	// Machine sans batterie, ou batterie pleine sur secteur : il n'y a rien a
	// projeter, la ligne disparait plutot que d'afficher un tiret.
	const std::string remaining =
		battery.empty() ? std::string() : battery_time(battery, read_word(battery / "status"));
	status_time_.set_text(remaining);
	if (remaining.empty())
		status_time_.hide();
	else
		status_time_.show();

	refresh_brightness();
	return true;
}

// Aligne le slider sur sysfs sans repasser par logind.
void SettingsWindow::refresh_brightness()
{
	// Pendant un glisser, la poignee appartient a l'utilisateur : la recaler
	// sur une valeur en retard la ferait sauter sous le doigt.
	if (!logind_ || brightness_.has_grab())
		return;

	const long cur =
		read_number(fs::path("/sys/class/backlight") / backlight_name_ / "brightness");
	if (cur < 0 || static_cast<long>(brightness_.get_value()) == cur)
		return;

	brightness_guard_ = true;
	brightness_.set_value(static_cast<double>(cur));
	brightness_guard_ = false;
}

// Scan au demarrage seulement : le panneau est relance a chaque clic sur la
// waybar, la valeur est donc fraiche sans timer ni surveillance.
void SettingsWindow::init_versions()
{
	const std::string waybar_gtk = scan_waybar_gtk();

	head_waybar_.set_text("waybar");
	head_settings_.set_text("settings");
	version_waybar_.set_text(waybar_gtk.empty() ? "?" : waybar_gtk);
	version_settings_.set_text(SETTINGS_GTK);

	for (Gtk::Label* l : {&head_waybar_, &head_settings_,
	                      &version_waybar_, &version_settings_})
		add_class(*l, "meta");
	add_class(head_waybar_, "head");
	add_class(head_settings_, "head");

	// Space-between : pack_start colle a gauche, pack_end a droite, et la
	// boite prend toute la largeur utile du panneau.
	for (Gtk::Box* b : {&versions_head_, &versions_row_}) {
		b->set_halign(Gtk::ALIGN_FILL);
		b->set_hexpand(true);
	}
	versions_head_.pack_start(head_waybar_, Gtk::PACK_SHRINK);
	versions_head_.pack_end(head_settings_, Gtk::PACK_SHRINK);
	versions_row_.pack_start(version_waybar_, Gtk::PACK_SHRINK);
	versions_row_.pack_end(version_settings_, Gtk::PACK_SHRINK);

	// Les deux lignes forment un bloc a espacement nul : le row_spacing de la
	// grille les separerait comme deux rangees independantes.
	versions_box_.pack_start(versions_head_, Gtk::PACK_SHRINK);
	versions_box_.pack_start(versions_row_, Gtk::PACK_SHRINK);

	add_class(versions_row_, "versions");
	// Une version illisible n'est pas une divergence : on ne signale que ce
	// qu'on a pu comparer.
	if (!waybar_gtk.empty() && waybar_gtk != SETTINGS_GTK)
		add_class(versions_row_, "mismatch");
}

void SettingsWindow::init_layer_shell()
{
	GtkWindow* w = GTK_WINDOW(gobj());
	gtk_layer_init_for_window(w);

	// Meme couche que la waybar, donc au-dessus des fenetres normales.
	gtk_layer_set_layer(w, GTK_LAYER_SHELL_LAYER_TOP);
	gtk_layer_set_namespace(w, "orca-settings");

	gtk_layer_set_anchor(w, GTK_LAYER_SHELL_EDGE_TOP, true);
	gtk_layer_set_anchor(w, GTK_LAYER_SHELL_EDGE_RIGHT, true);

	// Aligne sur la waybar : margin-top 6 + height 26 + une gouttiere de 6.
	gtk_layer_set_margin(w, GTK_LAYER_SHELL_EDGE_TOP, 38);
	gtk_layer_set_margin(w, GTK_LAYER_SHELL_EDGE_RIGHT, 12);

	// On demand : le panneau prend le clavier quand il a le focus, et le rend
	// aux autres fenetres sinon. Exclusive le confisquait tant que la surface
	// etait mappee, rendant le reste du bureau inutilisable.
	gtk_layer_set_keyboard_mode(w, GTK_LAYER_SHELL_KEYBOARD_MODE_ON_DEMAND);
}

void SettingsWindow::load_style()
{
	auto provider = Gtk::CssProvider::create();
	provider->load_from_data(STYLE);
	Gtk::StyleContext::add_provider_for_screen(
		get_screen(), provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

bool SettingsWindow::on_key_press_event(GdkEventKey* event)
{
	if (event->keyval == GDK_KEY_Escape) {
		hide();
		return true;
	}
	return Gtk::Window::on_key_press_event(event);
}
