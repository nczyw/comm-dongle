/**
 *******************************************************************************
 * @file  hc32_ll_aes_ex.c
 * @brief Streaming AES (ECB / CBC / CTR) implementation for HC32F4xx.
 *        The AES hardware only exposes single-block ECB encryption/decryption,
 *        so CBC chaining and CTR keystream generation are done in software on
 *        top of the single-block primitive. All state is kept in the caller-
 *        owned context; no file-global variable is used.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2026-11-24       CDT             First version
 @endverbatim
 *******************************************************************************
 * Copyright (C) 2022-2025, Xiaohua Semiconductor Co., Ltd. All rights reserved.
 *
 * This software component is licensed by XHSC under BSD 3-Clause license
 * (the "License"); You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                    opensource.org/licenses/BSD-3-Clause
 *
 *******************************************************************************
 */

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "hc32_ll_aes_ex.h"
#include "hc32_ll_utility.h"

/**
 * @addtogroup LL_Driver
 * @{
 */

/**
 * @defgroup LL_AES_EX AES_EX
 * @brief Streaming AES Driver Library
 * @{
 */

#if (LL_AES_ENABLE == DDL_ON)

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
#define AES_EX_TIMEOUT                  (30000UL)
#define AES_EX_BLOCK_WORDS              (AES_EX_BLOCK_SIZE / 4U)
#define AES_EX_KEY_WORDS_MAX            (AES_KEY_SIZE_32BYTE / 4U)

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
static int32_t AES_WaitDone_Ex(void);
static void AES_WriteKey_Ex(const uint8_t *pKey, uint8_t u8KeySize);
static void AES_WriteData_Ex(const uint8_t *pData);
static void AES_ReadData_Ex(uint8_t *pData);
static int32_t AES_CryptBlock_Ex(const uint8_t *pKey, uint8_t u8KeySize, uint8_t u8Dir,
                                 const uint8_t *pIn, uint8_t *pOut);
static void AES_MemSet(uint8_t *pDest, uint8_t u8Val, uint32_t u32Len);
static void AES_MemCopy(uint8_t *pDest, const uint8_t *pSrc, uint32_t u32Len);
static void AES_XorBlock(uint8_t *pOut, const uint8_t *pA, const uint8_t *pB);
static void AES_CTR_Increment(uint8_t *pCtr);
static void AES_ClearSensitive(stc_aes_ctx_t *pCtx);
static int32_t AES_EncryptOneBlock(stc_aes_ctx_t *pCtx, const uint8_t *pIn, uint8_t *pOut);
static int32_t AES_DecryptAndEmit(stc_aes_ctx_t *pCtx, uint8_t *pOut, uint32_t *pu32OutLen);
static int32_t AES_BlockEncrypt_Update(stc_aes_ctx_t *pCtx, const uint8_t *pIn, uint32_t u32InLen,
                                       uint8_t *pOut, uint32_t *pu32OutLen);
static int32_t AES_BlockDecrypt_Update(stc_aes_ctx_t *pCtx, const uint8_t *pIn, uint32_t u32InLen,
                                       uint8_t *pOut, uint32_t *pu32OutLen);
static int32_t AES_CTR_Update_Ex(stc_aes_ctx_t *pCtx, const uint8_t *pIn, uint32_t u32InLen,
                                 uint8_t *pOut, uint32_t *pu32OutLen);

/*******************************************************************************
 * Function implementation - local ('static')
 ******************************************************************************/
/**
 * @brief  Wait until the AES START bit is cleared by hardware.
 * @retval LL_OK on success, LL_ERR_TIMEOUT on timeout.
 */
static int32_t AES_WaitDone_Ex(void)
{
    uint32_t u32TimeOut = AES_EX_TIMEOUT;

    while (bCM_AES->CR_b.START != 0UL) {
        if (--u32TimeOut == 0UL) {
            return LL_ERR_TIMEOUT;
        }
    }
    return LL_OK;
}

/**
 * @brief  Write the AES key into the key registers and configure key size.
 * @param  [in] pKey        Pointer to the key buffer.
 * @param  [in] u8KeySize   Key size in bytes (16 / 24 / 32).
 */
