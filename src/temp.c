/*
 * SBC routing to other network interface
 */
static pj_bool_t sbc_request_inv_send(pjsip_rx_data *rdata)
{
    pj_status_t status;
    pjsip_tx_data *tdata;
    pjsip_transaction *tsx;
    unsigned timeout_ms = TIME_RESEND_REQUEST_MS;

    pj_str_t target = pj_str("sip:winehouse@10.25.72.75:5062");
    pj_str_t from   = pj_str("sip:sbc@10.25.72.130:7777");
    pj_str_t contact= pj_str("sip:Remark@10.25.72.130:5060");

    /*
     * We should recive tdata from rdata and change our headers
     */

    // Create independet request 
    status = pjsip_endpt_create_request(g_endpt,
                        pjsip_get_invite_method(),
                        &target,
                        &from,
                        &target,
                        &contact,
                        NULL,
                        -1,
                        NULL,
                        &tdata);
    if (status != PJ_SUCCESS)
        sbc_perror(THIS_FILE, "Unable to create request", status);

    // Create transaction
    status = pjsip_tsx_create_uac(&mod_sbc, tdata, &tsx);
    if (status != PJ_SUCCESS)
        sbc_perror(THIS_FILE, "Unable to create UAC transaction", status);

    // Can modified request before send

    // stop retransmit request
    status = pjsip_tsx_stop_retransmit(tsx);
    if (status != PJ_SUCCESS)
        sbc_perror(THIS_FILE, "Unable to stop retransmission", status);

    status = pjsip_tsx_set_timeout( tsx, timeout_ms);
    if (status != PJ_SUCCESS)
        sbc_perror(THIS_FILE, "Unable set up timeout_ms", status);

    // Send request to B side
    status = pjsip_tsx_send_msg(tsx, tdata);

    return PJ_TRUE;
}

/*
 * INVITE HANDLER
 */
static pj_bool_t sbc_invite_handler(pjsip_rx_data *rdata)
{
    pj_status_t status;
    pj_str_t st_text = pj_str("Trying to INVITE B side");
    pjsip_tx_data *tdata;
    pjsip_uri *dest;
    pjsip_transaction *uas_tsx, *uac_tsx;

    // save recive data from A side
    app_cfg.rdataA = rdata;
    PJ_LOG(3, (THIS_FILE, "saved recived data from A side\n"));

    // Create uas transaction
    status = pjsip_tsx_create_uas(&msg_logger, rdata, &uas_tsx);
    if (status != PJ_SUCCESS)
        sbc_perror(THIS_FILE, "Unable create UAS transaction\n", status);

    // Recive INVITE from A side
    pjsip_tsx_recv_msg(uas_tsx, rdata);

    // Create response 100 trying
    status = pjsip_endpt_create_response( g_endpt, rdata,
                                        PJSIP_SC_TRYING, 
                                        &st_text, &tdata);
    if (status != PJ_SUCCESS)
        sbc_perror(THIS_FILE, "Unable create response", status);

    // The response message may modified there

    pjsip_tsx_send_msg( uas_tsx, tdata);

    //send requset to B side
    sbc_request_inv_send(rdata);

    return PJ_TRUE;
}

/*
 * TRANSACTION CREATE STATEFULL with SBC and B side!!!
 */
static pj_bool_t sbc_request_inv_send(pjsip_rx_data *rdata)
{
    pj_status_t status;
    pjsip_tx_data *tdata;
    pjsip_transaction *tsx;
    unsigned timeout_ms = TIME_RESEND_REQUEST_MS;

    pj_str_t target = pj_str("sip:winehouse@10.25.72.75:5062");
    pj_str_t from   = pj_str("sip:sbc@10.25.72.130:7777");
    pj_str_t contact= pj_str("sip:Remark@10.25.72.130:5060");

    /*
     * We should recive tdata from rdata and change our headers
     */

    // Create independet request 
    status = pjsip_endpt_create_request(g_endpt,
                        pjsip_get_invite_method(),
                        &target,
                        &from,
                        &target,
                        &contact,
                        NULL,
                        -1,
                        NULL,
                        &tdata);
    if (status != PJ_SUCCESS)
        sbc_perror(THIS_FILE, "Unable to create request", status);

    // Create transaction
    status = pjsip_tsx_create_uac(&mod_sbc, tdata, &tsx);
    if (status != PJ_SUCCESS)
        sbc_perror(THIS_FILE, "Unable to create UAC transaction", status);

    // Can modified request before send

    // stop retransmit request
    status = pjsip_tsx_stop_retransmit(tsx);
    if (status != PJ_SUCCESS)
        sbc_perror(THIS_FILE, "Unable to stop retransmission", status);

    status = pjsip_tsx_set_timeout( tsx, timeout_ms);
    if (status != PJ_SUCCESS)
        sbc_perror(THIS_FILE, "Unable set up timeout_ms", status);

    // Send request to B side
    status = pjsip_tsx_send_msg(tsx, tdata);

    return PJ_TRUE;
}