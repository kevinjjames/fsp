/***********************************************************************************************************************
 * Copyright [2020-2022] Renesas Electronics Corporation and/or its affiliates.  All Rights Reserved.
 *
 * This software and documentation are supplied by Renesas Electronics America Inc. and may only be used with products
 * of Renesas Electronics Corp. and its affiliates ("Renesas").  No other uses are authorized.  Renesas products are
 * sold pursuant to Renesas terms and conditions of sale.  Purchasers are solely responsible for the selection and use
 * of Renesas products and Renesas assumes no liability.  No license, express or implied, to any intellectual property
 * right is granted by Renesas. This software is protected under all applicable laws, including copyright laws. Renesas
 * reserves the right to change or discontinue this software and/or this documentation. THE SOFTWARE AND DOCUMENTATION
 * IS DELIVERED TO YOU "AS IS," AND RENESAS MAKES NO REPRESENTATIONS OR WARRANTIES, AND TO THE FULLEST EXTENT
 * PERMISSIBLE UNDER APPLICABLE LAW, DISCLAIMS ALL WARRANTIES, WHETHER EXPLICITLY OR IMPLICITLY, INCLUDING WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NONINFRINGEMENT, WITH RESPECT TO THE SOFTWARE OR
 * DOCUMENTATION.  RENESAS SHALL HAVE NO LIABILITY ARISING OUT OF ANY SECURITY VULNERABILITY OR BREACH.  TO THE MAXIMUM
 * EXTENT PERMITTED BY LAW, IN NO EVENT WILL RENESAS BE LIABLE TO YOU IN CONNECTION WITH THE SOFTWARE OR DOCUMENTATION
 * (OR ANY PERSON OR ENTITY CLAIMING RIGHTS DERIVED FROM YOU) FOR ANY LOSS, DAMAGES, OR CLAIMS WHATSOEVER, INCLUDING,
 * WITHOUT LIMITATION, ANY DIRECT, CONSEQUENTIAL, SPECIAL, INDIRECT, PUNITIVE, OR INCIDENTAL DAMAGES; ANY LOST PROFITS,
 * OTHER ECONOMIC DAMAGE, PROPERTY DAMAGE, OR PERSONAL INJURY; AND EVEN IF RENESAS HAS BEEN ADVISED OF THE POSSIBILITY
 * OF SUCH LOSS, DAMAGES, CLAIMS OR COSTS.
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include "SCE_ProcCommon.h"
#include "hw_sce_ra_private.h"
#include "hw_sce_private.h"
#include "hw_sce_trng_private.h"

/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
#define INIT_REG_ADDR    0x400E0040UL

uint32_t S_RAM[HW_SCE_SRAM_WORD_SIZE];
uint32_t S_HEAP[HW_SCE_SHEAP_WORD_SIZE];
uint32_t S_INST[HW_SCE_SINST_WORD_SIZE];
uint32_t S_INST2[HW_SCE_SINST2_WORD_SIZE];

uint32_t INST_DATA_SIZE;

/*******************************************************
 * The following are valid SCE lifecycle states:
 *
 * CM1(Lifecycle state)
 *
 * CM2(Lifecycle state)
 *
 * SSD(Lifecycle state)
 *
 * NSECSD(Lifecycle state)
 *
 * DPL(Lifecycle state)
 *
 * LCK_DBG(Lifecycle state)
 *
 * LCK_BOOT(Lifecycle state)
 *
 * RMA_REQ(Lifecycle state)
 *
 * RMA_ACK(Lifecycle state)
 ****************************************************/

#define FSP_SCE_DLMMON_MASK    0x0000000F /* for lcs in stored in R_PSCU->DLMMON */

