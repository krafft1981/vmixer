
#include <gst/gst.h>
#include <gst/sdp/sdp.h>
#include <gst/rtp/rtp.h>

#include <string.h>
#include <stdlib.h>
#include "thread.h"
#include "vector.h"
#include "rbtree.h"
#include "logger.h"
#include "config.h"

typedef struct sipuac_s sipuac_t;
#define NUA_MAGIC_T sipuac_t

typedef struct session_s session_t;
#define NUA_HMAGIC_T session_t

typedef struct session_in_s session_in_t;
typedef struct session_out_s session_out_t;

#include <sofia-sip/nua.h>
#include <sofia-sip/sdp.h>
#include <sofia-sip/sip_status.h>
#include <sofia-sip/su_log.h>

struct session_s {

	nua_handle_t* nh;
	su_home_t* home;

	char* callid;

	vector_t* in;
	vector_t* out;
};

struct session_in_s {

	guint id;
	GstCaps* caps;
	GstElement* udpsrc;
	GstElement* depay;
	GstElement* decoder;
};

struct session_out_s {

	guint id;
	GstCaps* caps;
	GstElement* udpsink;
	GstElement* pay;
	GstElement* encoder;
};

struct sipuac_s {

	nua_t* nua;
	su_home_t* home;
	su_root_t* root;

	thread_t* thread;
	propes_t* propes;

	guint medias;

	struct {
		pthread_t td;
		GstElement* pipe;
		GstElement* rtpbin;
		GstElement* amixer;
		GstElement* vmixer;
		GMainLoop* loop;
	} media;

	struct {
		GList* depay;
		GList* decoder;
		GList* encoder;
		GList* pay;
	} factory;
};

static gboolean bus_callback(GstBus *bus, GstMessage *msg, gpointer data) {

	sipuac_t* sipuac = data;
	thread_t* thread = sipuac->thread;

	switch (GST_MESSAGE_TYPE(msg)) {

		case GST_MESSAGE_EOS: {
			ERROR("\'%s\':\'%s\' recved GST_MESSAGE_EOS", thread_module(thread)->name, thread_name(thread));
			g_main_loop_quit(sipuac->media.loop);
			break;
		}

		case GST_MESSAGE_WARNING: {
			GError *error = NULL;
			gchar *debug = NULL;
			gst_message_parse_warning(msg, &error, &debug);
			if (debug) {
				WARN("\'%s\':\'%s\' %s", debug, thread_module(thread)->name, thread_name(thread));
				g_free(debug);
			}

			if (error) {
				ERROR("\'%s\':\'%s\' %s", error->message, thread_module(thread)->name, thread_name(thread));
				g_error_free(error);
			}

			break;
		}

		case GST_MESSAGE_ERROR: {
			gchar* debug = NULL;
			GError* error = NULL;
			gst_message_parse_error(msg, &error, &debug);

			if (debug) {
				WARN("\'%s\':\'%s\' %s", debug, thread_module(thread)->name, thread_name(thread));
				g_free(debug);
			}

			if (error) {
				ERROR("\'%s\':\'%s\' %s", error->message, thread_module(thread)->name, thread_name(thread));
				g_error_free(error);
			}

			g_main_loop_quit(sipuac->media.loop);
			break;
		}

		case GST_MESSAGE_STATE_CHANGED: {
			GstState old_state, new_state, pending_state;
			gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
			DEBUG("\'%s\':\'%s\' element '%s' reset from %s to %s", thread_module(thread)->name, thread_name(thread), GST_MESSAGE_SRC_NAME(msg), gst_element_state_get_name(old_state), gst_element_state_get_name(new_state));
			break;
		}

		default:
			break;
	}

	return TRUE;
}

static void pad_added_cb(GstElement* rtpbin, GstPad* pad, sipuac_t* sipuac) {

	WARN("pad added: %s", GST_PAD_NAME(pad));
}

static void pad_removed_cb(GstElement* rtpbin, GstPad* pad, sipuac_t* sipuac) {

	WARN("pad removed: %s", GST_PAD_NAME(pad));
}

static GstElement* request_aux_receiver_cb(GstElement* rtpbin, guint session, sipuac_t* sipuac) {

	WARN("request_aux_receiver: %d", session);
	return NULL;
}

