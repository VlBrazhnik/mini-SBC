#pragma once

/* PJSUA framework */
#include <pjlib.h>
#include <pjlib-util.h>
#include <pjmedia.h>
#include <pjsua.h>
#include <pjmedia-codec.h>
#include <pjsip.h>
#include <pjsip_simple.h>
#include <pjsip_ua.h>
#include <pjsua-lib/pjsua.h>

#include "util.h"

/* for LOG */
#define THIS_FILE           "SBC_mini"
#define ROUTE_ADDR          "<sip:winehouse@10.25.72.86:5062>"
/* Settings for UDP transport*/
#define AF                  pj_AF_INET()
#define SBC_PORT            7777
#define SBC_PORT2           7779
#define RTP_PORT            4020
#define PORT2               8888
#define MAX_MEDIA_CNT       1        /* Media count, set to 1 for aud 2 for aud & video */
#define TIMEOUT_EVENTS_MS   5000

/* for all application */

/* define prototypes of func */

static void sbc_perror(const char *sender, const char *title, pj_status_t status);


static void call_on_state_changed( pjsip_inv_session *inv, pjsip_event *e);
static void call_on_forked(pjsip_inv_session *inv, pjsip_event *e);


static pj_status_t main_init(void);
static pj_status_t sbc_init(void);
static pj_status_t sbc_destroy(void);
static pj_status_t sbc_global_endpt_create(void);
static pj_status_t sbc_udp_transport_create(void);
static pj_status_t sbc_invite_mod_create(void);


/* Handler for INVITE request */
static pj_bool_t sbc_invite_handler(pjsip_rx_data *rdata);
static pj_bool_t sbc_request_inv_send(pjsip_rx_data *rdata);
static pj_bool_t sbc_response_code_send(pjsip_rx_data *rdata, unsigned code);

/* Logging */
static pj_status_t logging_on_tx_msg(pjsip_tx_data *tdata);
static pj_bool_t logging_on_rx_msg(pjsip_rx_data *rdata);
static void on_tsx_state( pjsip_transaction *tsx, pjsip_event *event);


/* Callback to be called to handle incoming requests outside dialogs: */
static pj_bool_t on_rx_request( pjsip_rx_data *rdata );
static pj_bool_t on_rx_response( pjsip_rx_data *rdata);