const uint32_t sce_oem_key_size[SCE_OEM_CMD_NUM] =
{
    SCE_OEM_KEY_SIZE_DUMMY_INST_DATA_WORD,
    SCE_OEM_KEY_SIZE_DUMMY_INST_DATA_WORD,
    SCE_OEM_KEY_SIZE_DUMMY_INST_DATA_WORD,
    SCE_OEM_KEY_SIZE_DUMMY_INST_DATA_WORD,
    SCE_OEM_KEY_SIZE_DUMMY_INST_DATA_WORD,
    SCE_OEM_KEY_SIZE_AES128_INST_DATA_WORD,
    SCE_OEM_KEY_SIZE_AES192_INST_DATA_WORD,
    SCE_OEM_KEY_SIZE_AES256_INST_DATA_WORD,
    SCE_OEM_KEY_SIZE_AES128_XTS_INST_DATA_WORD,
    SCE_OEM_KEY_SIZE_AES256_XTS_INST_DATA_WORD,
    SCE_OEM_KEY_SIZE_RSA1024_PUBLICK_KEY_INST_DATA_WORD,
    SCE_OEM_KEY_SIZE_RSA1024_PRIVATE_KEY_INST_DATA_WORD,
    SCE_OEM_KEY_SIZE_RSA2048_PUBLICK_KEY_INST_DATA_WORD,
    SCE_OEM_KEY_SIZE_RSA2048_PRIVATE_KEY_INST_DATA_WORD,
    SCE_OEM_KEY_SIZE_RSA3072_PUBLICK_KEY_INST_DATA_WORD,
    SCE_OEM_KEY_SIZE_RSA3072_PRIVATE_KEY_INST_DATA_WORD,
    SCE_OEM_KEY_SIZE_RSA4096_PUBLICK_KEY_INST_DATA_WORD,
    SCE_OEM_KEY_SIZE_RSA4096_PRIVATE_KEY_INST_DATA_WORD,
    SCE_OEM_KEY_SIZE_ECCP192_PUBLICK_KEY_INST_DATA_WORD,
    SCE_OEM_KEY_SIZE_ECCP192_PRIVATE_KEY_INST_DATA_WORD,
    SCE_OEM_KEY_SIZE_ECCP224_PUBLICK_KEY_INST_DATA_WORD,
    SCE_OEM_KEY_SIZE_ECCP224_PRIVATE_KEY_INST_DATA_WORD,
    SCE_OEM_KEY_SIZE_ECCP256_PUBLICK_KEY_INST_DATA_WORD,
    SCE_OEM_KEY_SIZE_ECCP256_PRIVATE_KEY_INST_DATA_WORD,
    SCE_OEM_KEY_SIZE_ECCP384_PUBLICK_KEY_INST_DATA_WORD,
    SCE_OEM_KEY_SIZE_ECCP384_PRIVATE_KEY_INST_DATA_WORD,
    SCE_OEM_KEY_SIZE_HMAC_SHA224_INST_DATA_WORD,
    SCE_OEM_KEY_SIZE_HMAC_SHA256_INST_DATA_WORD,
    SCE_OEM_KEY_SIZE_ECCP256R1_PUBLICK_KEY_INST_DATA_WORD,
    SCE_OEM_KEY_SIZE_ECCP256R1_PRIVATE_KEY_INST_DATA_WORD,
    SCE_OEM_KEY_SIZE_ECCP384R1_PUBLICK_KEY_INST_DATA_WORD,
    SCE_OEM_KEY_SIZE_ECCP384R1_PRIVATE_KEY_INST_DATA_WORD,
    SCE_OEM_KEY_SIZE_ECCP512R1_PUBLICK_KEY_INST_DATA_WORD,
    SCE_OEM_KEY_SIZE_ECCP512R1_PRIVATE_KEY_INST_DATA_WORD,
    SCE_OEM_KEY_SIZE_ECCSECP256K1_PUBLICK_KEY_INST_DATA_WORD,
    SCE_OEM_KEY_SIZE_ECCSECP256K1_PRIVATE_KEY_INST_DATA_WORD,
};

/* Find the lifecycle state load the hardware unique key */
/* returns fsp_err_t                                     */