static GstElement* request_aux_sender_cb(GstElement* rtpbin, guint session, sipuac_t* sipuac) {

	WARN("request_aux_sender: %d", session);
	return NULL;
}

static GstElement* request_rtp_decoder_cb(GstElement* rtpbin, guint session, sipuac_t* sipuac) {

	WARN("request_rtp_decoder: %d", session);
	return NULL;
}

static GstElement* request_rtp_encoder_cb(GstElement* rtpbin, guint session, sipuac_t* sipuac) {

	WARN("request_rtp_encoder: %d", session);
	return NULL;
}

static void request_pt_map_cb(GstElement* rtpbin, GstPad* pad, sipuac_t* sipuac) {

	WARN("pt map: %s", GST_PAD_NAME(pad));
}

void out_media_destroy(void* data) {

	if (data) {
		
		free(data);
	}
}

void in_media_destroy(void* data) {

	if (data) {
		
		free(data);
	}
}

void media_create(sipuac_t* sipuac, session_t* session, const char* r_sdp_str, const char* *l_sdp_str) {

	GstSDPMessage* grsdp; gst_sdp_message_new(&grsdp);
	GstSDPMessage* glsdp; gst_sdp_message_new(&glsdp);

	if (gst_sdp_message_parse_buffer(r_sdp_str, strlen(r_sdp_str), grsdp) != GST_SDP_OK) {
		gst_sdp_message_free(grsdp);
		gst_sdp_message_free(glsdp);
		return;
	}

	const GstSDPConnection* conn = gst_sdp_message_get_connection(grsdp);
	const GstSDPOrigin* grorign = gst_sdp_message_get_origin(grsdp);

	locker_set(thread_locker(sipuac->thread), THREAD_LOCK_READ);
	gst_sdp_message_set_origin(glsdp, PACKAGE_NAME, NULL, NULL, grorign->nettype, grorign->addrtype, get_from_propes(thread_propes(sipuac->thread), "rtpip"));
	gst_sdp_message_set_connection(glsdp, grorign->nettype, grorign->addrtype, get_from_propes(thread_propes(sipuac->thread), "rtpip"), conn->ttl, conn->addr_number);
	locker_set(thread_locker(sipuac->thread), THREAD_UNLOCK_READ);

	gst_sdp_message_set_session_name(glsdp, PACKAGE);
	gst_sdp_message_set_version(glsdp, gst_sdp_message_get_version(grsdp));

	guint media_id = 0;
	while (media_id < gst_sdp_message_medias_len(grsdp)) {
		const GstSDPMedia *rmedia = gst_sdp_message_get_media(grsdp, media_id);
		if (rmedia == NULL)
			break;

		GstSDPMedia *lmedia; gst_sdp_media_new(&lmedia);

		guint band_id = 0;
		while (band_id < gst_sdp_media_bandwidths_len(rmedia)) {

			const GstSDPBandwidth *bw = gst_sdp_media_get_bandwidth(rmedia, band_id);
			gst_sdp_media_add_bandwidth(lmedia, bw->bwtype, bw->bandwidth);
			band_id ++;
		}

		guint format_id = 0;
		while (format_id < gst_sdp_media_formats_len(rmedia)) {
			const gchar *payload = gst_sdp_media_get_format(rmedia, format_id);
//			if (atoi(payload) == 101)
//				continue;
			gst_sdp_media_add_format(lmedia, payload);
			gst_sdp_media_set_media(lmedia, gst_sdp_media_get_media(rmedia));
			gst_sdp_media_set_proto(lmedia, gst_sdp_media_get_proto(rmedia));
			gst_sdp_media_set_port_info(lmedia, gst_sdp_media_get_port(rmedia), 1);

			guint attr_id = 0;
			while (attr_id < gst_sdp_media_attributes_len(rmedia)) {
				const GstSDPAttribute* att = gst_sdp_media_get_attribute(rmedia, attr_id);
				if (g_str_has_prefix(att->value, payload))
					gst_sdp_media_add_attribute(lmedia, att->key, att->value);
				attr_id ++;
			}

			GList* l;
			GstCaps* caps = gst_sdp_media_get_caps_from_media(rmedia, atoi(payload));
			GstStructure* s = gst_caps_get_structure(caps, 0);
			gst_structure_set_name(s, "application/x-rtp");

//			INFO("%s", gst_caps_to_string(caps));

			GstElement* pay     = NULL;
			GstElement* depay   = NULL;
			GstElement* decoder = NULL;
			GstElement* encoder = NULL;
/*
			if (l = gst_element_factory_list_filter(sipuac->factory.pay, caps, GST_PAD_SINK, FALSE)) {
				pay = gst_element_factory_create (l->data, NULL);
				INFO("created payloader element %p, %s", pay, gst_element_get_name(pay));
				gst_plugin_feature_list_free(l);
			}

			if (l = gst_element_factory_list_filter(sipuac->factory.depay, caps, GST_PAD_SRC, FALSE)) {
				depay = gst_element_factory_create (l->data, NULL);
				INFO("created depayloader element %p, %s", depay, gst_element_get_name(depay));
				gst_plugin_feature_list_free(l);
			}
*/
			if (l = gst_element_factory_list_filter(sipuac->factory.decoder, caps, GST_PAD_SINK, FALSE)) {
				decoder = gst_element_factory_create (l->data, NULL);
				INFO("created decoder element %p, %s", decoder, gst_element_get_name(decoder));
				gst_plugin_feature_list_free(l);
			}

			if (l = gst_element_factory_list_filter(sipuac->factory.encoder, caps, GST_PAD_SRC, FALSE)) {
				encoder = gst_element_factory_create (l->data, NULL);
				INFO("created depayloader element %p, %s", encoder, gst_element_get_name(encoder));
				gst_plugin_feature_list_free(l);
			}

			gst_caps_unref(caps);

			if (pay && depay && decoder && encoder) {
				session_out_t* out = calloc(1, sizeof(*out)); set_to_vector(session->out, out);
				out->udpsink = gst_element_factory_make("udpsink", NULL);
				out->pay     = gst_element_factory_make("payload", NULL);
				out->encoder = gst_element_factory_make("encoder", NULL);
				// link to rtpbin and mixers

				session_in_t* in  = calloc(1, sizeof(*in )); set_to_vector(session->in,  in );
				in->udpsrc   = gst_element_factory_make("udpsrc",  NULL);
				in->depay    = gst_element_factory_make("depay",   NULL);
				in->decoder  = gst_element_factory_make("decoder", NULL);
				// link to rtpbin and mixers
				break;
			}
			format_id ++;
		}

		gst_sdp_message_add_media(glsdp, lmedia);
		media_id ++;
	}
	*l_sdp_str = gst_sdp_message_as_text(glsdp);
	gst_sdp_message_free(grsdp);
	gst_sdp_message_free(glsdp);
	INFO("%s", r_sdp_str);
}

