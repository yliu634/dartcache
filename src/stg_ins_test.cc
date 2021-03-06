//#include "fc_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include "fc_common.h"

#include "fc_queue.h"
#include "fc_log.h"

#include "fc_sha1.h"
#include "fc_time.h"
#include "fc_util.h"

#include "fc_slab.h"
#include "fc_itemx.h"
#include "fc_item.h"
#include "fc_settings.h"

#include "cuckoo_ht.h"
#include "hash2.h"

struct settings settings;          /* fatcache settings */

static void set_options(){

#define FC_CHUNK_SIZE       ITEM_CHUNK_SIZE
#define FC_SLAB_SIZE        SLAB_SIZE

#define FC_DAEMONIZE        true

    //#define FC_LOG_FILE         NULL
#define FC_LOG_FILE         "./log"
#define FC_LOG_DEFAULT      LOG_INFO
#define FC_LOG_MIN          LOG_EMERG
#define FC_LOG_MAX          LOG_PVERB

#define FC_PORT             11211
#define FC_ADDR             "127.0.0.1"

#define FC_HASH_POWER       ITEMX_HASH_POWER

#define FC_FACTOR           1.25

#define FC_INDEX_MEMORY     (64 * MB)
#define FC_SLAB_MEMORY      (64 * MB)

#define FC_SERVER_ID        0
#define FC_SERVER_N         1
    
    settings.daemonize = FC_DAEMONIZE;

    settings.log_filename = (char *)FC_LOG_FILE;
    settings.verbose = 6;//11;//FC_LOG_DEFAULT;

    settings.port = FC_PORT;
    settings.addr = (char *)FC_ADDR;
    settings.hash_power = FC_HASH_POWER;

    settings.factor = FC_FACTOR;
    settings.max_index_memory = FC_INDEX_MEMORY;
    settings.max_slab_memory = FC_SLAB_MEMORY;
    settings.chunk_size = FC_CHUNK_SIZE;
    settings.slab_size = FC_SLAB_SIZE;

    memset(settings.profile, 0, sizeof(settings.profile));
    settings.profile_last_id = SLABCLASS_MAX_ID;

    settings.ssd_device = "/dev/sdb"; //NULL;

    settings.server_id = FC_SERVER_ID;
    settings.server_n = FC_SERVER_N;
    return;
}

static rstatus_t
fc_generate_profile(void)   //generate ctable's size in each layer.
{
    size_t *profile = settings.profile; /* slab profile */
    uint8_t id;                         /* slab class id */
    size_t item_sz, last_item_sz;       /* current and last item chunk size */
    size_t min_item_sz, max_item_sz;    /* min and max item chunk size */

    ASSERT(settings.chunk_size % FC_ALIGNMENT == 0);
    ASSERT(settings.chunk_size <= slab_data_size());

    min_item_sz = settings.chunk_size;
    max_item_sz = slab_data_size();
    id = SLABCLASS_MIN_ID;
    item_sz = min_item_sz;

    while (id < SLABCLASS_MAX_ID && item_sz < max_item_sz) {
        /* save the cur item chunk size */
        last_item_sz = item_sz;
        profile[id] = item_sz;
        id++;
        //profile?????????????????????????????????class?????????;
        /* get the next item chunk size */
        item_sz *= settings.factor;
        if (item_sz == last_item_sz) {
            item_sz++;
        }
        item_sz = FC_ALIGN(item_sz, FC_ALIGNMENT);
    }

    /* last profile entry always has a 1 item/slab of maximum size */
    profile[id] = max_item_sz;
    settings.profile_last_id = id;
    settings.max_chunk_size = max_item_sz;

    return FC_OK;
}

