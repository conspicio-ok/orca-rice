#pragma once

#include <giomm/dbusproxy.h>
#include <glibmm/ustring.h>
#include <sigc++/signal.h>

// Interrupteur on/off d'une radio expose sur le bus systeme via la propriete
// booleenne "Powered" (BlueZ org.bluez.Adapter1, iwd net.connman.iwd.Device).
// Aucun sous-processus : lecture par cache du proxy, ecriture par
// org.freedesktop.DBus.Properties.Set. Polkit/dbus autorise la session locale.
class Radio {
public:
	Radio(const Glib::ustring& bus_name,
	      const Glib::ustring& object_path,
	      const Glib::ustring& interface);

	// false si le service ou l'adaptateur est absent : le widget doit etre grise.
	bool available() const { return static_cast<bool>(proxy_); }
	bool powered() const;
	void set_powered(bool on);

	// Emis quand la propriete change hors de notre fait (autre client, rfkill).
	sigc::signal<void, bool>& signal_changed() { return signal_changed_; }

	// Chemins deduits de sysfs : pas d'introspection ObjectManager a payer.
	static Glib::ustring bluez_adapter_path();
	static Glib::ustring iwd_device_path();

private:
	void on_properties_changed(const Gio::DBus::Proxy::MapChangedProperties& changed,
	                           const std::vector<Glib::ustring>& invalidated);

	Glib::ustring interface_;
	Glib::RefPtr<Gio::DBus::Proxy> proxy_;
	sigc::signal<void, bool> signal_changed_;
};