fsp_err_t HW_SCE_HUK_Load_LCS (void)
{
    volatile uint32_t * Initialize_Register = (volatile uint32_t *) INIT_REG_ADDR;
    uint32_t            LC[1];
    uint32_t            data[12];
    uint32_t            iLoop;
    LC[0] = (uint32_t) SCE_SSD;
    for (iLoop = 0; iLoop < 12; iLoop++)
    {
        data[iLoop] = change_endian_long(*(Initialize_Register + iLoop));
    }

    return HW_SCE_LoadHukSub(LC, data);
}

/* SCE5B specific initialization functions */
/* returns fsp_err_t                      */

fsp_err_t HW_SCE_McuSpecificInit (void)
{
    fsp_err_t iret = FSP_ERR_CRYPTO_SCE_FAIL;

    // power on the SCE module
    HW_SCE_PowerOn();

    HW_SCE_SoftwareResetSub();
    iret = HW_SCE_SelfCheck1Sub();

    if (FSP_SUCCESS == iret)
    {
        /* Change SCE to little endian mode */
        SCE->REG_1D4H = 0x0000ffff;

        iret = HW_SCE_SelfCheck2Sub();

        if (FSP_SUCCESS == iret)
        {
            iret = HW_SCE_HUK_Load_LCS();
            if (FSP_SUCCESS == iret)
            {
                iret = HW_SCE_SelectStateTransitionCryptgraphicSub();

                if (FSP_SUCCESS == iret)
                {
                    iret = HW_SCE_FwIntegrityCheck();
                }
            }
        }
    }

    return iret;
}

fsp_err_t HW_SCE_RNG_Read (uint32_t * OutData_Text)
{
    if (FSP_SUCCESS != HW_SCE_GenerateRandomNumberSub(OutData_Text))
    {
        return FSP_ERR_CRYPTO_SCE_FAIL;
    }

    return FSP_SUCCESS;
}

fsp_err_t HW_SCE_GenerateOemKeyIndexPrivate (const sce_oem_key_type_t key_type,
                                             const sce_oem_cmd_t      cmd,
                                             const uint8_t          * encrypted_provisioning_key,
                                             const uint8_t          * iv,
                                             const uint8_t          * encrypted_oem_key,
                                             uint32_t               * key_index)
{
    uint32_t indata_key_type[1]        = {0};
    uint32_t indata_cmd[1]             = {0};
    uint32_t install_key_ring_index[1] = {0};
    indata_key_type[0]        = key_type;
    indata_cmd[0]             = (cmd);
    install_key_ring_index[0] = 0U;

    INST_DATA_SIZE = sce_oem_key_size[cmd] - 4U;

    /* Casting uint32_t pointer is used for address. */
    return HW_SCE_GenerateOemKeyIndexSub(indata_key_type,
                                         indata_cmd,
                                         install_key_ring_index,
                                         (uint32_t *) encrypted_provisioning_key,
                                         (uint32_t *) iv,
                                         (uint32_t *) encrypted_oem_key,
                                         key_index);
}

uint32_t change_endian_long (uint32_t a)
{
    return __REV(a);
}

fsp_err_t HW_SCE_Aes192EncryptDecryptInitSub (const uint32_t * InData_Cmd,
                                              const uint32_t * InData_KeyIndex,
                                              const uint32_t * InData_IV)
{
    FSP_PARAMETER_NOT_USED(InData_Cmd);
    FSP_PARAMETER_NOT_USED(InData_KeyIndex);
    FSP_PARAMETER_NOT_USED(InData_IV);

    return FSP_ERR_UNSUPPORTED;
}

void HW_SCE_Aes192EncryptDecryptUpdateSub (const uint32_t * InData_Text, uint32_t * OutData_Text,
                                           const uint32_t MAX_CNT)
{
    FSP_PARAMETER_NOT_USED(InData_Text);
    FSP_PARAMETER_NOT_USED(OutData_Text);
    FSP_PARAMETER_NOT_USED(MAX_CNT);
}

