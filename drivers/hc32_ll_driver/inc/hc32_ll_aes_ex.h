/**
 *******************************************************************************
 * @file  hc32_ll_aes_ex.h
 * @brief Streaming AES driver (ECB / CBC / CTR) for HC32F4xx.
 *        This file provides incremental AES APIs whose state is fully held in
 *        a caller-owned context, so that concurrent callers do not share any
 *        file-global variable. Note: the AES hardware is a single instance on
 *        the chip, so callers must serialize hardware access with an external
 *        mutex / critical section when used from multiple tasks.
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
#ifndef __HC32_LL_AES_EX_H__
#define __HC32_LL_AES_EX_H__

#include "hc32_ll_aes.h"

#ifdef __cplusplus
extern "C" {
#endif

#if (LL_AES_ENABLE == DDL_ON)

/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/

/**
 * @defgroup AES_Ex_Mode AES streaming mode
 * @{
 */
#define AES_EX_MODE_ECB                 (0U)
#define AES_EX_MODE_CBC                 (1U)
#define AES_EX_MODE_CTR                 (2U)
/**
 * @}
 */

/**
 * @defgroup AES_Ex_Dir AES direction
 * @{
 */
#define AES_EX_DIR_ENCRYPT              (0U)
#define AES_EX_DIR_DECRYPT              (1U)
/**
 * @}
 */

#define AES_EX_BLOCK_SIZE               (16U)

/**
 * @brief Streaming AES context. All state is held here; no file-global state
 *        is used. The caller must provide a valid (stack/static/heap) instance.
 */
typedef struct {
    uint8_t  mode;                     /*!< AES_EX_MODE_xxx */
    uint8_t  dir;                      /*!< AES_EX_DIR_xxx (ignored for CTR) */
    uint8_t  key_size;                 /*!< 16 / 24 / 32 */
    uint8_t  started;                  /*!< Whether at least one block has been processed */
    uint8_t  finished;                 /*!< Whether Final has been called */
    uint8_t  key[32];                  /*!< AES key (biggest key size 32 bytes) */
    union {
        uint8_t prev[16];              /*!< CBC: previous ciphertext block (initialized to IV) */
        uint8_t ctr[16];               /*!< CTR: current counter block */
    } u;
    uint8_t  buffer[16];               /*!< ECB/CBC: buffered partial block, or withheld last ciphertext block when decrypting */
    uint32_t buffer_len;               /*!< Valid bytes in buffer */
    uint8_t  ks[16];                   /*!< CTR: current keystream block */
    uint32_t ks_pos;                   /*!< CTR: bytes already consumed in ks */
} stc_aes_ctx_t;

/*******************************************************************************
 * Global function prototypes (definition in C source)
 ******************************************************************************/

/**
 * @brief  Initialize a streaming AES context.
 * @param  [out] pCtx       Pointer to the AES context (caller-owned).
 * @param  [in]  mode       Working mode @ref AES_Ex_Mode.
 * @param  [in]  dir        Direction @ref AES_Ex_Dir (ignored in CTR).
 * @param  [in]  pKey       Pointer to the key buffer.
 * @param  [in]  key_size   Key size in bytes: AES_KEY_SIZE_16BYTE / 24 / 32.
 * @param  [in]  pIv        Pointer to the 16-byte IV/Counter. NULL for ECB.
 * @retval LL_OK on success, LL_ERR_INVD_PARAM on invalid argument.
 */
int32_t AES_Init_Ex(stc_aes_ctx_t *pCtx, uint8_t mode, uint8_t dir,
                    const uint8_t *pKey, uint8_t key_size, const uint8_t *pIv);

/**
 * @brief  Feed data to the streaming AES operation. May be called multiple times.
 *         Output bytes are written to pOut; the number of bytes produced is
 *         reported in *pu32OutLen. For ECB/CBC encryption a short final block
 *         is buffered until Final. For ECB/CBC decryption output lags one block
 *         (the last ciphertext block is withheld for padding check in Final).
 *         For CTR every input byte produces one output byte immediately.
 * @param  [in,out] pCtx        Pointer to the AES context.
 * @param  [in]     pIn         Pointer to the input data.
 * @param  [in]     u32InLen    Input length in bytes.
 * @param  [out]    pOut        Pointer to the output buffer.
 * @param  [out]    pu32OutLen  Receives the number of bytes written to pOut.
 * @retval LL_OK on success, LL_ERR_INVD_PARAM on invalid argument/state,
 *         LL_ERR_TIMEOUT on hardware timeout.
 */
int32_t AES_Update_Ex(stc_aes_ctx_t *pCtx, const uint8_t *pIn, uint32_t u32InLen,
                      uint8_t *pOut, uint32_t *pu32OutLen);

/**
 * @brief  Finalize the streaming AES operation.
 *         For ECB/CBC encryption: applies PKCS#7 padding, outputs the last
 *         block(s). For ECB/CBC decryption: decrypts the withheld last block,
 *         validates and strips PKCS#7 padding. For CTR: no-op (zero output).
 *         After a successful Final the key material inside the context is
 *         zeroed and the context cannot be reused.
 * @param  [in,out] pCtx        Pointer to the AES context.
 * @param  [out]    pOut        Pointer to the output buffer.
 * @param  [out]    pu32OutLen  Receives the number of bytes written to pOut.
 * @retval LL_OK on success, LL_ERR_INVD_PARAM on invalid argument/state or
 *         bad PKCS#7 padding / non-block-aligned ciphertext, LL_ERR_TIMEOUT
 *         on hardware timeout.
 */
int32_t AES_Final_Ex(stc_aes_ctx_t *pCtx, uint8_t *pOut, uint32_t *pu32OutLen);

#endif /* LL_AES_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* __HC32_LL_AES_EX_H__ */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
