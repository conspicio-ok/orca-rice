#include "radio.hpp"

#include <glibmm/variant.h>
#include <giomm/file.h>

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace {

// Premiere entree d'un repertoire sysfs, ou chaine vide s'il est absent/vide.
std::string first_entry(const char* dir)
{
	std::error_code ec;
	fs::directory_iterator it(dir, ec);
	if (ec || it == fs::directory_iterator())
		return {};
	return it->path().filename().string();
}

std::string read_line(const fs::path& p)
{
	std::ifstream in(p);
	std::string s;
	std::getline(in, s);
	return s;
}

} // namespace

// /sys/class/bluetooth/hciN -> /org/bluez/hciN
Glib::ustring Radio::bluez_adapter_path()
{
	const std::string hci = first_entry("/sys/class/bluetooth");
	if (hci.empty())
		return {};
	return "/org/bluez/" + hci;
}

// iwd nomme ses objets /net/connman/iwd/<index phy>/<ifindex>.
// Les deux valeurs sont dans sysfs : phy80211 est un lien vers phyN.
Glib::ustring Radio::iwd_device_path()
{
	std::error_code ec;
	for (const auto& entry : fs::directory_iterator("/sys/class/net", ec)) {
		const fs::path phy_link = entry.path() / "phy80211";
		if (!fs::exists(phy_link, ec))
			continue;

		const std::string phy = fs::read_symlink(phy_link, ec).filename().string();
		if (ec || phy.compare(0, 3, "phy") != 0)
			continue;

		const std::string ifindex = read_line(entry.path() / "ifindex");
		if (ifindex.empty())
			continue;

		return "/net/connman/iwd/" + phy.substr(3) + "/" + ifindex;
	}
	return {};
}

Radio::Radio(const Glib::ustring& bus_name,
             const Glib::ustring& object_path,
             const Glib::ustring& interface)
	: interface_(interface)
{
	if (object_path.empty())
		return;

	try {
		proxy_ = Gio::DBus::Proxy::create_for_bus_sync(
			Gio::DBus::BUS_TYPE_SYSTEM, bus_name, object_path, interface);
	} catch (const Glib::Error&) {
		return; // service absent : available() reste false
	}

	// Un proxy sans proprietaire ne portera jamais de proprietes en cache.
	if (proxy_->get_name_owner().empty()) {
		proxy_.reset();
		return;
	}

	proxy_->signal_properties_changed().connect(
		sigc::mem_fun(*this, &Radio::on_properties_changed));
}

bool Radio::powered() const
{
	if (!proxy_)
		return false;

	Glib::VariantBase v;
	proxy_->get_cached_property(v, "Powered");
	if (!v.gobj())
		return false;

	return Glib::VariantBase::cast_dynamic<Glib::Variant<bool>>(v).get();
}

void Radio::set_powered(bool on)
{
	if (!proxy_ || powered() == on)
		return;

	const auto args = Glib::VariantContainerBase::create_tuple({
		Glib::Variant<Glib::ustring>::create(interface_),
		Glib::Variant<Glib::ustring>::create("Powered"),
		Glib::Variant<Glib::VariantBase>::create(Glib::Variant<bool>::create(on)),
	});

	// Set n'est pas sur l'interface du proxy : on passe par la connexion.
	proxy_->get_connection()->call(
		proxy_->get_object_path(),
		"org.freedesktop.DBus.Properties",
		"Set",
		args,
		{},
		proxy_->get_name());
}

void Radio::on_properties_changed(const Gio::DBus::Proxy::MapChangedProperties& changed,
                                  const std::vector<Glib::ustring>&)
{
	const auto it = changed.find("Powered");
	if (it == changed.end())
		return;

	signal_changed_.emit(
		Glib::VariantBase::cast_dynamic<Glib::Variant<bool>>(it->second).get());
}
