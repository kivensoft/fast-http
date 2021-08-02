#include <time.h>
#include "sharedptr.h"
#include "md5.h"
#include "aes.h"
#include "aidb.h"

#include <stdio.h>

/* aidb文件结构:
        文件          = 文件头 + 记录分配表超级块(第1个块) + 区块...
        文件头        = 魔法值 + 版本号 + 块大小位数 + 保留值 + 最后更新日期
        区块          = 下个块序号(0为未使用, -1为结尾块) + 内容
        内容          = 记录分配表 | 记录
        记录分配表     = 记录首块序号(0表示未用的记录)...
        记录          = 记录加密大小 + 记录实际大小 + 记录内容
*/

/** 获取"MEMBER成员"在"结构体TYPE"中的位置偏移 */
#ifndef offsetof
#   define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

// aidb文件魔法值
#define AIDB_MAGIC 0x42444941
#define AIDB_VERSION 0x01
#define AIDB_BLOCK_BITS 8
#define AIDB_INDEX_END 0xFFFFFFFF
#define AIDB_MAX_BLOCK 0xFFFFFFFF

// aidb句柄的内部结构定义
struct aidb_t {
    FILE*       fp;             // 已打开的文件指针
    uint64_t    size;           // 文件长度
    uint32_t    count;          // 记录数量
    uint8_t     block_bits;     // 块大小位数
    _Bool       modified;       // 文件变动标志
    uint8_t     key[16];        // 秘钥
};

// aidb文件头定义
typedef struct aidb_header_t {
    uint32_t    magic;          // aidb文件魔法值, 固定为"AIDB"
    uint8_t     version;        // 版本号, 最大版本号255
    uint8_t     block_bits;     // 块大小位数
    uint16_t    unused;         // 保留值
    int64_t     updated;        // 最后更新日期, 采用time函数返回的时间
} __attribute__ ((packed)) aidb_header_t;

// aidb记录的内部结构定义
typedef struct aidb_record_t {
    uint32_t    size;           // 记录大小
    uint32_t    capacity;       // 记录容量大小
    uint8_t     deleted;        // 删除标志, 0: 未删除, 1: 已删除
} __attribute__ ((packed)) aidb_record_t;

// aidb记录的内部结构定义
struct aidb_iterator_t {
    uint32_t        offset;     // 记录所在文件偏移位置(包含本记录头)
    aidb_t*         aidb;       // aidb句柄
    aidb_record_t*  record;     // 记录结构
} __attribute__ ((packed));

// 区块结构定义
typedef struct aidb_block_t {
    int32_t    next;           // 下个块序号, 小于0表示是被删除的块
    _Bool       deleted;        // 删除标志, 0: 使用中, 1: 已删除
} __attribute__ ((packed)) aidb_block_t;

const static unsigned char IV[16] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
    0x02, 0x46, 0x8a, 0xce, 0x13, 0x57, 0x9b, 0xdf };

// 将用户输入的秘钥进行变换，生成实际的秘钥
inline static void gen_key(const char* key, uint8_t out[16]) {
    md5_ctx_t ctx;
    md5_init(&ctx);
    md5_update(&ctx, key, strlen(key));
    md5_final(&ctx, out); // 设置aidb秘钥
}

// 移动文件指针到指定序号的区块位置, 返回 0:成功, -1:失败
inline static int seek_index(FILE* fp, int32_t index, uint8_t block_bits) {
    return fseek(fp, sizeof(aidb_header_t) + (index << block_bits), SEEK_SET);
}

// 计算记录总数, 返回: 0xFFFFFFFF:错误, 其它值: 记录总数
inline static uint32_t aidb_calc_record_count(FILE* fp, uint8_t block_bits) {
    size_t rec_size = 1 << (block_bits - 2);
    uint32_t buf[rec_size], count = 0, next_index = 0;

    do {
        // 任意一个环节出错都直接返回错误
        if (!seek_index(fp, next_index, block_bits)) {
            if (fread(buf, rec_size << 2, 1, fp)) {
                next_index = buf[0];
                if (next_index) {
                    for (uint32_t *b = buf + rec_size - 1; b > buf; --b)
                        if (*b) ++count;
                    continue;
                }
            }
        }
        return AIDB_MAX_BLOCK;
    } while (next_index != AIDB_INDEX_END);

    return count;
}

