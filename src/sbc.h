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

/* Settings for UDP transport*/
#define AF                  pj_AF_INET()
#define SBC_PORT            7777
#define RTP_PORT            4020
#define PORT2               8888
#define MAX_MEDIA_CNT       1        /* Media count, set to 1 for aud 2 for aud & video */
#define TIMEOUT_EVENTS_MS   5000

/* for all application */
struct app_confg_t
{
    pj_pool_t                   *pool;      /* pool for media */
    // pjsip_inv_callback          inv_cb;     /* callback funtion */

    // pj_sockaddr                 hostaddr;    host address 
    // pj_str_t                    local_uri;  /* 
    pjsip_dialog                *dlg_inv;
    pjsip_dialog                *dlg_out;

} app_cfg;

/* define prototypes of func */

static void sbc_perror(const char *sender, const char *title, pj_status_t status);


static void call_on_state_changed( pjsip_inv_session *inv, pjsip_event *e);
static void call_on_forked(pjsip_inv_session *inv, pjsip_event *e);
static void call_on_media_update( pjsip_inv_session *inv, pj_status_t status);

static pj_status_t main_init(void);
static pj_status_t sbc_init(void);
static pj_status_t sbc_destroy(void);
static pj_status_t sbc_global_endpt_create(void);
static pj_status_t sbc_udp_transport_create(void);
static pj_status_t sbc_invite_mod_create(void);
static pj_status_t sbc_media_init(void);


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
