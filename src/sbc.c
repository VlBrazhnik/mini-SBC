#include "sbc.h"

/*
 *    Static variables 
 */
static pj_caching_pool          cash_pool;      /* Global pool factory */
static pjsip_endpoint           *g_endpt;       /* SIP endpoint        */
static pjmedia_endpt            *g_med_endpt;   /* Media endpoint      */
static pj_bool_t                g_complete;     /* Quit flag           */

/* Call variables */
static pjsip_inv_session        *g_inv;         /* Current invite session A <-> SBC */
static pjsip_inv_session        *g_out;         /* SBC <-> B side */

/* Init PJSIP module to be registered by application to handle
 * incoming requests outside any dialogs/transactions
 */
static pjsip_module mod_sbc =
{
    NULL, NULL,                 /* prev, next.      */
    { "mini-sbc", 10 },         /* Name.            */
    -1,                         /* Id           */
    PJSIP_MOD_PRIORITY_APPLICATION, /* Priority         */
    NULL,                       /* load()           */
    NULL,                       /* start()          */
    NULL,                       /* stop()           */
    NULL,                       /* unload()         */
    &on_rx_request,             /* &on_rx_request()      */
    &on_rx_response,            /* &on_rx_response()     */
    NULL,                       /* on_tx_request.       */
    NULL,                       /* &on_tx_response()     */
    &on_tsx_state,              /* &on_tsx_state()       */
};

/* The module for logging messages. */
static pjsip_module msg_logger = 
{
    NULL, NULL,                 /* prev, next.      */
    { "mod-msg-log", 13 },      /* Name.        */
    -1,                         /* Id           */
    PJSIP_MOD_PRIORITY_TRANSPORT_LAYER-1,/* Priority            */
    NULL,                       /* load()       */
    NULL,                       /* start()      */
    NULL,                       /* stop()       */
    NULL,                       /* unload()     */
    &logging_on_rx_msg,         /* on_rx_request()  */
    &logging_on_rx_msg,         /* on_rx_response() */
    &logging_on_tx_msg,         /* on_tx_request.   */
    &logging_on_tx_msg,         /* on_tx_response() */
    NULL,                       /* on_tsx_state()   */
};

int 
main(int argc, char *argv[])
{
    pj_status_t status; 

    /* init application */
    status = main_init();
    if (status != PJ_SUCCESS)
    {
        sbc_perror(THIS_FILE, "Eror in main_init()", status);
        return PJ_FALSE;
    }

    PJ_LOG(3, (THIS_FILE, "Press: Cntrl+C for quit\n"));
    /* Loop */
    while(1)
    {
        pj_time_val timeout = {0, TIMEOUT_EVENTS_MS};
        status = pjsip_endpt_handle_events(g_endpt, &timeout);
        if (status != PJ_SUCCESS)
        {
            sbc_perror(THIS_FILE, "Error in handle_events()", status);
            break;
        }
    }

    status = sbc_destroy();
    if (status != PJ_SUCCESS)
    {
        sbc_perror(THIS_FILE, "Error in sbc_destroy()", status);
    }

    return status;
}

static pj_status_t main_init(void)
{
    pj_status_t status; 

    status = sbc_init();
    if (status != PJ_SUCCESS)
        sbc_perror(THIS_FILE, "Error in sbc_init()", status);

    status = sbc_global_endpt_create();
    if (status != PJ_SUCCESS)
        sbc_perror(THIS_FILE, "Error in global_endpt_create()", status);

    status = sbc_udp_transport_create();
    if (status != PJ_SUCCESS)
        sbc_perror(THIS_FILE, "Error in sbc_udp_transport_create()", status);

    /*
     * Init High-Level dialog API
     */
    status = sbc_invite_mod_create();
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);

    /* 
     * Init transaction layer.
     * This will create/initialize transaction hash tables etc.
     */
    status = pjsip_tsx_layer_init_module(g_endpt);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);

    /* 
     * Initialize UA layer module.
     * This will create/initialize dialog hash tables etc.
     */
    status = pjsip_ua_init_module(g_endpt, NULL );
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);

    /*
     * Register message logger module.
     */
    status = pjsip_endpt_register_module(g_endpt, &msg_logger);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);

    /*
     * Register our module to receive incoming requests.
     */
    status = pjsip_endpt_register_module(g_endpt, &mod_sbc);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);

    return status;
}