// 校验aidb文件头部
inline static AIDB_ERROR aidb_check_header(aidb_t* aidb) {
    aidb_header_t header;
    // 读取文件头
    if (!fread(&header, sizeof(aidb_header_t), 1, aidb->fp))
        return AIDB_ERR_TOO_SMALL;
    // 校验魔法值
    if (header.magic != AIDB_MAGIC)
        return AIDB_ERR_MAGIC;
    // 校验版本号
    if (header.version != AIDB_VERSION)
        return AIDB_ERR_VERSION;
    
    // 计算记录总数
    aidb->count = aidb_calc_record_count(aidb->fp, header.block_bits);
    if (aidb->count == AIDB_MAX_BLOCK)
        return AIDB_ERR_BLOCK;

    aidb->block_bits = header.block_bits;

    return AIDB_OK;
}

// 初始化aidb文件
inline static AIDB_ERROR aidb_init(aidb_t* aidb, uint8_t block_bits) {
    aidb->block_bits = block_bits;

    // 写入文件头
    aidb_header_t header;
    memset(&header, 0, sizeof(aidb_header_t));
    header.magic = AIDB_MAGIC;
    header.block_bits = block_bits;
    header.version = AIDB_VERSION;
    header.updated = _time64(NULL);
    size_t count = fwrite(&header, sizeof(aidb_header_t), 1, aidb->fp);
    if (count != 1) return AIDB_ERR_WRITE;

    // 写入超级块
    uint32_t block_size = 1 << block_bits;
    char* buf = malloc(block_size);
    memset(buf, 0, block_size);
    *((uint32_t*) buf) = AIDB_INDEX_END;
    count = fwrite(buf, block_size, 1, aidb->fp);
    if (count != 1) return AIDB_ERR_WRITE;

    aidb->size = sizeof(aidb_header_t) + block_size;
    
    return AIDB_OK;
}

// 写入更新时间, 返回 0:失败, 1:成功
inline static _Bool aidb_write_update(FILE *fp) {
    int64_t t = _time64(NULL);
    if (fseek(fp, offsetof(aidb_header_t, updated), SEEK_SET))
        return 0;
    return fwrite(&t, sizeof(int64_t), 1, fp);
}

size_t aidb_sizeof() { return sizeof(aidb_t); }
size_t aidb_iterator_sizeof() { return sizeof(aidb_iterator_t); }

AIDB_ERROR aidb_create(aidb_t* aidb, const char* filename, const char* key) {
    memset(aidb, 0, sizeof(aidb_t));

    // 打开文件
    FILE *fp = fopen(filename, "wb+");
    if (!fp) return AIDB_ERR_OPEN;
    aidb->fp = fp; // 设置aidb文件指针

    AIDB_ERROR ret_code = aidb_init(aidb, AIDB_BLOCK_BITS);
    
    // 设置aidb的实际秘钥
    if (ret_code == AIDB_OK)
        gen_key(key, aidb->key);
    else
        aidb_close(aidb);

    return ret_code;
}

AIDB_ERROR aidb_open(aidb_t* aidb, const char* filename, const char* key) {
    memset(aidb, 0, sizeof(aidb_t));

    // 打开文件
    FILE *fp = fopen(filename, "rb+");
    if (!fp) return AIDB_ERR_OPEN;
    aidb->fp = fp; // 设置aidb文件指针

    // 获取文件长度
    fseek(fp, 0, SEEK_END);
    aidb->size = ftell(fp); // 设置aidb文件长度
    fseek(fp, 0, SEEK_SET);

    // 校验文件头
    AIDB_ERROR ret_code = aidb_check_header(aidb);

    // 设置aidb的实际秘钥
    if (ret_code == AIDB_OK)
        gen_key(key, aidb->key);
    else
        aidb_close(aidb);

    return ret_code;
}

void aidb_close(aidb_t* aidb) {
    if (aidb->fp) {
        aidb_flush(aidb);
        fclose(aidb->fp);
        memset(&aidb, 0, sizeof(aidb_t));
    }
}

void aidb_flush(aidb_t* aidb) {
    if (aidb->modified) {
        aidb_write_update(aidb->fp);
        fflush(aidb->fp);
        aidb->modified = 0;
    }
}

