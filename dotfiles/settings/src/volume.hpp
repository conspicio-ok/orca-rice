#pragma once

#include <pulse/glib-mainloop.h>
#include <pulse/pulseaudio.h>

#include <sigc++/signal.h>

#include <cstdint>
#include <string>

// Volume du sink par defaut, via l'API asynchrone de libpulse greffee sur la
// boucle GLib. Pas de sous-processus pactl/wpctl : un slider en cours de
// glissement en forkerait a chaque pas. PipeWire repond sur la couche de
// compatibilite PulseAudio, le protocole est le meme.
class Volume {
public:
	Volume();
	~Volume();

	// Les callbacks C portent l'adresse de l'instance : elle ne doit ni bouger
	// ni etre dupliquee.
	Volume(const Volume&) = delete;
	Volume& operator=(const Volume&) = delete;

	// false tant que le contexte n'a pas atteint PA_CONTEXT_READY, ou apres un
	// echec : le widget doit etre grise.
	bool available() const { return ready_; }
	double level() const { return level_; }
	bool muted() const { return muted_; }

	void set_level(double value);
	void set_muted(bool mute);

	// Emis a chaque etat lu du serveur, y compris pour un changement venu
	// d'ailleurs (touches media, pavucontrol).
	sigc::signal<void>& signal_changed() { return signal_changed_; }

private:
	// Trampolines C : userdata porte le this.
	static void state_cb(pa_context* ctx, void* userdata);
	static void server_info_cb(pa_context* ctx, const pa_server_info* info, void* userdata);
	static void sink_info_cb(pa_context* ctx, const pa_sink_info* info, int eol, void* userdata);
	static void subscribe_cb(pa_context* ctx, pa_subscription_event_type_t type,
	                         uint32_t index, void* userdata);

	void request_server_info();
	void request_sink_info();
	void fail();

	pa_glib_mainloop* mainloop_ = nullptr;
	pa_context* ctx_ = nullptr;
	bool ready_ = false;
	std::string sink_;
	double level_ = 0.0;
	bool muted_ = false;
	// Nombre de canaux du sink : pa_cvolume_set doit ecrire exactement le meme.
	uint8_t channels_ = 2;
	sigc::signal<void> signal_changed_;
};