static void AES_WriteKey_Ex(const uint8_t *pKey, uint8_t u8KeySize)
{
    uint8_t i;
    uint8_t u8KeyWordSize = (uint8_t)(u8KeySize / 4U);
    __IO uint32_t *regKR = &CM_AES->KR0;
    uint8_t u8KeySizeCode;

    for (i = 0U; i < u8KeyWordSize; i++) {
        uint32_t u32W = ((uint32_t)pKey[i * 4U])
                      | ((uint32_t)pKey[i * 4U + 1U] << 8)
                      | ((uint32_t)pKey[i * 4U + 2U] << 16)
                      | ((uint32_t)pKey[i * 4U + 3U] << 24);
        regKR[i] = u32W;
    }
    switch (u8KeySize) {
        case AES_KEY_SIZE_16BYTE:
            u8KeySizeCode = 0U;
            break;
        case AES_KEY_SIZE_24BYTE:
            u8KeySizeCode = 1U;
            break;
        case AES_KEY_SIZE_32BYTE:
            u8KeySizeCode = 2U;
            break;
        default:
            u8KeySizeCode = 0U;
            break;
    }
    MODIFY_REG32(CM_AES->CR, AES_CR_KEYSIZE, ((uint32_t)u8KeySizeCode << AES_CR_KEYSIZE_POS));
}

/**
 * @brief  Write one 16-byte block into the AES data registers (DR0..DR3).
 * @param  [in] pData  Pointer to the 16-byte input block.
 */
static void AES_WriteData_Ex(const uint8_t *pData)
{
    __IO uint32_t *regDR = &CM_AES->DR0;
    uint8_t i;

    for (i = 0U; i < AES_EX_BLOCK_WORDS; i++) {
        uint32_t u32W = ((uint32_t)pData[i * 4U])
                      | ((uint32_t)pData[i * 4U + 1U] << 8)
                      | ((uint32_t)pData[i * 4U + 2U] << 16)
                      | ((uint32_t)pData[i * 4U + 3U] << 24);
        regDR[i] = u32W;
    }
}

/**
 * @brief  Read one 16-byte block from the AES data registers (DR0..DR3).
 * @param  [out] pData  Pointer to the 16-byte output buffer.
 */
static void AES_ReadData_Ex(uint8_t *pData)
{
    __IO uint32_t *regDR = &CM_AES->DR0;
    uint8_t i;

    for (i = 0U; i < AES_EX_BLOCK_WORDS; i++) {
        uint32_t u32W = regDR[i];
        pData[i * 4U]       = (uint8_t)(u32W);
        pData[i * 4U + 1U]  = (uint8_t)(u32W >> 8);
        pData[i * 4U + 2U]  = (uint8_t)(u32W >> 16);
        pData[i * 4U + 3U]  = (uint8_t)(u32W >> 24);
    }
}

/**
 * @brief  Encrypt or decrypt one 16-byte block in ECB mode using current key.
 *         Key and mode are (re)written every call so the shared hardware always
 *         ends up in the expected state regardless of prior use.
 * @param  [in]  pKey        Pointer to the key.
 * @param  [in]  u8KeySize   Key size in bytes.
 * @param  [in]  u8Dir       AES_EX_DIR_ENCRYPT / AES_EX_DIR_DECRYPT.
 * @param  [in]  pIn         Pointer to the 16-byte input block.
 * @param  [out] pOut        Pointer to the 16-byte output block.
 * @retval LL_OK on success, LL_ERR_TIMEOUT on hardware timeout.
 */
static int32_t AES_CryptBlock_Ex(const uint8_t *pKey, uint8_t u8KeySize, uint8_t u8Dir,
                                 const uint8_t *pIn, uint8_t *pOut)
{
    int32_t i32Ret;

    AES_WriteKey_Ex(pKey, u8KeySize);
    WRITE_REG32(bCM_AES->CR_b.MODE, (u8Dir == AES_EX_DIR_ENCRYPT) ? 0UL : 1UL);
    AES_WriteData_Ex(pIn);
    WRITE_REG32(bCM_AES->CR_b.START, 1UL);
    i32Ret = AES_WaitDone_Ex();
    if (i32Ret != LL_OK) {
        return i32Ret;
    }
    AES_ReadData_Ex(pOut);
    return LL_OK;
}

/**
 * @brief  Fill memory with a constant byte.
 */
static void AES_MemSet(uint8_t *pDest, uint8_t u8Val, uint32_t u32Len)
{
    while (u32Len-- > 0U) {
        *pDest++ = u8Val;
    }
}