rstatus_t init(){
    rstatus_t status;

    status = log_init(settings.verbose, settings.log_filename);
    if (status != FC_OK) {
        return status;
    }
    
    status = time_init();
    if (status != FC_OK) {
        return status;
    }
    
    status = itemx_init();// how many itemx could be inited within memory.
    if (status != FC_OK) {
        return status;
    }
    
    item_init();// useless

    status = slab_init();
    if (status != FC_OK) {
        return status;
    }

    return FC_OK;
}

int put(char* key, int nkey, char* value, int vlen, int expiry, int flags){
    uint8_t  md[20];
    uint32_t hash;
    uint8_t * tmp_key = (uint8_t *) key;
    sha1(tmp_key, nkey, md);
    hash = sha1_hash(md);

    uint8_t cid;
    struct item *it;
    cid = item_slabcid(nkey, vlen);//remain
    if (cid == SLABCLASS_INVALID_ID) {
        return -1;
    }

    itemx_removex(key, nkey);
    it = item_get((uint8_t*)key, nkey, cid, vlen, time_reltime(expiry),
                  flags, md, hash);
    if (it == NULL) {
        return 0;
    }
    memcpy(item_data(it), value, (size_t)(vlen));
    return 1;
}

int get(char* key, int nkey, char* value){
    /*uint8_t  md[20];
    uint32_t hash;

    struct itemx *itx;
    struct item *it;

    uint8_t * tmp_key = (uint8_t *) key;
    sha1(tmp_key, nkey, md);
    hash = sha1_hash(md);
    
    itx = itemx_getx(hash,md);
    if (itx == NULL) {
        return 1;
    }
    if (itemx_expired(itx)) {
        return 2;
    }*/

    struct item *it;
    uint32_t sid(0), offset(0);
    bool status = itemx_getx(key, nkey, sid, offset);
    if (status) {
        it = slab_read_item(sid, offset);
        if (it == NULL) {
            //rsp_send_error(ctx, conn, msg, MSG_RSP_SERVER_ERROR, errno);
            return -1;
        }
        memcpy(value, item_data(it), it->ndata);
        return 1;
    }
    return 0;
}

int delete_test(char* key, int nkey){
    /*uint8_t  md[20];
    uint32_t hash;
    uint8_t * tmp_key = (uint8_t *) key;
    sha1(tmp_key, nkey, md);
    hash = sha1_hash(md);
    
    uint8_t cid;
    struct itemx *itx;

    itx = itemx_getx(hash, md);
    if (itx == NULL) {
        // rsp_send_status(ctx, conn, msg, MSG_RSP_NOT_FOUND);
        return 0;
    }
    cid = slab_get_cid(itx->sid);
    */
    bool status = itemx_removex((uint8_t*) key, (uint8_t) nkey);   
    if (status) 
        return 1;
    return 0;
}

int num(char *src_key, int src_key_len, int num, int expiry, int flags){
    rstatus_t status;
    uint8_t *pkey, nkey, cid;
    struct item *it;
    //struct itemx *itx;
    uint64_t cnum;
    int64_t nnum;
    char numstr[FC_UINT64_MAXLEN];
    int n;
    
    pkey = (uint8_t *)src_key;
    nkey = (uint8_t)(src_key_len);

    uint8_t  md[20];
    uint32_t hash;
    sha1(pkey, nkey, md);
    hash = sha1_hash(md);
    
    /* 1). look up existing itemx 
    itx = itemx_getx(hash, md);
    if (itx == NULL || itemx_expired(itx)) {
        // 2a). miss -> return NOT_FOUND 
        //rsp_send_status(ctx, conn, msg, MSG_RSP_NOT_FOUND);
        return -1;
    }*/

    //1)
    uint32_t sid(0), offset(0);
    itemx_getx(pkey, nkey, sid, offset);

    /* 2b). hit -> read existing item into it */
    it = slab_read_item(sid, offset);
    if (it == NULL) {
        //rsp_send_error(ctx, conn, msg, MSG_RSP_SERVER_ERROR, errno);
        return -2;
    }

    /* 3). sanity check item data to be a number */
    status = fc_atou64(item_data(it), it->ndata, &cnum);
    if (status != FC_OK) {
        //rsp_send_error(ctx, conn, msg, MSG_RSP_CLIENT_ERROR, EINVAL);
        return -3;
    }

    /* 4). remove existing itemx of it */
    itemx_removex(pkey, nkey);

    /* 5). compute the new incr/decr number nnum and numstr */
    nnum = cnum + num;
    if (nnum<0)
        nnum = 0;
    n = _scnprintf(numstr, sizeof(numstr), "%"PRIu64"", (uint64_t)nnum);

    /* 6). alloc new item that can hold n worth of bytes */
    cid = item_slabcid(nkey, n);
    ASSERT(cid != SLABCLASS_INVALID_ID);

    it = item_get(pkey, nkey, cid, n, time_reltime(expiry), flags,
                   md, hash);
    if (it == NULL) {
        //rsp_send_error(ctx, conn, msg, MSG_RSP_SERVER_ERROR, ENOMEM);
        return -2;
    }

    /* 7). copy numstr to it */
    memcpy(item_data(it), numstr, n);
    return 0;
}

