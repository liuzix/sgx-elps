/*
 * Generated by util/mkerr.pl DO NOT EDIT
 * Copyright 1995-2018 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#include <openssl/err.h>
#include <openssl/bnerr.h>

#ifndef OPENSSL_NO_ERR

static const ERR_STRING_DATA BN_str_functs[] = {
    {ERR_PACK(ERR_LIB_BN, BN_F_BNRAND, 0), "bnrand"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BNRAND_RANGE, 0), "bnrand_range"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_BLINDING_CONVERT_EX, 0),
     "BN_BLINDING_convert_ex"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_BLINDING_CREATE_PARAM, 0),
     "BN_BLINDING_create_param"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_BLINDING_INVERT_EX, 0),
     "BN_BLINDING_invert_ex"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_BLINDING_NEW, 0), "BN_BLINDING_new"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_BLINDING_UPDATE, 0), "BN_BLINDING_update"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_BN2DEC, 0), "BN_bn2dec"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_BN2HEX, 0), "BN_bn2hex"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_COMPUTE_WNAF, 0), "bn_compute_wNAF"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_CTX_GET, 0), "BN_CTX_get"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_CTX_NEW, 0), "BN_CTX_new"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_CTX_START, 0), "BN_CTX_start"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_DIV, 0), "BN_div"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_DIV_RECP, 0), "BN_div_recp"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_EXP, 0), "BN_exp"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_EXPAND_INTERNAL, 0), "bn_expand_internal"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_GENCB_NEW, 0), "BN_GENCB_new"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_GENERATE_DSA_NONCE, 0),
     "BN_generate_dsa_nonce"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_GENERATE_PRIME_EX, 0),
     "BN_generate_prime_ex"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_GF2M_MOD, 0), "BN_GF2m_mod"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_GF2M_MOD_EXP, 0), "BN_GF2m_mod_exp"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_GF2M_MOD_MUL, 0), "BN_GF2m_mod_mul"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_GF2M_MOD_SOLVE_QUAD, 0),
     "BN_GF2m_mod_solve_quad"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_GF2M_MOD_SOLVE_QUAD_ARR, 0),
     "BN_GF2m_mod_solve_quad_arr"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_GF2M_MOD_SQR, 0), "BN_GF2m_mod_sqr"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_GF2M_MOD_SQRT, 0), "BN_GF2m_mod_sqrt"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_LSHIFT, 0), "BN_lshift"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_MOD_EXP2_MONT, 0), "BN_mod_exp2_mont"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_MOD_EXP_MONT, 0), "BN_mod_exp_mont"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_MOD_EXP_MONT_CONSTTIME, 0),
     "BN_mod_exp_mont_consttime"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_MOD_EXP_MONT_WORD, 0),
     "BN_mod_exp_mont_word"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_MOD_EXP_RECP, 0), "BN_mod_exp_recp"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_MOD_EXP_SIMPLE, 0), "BN_mod_exp_simple"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_MOD_INVERSE, 0), "BN_mod_inverse"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_MOD_INVERSE_NO_BRANCH, 0),
     "BN_mod_inverse_no_branch"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_MOD_LSHIFT_QUICK, 0), "BN_mod_lshift_quick"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_MOD_SQRT, 0), "BN_mod_sqrt"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_MONT_CTX_NEW, 0), "BN_MONT_CTX_new"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_MPI2BN, 0), "BN_mpi2bn"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_NEW, 0), "BN_new"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_POOL_GET, 0), "BN_POOL_get"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_RAND, 0), "BN_rand"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_RAND_RANGE, 0), "BN_rand_range"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_RECP_CTX_NEW, 0), "BN_RECP_CTX_new"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_RSHIFT, 0), "BN_rshift"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_SET_WORDS, 0), "bn_set_words"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_STACK_PUSH, 0), "BN_STACK_push"},
    {ERR_PACK(ERR_LIB_BN, BN_F_BN_USUB, 0), "BN_usub"},
    {0, NULL}
};

static const ERR_STRING_DATA BN_str_reasons[] = {
    {ERR_PACK(ERR_LIB_BN, 0, BN_R_ARG2_LT_ARG3), "arg2 lt arg3"},
    {ERR_PACK(ERR_LIB_BN, 0, BN_R_BAD_RECIPROCAL), "bad reciprocal"},
    {ERR_PACK(ERR_LIB_BN, 0, BN_R_BIGNUM_TOO_LONG), "bignum too long"},
    {ERR_PACK(ERR_LIB_BN, 0, BN_R_BITS_TOO_SMALL), "bits too small"},
    {ERR_PACK(ERR_LIB_BN, 0, BN_R_CALLED_WITH_EVEN_MODULUS),
    "called with even modulus"},
    {ERR_PACK(ERR_LIB_BN, 0, BN_R_DIV_BY_ZERO), "div by zero"},
    {ERR_PACK(ERR_LIB_BN, 0, BN_R_ENCODING_ERROR), "encoding error"},
    {ERR_PACK(ERR_LIB_BN, 0, BN_R_EXPAND_ON_STATIC_BIGNUM_DATA),
    "expand on static bignum data"},
    {ERR_PACK(ERR_LIB_BN, 0, BN_R_INPUT_NOT_REDUCED), "input not reduced"},
    {ERR_PACK(ERR_LIB_BN, 0, BN_R_INVALID_LENGTH), "invalid length"},
    {ERR_PACK(ERR_LIB_BN, 0, BN_R_INVALID_RANGE), "invalid range"},
    {ERR_PACK(ERR_LIB_BN, 0, BN_R_INVALID_SHIFT), "invalid shift"},
    {ERR_PACK(ERR_LIB_BN, 0, BN_R_NOT_A_SQUARE), "not a square"},
    {ERR_PACK(ERR_LIB_BN, 0, BN_R_NOT_INITIALIZED), "not initialized"},
    {ERR_PACK(ERR_LIB_BN, 0, BN_R_NO_INVERSE), "no inverse"},
    {ERR_PACK(ERR_LIB_BN, 0, BN_R_NO_SOLUTION), "no solution"},
    {ERR_PACK(ERR_LIB_BN, 0, BN_R_PRIVATE_KEY_TOO_LARGE),
    "private key too large"},
    {ERR_PACK(ERR_LIB_BN, 0, BN_R_P_IS_NOT_PRIME), "p is not prime"},
    {ERR_PACK(ERR_LIB_BN, 0, BN_R_TOO_MANY_ITERATIONS), "too many iterations"},
    {ERR_PACK(ERR_LIB_BN, 0, BN_R_TOO_MANY_TEMPORARY_VARIABLES),
    "too many temporary variables"},
    {0, NULL}
};

#endif

int ERR_load_BN_strings(void)
{
#ifndef OPENSSL_NO_ERR
    if (ERR_func_error_string(BN_str_functs[0].error) == NULL) {
        ERR_load_strings_const(BN_str_functs);
        ERR_load_strings_const(BN_str_reasons);
    }
#endif
    return 1;
}