/**
 * @brief  Copy memory (non-overlapping).
 */
static void AES_MemCopy(uint8_t *pDest, const uint8_t *pSrc, uint32_t u32Len)
{
    while (u32Len-- > 0U) {
        *pDest++ = *pSrc++;
    }
}

/**
 * @brief  pOut = pA XOR pB (16 bytes).
 */
static void AES_XorBlock(uint8_t *pOut, const uint8_t *pA, const uint8_t *pB)
{
    uint8_t i;
    for (i = 0U; i < AES_EX_BLOCK_SIZE; i++) {
        pOut[i] = (uint8_t)(pA[i] ^ pB[i]);
    }
}

/**
 * @brief  Increment a 16-byte big-endian counter by one.
 */
static void AES_CTR_Increment(uint8_t *pCtr)
{
    int32_t i;
    for (i = (int32_t)(AES_EX_BLOCK_SIZE - 1U); i >= 0; i--) {
        if (++pCtr[i] != 0U) {
            break;
        }
    }
}

/**
 * @brief  Zero all sensitive material inside the context.
 */
static void AES_ClearSensitive(stc_aes_ctx_t *pCtx)
{
    AES_MemSet(pCtx->key, 0U, sizeof(pCtx->key));
    AES_MemSet(pCtx->buffer, 0U, sizeof(pCtx->buffer));
    AES_MemSet(pCtx->ks, 0U, sizeof(pCtx->ks));
    AES_MemSet(pCtx->u.prev, 0U, sizeof(pCtx->u.prev));
}

/**
 * @brief  Encrypt one block for ECB/CBC, applying CBC chaining (XOR prev,
 *         update prev = ciphertext) when in CBC mode.
 */
static int32_t AES_EncryptOneBlock(stc_aes_ctx_t *pCtx, const uint8_t *pIn, uint8_t *pOut)
{
    uint8_t au8InBlock[AES_EX_BLOCK_SIZE];
    int32_t i32Ret;

    if (pCtx->mode == AES_EX_MODE_CBC) {
        AES_XorBlock(au8InBlock, pIn, pCtx->u.prev);
    } else {
        AES_MemCopy(au8InBlock, pIn, AES_EX_BLOCK_SIZE);
    }
    i32Ret = AES_CryptBlock_Ex(pCtx->key, pCtx->key_size, AES_EX_DIR_ENCRYPT, au8InBlock, pOut);
    if ((i32Ret == LL_OK) && (pCtx->mode == AES_EX_MODE_CBC)) {
        AES_MemCopy(pCtx->u.prev, pOut, AES_EX_BLOCK_SIZE);
    }
    return i32Ret;
}

/**
 * @brief  Decrypt the block currently held in pCtx->buffer and emit it to pOut,
 *         applying CBC chaining (XOR prev, then prev = ciphertext) when in CBC.
 *         Does NOT clear buffer_len; the caller decides.
 */
static int32_t AES_DecryptAndEmit(stc_aes_ctx_t *pCtx, uint8_t *pOut, uint32_t *pu32OutLen)
{
    uint8_t au8Tmp[AES_EX_BLOCK_SIZE];
    uint8_t au8OutBlock[AES_EX_BLOCK_SIZE];
    int32_t i32Ret;

    i32Ret = AES_CryptBlock_Ex(pCtx->key, pCtx->key_size, AES_EX_DIR_DECRYPT, pCtx->buffer, au8Tmp);
    if (i32Ret != LL_OK) {
        return i32Ret;
    }
    if (pCtx->mode == AES_EX_MODE_CBC) {
        AES_XorBlock(au8OutBlock, au8Tmp, pCtx->u.prev);
        AES_MemCopy(pCtx->u.prev, pCtx->buffer, AES_EX_BLOCK_SIZE);
    } else {
        AES_MemCopy(au8OutBlock, au8Tmp, AES_EX_BLOCK_SIZE);
    }
    AES_MemCopy(&pOut[*pu32OutLen], au8OutBlock, AES_EX_BLOCK_SIZE);
    *pu32OutLen += AES_EX_BLOCK_SIZE;
    return LL_OK;
}

/**
 * @brief  Update for ECB/CBC encryption: emit each full 16-byte block, buffer
 *         the trailing partial block for padding in Final.
 */