void sipuac_r_invite(int state, char const* phrase, nua_t* nua, sipuac_t* sipuac, nua_handle_t* nh, session_t* session, sip_t const* sip, tagi_t tags[]) {

	if (!session) {
		if (session = su_zalloc(sipuac->home, sizeof(*session))) {
			nua_handle_bind(session->nh = nh, session);
			session->callid = su_strdup(sipuac->home, sip->sip_call_id->i_id);
			session->in  = vector_create(0, in_media_destroy );
			session->out = vector_create(0, out_media_destroy);
		}

		else {
			nua_respond(nh, SIP_500_INTERNAL_SERVER_ERROR, TAG_END());
			nua_handle_destroy(nh);
		}
	}

	else {
		if (sip->sip_payload && sip->sip_payload->pl_data)
			nua_respond(nh, SIP_500_INTERNAL_SERVER_ERROR, TAG_END());
	}
}

void sipuac_i_invite(int state, char const* phrase, nua_t* nua, sipuac_t* sipuac, nua_handle_t* nh, session_t* session, sip_t const* sip, tagi_t tags[]) {

	if (!session) {
		if (session = su_zalloc(sipuac->home, sizeof(*session))) {
			nua_handle_bind(session->nh = nh, session);
			session->callid = su_strdup(sipuac->home, sip->sip_call_id->i_id);
			session->in  = vector_create(0, in_media_destroy );
			session->out = vector_create(0, out_media_destroy);
		}

		else {
			nua_respond(nh, SIP_500_INTERNAL_SERVER_ERROR, TAG_END());
			nua_handle_destroy(nh);
		}
	}

	else {
		if (sip->sip_payload && sip->sip_payload->pl_data)
			nua_respond(nh, SIP_500_INTERNAL_SERVER_ERROR, TAG_END());
	}
}

