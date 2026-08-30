#include "window.hpp"

#include <gtkmm/application.h>

int main(int argc, char** argv)
{
	// NON_UNIQUE : le panneau est une bascule lancee/tuee par la waybar, une
	// instance unique transformerait le second clic en no-op silencieux.
	auto app = Gtk::Application::create(
		argc, argv, "org.conspicio.settings", Gio::APPLICATION_NON_UNIQUE);

	SettingsWindow window;
	return app->run(window);
}
