/*
 * fatcache - memcache on ssd.
 * Copyright (C) 2013 Twitter, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _FC_ITEMX_H_
#define _FC_ITEMX_H_

#define ITEMX_HASH_POWER   20

struct itemx {
    STAILQ_ENTRY(itemx) tqe;    /* link in index / free q */
    uint8_t             md[20]; /* sha1 message digest */
    uint32_t            sid;    /* owner slab id */
    uint32_t            offset; /* item offset from owner slab base */
    rel_time_t          expiry; /* expiry in secs */
    uint64_t            cas;    /* cas */
} __attribute__ ((__packed__));

STAILQ_HEAD(itemx_tqh, itemx);

rstatus_t itemx_init(void);
void itemx_deinit(void);

bool itemx_empty(void);
bool itemx_expired(struct itemx *itx);
bool itemx_getx(uint8_t* key, uint8_t nkey, uint32_t &sid, uint32_t &off);
void itemx_putx(uint8_t* key, uint8_t nkey, uint32_t sid, uint32_t offset,
           rel_time_t expiry, uint64_t cas);
bool itemx_removex(uint8_t* key, uint8_t nkey);

uint64_t itemx_nalloc(void);
uint64_t itemx_nfree(void);
#endif