void sipuac_i_terminated(int state, char const* phrase, nua_t* nua, sipuac_t* sipuac, nua_handle_t *nh, session_t *session, sip_t const *sip, tagi_t tags[]) {

	if (session && session->nh) {
		if (session->callid)
			su_free(sipuac->home, session->callid);
		vector_destroy(session->in);
		vector_destroy(session->out);
		nua_handle_destroy(session->nh);
		session->nh = NULL;
	}

	su_free(sipuac->home, session);
}

void sipuac_i_state(int state, char const* phrase, nua_t* nua, sipuac_t* sipuac, nua_handle_t *nh, session_t *session, sip_t const *sip, tagi_t tags[]) {

	if (!session)
		return;

	int callstate;
	tl_gets(tags, NUTAG_CALLSTATE_REF(callstate), TAG_END());

	switch (callstate) {
		case nua_callstate_received: {


			const char* r_sdp = NULL;
			const char* l_sdp = NULL;
			tl_gets(tags, SOATAG_REMOTE_SDP_STR_REF(r_sdp), TAG_END());
			media_create(sipuac, session, r_sdp, &l_sdp);
			nua_respond(nh, SIP_200_OK, SOATAG_USER_SDP_STR(l_sdp), TAG_END());
			break;
		}

		case nua_callstate_terminated:  { break; }
		case nua_callstate_completed:   { break; }
		case nua_callstate_completing:  { break; }
		case nua_callstate_terminating: { break; }
		case nua_callstate_ready:       { break; }
		default:
			ERROR("unhandled SIP state %d", callstate);
	}
}

void sipuac_callback(nua_event_t event, int state, char const *phrase, nua_t *nua, sipuac_t *sipuac, nua_handle_t *nh, session_t *session, sip_t const *sip, tagi_t tags[]) {

	switch (event) {
		case nua_r_shutdown: {
			if (state >= 200)
				su_root_break(sipuac->root);
			break;
		}

		case nua_i_invite:
			sipuac_i_invite(state, phrase, nua, sipuac, nh, session, sip, tags);
			break;

		case nua_i_terminated:
			sipuac_i_terminated(state, phrase, nua, sipuac, nh, session, sip, tags);
			break;

		case nua_i_state:
			sipuac_i_state(state, phrase, nua, sipuac, nh, session, sip, tags);
			break;

		case nua_r_invite:
			sipuac_r_invite(state, phrase, nua, sipuac, nh, session, sip, tags);
			break;

		case nua_i_options:
			nua_respond(nh, SIP_200_OK, TAG_END());
			break;

		default:
			break;
	}
}

extern su_log_t tport_log[];
extern su_log_t iptsec_log[];
extern su_log_t nea_log[];
extern su_log_t nta_log[];
extern su_log_t nth_client_log[];
extern su_log_t nth_server_log[];
extern su_log_t nua_log[];
extern su_log_t soa_log[];
extern su_log_t sresolv_log[];

void sofia_logger(void *logarg, char const *format, va_list ap) {

	char buffer[2048];
	vsnprintf(buffer, sizeof(buffer), format, ap);
	buffer[strlen(buffer) - 1] = 0;
	DEBUG("%s", buffer);
}

