/*
 * Some of the code is based on Intel's sample code, which has the following license:
 */

/*
 * Copyright (C) 2011-2018 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


#ifndef SGX_ARCH_H
#define SGX_ARCH_H

#include <stdint.h>

#define SGX_PAGE_SIZE 0x1000

#define SGX_FLAGS_INITTED        0x0000000000000001ULL
#define SGX_FLAGS_DEBUG          0x0000000000000002ULL
#define SGX_FLAGS_MODE64BIT      0x0000000000000004ULL
#define SGX_FLAGS_PROVISION_KEY  0x0000000000000010ULL
#define SGX_FLAGS_EINITTOKEN_KEY 0x0000000000000020ULL

#define SGX_SECS_RESERVED1_SIZE 24
#define SGX_SECS_RESERVED2_SIZE 32
#define SGX_SECS_RESERVED3_SIZE 96
#define SGX_SECS_RESERVED4_SIZE 3836

/*SECS data structure*/
typedef struct _secs_t
{
    uint64_t size;
    uint64_t base;
    uint32_t ssaframesize;
    uint32_t miscselect;
    uint8_t reserved1[SGX_SECS_RESERVED1_SIZE];
    uint64_t attributes;
    uint64_t xfrm;
    uint32_t mrenclave[8];
    uint8_t reserved2[SGX_SECS_RESERVED2_SIZE];
    uint32_t mrsigner[8];
    uint8_t reserved3[SGX_SECS_RESERVED3_SIZE];
    uint16_t isvprodid;
    uint16_t isvsvn;
    uint8_t reserved4[SGX_SECS_RESERVED4_SIZE];
} secs_t;

#endif