uint32_t aidb_record_count(aidb_t* aidb) {
    return aidb->count;
}

void aidb_foreach(aidb_t* aidb, aidb_iterator_t* iterator) {
    memset(iterator, 0, sizeof(aidb_iterator_t));
    iterator->offset = sizeof(aidb_header_t);
    iterator->aidb = aidb;
}

uint32_t aidb_next(aidb_iterator_t* iterator) {
    uint32_t off = iterator->offset;
    FILE *fp = iterator->aidb->fp;
    aidb_record_t *prec = &iterator->record;
    if (!off) return 0;
    do {
        off += prec->capacity;
        fseek(fp, off, SEEK_SET);
        // 读取失败，已经到文件末尾
        if (!fread(prec, sizeof(aidb_record_t), 1, fp))
            break;
        // 记录是已删除记录，需要跳过
        if (prec->deleted)
            continue;
        // 读取成功
        iterator->offset = off;
        return prec->size;
    } while (true);

    // 读取下一条记录信息失败
    iterator = off;
    iterator->record.size = 0;
    iterator->record.capacity = 0;
    return 0;
}

uint32_t aidb_fetch(aidb_iterator_t* iterator, void* output, uint32_t size) {
    // TODO: 加载记录需要解密，注意16字节对齐问题，包括写入记录的时候，如何处理16字节对齐问题
    uint32_t rsize = iterator->record.size;
    if (size > rsize) size = rsize;
    fseek(iterator->aidb->fp, iterator->offset + sizeof(aidb_record_t), SEEK_SET);
    return fread(output, 1, size, iterator->aidb->fp);
}

//------------------------------------------------------------------------------


void gen_key(const char* key, uint8_t out[16]) {
    md5_ctx_t md5;
    md5_init(&md5);
    md5_update(&md5, key, strlen(key));
    md5_final(&md5, out);
}

pshared_ptr_t loadfile(const char* file) {
    FILE *fp = fopen(file, "rb");
    if (!fp) {
        fprintf(stderr, "can't open erad file %s\n", file);
        return NULL;
    }
    // 获取文件长度
    fseek(fp, 0, SEEK_END);
    uint32_t flen = (uint32_t) ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // 分配缓冲区并读取文件内容到缓冲区
    uint32_t cap = aes_pkcs7_padding_size(flen);
    pshared_ptr_t ret = shared_ptr_malloc(cap);
    ret->idx = fread(ret->data, 1, flen, fp);
    
    fclose(fp);

    return ret;
}

bool savefile(const char *file, pshared_ptr_t data) {
    FILE *fp = fopen(file, "wb");
    if (!fp) {
        fprintf(stderr, "can't open write file %s\n", file);
        return false;
    }

    fwrite(data->data, 1, data->idx, fp);
    fclose(fp);
    return true;
}

void make_file(const char* infile, const char* outfile, const char* key, int mode) {
    uint8_t key_enc[16];
    gen_key(key, key_enc);

    pshared_ptr_t data = loadfile(infile);
    if (!data) return;

    // 加密内容
    aes_ctx_t ac;
    aes_init(&ac, mode, key_enc, 128, IV);
    data->idx = (uint32_t) aes_final(&ac, data->data, data->idx, data->data);

    if (savefile(outfile, data)) {
        printf("%s [%s] -> [%s] succeed!\n", mode ? "decrypt" : "encrypt", infile, outfile);
    }
    shared_ptr_free(data);
}

void make_aidb_file(const char* infile, const char* outfile, const char* key) {
    make_file(infile, outfile, key, AES_ENCRYPT);
}

void make_xml_file(const char* infile, const char* outfile, const char* key) {
    make_file(infile, outfile, key, AES_DECRYPT);
}

pshared_ptr_t load_aidb(const char* infile, const char* key) {
    uint8_t key_enc[16];
    gen_key(key, key_enc);

    pshared_ptr_t ret = loadfile(infile);
    if (ret) {
        // 加密内容
        aes_ctx_t ac;
        aes_init(&ac, AES_ENCRYPT, key_enc, 128, IV);
        ret->idx = (uint32_t) aes_final(&ac, ret->data, ret->idx, ret->data);
    }

    return ret;
}