void sipuac_routine(thread_t* thread) {

	su_init();
	sipuac_t* sipuac = thread_data(thread);
	sipuac->root = su_root_create(NULL);
	sipuac->home = su_home_create();

	sipuac->nua = nua_create(sipuac->root, sipuac_callback, sipuac,
		NUTAG_URL(get_from_propes(sipuac->propes, "bind_url")),
		SIPTAG_USER_AGENT_STR(get_from_propes(sipuac->propes, "user_agent")),
		SIPTAG_ORGANIZATION_STR(get_from_propes(sipuac->propes, "organization")),
		NTATAG_SIP_T1X64(atoi(get_from_propes(sipuac->propes, "timer_t1x64"))),
		NTATAG_SIP_T1(atoi(get_from_propes(sipuac->propes, "timer_t1"))),
		NTATAG_SIP_T2(atoi(get_from_propes(sipuac->propes, "timer_t2"))),
		NTATAG_SIP_T4(atoi(get_from_propes(sipuac->propes, "timer_t4"))),
		NTATAG_RPORT(atoi(get_from_propes(sipuac->propes, "use_rport"))),
		TAG_END()
	);

	if (sipuac->nua) {
		su_log_redirect(su_log_default, sofia_logger, NULL);
		su_log_redirect(tport_log, sofia_logger, NULL);
		su_log_redirect(iptsec_log, sofia_logger, NULL);
		su_log_redirect(nea_log, sofia_logger, NULL);
		su_log_redirect(nta_log, sofia_logger, NULL);
		su_log_redirect(nua_log, sofia_logger, NULL);
		su_log_redirect(soa_log, sofia_logger, NULL);
		su_log_redirect(nth_client_log, sofia_logger, NULL);
		su_log_redirect(nth_server_log, sofia_logger, NULL);
		su_log_redirect(sresolv_log, sofia_logger, NULL);

		thread_update(thread, THREAD_STATE_STARTED);
		su_root_run(sipuac->root);
		nua_destroy(sipuac->nua);
		sipuac->nua = NULL;
	}

	else
		thread_update(thread, THREAD_STATE_INVALID);

	su_home_unref(sipuac->home);
	su_root_destroy(sipuac->root);
	sipuac->root = NULL;
	su_deinit();
}

void media_thread(sipuac_t* sipuac) {

	gst_element_set_state(sipuac->media.pipe, GST_STATE_PLAYING);
	if (gst_element_get_state (sipuac->media.pipe, NULL, NULL, -1) == GST_STATE_CHANGE_FAILURE)
		ERROR("Failed to go into PLAYING state");
	else	g_main_loop_run(sipuac->media.loop);
	gst_element_set_state(sipuac->media.pipe, GST_STATE_NULL);

	thread_update(sipuac->thread, THREAD_STATE_INVALID);
}

void* on_sipuac_create(thread_t* thread) {

	sipuac_t* sipuac = calloc(1, sizeof(sipuac_t));
	if (sipuac) {
		sipuac->thread = thread;
		sipuac->propes = thread_propes(thread);
		sipuac->factory.pay     = gst_element_factory_list_get_elements(GST_ELEMENT_FACTORY_TYPE_PAYLOADER,   GST_RANK_NONE);
		sipuac->factory.depay   = gst_element_factory_list_get_elements(GST_ELEMENT_FACTORY_TYPE_DEPAYLOADER, GST_RANK_NONE);
		sipuac->factory.decoder = gst_element_factory_list_get_elements(GST_ELEMENT_FACTORY_TYPE_DECODER,     GST_RANK_NONE);
		sipuac->factory.encoder = gst_element_factory_list_get_elements(GST_ELEMENT_FACTORY_TYPE_ENCODER,     GST_RANK_NONE);
	}

	return sipuac;
}

void on_sipuac_destroy(thread_t* thread) {

	sipuac_t* sipuac = thread_data(thread);
	if (sipuac) {
		gst_plugin_feature_list_free(sipuac->factory.pay);
		gst_plugin_feature_list_free(sipuac->factory.depay);
		gst_plugin_feature_list_free(sipuac->factory.decoder);
		gst_plugin_feature_list_free(sipuac->factory.encoder);
		free(sipuac);
	}
}