/* init application data */
static pj_status_t sbc_init(void)
{
    pj_status_t status;

    /* Init PJLIB first */
    status = pj_init();
    if (status != PJ_SUCCESS)
    {
        sbc_perror("PJ_INIT", "Error: ", status);
    }

    pj_log_set_level(5);

    /* Init PJLIB-UTIL */
    status = pjlib_util_init();
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);

    /* Create a pool factory before allocate memory */
    pj_caching_pool_init(&cash_pool, &pj_pool_factory_default_policy, 0);

    /* Logging success */
    PJ_LOG(3, (THIS_FILE, "initialized successfully\n"));

    return status;
}

/*init global endpoint */
static pj_status_t sbc_global_endpt_create(void)
{
    pj_status_t status;
    const pj_str_t      *hostname; /* hostname for global endpoint */
    const char          *endpt_name;

    /* use hostname for simplicity */

    hostname = pj_gethostname();
    endpt_name = hostname->ptr;

    /* Create the global endpoint */

    status = pjsip_endpt_create(&cash_pool.factory, endpt_name, &g_endpt);
    if (status != PJ_SUCCESS)
    {
        sbc_perror(THIS_FILE, "Global endpt not create!", status);
    }

    PJ_LOG(3, (THIS_FILE, "Global endpoint create!\n"));

    return status;
}

static pj_status_t sbc_udp_transport_create(void)
{
    pj_status_t status;
    pj_sockaddr addr;
    pj_int32_t af = AF;

    /* Socket init */
    pj_sockaddr_init(af, &addr, NULL, (pj_uint16_t)SBC_PORT);

    status = pjsip_udp_transport_start(g_endpt, &addr.ipv4, NULL, 1, NULL);
    if (status != PJ_SUCCESS)
    {
        sbc_perror(THIS_FILE, "Unable to start UDP transport", status);
    }

    return status;
}

static pj_status_t sbc_destroy(void)
{
    pj_status_t status;
    pj_int32_t i = 0;

    /* On exit, dump current memory usage: */
    dump_pool_usage(THIS_FILE, &cash_pool);

    /* Destroy the audio port first before the stream since 
     * the audio port has threads that get/put frames to the stream.
     */
    if (g_snd_port)
        pjmedia_snd_port_destroy(g_snd_port);

    /* Destroy streams */
    if (g_med_stream)
        pjmedia_stream_destroy(g_med_stream);

    /* Destroy media transports */
    for (i = 0; i < MAX_MEDIA_CNT; i++) 
    {
        if (g_med_transport[i])
            pjmedia_transport_close(g_med_transport[i]);
    }

    /*
     * Check INVITE session A <-> SBC
     */
    // if (g_inv)
    // {
    //     pjsip_tx_data *p_tdata;
    //     status = pjsip_inv_end_session(g_inv, PJSIP_SC_NOT_ACCEPTABLE, NULL, &p_tdata);
    //     if (status != PJ_SUCCESS)
    //         sbc_perror(THIS_FILE, "Unable terminate A session", status);

    //     status = pjsip_inv_send_msg(g_inv, p_tdata);
    //     if (status != PJ_SUCCESS)
    //         sbc_perror(THIS_FILE, "Unable send terminate msg A", status);
    // }

    /* Destroy event manager */
    // pjmedia_event_mgr_destroy(NULL); 

    /* Deinit pjmedia endpoint */
    if (g_med_endpt)
        pjmedia_endpt_destroy(g_med_endpt);

    /* Deinit pjsip endpoint */
    if (g_endpt)
        pjsip_endpt_destroy(g_endpt);
        g_endpt = NULL;

    /* Release pool */
    if (app_cfg.pool)
        pj_pool_release(app_cfg.pool);
        app_cfg.pool = NULL;
        PJ_LOG(3, (THIS_FILE, "Peak memory size: %uMB", 
            cash_pool.peak_used_size / 1000000));

    pj_caching_pool_destroy(&cash_pool);

    /* Shutdown PJLIB */
    pj_shutdown();

    return status;
}

static pj_status_t sbc_invite_mod_create(void)
{
    pj_status_t status;
    pjsip_inv_callback inv_cb;

    /* Init the callback for INVITE */
    pj_bzero(&inv_cb, sizeof(inv_cb));
    inv_cb.on_state_changed = &call_on_state_changed;
    inv_cb.on_new_session = &call_on_forked;

    /* Initialize invite session module:  */
    status = pjsip_inv_usage_init(g_endpt, &inv_cb);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);

    return status;
}