static int32_t AES_BlockEncrypt_Update(stc_aes_ctx_t *pCtx, const uint8_t *pIn, uint32_t u32InLen,
                                       uint8_t *pOut, uint32_t *pu32OutLen)
{
    uint32_t u32Offset = 0U;

    *pu32OutLen = 0U;
    while (u32Offset < u32InLen) {
        uint32_t u32Space = AES_EX_BLOCK_SIZE - pCtx->buffer_len;
        uint32_t u32Copy = ((u32InLen - u32Offset) < u32Space) ? (u32InLen - u32Offset) : u32Space;
        uint8_t au8OutBlock[AES_EX_BLOCK_SIZE];
        int32_t i32Ret;

        AES_MemCopy(&pCtx->buffer[pCtx->buffer_len], &pIn[u32Offset], u32Copy);
        pCtx->buffer_len += u32Copy;
        u32Offset += u32Copy;

        if (pCtx->buffer_len == AES_EX_BLOCK_SIZE) {
            i32Ret = AES_EncryptOneBlock(pCtx, pCtx->buffer, au8OutBlock);
            if (i32Ret != LL_OK) {
                return i32Ret;
            }
            AES_MemCopy(&pOut[*pu32OutLen], au8OutBlock, AES_EX_BLOCK_SIZE);
            *pu32OutLen += AES_EX_BLOCK_SIZE;
            pCtx->buffer_len = 0U;
            pCtx->started = 1U;
        }
    }
    return LL_OK;
}

/**
 * @brief  Update for ECB/CBC decryption: output lags one block. A full block is
 *         only decrypted+emitted when more data follows; otherwise it is
 *         withheld in the buffer and finalized in Final (for PKCS#7 check).
 */
static int32_t AES_BlockDecrypt_Update(stc_aes_ctx_t *pCtx, const uint8_t *pIn, uint32_t u32InLen,
                                       uint8_t *pOut, uint32_t *pu32OutLen)
{
    uint32_t u32Offset = 0U;
    int32_t i32Ret;

    *pu32OutLen = 0U;
    /* A block withheld from a previous Update is no longer the last one now
       that new data has arrived -> emit it first. */
    if ((pCtx->buffer_len == AES_EX_BLOCK_SIZE) && (u32InLen > 0U)) {
        i32Ret = AES_DecryptAndEmit(pCtx, pOut, pu32OutLen);
        if (i32Ret != LL_OK) {
            return i32Ret;
        }
        pCtx->buffer_len = 0U;
        pCtx->started = 1U;
    }

    while (u32Offset < u32InLen) {
        uint32_t u32Space = AES_EX_BLOCK_SIZE - pCtx->buffer_len;
        uint32_t u32Copy = ((u32InLen - u32Offset) < u32Space) ? (u32InLen - u32Offset) : u32Space;

        AES_MemCopy(&pCtx->buffer[pCtx->buffer_len], &pIn[u32Offset], u32Copy);
        pCtx->buffer_len += u32Copy;
        u32Offset += u32Copy;

        if (pCtx->buffer_len == AES_EX_BLOCK_SIZE) {
            if (u32Offset < u32InLen) {
                /* Not the last block: safe to decrypt and emit. */
                i32Ret = AES_DecryptAndEmit(pCtx, pOut, pu32OutLen);
                if (i32Ret != LL_OK) {
                    return i32Ret;
                }
                pCtx->buffer_len = 0U;
                pCtx->started = 1U;
            }
            /* else: withhold the last block for Final. */
        }
    }
    return LL_OK;
}

/**
 * @brief  Update for CTR mode: every input byte produces one output byte by
 *         XOR with the keystream generated from the current counter.
 */