int main(int argc, char** argv) {

    set_options();
    fc_generate_profile();
    init();

    char key[5]= "12345";
    //char value[3]="100";
    char *value=NULL;
    char *ret=NULL;
    int vl = 0;
    if (argc > 1){
        printf("%d\n", (int)strlen(argv[1]));
        vl =  strlen(argv[1]);
        value = (char *) malloc((strlen(argv[1])+1) * sizeof(char));
        ret = (char *) malloc((strlen(argv[1])+1) * sizeof(char));
        memcpy(value, argv[1], strlen(argv[1]));
    } else {
        ret = (char *) malloc(10 * sizeof(char));
        value = "123";
    }
      

    /*CuckooHashTable<uint8_t*, uint64_t> cci(1024);
    printf("num: %u\n",cci.BucketCount());
    uint8_t *yyey = nullptr;
    uint64_t bbi(0);
    yyey = (uint8_t *) malloc(8 * sizeof(char));
    for (uint32_t nn = 123456; nn < 124456; nn++) {
        //uint32_t nn = 123456;
        sprintf((char *) yyey, "%d", nn);
        //printf("yyey: %s len:%d\n",yyey, strlen(yyey));
        cci.insert(yyey, 6, nn);
        //cci.lookUp(yyey, 6, bbi);
        //printf("yyey: %s value: %lu\n",yyey, bbi);
    }
    
    for (uint32_t nn = 123456; nn < 124456; nn++) {
        //uint32_t nn = 123456;
        sprintf((char *) yyey, "%d", nn);
        //printf("yyey: %s len:%d\n",yyey, strlen(yyey));
        
        cci.lookUp(yyey, 6, bbi);
        printf("yyey: %s value: %lu\n",yyey, bbi);
    }
  */  


    /*
    no data
    set data ok
    get data 123
    delete no data
    set data ok
    get data: 123
    num data ok
    */

    #if 1
    if (get(key, 5, ret) == 0){
        printf("no data\n");
    }
    if (put(key, 5, value, strlen(value), 0, 1) == 1){
        printf("set data ok\n");
    }
    if (get(key, 5, ret) == 1){
        printf("get data %s\n", ret);
    }
    delete_test(key, 5);
    if (get(key, 5, ret) == 0){
        printf("delete no data\n");
    }
    if (put(key, 5, value, strlen(value), 0, 1) == 1){
        printf("set data ok\n");
    }
    if (get(key, 5, ret) == 0){
        printf("error, no data\n");
    }else{
        printf("get data: %s\n", ret);
    }
    if (num(key, 5, 12, 0, 1) == 0){
        printf("num data ok\n");
    }
    if (get(key, 5, ret) == 1){
        printf("get data: %s\n", ret);
    }
    #endif
    
    return 0;
}

