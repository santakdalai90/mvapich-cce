/*
 * Copyright (C) 1999-2001 The Regents of the University of California
 * (through E.O. Lawrence Berkeley National Laboratory), subject to
 * approval by the U.S. Department of Energy.
 *
 * Use of this software is under license. The license agreement is included
 * in the file MVICH_LICENSE.TXT.
 *
 * Developed at Berkeley Lab as part of MVICH.
 *
 * Authors: Bill Saphir      <wcsaphir@lbl.gov>
 *          Michael Welcome  <mlwelcome@lbl.gov>
 */

/* Copyright (c) 2002-2010, The Ohio State University. All rights
 * reserved.
 *
 * This file is part of the MVAPICH software package developed by the
 * team members of The Ohio State University's Network-Based Computing
 * Laboratory (NBCL), headed by Professor Dhabaleswar K. (DK) Panda.
 *
 * For detailed copyright and licensing information, please refer to the
 * copyright file COPYRIGHT_MVAPICH in the top level MPICH directory.
 *
 */


#include "mpid_bind.h"
#include "viadev.h"
#include "viaparam.h"
#include "viapriv.h"
#include "mpid_smpi.h"
#include <unistd.h>

#include "cm_user.h"
#include "cm.h"


inline void MPID_IsendContig(struct MPIR_COMMUNICATOR *comm_ptr,
                             void *buf,
                             int len,
                             int src_lrank,
                             int tag,
                             int context_id,
                             int dest_grank,
                             MPID_Msgrep_t msgrep,
                             MPI_Request request, int *error_code)
{

    int rc;
    MPIR_SHANDLE *shandle = (MPIR_SHANDLE *) request;

    /*
     * This is the one argument check done in the model adi2 implementation 
     */

    if (VIADEV_UNLIKELY(buf == 0 && len > 0)) {
        *error_code = MPI_ERR_BUFFER;
        return;
    }
#ifdef _SMP_
    if ((!disable_shared_mem) && (smpi.local_nodes[dest_grank] != -1)
        && (smpi.my_local_id != smpi.local_nodes[dest_grank])) {
        if (len < smp_eagersize) {
            MPID_SMP_Eagerb_isend_short(buf, len, src_lrank,
                                        tag, context_id, dest_grank,
                                        msgrep, shandle);

            if (NULL != request->shandle.finish)
                request->shandle.finish(&(request->shandle));
            return;
        }
#ifdef _SMP_RNDV_
        MPID_SMP_Rndvn_isend(buf, len, src_lrank,
                             tag, context_id, dest_grank, msgrep, shandle);
        return;
#endif
    }
#endif


    if (VIADEV_UNLIKELY(dest_grank == viadev.me)) {
        rc = MPID_VIA_self_start(buf, len, src_lrank, tag,
                                 context_id, shandle);
    } else {

        if (VIADEV_UNLIKELY(viadev_use_on_demand && 
            MPICM_IB_RC_PT2PT != cm_conn_state[dest_grank]))
        {
            /* Enqueue */
            request->shandle.is_complete = 0;

            cm_pending_request *req = (cm_pending_request *)
                malloc(sizeof(cm_pending_request));
            req->buf = buf;
            req->comm_ptr = comm_ptr;
            req->context_id = context_id;
            req->dest_grank = dest_grank;
            req->len = len;
            req->request = request;
            req->src_lrank = src_lrank;
            req->tag = tag;
            req->error_code = error_code;
            req->next = NULL;
            req->type = MPID_VIA_SEND;

            if (viadev.pending_req_head[dest_grank] == NULL) {
                /*First pending request */
                viadev.pending_req_head[dest_grank] =
                    viadev.pending_req_tail[dest_grank] = req;
                MPICM_Connect_req(dest_grank);
            } 
            else 
            {
                viadev.pending_req_tail[dest_grank]->next = req;
                viadev.pending_req_tail[dest_grank] = req;
            }
            return;
        }
        if (VIADEV_UNLIKELY(viadev_use_on_demand 
         && MPICM_IB_RC_PT2PT == cm_conn_state[dest_grank]
         && viadev.pending_req_head[dest_grank] != NULL)) {
            cm_process_queue(dest_grank);
        }
        if (len < viadev_rendezvous_threshold &&
            VIADEV_EAGER_OK(len, &viadev.connections[dest_grank])) {
            rc = MPID_VIA_eager_send(buf, len, src_lrank,
                                     tag, context_id, dest_grank, shandle);
        } else {
            if (VIADEV_UNLIKELY(NULL == buf || 0 == len)) {
                buf = &nullsbuffer;
            }
            rc = MPID_VIA_rendezvous_start(buf, len, src_lrank,
                                           tag, context_id,
                                           dest_grank, shandle);
        }
    }
}