static int32_t AES_CTR_Update_Ex(stc_aes_ctx_t *pCtx, const uint8_t *pIn, uint32_t u32InLen,
                                 uint8_t *pOut, uint32_t *pu32OutLen)
{
    uint32_t u32Idx = 0U;

    *pu32OutLen = 0U;
    while (u32Idx < u32InLen) {
        int32_t i32Ret;
        uint32_t u32Avail;
        uint32_t u32Chunk;
        uint32_t u32J;

        if (pCtx->ks_pos == AES_EX_BLOCK_SIZE) {
            i32Ret = AES_CryptBlock_Ex(pCtx->key, pCtx->key_size, AES_EX_DIR_ENCRYPT,
                                       pCtx->u.ctr, pCtx->ks);
            if (i32Ret != LL_OK) {
                return i32Ret;
            }
            pCtx->ks_pos = 0U;
            pCtx->started = 1U;
        }
        u32Avail = AES_EX_BLOCK_SIZE - pCtx->ks_pos;
        u32Chunk = ((u32InLen - u32Idx) < u32Avail) ? (u32InLen - u32Idx) : u32Avail;
        for (u32J = 0U; u32J < u32Chunk; u32J++) {
            pOut[*pu32OutLen + u32J] = (uint8_t)(pIn[u32Idx + u32J] ^ pCtx->ks[pCtx->ks_pos + u32J]);
        }
        *pu32OutLen += u32Chunk;
        u32Idx += u32Chunk;
        pCtx->ks_pos += u32Chunk;
        if (pCtx->ks_pos == AES_EX_BLOCK_SIZE) {
            AES_CTR_Increment(pCtx->u.ctr);
        }
    }
    return LL_OK;
}

/*******************************************************************************
 * Function implementation - global ('extern')
 ******************************************************************************/
/**
 * @defgroup AES_EX_Global_Functions AES_EX Global Functions
 * @{
 */

int32_t AES_Init_Ex(stc_aes_ctx_t *pCtx, uint8_t mode, uint8_t dir,
                    const uint8_t *pKey, uint8_t key_size, const uint8_t *pIv)
{
    if ((pCtx == NULL) || (pKey == NULL)) {
        return LL_ERR_INVD_PARAM;
    }
    if ((mode != AES_EX_MODE_ECB) && (mode != AES_EX_MODE_CBC) && (mode != AES_EX_MODE_CTR)) {
        return LL_ERR_INVD_PARAM;
    }
    if ((dir != AES_EX_DIR_ENCRYPT) && (dir != AES_EX_DIR_DECRYPT)) {
        return LL_ERR_INVD_PARAM;
    }
    if ((key_size != AES_KEY_SIZE_16BYTE) &&
        (key_size != AES_KEY_SIZE_24BYTE) &&
        (key_size != AES_KEY_SIZE_32BYTE)) {
        return LL_ERR_INVD_PARAM;
    }
    if ((mode != AES_EX_MODE_ECB) && (pIv == NULL)) {
        return LL_ERR_INVD_PARAM;
    }

    AES_MemSet((uint8_t *)pCtx, 0U, sizeof(stc_aes_ctx_t));
    pCtx->mode = mode;
    pCtx->dir = dir;
    pCtx->key_size = key_size;
    AES_MemCopy(pCtx->key, pKey, key_size);
    if (mode != AES_EX_MODE_ECB) {
        /* CBC: prev initialized to IV. CTR: counter initialized to pIv. */
        AES_MemCopy(pCtx->u.prev, pIv, AES_EX_BLOCK_SIZE);
    }
    pCtx->ks_pos = AES_EX_BLOCK_SIZE;   /* force first CTR keystream generation */
    pCtx->started = 0U;
    pCtx->finished = 0U;
    return LL_OK;
}

int32_t AES_Update_Ex(stc_aes_ctx_t *pCtx, const uint8_t *pIn, uint32_t u32InLen,
                      uint8_t *pOut, uint32_t *pu32OutLen)
{
    if ((pCtx == NULL) || (pu32OutLen == NULL)) {
        return LL_ERR_INVD_PARAM;
    }
    if (pCtx->finished != 0U) {
        return LL_ERR_INVD_PARAM;
    }
    if ((pIn == NULL) && (u32InLen != 0U)) {
        return LL_ERR_INVD_PARAM;
    }
    if ((u32InLen != 0U) && (pOut == NULL)) {
        return LL_ERR_INVD_PARAM;
    }
    *pu32OutLen = 0U;
    if (u32InLen == 0U) {
        return LL_OK;
    }

    switch (pCtx->mode) {
        case AES_EX_MODE_CTR:
            return AES_CTR_Update_Ex(pCtx, pIn, u32InLen, pOut, pu32OutLen);
        case AES_EX_MODE_ECB:
        case AES_EX_MODE_CBC:
            if (pCtx->dir == AES_EX_DIR_ENCRYPT) {
                return AES_BlockEncrypt_Update(pCtx, pIn, u32InLen, pOut, pu32OutLen);
            }
            return AES_BlockDecrypt_Update(pCtx, pIn, u32InLen, pOut, pu32OutLen);
        default:
            return LL_ERR_INVD_PARAM;
    }
}

