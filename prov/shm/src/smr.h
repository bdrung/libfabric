/*
 * Copyright (c) 2015-2017 Intel Corporation, Inc.  All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <pthread.h>
#include <stdint.h>
#include <stddef.h>

#include <rdma/fabric.h>
#include <rdma/fi_atomic.h>
#include <rdma/fi_cm.h>
#include <rdma/fi_domain.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_eq.h>
#include <rdma/fi_errno.h>
#include <rdma/fi_rma.h>
#include <rdma/fi_tagged.h>
#include <rdma/fi_trigger.h>
#include <rdma/providers/fi_prov.h>

#include <fi.h>
#include <fi_enosys.h>
#include <fi_shm.h>
#include <fi_rbuf.h>
#include <fi_list.h>
#include <fi_signal.h>
#include <fi_util.h>

#ifndef _SMR_H_
#define _SMR_H_


#define SMR_MAJOR_VERSION 0
#define SMR_MINOR_VERSION 1

extern struct fi_provider smr_prov;
extern struct fi_info smr_info;
extern struct util_prov smr_util_prov;

int smr_check_info(struct fi_info *info);
int smr_fabric(struct fi_fabric_attr *attr, struct fid_fabric **fabric,
		void *context);

struct smr_av {
	struct util_av		util_av;
	struct smr_map		*smr_map;
};

int smr_domain_open(struct fid_fabric *fabric, struct fi_info *info,
		struct fid_domain **dom, void *context);

int smr_eq_open(struct fid_fabric *fabric, struct fi_eq_attr *attr,
		struct fid_eq **eq, void *context);

int smr_av_open(struct fid_domain *domain, struct fi_av_attr *attr,
		struct fid_av **av, void *context);

#define SMR_IOV_LIMIT		4

struct smr_ep_entry {
	struct dlist_entry	entry;
	void			*context;
	fi_addr_t		addr;
	uint64_t		tag;
	uint64_t		ignore;
	struct iovec		iov[SMR_IOV_LIMIT];
	uint32_t		iov_count;
	uint32_t		flags;
	uint64_t		err;
};

struct smr_ep;
typedef int (*smr_rx_comp_func)(struct smr_ep *ep, void *context,
		uint64_t flags, size_t len, void *buf, void *addr,
		uint64_t tag, uint64_t err);
typedef int (*smr_tx_comp_func)(struct smr_ep *ep, void *context,
		uint64_t flags, uint64_t err);


struct smr_match_attr {
	fi_addr_t	addr;
	uint64_t	tag;
	uint64_t	ignore;
	uint64_t	ctx;
};

static inline int smr_match_addr(fi_addr_t addr, fi_addr_t match_addr)
{
	return (addr == FI_ADDR_UNSPEC) || (match_addr == FI_ADDR_UNSPEC) ||
		(addr == match_addr);
}

static inline int smr_match_tag(uint64_t tag, uint64_t ignore, uint64_t match_tag)
{
	return ((tag | ignore) == (match_tag | ignore));
}

struct smr_pending_cmd {
	struct dlist_entry entry;
	struct smr_cmd cmd;
};

DECLARE_FREESTACK(struct smr_ep_entry, smr_recv_fs);
DECLARE_FREESTACK(struct smr_pending_cmd, smr_unexp_fs);
DECLARE_FREESTACK(struct smr_pending_cmd, smr_pend_fs);

struct smr_recv_queue {
	struct dlist_entry recv_list;
	dlist_func_t *match_recv;
};

struct smr_pending_queue {
	struct dlist_entry msg_list;
	dlist_func_t *match_msg;
};

struct smr_ep {
	struct util_ep		util_ep;
	smr_rx_comp_func	rx_comp;
	smr_tx_comp_func	tx_comp;
	size_t			tx_size;
	size_t			rx_size;
	const char		*name;
	struct smr_region	*region;
	struct smr_recv_fs	*recv_fs; /* protected by rx_cq lock */
	struct smr_recv_queue	recv_queue;
	struct smr_recv_queue	trecv_queue;
	struct smr_unexp_fs	*unexp_fs;
	struct smr_pend_fs	*pend_fs;
	struct smr_pending_queue	unexp_queue;
	struct smr_pending_queue	pend_queue;
};

int smr_endpoint(struct fid_domain *domain, struct fi_info *info,
		  struct fid_ep **ep, void *context);

int smr_cq_open(struct fid_domain *domain, struct fi_cq_attr *attr,
		struct fid_cq **cq_fid, void *context);


#endif