void MPID_SendContig(struct MPIR_COMMUNICATOR *comm_ptr,
                     void *buf,
                     int len,
                     int src_lrank,
                     int tag,
                     int context_id,
                     int dest_grank, MPID_Msgrep_t msgrep, int *error_code)
{

    MPIR_SHANDLE *shandle;
    MPID_SendAlloc(shandle);
#ifndef NO_PROGRESS
    viadev_connection_t *c = &viadev.connections[dest_grank];
#endif

    /* argument checking will be done by IsendContig */
#ifdef _SMP_
    if ((!disable_shared_mem) && (smpi.local_nodes[dest_grank] != -1)
        && (smpi.my_local_id != smpi.local_nodes[dest_grank])) {

           if (buf == 0 && len > 0) {
               *error_code = MPI_ERR_BUFFER;
               return;
           }

        if (len < smp_eagersize){
           MPID_SMP_Eagerb_send_short(buf, len, src_lrank,
                                      tag, context_id, dest_grank, msgrep,
                                      shandle);

           if(NULL != ((MPI_Request) shandle)->shandle.finish)
              ((MPI_Request) shandle)->shandle.finish(&(((MPI_Request) shandle)->shandle));
        }

#ifdef _SMP_RNDV_
        else {
          MPID_SMP_Rndvn_isend(buf, len, src_lrank,
                               tag, context_id, dest_grank, msgrep,
                               shandle);
        }
    }
    else {
#else
        else{
            MPID_IsendContig(comm_ptr, buf, len, src_lrank, tag, context_id,
                     dest_grank, msgrep, (MPI_Request) shandle,
                     error_code);
        }
    }
    else {
#endif
#endif

    MPID_IsendContig(comm_ptr, buf, len, src_lrank, tag, context_id,
                     dest_grank, msgrep, (MPI_Request) shandle,
                     error_code);
#ifdef _SMP_
    }
#endif

    while (!shandle->is_complete)
        MPID_DeviceCheck(MPID_BLOCKING);

#ifndef NO_PROGRESS
    if(VIADEV_UNLIKELY(c->ext_sendq_size >= viadev_progress_threshold))
        MPID_DeviceCheck(MPID_NOTBLOCKING);
#endif

    MPID_SendFree(shandle);
}

void MPID_SsendContig(struct MPIR_COMMUNICATOR *comm_ptr,
                      void *buf,
                      int len,
                      int src_lrank,
                      int tag,
                      int context_id,
                      int dest_grank,
                      MPID_Msgrep_t msgrep, int *error_code)
{

    MPIR_SHANDLE *shandle;
    MPID_SendAlloc(shandle);
#ifndef NO_PROGRESS
    viadev_connection_t *c = &viadev.connections[dest_grank];
#endif

    /* argument checking will be done by IsendContig */

    MPID_IssendContig(comm_ptr, buf, len, src_lrank, tag, context_id,
                      dest_grank, msgrep, (MPI_Request) shandle,
                      error_code);

    while (!shandle->is_complete)
        MPID_DeviceCheck(MPID_BLOCKING);

#ifndef NO_PROGRESS
    if(VIADEV_UNLIKELY(c->ext_sendq_size >= viadev_progress_threshold))
        MPID_DeviceCheck(MPID_NOTBLOCKING);
#endif

    MPID_SendFree(shandle);

}

