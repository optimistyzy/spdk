/*-
 *   BSD LICENSE
 *
 *   Copyright (C) 2008-2012 Daisuke Aoyama <aoyama@peach.ne.jp>.
 *   Copyright (c) Intel Corporation.
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SPDK_ISCSI_TASK_H
#define SPDK_ISCSI_TASK_H

#include "iscsi/iscsi.h"
#include "spdk/scsi.h"

struct spdk_iscsi_task {
	struct spdk_scsi_task	scsi;

	struct spdk_iscsi_pdu *pdu;
	uint32_t outstanding_r2t;

	/*
	 * Tracks the current offset of large read io.
	 */
	uint32_t current_datain_offset;

	/*
	 * next_expected_r2t_offset is used when we receive
	 * the DataOUT PDU.
	 */
	uint32_t next_expected_r2t_offset;

	/*
	 * Tracks the length of the R2T that is in progress.
	 * Used to check that an R2T burst does not exceed
	 *  MaxBurstLength.
	 */
	uint32_t current_r2t_length;

	/*
	 * next_r2t_offset is used when we are sending the
	 * R2T packet to keep track of next offset of r2t.
	 */
	uint32_t next_r2t_offset;
	uint32_t R2TSN;
	uint32_t r2t_datasn; /* record next datasn for a r2tsn*/
	uint32_t acked_r2tsn; /* next r2tsn to be acked */
	uint32_t datain_datasn;
	uint32_t acked_data_sn; /* next expected datain datasn */
	uint32_t ttt;

	TAILQ_ENTRY(spdk_iscsi_task) link;
};

static inline void
spdk_iscsi_task_put(struct spdk_iscsi_task *task)
{
	spdk_scsi_task_put(&task->scsi);
}

static inline struct spdk_iscsi_pdu *
spdk_iscsi_task_get_pdu(struct spdk_iscsi_task *task)
{
	return task->pdu;
}

static inline void
spdk_iscsi_task_set_pdu(struct spdk_iscsi_task *task, struct spdk_iscsi_pdu *pdu)
{
	task->pdu = pdu;
}

static inline struct iscsi_bhs *
spdk_iscsi_task_get_bhs(struct spdk_iscsi_task *task)
{
	return &spdk_iscsi_task_get_pdu(task)->bhs;
}

static inline void
spdk_iscsi_task_associate_pdu(struct spdk_iscsi_task *task, struct spdk_iscsi_pdu *pdu)
{
	spdk_iscsi_task_set_pdu(task, pdu);
	pdu->ref++;
}

static inline void
spdk_iscsi_task_disassociate_pdu(struct spdk_iscsi_task *task)
{
	if (spdk_iscsi_task_get_pdu(task)) {
		spdk_put_pdu(spdk_iscsi_task_get_pdu(task));
		spdk_iscsi_task_set_pdu(task, NULL);
	}
}

static inline int
spdk_iscsi_task_is_immediate(struct spdk_iscsi_task *task)
{
	struct iscsi_bhs_scsi_req *scsi_req;

	scsi_req = (struct iscsi_bhs_scsi_req *)spdk_iscsi_task_get_bhs(task);
	return (scsi_req->immediate == 1);
}

static inline int
spdk_iscsi_task_is_read(struct spdk_iscsi_task *task)
{
	struct iscsi_bhs_scsi_req *scsi_req;

	scsi_req = (struct iscsi_bhs_scsi_req *)spdk_iscsi_task_get_bhs(task);
	return (scsi_req->read == 1);
}

static inline uint32_t
spdk_iscsi_task_get_cmdsn(struct spdk_iscsi_task *task)
{
	return spdk_iscsi_task_get_pdu(task)->cmd_sn;
}

struct spdk_iscsi_task *spdk_iscsi_task_get(uint32_t *owner_task_ctr,
		struct spdk_iscsi_task *parent);

static inline struct spdk_iscsi_task *
spdk_iscsi_task_get_primary(struct spdk_iscsi_task *task)
{
	struct spdk_scsi_task *scsi_task;
	struct spdk_scsi_task *scsi_primary_task;

	scsi_task = &task->scsi;
	scsi_primary_task = spdk_scsi_task_get_primary(scsi_task);
	return (struct spdk_iscsi_task *)scsi_primary_task;
}

#endif /* SPDK_ISCSI_TASK_H */