static pj_bool_t sbc_invite_handler(pjsip_rx_data *rdata)
{
    pj_status_t         status;
    pjsip_tx_data       *p_tdata;
    unsigned            options = 0;
    pj_str_t            local_uri;
    pj_sockaddr         hostaddr;
    char                hostip[PJ_INET6_ADDRSTRLEN+2];
    char                temp[80] = {0};
    pjsip_dialog        *uas_dlg;

    /* 
     * Respond (statelessly) any non-INVITE requests with 500 
     */
    if (rdata->msg_info.msg->line.req.method.id != PJSIP_INVITE_METHOD) 
    {
        if (rdata->msg_info.msg->line.req.method.id != PJSIP_ACK_METHOD) 
        {
            pj_str_t reason = pj_str("Simple UA unable to handle this request");
            pjsip_endpt_respond_stateless( g_endpt, rdata, 500, &reason,
                           NULL, NULL);
        }
        return PJ_TRUE;
    }

    /*
     * Reject INVITE if we already have an INVITE session in progress.
     */
    if (g_inv)
    { 
        pj_str_t reason = pj_str("Another call is in progress");
        pjsip_endpt_respond_stateless(g_endpt, rdata, 500, &reason,
                           NULL, NULL);
        return PJ_TRUE;
    }

    /* 
     * Verify that we can handle the request 
     */
    status = pjsip_inv_verify_request(rdata, &options, NULL, NULL,
                                    g_endpt, NULL);
    if (status != PJ_SUCCESS) 
    {
        pj_str_t reason = pj_str("Sorry UA can't handle this INVITE");
        pjsip_endpt_respond_stateless( g_endpt, rdata, 500, &reason,
                           NULL, NULL);
        sbc_perror(THIS_FILE, "shutdown application", status);
    }

    /*
     * Get host URI
     */
    if (pj_gethostip(AF, &hostaddr) != PJ_SUCCESS)
        sbc_perror(THIS_FILE, "Unable to retrieve local host IP", status);
    pj_sockaddr_print(&hostaddr, hostip, sizeof(hostip), 2);
    pj_ansi_sprintf(temp, "<sip:sbc@%s:%d>", hostip, SBC_PORT);
    local_uri = pj_str(temp);

    /*
     * Create UAS dialog
     */
    status = pjsip_dlg_create_uas_and_inc_lock(pjsip_ua_instance(),
                        rdata, &local_uri, &uas_dlg);
    if (status != PJ_SUCCESS) 
    {
        pjsip_endpt_respond_stateless(g_endpt, rdata, 500, NULL,
                          NULL, NULL);
        sbc_perror(THIS_FILE, "shutdown application", status);
    }

    /*
     * Add application module to dialog usages
     */
    status = pjsip_dlg_add_usage(uas_dlg, &mod_sbc, NULL);
    if (status != PJ_SUCCESS)
    {
        pjsip_dlg_dec_lock(uas_dlg);
        sbc_perror(THIS_FILE, "shutdown application", status);
    }
 
    /* 
     * Create invite session, and pass both the UAS dialog
     */
    status = pjsip_inv_create_uas( uas_dlg, rdata, NULL, 0, &g_inv);
    pj_assert(status == PJ_SUCCESS);
    if (status != PJ_SUCCESS) 
    {
        pjsip_dlg_dec_lock(uas_dlg);
        sbc_perror(THIS_FILE, "shutdown application", status);
    }

    /*
     * Invite session has been created, decrement & release dialog lock.
     */
    pjsip_dlg_dec_lock(uas_dlg);

    /*
     * Get SDP body
     */ 
    pjsip_rdata_sdp_info *sdp_info;
    sdp_info = pjsip_rdata_get_sdp_info(rdata);

    /*
     * Set local SDP offer / answer for g_inv
     */
    status = pjsip_inv_set_local_sdp(g_inv, sdp_info->sdp);
    if (status != PJ_SUCCESS)
        sbc_perror(THIS_FILE, "Error local_SDP\n", status);

    /*
     * Initially first response & send 100 trying
     */
    status = pjsip_inv_initial_answer(g_inv, rdata, PJSIP_SC_TRYING, NULL, NULL, &p_tdata);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, PJ_TRUE);

    /* Send the 100 response. */  
    status = pjsip_inv_send_msg(g_inv, p_tdata); 
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, PJ_TRUE);

    /*
     * Send INVITE to other side
     */
    sbc_request_inv_send(rdata);

    return PJ_TRUE;
}

/*
 * SBC routing to other network interface
 */
