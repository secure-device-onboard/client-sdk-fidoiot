/*
 * Copyright 2020 Intel Corporation
 * SPDX-License-Identifier: Apache 2.0
 */

/*!
 * \file
 * \brief Implementation of msg number 13.
 */

#include "load_credentials.h"
#include "util.h"

/**
 * msg13() - DIDone, Type 13
 *
 * The device receives message once the manufacturer has storedall the relevant
 * information, and is ready with the Ownership Voucher.
 *
 * DI.Done = [] ;; empty message
 */
int32_t msg13(sdo_prot_t *ps)
{
	int ret = -1;
	char prot[] = "SDOProtDI";
	sdo_dev_cred_t *dev_cred = app_get_credentials();

	/* Check if we are able to read the device credentials from storage */
	if (dev_cred == NULL) {
		LOG(LOG_ERROR, "Device credentials missing\n");
		goto err;
	}

	/* Read from the internal buffer to see if the data is there */
	if (!sdo_prot_rcv_msg(&ps->sdor, &ps->sdow, prot, &ps->state)) {
		ret = 0; /* Try again */
		goto err;
	}

	size_t num_array_entries;
	if (!sdor_array_length(&ps->sdor, &num_array_entries) || num_array_entries != 0) {
		goto err;
	}
	if (!sdor_start_array(&ps->sdor)) {
		goto err;
	}
	if (!sdor_end_array(&ps->sdor)) {
		goto err;
	}

	/* Generate hash of the public key received in msg11 */
	sdo_block_reset(&ps->sdor.b);
	dev_cred->owner_blk->pkh = sdo_pub_key_hash(dev_cred->owner_blk->pk);
	if (!dev_cred->owner_blk->pkh) {
		LOG(LOG_ERROR, "Hash creation of manufacturer pk failed\n");
		goto err;
	}

	/* Update the state of device to be ready for TO1 */
	ps->dev_cred->ST = SDO_DEVICE_STATE_READY1;
	if (store_credential(ps->dev_cred) != 0) {
		LOG(LOG_ERROR, "Failed to store updated device state\n");
		goto err;
	}
	LOG(LOG_DEBUG, "Device credentials successfully written!!\n");

	/* Mark as success, and DI done */
	ps->state = SDO_STATE_DONE;
	ps->sdor.have_block = false;
	LOG(LOG_DEBUG, "DIDone completed\n");
	ret = 0;

err:
	return ret;
}