fsp_err_t HW_SCE_Aes192EncryptDecryptFinalSub (void)
{
    return FSP_ERR_UNSUPPORTED;
}

fsp_err_t HW_SCE_Aes192GcmEncryptInitSub(uint32_t *InData_KeyIndex, uint32_t *InData_IV)
{
    FSP_PARAMETER_NOT_USED(InData_KeyIndex);
    FSP_PARAMETER_NOT_USED(InData_IV);
    return FSP_ERR_UNSUPPORTED;
}

void HW_SCE_Aes192GcmEncryptUpdateSub(uint32_t *InData_Text, uint32_t *OutData_Text, uint32_t MAX_CNT)
{
    FSP_PARAMETER_NOT_USED(InData_Text);
    FSP_PARAMETER_NOT_USED(OutData_Text);
    FSP_PARAMETER_NOT_USED(MAX_CNT);
}

fsp_err_t HW_SCE_Aes192GcmEncryptFinalSub(uint32_t *InData_Text, uint32_t *InData_DataALen,
        uint32_t *InData_TextLen, uint32_t *OutData_Text, uint32_t *OutData_DataT)
{
    FSP_PARAMETER_NOT_USED(InData_Text);
    FSP_PARAMETER_NOT_USED(OutData_Text);
    FSP_PARAMETER_NOT_USED(InData_DataALen);
    FSP_PARAMETER_NOT_USED(InData_TextLen);
    FSP_PARAMETER_NOT_USED(OutData_DataT);
    return FSP_ERR_UNSUPPORTED;
}

fsp_err_t HW_SCE_Aes192GcmDecryptInitSub(uint32_t *InData_KeyIndex, uint32_t *InData_IV)
{
    FSP_PARAMETER_NOT_USED(InData_KeyIndex);
    FSP_PARAMETER_NOT_USED(InData_IV);
    return FSP_ERR_UNSUPPORTED;
}

void HW_SCE_Aes192GcmDecryptUpdateSub(uint32_t *InData_Text, uint32_t *OutData_Text, uint32_t MAX_CNT)
{
    FSP_PARAMETER_NOT_USED(InData_Text);
    FSP_PARAMETER_NOT_USED(OutData_Text);
    FSP_PARAMETER_NOT_USED(MAX_CNT);
}

fsp_err_t HW_SCE_Aes192GcmDecryptFinalSub(uint32_t *InData_Text, uint32_t *InData_DataT, uint32_t *InData_DataALen,
        uint32_t *InData_TextLen, uint32_t *InData_DataTLen, uint32_t *OutData_Text)
{
    FSP_PARAMETER_NOT_USED(InData_Text);
    FSP_PARAMETER_NOT_USED(InData_DataT);
    FSP_PARAMETER_NOT_USED(InData_DataALen);
    FSP_PARAMETER_NOT_USED(InData_TextLen);
    FSP_PARAMETER_NOT_USED(InData_DataTLen);
    FSP_PARAMETER_NOT_USED(OutData_Text);
    return FSP_ERR_UNSUPPORTED;
}

void HW_SCE_Aes192GcmDecryptUpdateTransitionSub(void)
{

}

void HW_SCE_Aes192GcmEncryptUpdateTransitionSub(void)
{

}

void HW_SCE_Aes192GcmEncryptUpdateAADSub(uint32_t *InData_DataA, uint32_t MAX_CNT)
{
    FSP_PARAMETER_NOT_USED(InData_DataA);
    FSP_PARAMETER_NOT_USED(MAX_CNT);
}

void HW_SCE_Aes192GcmDecryptUpdateAADSub(uint32_t *InData_DataA, uint32_t MAX_CNT)
{
    FSP_PARAMETER_NOT_USED(InData_DataA);
    FSP_PARAMETER_NOT_USED(MAX_CNT);
}

