/* SPDX-License-Identifier: CDDL-1.0 */
/*
 * Copyright (c) 2017, Datto, Inc. All rights reserved.
 *
 * Encryption is intentionally unsupported during NetBSD bring-up. Keep the
 * upstream property descriptions, but never report success for an operation
 * that would encrypt, decrypt, or authenticate data.
 */

#include <sys/zfs_context.h>
#include <sys/zio_crypt.h>

const zio_crypt_info_t zio_crypt_table[ZIO_CRYPT_FUNCTIONS] = {
	{"",			ZC_TYPE_NONE,	0,	"inherit"},
	{"",			ZC_TYPE_NONE,	0,	"on"},
	{"",			ZC_TYPE_NONE,	0,	"off"},
	{SUN_CKM_AES_CCM,	ZC_TYPE_CCM,	16,	"aes-128-ccm"},
	{SUN_CKM_AES_CCM,	ZC_TYPE_CCM,	24,	"aes-192-ccm"},
	{SUN_CKM_AES_CCM,	ZC_TYPE_CCM,	32,	"aes-256-ccm"},
	{SUN_CKM_AES_GCM,	ZC_TYPE_GCM,	16,	"aes-128-gcm"},
	{SUN_CKM_AES_GCM,	ZC_TYPE_GCM,	24,	"aes-192-gcm"},
	{SUN_CKM_AES_GCM,	ZC_TYPE_GCM,	32,	"aes-256-gcm"},
};

#define	CRYPT_UNSUPPORTED() \
	panic("%s: OpenZFS encryption is not supported on NetBSD", __func__)

void
zio_crypt_key_destroy(zio_crypt_key_t *key)
{
	CRYPT_UNSUPPORTED();
}

int
zio_crypt_key_init(uint64_t crypt, zio_crypt_key_t *key)
{
	return (SET_ERROR(ENOTSUP));
}

int
zio_crypt_key_get_salt(zio_crypt_key_t *key, uint8_t *salt)
{
	return (SET_ERROR(ENOTSUP));
}

int
zio_crypt_key_wrap(crypto_key_t *cwkey, zio_crypt_key_t *key, uint8_t *iv,
    uint8_t *mac, uint8_t *keydata, uint8_t *hmac_keydata)
{
	return (SET_ERROR(ENOTSUP));
}

int
zio_crypt_key_unwrap(crypto_key_t *cwkey, uint64_t crypt, uint64_t version,
    uint64_t guid, uint8_t *keydata, uint8_t *hmac_keydata, uint8_t *iv,
    uint8_t *mac, zio_crypt_key_t *key)
{
	return (SET_ERROR(ENOTSUP));
}

int
zio_crypt_generate_iv(uint8_t *iv)
{
	return (SET_ERROR(ENOTSUP));
}

int
zio_crypt_generate_iv_salt_dedup(zio_crypt_key_t *key, uint8_t *data,
    uint_t datalen, uint8_t *iv, uint8_t *salt)
{
	return (SET_ERROR(ENOTSUP));
}

void
zio_crypt_encode_params_bp(blkptr_t *bp, uint8_t *salt, uint8_t *iv)
{
	CRYPT_UNSUPPORTED();
}

void
zio_crypt_decode_params_bp(const blkptr_t *bp, uint8_t *salt, uint8_t *iv)
{
	CRYPT_UNSUPPORTED();
}

void
zio_crypt_encode_mac_bp(blkptr_t *bp, uint8_t *mac)
{
	CRYPT_UNSUPPORTED();
}

void
zio_crypt_decode_mac_bp(const blkptr_t *bp, uint8_t *mac)
{
	CRYPT_UNSUPPORTED();
}

void
zio_crypt_encode_mac_zil(void *data, uint8_t *mac)
{
	CRYPT_UNSUPPORTED();
}

void
zio_crypt_decode_mac_zil(const void *data, uint8_t *mac)
{
	CRYPT_UNSUPPORTED();
}

void
zio_crypt_copy_dnode_bonus(abd_t *src, uint8_t *dst, uint_t datalen)
{
	CRYPT_UNSUPPORTED();
}

int
zio_crypt_do_indirect_mac_checksum(boolean_t generate, void *buf,
    uint_t datalen, boolean_t byteswap, uint8_t *cksum)
{
	return (SET_ERROR(ENOTSUP));
}

int
zio_crypt_do_indirect_mac_checksum_abd(boolean_t generate, abd_t *abd,
    uint_t datalen, boolean_t byteswap, uint8_t *cksum)
{
	return (SET_ERROR(ENOTSUP));
}

int
zio_crypt_do_hmac(zio_crypt_key_t *key, uint8_t *data, uint_t datalen,
    uint8_t *digest, uint_t digestlen)
{
	return (SET_ERROR(ENOTSUP));
}

int
zio_crypt_do_objset_hmacs(zio_crypt_key_t *key, void *data, uint_t datalen,
    boolean_t byteswap, uint8_t *portable_mac, uint8_t *local_mac)
{
	return (SET_ERROR(ENOTSUP));
}

int
zio_do_crypt_data(boolean_t encrypt, zio_crypt_key_t *key, dmu_object_type_t ot,
    boolean_t byteswap, uint8_t *salt, uint8_t *iv, uint8_t *mac, uint_t datalen,
    uint8_t *plainbuf, uint8_t *cipherbuf, boolean_t *no_crypt)
{
	return (SET_ERROR(ENOTSUP));
}

int
zio_do_crypt_abd(boolean_t encrypt, zio_crypt_key_t *key, dmu_object_type_t ot,
    boolean_t byteswap, uint8_t *salt, uint8_t *iv, uint8_t *mac, uint_t datalen,
    abd_t *pabd, abd_t *cabd, boolean_t *no_crypt)
{
	return (SET_ERROR(ENOTSUP));
}
