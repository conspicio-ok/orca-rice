#include "volume.hpp"

#include <glib.h>

#include <algorithm>

namespace {

// Une operation retournee par libpulse est comptee : sans unref elle fuit,
// meme quand on ne s'en sert pas pour suivre l'avancement.
void drop(pa_operation* op)
{
	if (op)
		pa_operation_unref(op);
}

} // namespace

Volume::Volume()
{
	mainloop_ = pa_glib_mainloop_new(g_main_context_default());
	if (!mainloop_)
		return;

	ctx_ = pa_context_new(pa_glib_mainloop_get_api(mainloop_), "orca-settings");
	if (!ctx_)
		return;

	pa_context_set_state_callback(ctx_, &Volume::state_cb, this);

	if (pa_context_connect(ctx_, nullptr, PA_CONTEXT_NOFAIL, nullptr) < 0)
		fail();
}

Volume::~Volume()
{
	if (ctx_) {
		pa_context_set_state_callback(ctx_, nullptr, nullptr);
		pa_context_set_subscribe_callback(ctx_, nullptr, nullptr);
		pa_context_disconnect(ctx_);
		pa_context_unref(ctx_);
	}
	if (mainloop_)
		pa_glib_mainloop_free(mainloop_);
}

// Le contexte a echoue ou s'est termine : on grise sans rien casser d'autre.
void Volume::fail()
{
	if (!ready_)
		return;
	ready_ = false;
	signal_changed_.emit();
}

void Volume::state_cb(pa_context* ctx, void* userdata)
{
	auto* self = static_cast<Volume*>(userdata);

	switch (pa_context_get_state(ctx)) {
	case PA_CONTEXT_READY:
		// Le nom du sink par defaut n'est connu que du serveur : on le demande
		// avant de pouvoir lire quoi que ce soit.
		pa_context_set_subscribe_callback(ctx, &Volume::subscribe_cb, self);
		drop(pa_context_subscribe(
			ctx, static_cast<pa_subscription_mask_t>(
				PA_SUBSCRIPTION_MASK_SINK | PA_SUBSCRIPTION_MASK_SERVER),
			nullptr, nullptr));
		self->request_server_info();
		break;
	case PA_CONTEXT_FAILED:
	case PA_CONTEXT_TERMINATED:
		self->fail();
		break;
	default:
		break;
	}
}

void Volume::request_server_info()
{
	drop(pa_context_get_server_info(ctx_, &Volume::server_info_cb, this));
}

void Volume::request_sink_info()
{
	if (sink_.empty())
		return;
	drop(pa_context_get_sink_info_by_name(
		ctx_, sink_.c_str(), &Volume::sink_info_cb, this));
}

void Volume::server_info_cb(pa_context*, const pa_server_info* info, void* userdata)
{
	auto* self = static_cast<Volume*>(userdata);
	if (!info || !info->default_sink_name) {
		self->fail();
		return;
	}

	self->sink_ = info->default_sink_name;
	self->request_sink_info();
}

void Volume::sink_info_cb(pa_context*, const pa_sink_info* info, int eol, void* userdata)
{
	// eol > 0 termine l'enumeration, eol < 0 signale l'erreur : dans les deux
	// cas il n'y a pas de info a lire.
	if (eol != 0 || !info)
		return;

	auto* self = static_cast<Volume*>(userdata);

	// pa_cvolume_avg amortit un eventuel desequilibre gauche/droite ; le
	// slider est mono, il n'a qu'une valeur a montrer.
	const double raw = static_cast<double>(pa_cvolume_avg(&info->volume))
		/ static_cast<double>(PA_VOLUME_NORM);

	self->channels_ = info->volume.channels ? info->volume.channels : 2;
	self->level_ = std::clamp(raw, 0.0, 1.0);
	self->muted_ = info->mute != 0;
	self->ready_ = true;
	self->signal_changed_.emit();
}

// Toute notification sink/serveur invalide ce qu'on a en cache : le sink par
// defaut a pu changer, donc on repart du serveur.
void Volume::subscribe_cb(pa_context*, pa_subscription_event_type_t type,
                          uint32_t, void* userdata)
{
	auto* self = static_cast<Volume*>(userdata);

	switch (type & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) {
	case PA_SUBSCRIPTION_EVENT_SERVER:
		self->request_server_info();
		break;
	case PA_SUBSCRIPTION_EVENT_SINK:
		self->request_sink_info();
		break;
	default:
		break;
	}
}

void Volume::set_level(double value)
{
	if (!ready_ || sink_.empty())
		return;

	const double clamped = std::clamp(value, 0.0, 1.0);

	pa_cvolume cv;
	pa_cvolume_set(&cv, channels_,
		static_cast<pa_volume_t>(clamped * PA_VOLUME_NORM + 0.5));

	drop(pa_context_set_sink_volume_by_name(
		ctx_, sink_.c_str(), &cv, nullptr, nullptr));
}

void Volume::set_muted(bool mute)
{
	if (!ready_ || sink_.empty())
		return;

	drop(pa_context_set_sink_mute_by_name(
		ctx_, sink_.c_str(), mute ? 1 : 0, nullptr, nullptr));
}