static pj_bool_t sbc_request_inv_send(pjsip_rx_data *rdata)
{
    pj_status_t         status;
    pjsip_tx_data       *p_tdata;
    pjsip_dialog        *uac_dlg;
    pj_str_t            local_uri = pj_str("<sip:vlbrazhnikov@10.25.72.130:6020>");
    pj_str_t            contact_uri = pj_str("<sip:sbc@10.25.72.130:7777>");
    pj_str_t            dest_uri  = pj_str("<sip:winehouse@10.25.72.75:5062>");

    /* 
     * We already verify request, just create UAC
     */
    status = pjsip_dlg_create_uac(pjsip_ua_instance(), 
                        &local_uri,
                        &local_uri,
                        &dest_uri,
                        &dest_uri,
                        &uac_dlg);
    if (status != PJ_SUCCESS)
        sbc_perror(THIS_FILE, "Unable create UAC", status);

    pjsip_dlg_inc_lock(uac_dlg);

    /*
     * Add application module to dialog usages
     */
    status = pjsip_dlg_add_usage(uac_dlg, &mod_sbc, NULL);
    if (status != PJ_SUCCESS)
    {
        pjsip_dlg_dec_lock(uac_dlg);
        sbc_perror(THIS_FILE, "shutdown application", status);
    }

    /* 
     * Create the INVITE session for B side
     */
    status = pjsip_inv_create_uac(uac_dlg, NULL, 0, &g_out);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);

     /*
     * Get SDP body
     */ 
    pjsip_rdata_sdp_info *sdp_info;

    sdp_info = pjsip_rdata_get_sdp_info(rdata);

    /*
     * Set local SDP offer / answer for g_inv
     */
    status = pjsip_inv_set_local_sdp(g_out, sdp_info->sdp);
    if (status != PJ_SUCCESS)
        sbc_perror(THIS_FILE, "Error local_SDP\n", status);

    /*
     * Create INVITE request 
     */
    status = pjsip_inv_invite(g_out, &p_tdata);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);

    /*
     * Send INVITE to B
     */
    status = pjsip_inv_send_msg(g_out, p_tdata);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);

    return PJ_TRUE;
}

/*
 * SBC send response to B side
 */
static pj_bool_t sbc_response_code_send(pjsip_rx_data * rdata, unsigned code)
{
    pj_status_t                 status;
    pjsip_tx_data               *p_tdata;
    pjsip_rdata_sdp_info        *sdp_info_b;
    pjsip_msg_body              *p_body;

    /*
     * Before we sent response, SBC SHOULD recive response and handling it
     */

    /* 
     * get from rdata sdp_info->pjmedia_sdp_session
     * with pjsip_rdata_get_sdp_info(rdata);
     * create msg_body for A side with pjsip_create_sdp_body();
     * set msg_body for A?
     */
    sdp_info_b = pjsip_rdata_get_sdp_info(rdata);

    PJ_LOG(3, (THIS_FILE, "sent response_code to A side\n"));

    status = pjsip_inv_answer(g_inv, code, NULL, NULL, &p_tdata);
    if (status != PJ_SUCCESS)
        sbc_perror(THIS_FILE, "Unable create response A", status);

    if (code == 200)
    {
        status = pjsip_create_sdp_body(p_tdata->pool, 
                                sdp_info_b->sdp,
                                &p_body);
        if (status != PJ_SUCCESS)
            sbc_perror(THIS_FILE, "Error in create_sdp_body", status);

        /* set new body */
        pj_size_t size = 100;
        char buf[size];
        p_tdata->msg->body = p_body;
        p_tdata->msg->body->print_body(p_tdata->msg->body, buf, size);
        PJ_LOG(3, (THIS_FILE, "%s, len: %lu", buf, size));
    }

    status = pjsip_inv_send_msg(g_inv, p_tdata);
    if (status != PJ_SUCCESS)
        sbc_perror(THIS_FILE, "Unable send response A", status);

    return PJ_TRUE;
}

/* 
 * SBC recive incoming request from A side and handling it
 */
static pj_bool_t on_rx_request( pjsip_rx_data *rdata )
{
    // pj_status_t status;
    switch (rdata->msg_info.msg->line.req.method.id)
    {
        case PJSIP_INVITE_METHOD:
            sbc_invite_handler(rdata);
            break;
        
        default:
            PJ_LOG(3, (THIS_FILE, "default \n"));
            break;
    }
    return PJ_TRUE;
}

/*
 * Recive response from B side
 */
static pj_bool_t on_rx_response( pjsip_rx_data *rdata)
{
    pj_status_t     status;
    unsigned        response_code = rdata->msg_info.msg->line.status.code;

    PJ_LOG(3, (THIS_FILE, "Should check rdata in on_rx_response!\n"));

    if (g_inv != NULL)
    {
        switch(response_code)
        {
            case PJSIP_SC_RINGING:
                sbc_response_code_send(rdata, PJSIP_SC_RINGING);
                break;

            case PJSIP_SC_OK:
                sbc_response_code_send(rdata, PJSIP_SC_OK);
                break;
            default:
                PJ_LOG(3, (THIS_FILE, "response not found", status));
        }
    }
    else
    {
        PJ_LOG(6, (THIS_FILE, "session A<->SBC already TERMINATED\n"));
        return PJ_FALSE;
    }
    return PJ_TRUE;
}