void MPID_IssendContig(struct MPIR_COMMUNICATOR *comm_ptr,
                       void *buf,
                       int len,
                       int src_lrank,
                       int tag,
                       int context_id,
                       int dest_grank,
                       MPID_Msgrep_t msgrep,
                       MPI_Request request, int *error_code)
{

    int rc;
    MPIR_SHANDLE *shandle = (MPIR_SHANDLE *) request;


    /*
     * This is the one argument check done in the model adi2 implementation 
     * Why isn't it done inside MPI? xxx
     */
    if (buf == 0 && len > 0) {
        *error_code = MPI_ERR_BUFFER;
        return;
    }

    if (dest_grank == viadev.me) {
        rc = MPID_VIA_self_start(buf, len, src_lrank, tag, context_id,
                                 shandle);
    } else {
        if (buf == NULL || len == 0) {
            buf = &nullsbuffer;
        }
#ifdef _SMP_
#ifdef _SMP_RNDV_
        if ((!disable_shared_mem) && smpi.local_nodes[dest_grank] != -1) {
            MPID_SMP_Rndvn_isend(buf, len, src_lrank, tag, context_id,
                                 dest_grank, msgrep, shandle);
            return;
        }
#endif
#endif

        if (viadev_use_on_demand &&
            MPICM_IB_RC_PT2PT != cm_conn_state[dest_grank])
        {
            /* Enqueue */
            request->shandle.is_complete = 0;

            cm_pending_request *req =
                (cm_pending_request *)
                malloc(sizeof(cm_pending_request));
            req->buf = buf;
            req->comm_ptr = comm_ptr;
            req->context_id = context_id;
            req->dest_grank = dest_grank;
            req->len = len;
            req->request = request;
            req->src_lrank = src_lrank;
            req->tag = tag;
            req->error_code = error_code;
            req->next = NULL;
            req->type = MPID_VIA_SEND_RDNV;

            if (viadev.pending_req_head[dest_grank] == NULL) {
                /*First pending request */
                viadev.pending_req_head[dest_grank] =
                    viadev.pending_req_tail[dest_grank] = req;
                MPICM_Connect_req(dest_grank);
            } 
            else 
            {
                viadev.pending_req_tail[dest_grank]->next = req;
                viadev.pending_req_tail[dest_grank] = req;
            }
            return;
        }
        if (viadev_use_on_demand 
         && MPICM_IB_RC_PT2PT == cm_conn_state[dest_grank]
         && viadev.pending_req_head[dest_grank] != NULL) {
            cm_process_queue(dest_grank);
        }

        rc = MPID_VIA_rendezvous_start(buf, len, src_lrank,
                                       tag, context_id, dest_grank,
                                       shandle);
    }


}


void MPID_SendComplete(MPI_Request request, int *error_code)
{
    MPIR_SHANDLE *shandle = (MPIR_SHANDLE *) request;
    while (!shandle->is_complete)
        MPID_DeviceCheck(MPID_BLOCKING);
}

int MPID_SendIcomplete(MPI_Request request, int *error_code)
{
    MPIR_SHANDLE *shandle = (MPIR_SHANDLE *) request;
    MPID_DeviceCheck(MPID_NOTBLOCKING);
    if (shandle->is_complete)
        return 1;
    return 0;
}

void MPID_SendCancel(MPI_Request r, int *error_code)
{
#ifdef VIADEV_SEND_CANCEL
    MPIR_SHANDLE *shandle = (MPIR_SHANDLE *) r;

    if (shandle->is_complete) {
        /* too late to cancel, send is already complete */

    } else {
        /* Try to suspend the send operations on this end */
        shandle->send_cancel = MPID_SC_STALL;

        /* Send a cancel message to the remote process */

        /* Wait for message from remote process */
    }
#else
    error_abort_all(GEN_EXIT_ERR, "MPID_SendCancel not implemented");
#endif
}

#ifdef MCST_SUPPORT

void MPID_RC_Send(void *, int, int, int, int, int, int *);
void MPID_RC_Send(void *buf,
                  int len,
                  int src_lrank,
                  int tag, int context_id, int dest_grank, int *error_code)
{

    int rc;
    MPIR_SHANDLE *shandle;

    MPID_SendAlloc(shandle);


    if (buf == 0 && len > 0) {
        *error_code = MPI_ERR_BUFFER;
        return;
    }

    if (dest_grank == viadev.me) {
        rc = MPID_VIA_self_start(buf, len, src_lrank, tag, context_id,
                                 shandle);
    } else {
        if (len < viadev_rendezvous_threshold &&
            VIADEV_EAGER_OK(len, &viadev.connections[dest_grank])) {
            rc = MPID_VIA_eager_send(buf, len, src_lrank,
                                     tag, context_id, dest_grank, shandle);
        } else {
            if (buf == NULL || len == 0) {
                buf = &nullsbuffer;
            }
            rc = MPID_VIA_rendezvous_start(buf, len, src_lrank,
                                           tag, context_id,
                                           dest_grank, shandle);
        }
    }
    co_print("Wait for device\n");
    co_print("The_shandle:%x\n", shandle);

    while (!shandle->is_complete)
        MPID_DeviceCheck(MPID_BLOCKING);
    co_print("Send complete\n");

    MPID_SendFree(shandle);
}

#endif
