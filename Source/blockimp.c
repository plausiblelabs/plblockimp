/*
 * Author: Landon Fuller <landonf@plausible.coop>
 *
 * Copyright 2010-2011 Plausible Labs Cooperative, Inc.
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge,
 * to any person obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "blockimp.h"
#include "blockimp_private.h"

#include "trampoline_table.h"

#include <stdio.h>
#include <Block.h>
#include <dispatch/dispatch.h>
#include <dlfcn.h>

// Convenience for falling through to the system implementation.
static BOOL fallthroughEnabled = YES;

#define NEXT(name, ...) do { \
static dispatch_once_t fptrOnce; \
static __typeof__(&name) fptr; \
dispatch_once(&fptrOnce, ^{ fptr = dlsym(RTLD_NEXT, #name); });\
if (fallthroughEnabled && fptr != NULL) \
return fptr(__VA_ARGS__); \
} while(0)

void PLBlockIMPSetFallthroughEnabled(BOOL enabled) {
    fallthroughEnabled = enabled;
}

IMP imp_implementationWithBlock(PLObjectPtr block) {
    NEXT(imp_implementationWithBlock, block);

    return pl_imp_implementationWithBlock(block);
}

PLObjectPtr imp_getBlock(IMP anImp) {
    NEXT(imp_getBlock, anImp);

    return pl_imp_getBlock(anImp);
}

BOOL imp_removeBlock(IMP anImp) {
    NEXT(imp_removeBlock, anImp);

    return imp_removeBlock(anImp);
}

/* The ARM64 ABI does not require (or support) the _stret objc_msgSend variant */
#ifdef __arm64__
#define STRET_TABLE_REQUIRED 0
#define STRET_TABLE_CONFIG pl_blockimp_table_page_config
#define STRET_TABLE blockimp_table
#else
#define STRET_TABLE_REQUIRED 1
#define STRET_TABLE_CONFIG pl_blockimp_table_page_config
#define STRET_TABLE blockimp_table_stret
#endif

#pragma mark Trampolines

/* Global lock for our mutable state. Must be held when accessing the trampoline tables. */
static pthread_mutex_t blockimp_lock = PTHREAD_MUTEX_INITIALIZER;

/* Trampoline tables for objc_msgSend() dispatch. */
static pl_trampoline_table *blockimp_table = NULL;

#if STRET_TABLE_REQUIRED
/* Trampoline tables for objc_msgSend_stret() dispatch. */
static pl_trampoline_table *blockimp_table_stret = NULL;
#endif /* STRET_TABLE_REQUIRED */

/**
 * 
 */
IMP pl_imp_implementationWithBlock (void *block) {
    /* Allocate the appropriate trampoline type. */
    pl_trampoline *tramp;
    struct Block_layout *bl = block;
    if (bl->flags & BLOCK_USE_STRET) {
        tramp = pl_trampoline_alloc(&STRET_TABLE_CONFIG, &blockimp_lock, &STRET_TABLE);
    } else {
        tramp = pl_trampoline_alloc(&pl_blockimp_table_page_config, &blockimp_lock, &blockimp_table);
    }
    
    /* Configure the trampoline */
    void **config = pl_trampoline_data_ptr(tramp->trampoline);
    config[0] = Block_copy(block);
    config[1] = tramp;

    /* Return the function pointer. */
    return (IMP) tramp->trampoline;
}

/**
 *
 */
void *pl_imp_getBlock(IMP anImp) {
    /* Fetch the config data and return the block reference. */
    void **config = pl_trampoline_data_ptr(anImp);
    return config[0];
}

/**
 *
 */
BOOL pl_imp_removeBlock(IMP anImp) {
    /* Fetch the config data */
    void **config = pl_trampoline_data_ptr(anImp);
    struct Block_layout *bl = config[0];
    pl_trampoline *tramp = config[1];

    /* Drop the trampoline allocation */
    if (bl->flags & BLOCK_USE_STRET) {
        pl_trampoline_free(&blockimp_lock, &STRET_TABLE, tramp);
    } else {
        pl_trampoline_free(&blockimp_lock, &blockimp_table, tramp);
    }

    /* Release the block */
    Block_release(config[0]);

    // TODO - what does this return value mean?
    return YES;
}