int32_t AES_Final_Ex(stc_aes_ctx_t *pCtx, uint8_t *pOut, uint32_t *pu32OutLen)
{
    int32_t i32Ret;

    if ((pCtx == NULL) || (pOut == NULL) || (pu32OutLen == NULL)) {
        return LL_ERR_INVD_PARAM;
    }
    if (pCtx->finished != 0U) {
        return LL_ERR_INVD_PARAM;
    }

    *pu32OutLen = 0U;

    if (pCtx->mode == AES_EX_MODE_CTR) {
        /* CTR is a stream cipher: nothing to flush. */
        pCtx->finished = 1U;
        AES_ClearSensitive(pCtx);
        return LL_OK;
    }

    if (pCtx->dir == AES_EX_DIR_ENCRYPT) {
        uint8_t au8Block[AES_EX_BLOCK_SIZE];
        uint8_t u8Pad;

        /* If a full block was buffered, emit it first, then append an all-
           padding block (PKCS#7 always produces at least one padding byte). */
        if (pCtx->buffer_len == AES_EX_BLOCK_SIZE) {
            i32Ret = AES_EncryptOneBlock(pCtx, pCtx->buffer, pOut);
            if (i32Ret != LL_OK) {
                AES_ClearSensitive(pCtx);
                return i32Ret;
            }
            *pu32OutLen += AES_EX_BLOCK_SIZE;
            pOut += AES_EX_BLOCK_SIZE;
            pCtx->buffer_len = 0U;
        }

        u8Pad = (uint8_t)(AES_EX_BLOCK_SIZE - pCtx->buffer_len);
        AES_MemCopy(au8Block, pCtx->buffer, pCtx->buffer_len);
        AES_MemSet(&au8Block[pCtx->buffer_len], u8Pad, (uint32_t)u8Pad);

        i32Ret = AES_EncryptOneBlock(pCtx, au8Block, pOut);
        if (i32Ret != LL_OK) {
            AES_ClearSensitive(pCtx);
            return i32Ret;
        }
        *pu32OutLen += AES_EX_BLOCK_SIZE;
    } else {
        uint8_t au8Tmp[AES_EX_BLOCK_SIZE];
        uint8_t au8OutBlock[AES_EX_BLOCK_SIZE];
        uint8_t u8Pad;
        uint8_t i;

        /* ECB/CBC ciphertext must be a multiple of the block size; the last
           block is withheld in the buffer and handled here. */
        if (pCtx->buffer_len != AES_EX_BLOCK_SIZE) {
            AES_ClearSensitive(pCtx);
            return LL_ERR_INVD_PARAM;
        }
        i32Ret = AES_CryptBlock_Ex(pCtx->key, pCtx->key_size, AES_EX_DIR_DECRYPT,
                                   pCtx->buffer, au8Tmp);
        if (i32Ret != LL_OK) {
            AES_ClearSensitive(pCtx);
            return i32Ret;
        }
        if (pCtx->mode == AES_EX_MODE_CBC) {
            AES_XorBlock(au8OutBlock, au8Tmp, pCtx->u.prev);
        } else {
            AES_MemCopy(au8OutBlock, au8Tmp, AES_EX_BLOCK_SIZE);
        }
        /* Strict PKCS#7 validation. */
        u8Pad = au8OutBlock[AES_EX_BLOCK_SIZE - 1U];
        if ((u8Pad == 0U) || (u8Pad > AES_EX_BLOCK_SIZE)) {
            AES_ClearSensitive(pCtx);
            return LL_ERR_INVD_PARAM;
        }
        for (i = (uint8_t)(AES_EX_BLOCK_SIZE - u8Pad); i < AES_EX_BLOCK_SIZE; i++) {
            if (au8OutBlock[i] != u8Pad) {
                AES_ClearSensitive(pCtx);
                return LL_ERR_INVD_PARAM;
            }
        }
        AES_MemCopy(pOut, au8OutBlock, (uint32_t)(AES_EX_BLOCK_SIZE - u8Pad));
        *pu32OutLen = (uint32_t)(AES_EX_BLOCK_SIZE - u8Pad);
    }

    pCtx->finished = 1U;
    AES_ClearSensitive(pCtx);
    return LL_OK;
}

/**
 * @}
 */

#endif /* LL_AES_ENABLE */

/**
 * @}
 */

/**
 * @}
 */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