/*
 * Transaction state
 */
static void on_tsx_state( pjsip_transaction *tsx, pjsip_event *event)
{
    pj_assert(event->type == PJSIP_EVENT_TSX_STATE);
    PJ_LOG(3, (THIS_FILE, "Transaction %s: state changed to %s",
                            tsx->obj_name, pjsip_tsx_state_str(tsx->state)));
}

/*
 * Callback when INVITE session state has changed.
 * After invite session module is initialized.
 * If invite session has been disconnected, we can quit the application.
 */
static void call_on_state_changed( pjsip_inv_session *inv, pjsip_event *e)
{
    pj_status_t         status;
    pjsip_tx_data       *p_tdata;
    PJ_UNUSED_ARG(e);

    if (inv->state == PJSIP_INV_STATE_DISCONNECTED) 
    {
        PJ_LOG(3,(THIS_FILE, "Call DISCONNECTED [reason=%d (%s)]", 
                    inv->cause,
                    pjsip_get_status_text(inv->cause)->ptr));

        PJ_LOG(6, (THIS_FILE, "INV role: %s", inv->role));

        if (inv == g_inv)
        {
            PJ_LOG(3, (THIS_FILE, "A sent BYE SBC, SBC sent BYE B"));

            status = pjsip_inv_end_session(g_out, inv->cause, NULL, &p_tdata);
            if (status != PJ_SUCCESS)
                sbc_perror(THIS_FILE, "Error end_session()", status);

            status = pjsip_inv_send_msg(g_out, p_tdata);
            if (status != PJ_SUCCESS)
                sbc_perror(THIS_FILE, "Error sent BYE to B", status);

            do
            {
                status = pjsip_inv_dec_ref(g_inv);
            }
            while (status != PJ_EGONE);
            g_inv = NULL;
            PJ_LOG(6, (THIS_FILE, "g_inv is destroyed\n"));
        } 
    }
    else 
    {
        PJ_LOG(3,(THIS_FILE, "Call state changed to %s", 
                    pjsip_inv_state_name(inv->state)));
    }
}

/* This callback is called when dialog has forked. */
static void call_on_forked(pjsip_inv_session *inv, pjsip_event *e)
{
    /* To be done... */
    PJ_UNUSED_ARG(inv);
    PJ_UNUSED_ARG(e);
}

/* Notification on outgoing messages */
static pj_status_t logging_on_tx_msg(pjsip_tx_data *tdata)
{
    
    /* Important note:
     *  tp_info field is only valid after outgoing messages has passed
     *  transport layer. So don't try to access tp_info when the module
     *  has lower priority than transport layer.
     */

    PJ_LOG(4,("-LOG-", "TX %d bytes %s to %s %s:%d:\n"
             "%.*s\n"
             "--end msg--",
             (tdata->buf.cur - tdata->buf.start),
             pjsip_tx_data_get_info(tdata),
             tdata->tp_info.transport->type_name,
             tdata->tp_info.dst_name,
             tdata->tp_info.dst_port,
             (int)(tdata->buf.cur - tdata->buf.start),
             tdata->buf.start));

    /* Always return success, otherwise message will not get sent! */
    return PJ_SUCCESS;
}

/* Notification on incoming messages */
static pj_bool_t logging_on_rx_msg(pjsip_rx_data *rdata)
{
    PJ_LOG(4,("-LOG-", "RX %d bytes %s from %s %s:%d:\n"
             "%.*s\n"
             "--end msg--",
             rdata->msg_info.len,
             pjsip_rx_data_get_info(rdata),
             rdata->tp_info.transport->type_name,
             rdata->pkt_info.src_name,
             rdata->pkt_info.src_port,
             (int)rdata->msg_info.len,
             rdata->msg_info.msg_buf));

    /* Always return false, otherwise messages will not get processed! */
    return PJ_FALSE;
}

static void sbc_perror(const char *sender, const char *title, 
                pj_status_t status)
{
    char errmsg[PJ_ERR_MSG_SIZE];

    pj_strerror(status, errmsg, sizeof(errmsg));
    PJ_LOG(1,(sender, "%s: %s [code=%d]", title, errmsg, status));
    sbc_destroy();
}