void on_sipuac_start(thread_t* thread) {

	sipuac_t* sipuac = thread_data(thread);
	sipuac->media.loop = g_main_loop_new(NULL, FALSE);
	sipuac->media.pipe = gst_pipeline_new("pipeline");

	gst_bin_add(GST_BIN(sipuac->media.pipe), sipuac->media.rtpbin = gst_element_factory_make("rtpbin", "rtpbin"));
	gst_bin_add(GST_BIN(sipuac->media.pipe), sipuac->media.amixer = gst_element_factory_make("audiomixer", "audiomixer"));
	gst_bin_add(GST_BIN(sipuac->media.pipe), sipuac->media.vmixer = gst_element_factory_make("videomixer", "videomixer"));

	GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(sipuac->media.pipe));
	gst_bus_add_watch(bus, bus_callback, sipuac);
	gst_object_unref(bus);

	g_signal_connect(sipuac->media.rtpbin, "pad-added",           G_CALLBACK(pad_added_cb),           sipuac);
	g_signal_connect(sipuac->media.rtpbin, "pad-removed",         G_CALLBACK(pad_removed_cb),         sipuac);
	g_signal_connect(sipuac->media.rtpbin, "request-aux-receiver",G_CALLBACK(request_aux_receiver_cb),sipuac);
	g_signal_connect(sipuac->media.rtpbin, "request-aux-sender",  G_CALLBACK(request_aux_sender_cb),  sipuac);
	g_signal_connect(sipuac->media.rtpbin, "request-rtp-decoder", G_CALLBACK(request_rtp_decoder_cb), sipuac);
	g_signal_connect(sipuac->media.rtpbin, "request-rtp-encoder", G_CALLBACK(request_rtp_encoder_cb), sipuac);
	g_signal_connect(sipuac->media.rtpbin, "request-pt-map",      G_CALLBACK(request_pt_map_cb),      sipuac);

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_create(&sipuac->media.td, &attr, (void*(*)(void*))media_thread, sipuac);
	pthread_attr_destroy(&attr);
}

void on_sipuac_stop(thread_t* thread) {

	sipuac_t* sipuac = thread_data(thread);
	if (sipuac && sipuac->nua) {
		nua_shutdown(sipuac->nua);

		if (sipuac->media.loop) {
			if (g_main_loop_is_running(sipuac->media.loop))
				g_main_loop_quit(sipuac->media.loop);
			g_main_loop_unref(sipuac->media.loop);
			sipuac->media.loop = NULL;
		}

		if (sipuac->media.pipe) {
			gst_object_unref(GST_OBJECT(sipuac->media.pipe));
			sipuac->media.pipe = NULL;
		}

		if (sipuac->media.td) {
			pthread_join(sipuac->media.td, NULL);
			sipuac->media.td = 0;
		}
	}
}

void on_sipuac_init() {

	gst_init(0, NULL);
}

property_t sipuac_props[] = {

	{	"bind_url",		"sip:0.0.0.0:5060",		"bind url",				check_type_str		},
	{	"organization",		"HAPPY Business Company",	"organization header",			check_type_str		},
	{	"user_agent",		PACKAGE_STRING,			"user agent name",			check_type_str		},
	{	"timer_t1x64",		"32000",			"RFC 3261 t1x64 timer",			check_type_int		},
	{	"timer_t1",		"500",				"RFC 3261 t1 timer",			check_type_int		},
	{	"timer_t2",		"4000",				"RFC 3261 t3 timer",			check_type_int		},
	{	"timer_t4",		"4000",				"RFC 3261 t4 timer",			check_type_int		},
	{	"use_rport",		"1",				"use rport parameter",			check_type_int		},
	{	"rtpip",		"127.0.0.1",			"media rtp address",			check_type_ip		},
	{	NULL, NULL, NULL, NULL }
};

module_t sipuac = {

	.name		= "sipuac",
	.description	= "SIP v2.0 User Agent (sofia)",

	.routine_f	= sipuac_routine,
	.methods	= NULL,
	.props		= sipuac_props,

	.on_create_f	= on_sipuac_create,
	.on_destroy_f	= on_sipuac_destroy,
	.on_start_f	= on_sipuac_start,
	.on_stop_f	= on_sipuac_stop,

	.on_save_f	= NULL,
	.on_init_f	= on_sipuac_init,
};

int main(int argc, char* argv[]) {

	setConsoleLog(1);
	setDebugMode(1);

	sipuac.on_init_f();

	thread_t* thread = thread_create(&sipuac, "tester");

	set_to_propes(thread_propes(thread), "rtpip", "127.0.0.1");
	set_to_propes(thread_propes(thread), "bind_url", "sip:127.0.0.1:5060;transport=udp");
	set_to_propes(thread_propes(thread), "organization", "AO 'Roga and kopita'");

	thread_state_set(thread, THREAD_STATE_STARTED);
	thread_run_wait(thread);
	thread_destroy(thread);

	return 0;
}
