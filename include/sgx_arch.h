/*
 * Some of the code is based on Intel's sample code, which has the following
 * license:
 */

/*
 * Copyright (C) 2011-2018 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
*/
#ifndef SGX_ARCH_H
#define SGX_ARCH_H
#include <stdint.h>

using vaddr = uint64_t;
#define VADDR_VOIDP(x) ((void *)x)
#define VADDR_PTR(x, t) ((t *)x)
#define PTR_VADDR(x) ((vaddr)x)
#define SGX_PAGE_SIZE 0x1000

#define SGX_FLAGS_INITTED 0x0000000000000001ULL
#define SGX_FLAGS_DEBUG 0x0000000000000002ULL
#define SGX_FLAGS_MODE64BIT 0x0000000000000004ULL
#define SGX_FLAGS_PROVISION_KEY 0x0000000000000010ULL
#define SGX_FLAGS_EINITTOKEN_KEY 0x0000000000000020ULL

#define SGX_SECS_RESERVED1_SIZE 24
#define SGX_SECS_RESERVED2_SIZE 32
#define SGX_SECS_RESERVED3_SIZE 96
#define SGX_SECS_RESERVED4_SIZE 3836

typedef uint64_t si_flags_t;
#define SI_FLAG_NONE 0x0
#define SI_FLAG_R 0x1                                /* Read Access */
#define SI_FLAG_W 0x2                                /* Write Access */
#define SI_FLAG_X 0x4                                /* Execute Access */
#define SI_FLAG_PT_LOW_BIT 0x8                       /* PT low bit */
#define SI_FLAG_PT_MASK (0xFF << SI_FLAG_PT_LOW_BIT) /* Page Type Mask [15:8]  \
                                                      */
#define SI_FLAG_SECS (0x00 << SI_FLAG_PT_LOW_BIT)    /* SECS */
#define SI_FLAG_TCS (0x01 << SI_FLAG_PT_LOW_BIT)     /* TCS */
#define SI_FLAG_REG (0x02 << SI_FLAG_PT_LOW_BIT)     /* Regular Page */
#define SI_FLAG_TRIM (0x04 << SI_FLAG_PT_LOW_BIT)    /* Trim Page */
#define SI_FLAG_PENDING 0x8
#define SI_FLAG_MODIFIED 0x10
#define SI_FLAG_PR 0x20

#define SI_FLAGS_EXTERNAL                                                      \
    (SI_FLAG_PT_MASK | SI_FLAG_R | SI_FLAG_W |                                 \
     SI_FLAG_X) /* Flags visible/usable by instructions */
#define SI_FLAGS_R (SI_FLAG_R | SI_FLAG_REG)
#define SI_FLAGS_RW (SI_FLAG_R | SI_FLAG_W | SI_FLAG_REG)
#define SI_FLAGS_RX (SI_FLAG_R | SI_FLAG_X | SI_FLAG_REG)
#define SI_FLAGS_TCS (SI_FLAG_TCS)
#define SI_FLAGS_SECS (SI_FLAG_SECS)
#define SI_MASK_TCS (SI_FLAG_PT_MASK)
#define SI_MASK_MEM_ATTRIBUTE (0x7)

#define THREAD_STACK_SIZE 2

/*SECS data structure*/
typedef struct _secs_t {
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

typedef enum {
    ENCLAVE_PAGE_READ =
        1 << 0, /* Enables read access to the committed region of pages. */
    ENCLAVE_PAGE_WRITE =
        1 << 1, /* Enables write access to the committed region of pages. */
    ENCLAVE_PAGE_EXECUTE =
        1 << 2, /* Enables execute access to the committed region of pages. */
    ENCLAVE_PAGE_THREAD_CONTROL =
        1 << 8, /* The page contains a thread control structure. */
    ENCLAVE_PAGE_UNVALIDATED =
        1 << 12, /* The page contents that you supply are excluded from
                    measurement and content validation. */
} enclave_page_properties_t;

typedef struct _sec_info_t {
    si_flags_t flags;
    uint64_t reserved[7];
} sec_info_t;

/* TCS data structure */
typedef struct _tcs_t {
    uint64_t state;
    uint64_t flags;
    uint64_t ossa;
    uint32_t cssa;
    uint32_t nssa;
    uint64_t oentry;
    uint64_t aep;
    uint64_t ofsbase;
    uint64_t ogsbase;
    uint32_t fslimit;
    uint32_t gslimit;
    uint64_t reserved[503];
} tcs_t;

typedef struct {
    uint64_t enclave_size;
    uint64_t tcs_offset;
    uint64_t initial_stack_offset;
    void *ssa;
    void *stack;
} enclave_tls;
#endif
