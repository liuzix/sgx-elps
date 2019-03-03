/*
 * Generated by util/mkerr.pl DO NOT EDIT
 * Copyright 1995-2018 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#ifndef HEADER_DSAERR_H
# define HEADER_DSAERR_H

# include <openssl/opensslconf.h>

# ifndef OPENSSL_NO_DSA

#  ifdef  __cplusplus
extern "C"
#  endif
int ERR_load_DSA_strings(void);

/*
 * DSA function codes.
 */
#  define DSA_F_DSAPARAMS_PRINT                            100
#  define DSA_F_DSAPARAMS_PRINT_FP                         101
#  define DSA_F_DSA_BUILTIN_PARAMGEN                       125
#  define DSA_F_DSA_BUILTIN_PARAMGEN2                      126
#  define DSA_F_DSA_DO_SIGN                                112
#  define DSA_F_DSA_DO_VERIFY                              113
#  define DSA_F_DSA_METH_DUP                               127
#  define DSA_F_DSA_METH_NEW                               128
#  define DSA_F_DSA_METH_SET1_NAME                         129
#  define DSA_F_DSA_NEW_METHOD                             103
#  define DSA_F_DSA_PARAM_DECODE                           119
#  define DSA_F_DSA_PRINT_FP                               105
#  define DSA_F_DSA_PRIV_DECODE                            115
#  define DSA_F_DSA_PRIV_ENCODE                            116
#  define DSA_F_DSA_PUB_DECODE                             117
#  define DSA_F_DSA_PUB_ENCODE                             118
#  define DSA_F_DSA_SIGN                                   106
#  define DSA_F_DSA_SIGN_SETUP                             107
#  define DSA_F_DSA_SIG_NEW                                102
#  define DSA_F_OLD_DSA_PRIV_DECODE                        122
#  define DSA_F_PKEY_DSA_CTRL                              120
#  define DSA_F_PKEY_DSA_CTRL_STR                          104
#  define DSA_F_PKEY_DSA_KEYGEN                            121

/*
 * DSA reason codes.
 */
#  define DSA_R_BAD_Q_VALUE                                102
#  define DSA_R_BN_DECODE_ERROR                            108
#  define DSA_R_BN_ERROR                                   109
#  define DSA_R_DECODE_ERROR                               104
#  define DSA_R_INVALID_DIGEST_TYPE                        106
#  define DSA_R_INVALID_PARAMETERS                         112
#  define DSA_R_MISSING_PARAMETERS                         101
#  define DSA_R_MODULUS_TOO_LARGE                          103
#  define DSA_R_NO_PARAMETERS_SET                          107
#  define DSA_R_PARAMETER_ENCODING_ERROR                   105
#  define DSA_R_Q_NOT_PRIME                                113
#  define DSA_R_SEED_LEN_SMALL                             110

# endif
#endif
