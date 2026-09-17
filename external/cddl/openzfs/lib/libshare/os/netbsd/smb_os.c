/* SPDX-License-Identifier: BSD-2-Clause */
#include <libshare.h>
#include "libshare_impl.h"

/* SMB sharing was not supported by the old NetBSD integration either. */
static int
smb_toggle(sa_share_impl_t share)
{
	(void) share;
	return (SA_NOT_SUPPORTED);
}

static boolean_t
smb_shared(sa_share_impl_t share)
{
	(void) share;
	return (B_FALSE);
}

static int
smb_validate(const char *opts)
{
	(void) opts;
	return (SA_NOT_SUPPORTED);
}

static int
smb_commit(void)
{
	return (SA_OK);
}

const sa_fstype_t libshare_smb_type = {
	.enable_share = smb_toggle,
	.disable_share = smb_toggle,
	.is_shared = smb_shared,
	.validate_shareopts = smb_validate,
	.commit_shares = smb_commit
};
