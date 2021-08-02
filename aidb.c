#include <time.h>
#include "sharedptr.h"
#include "crc32.h"
#include "md5.h"
#include "aes.h"
#include "aidb.h"

#include <stdio.h>

/* aidb文件结构:
        文件   = 文件头 + 记录...
        文件头 = AIDB标志 + 版本 + 保留字段 + CRC32校验值 + 最后更新时间 
        记录   = 记录大小 + 记录内容
*/

/** 获取"MEMBER成员"在"结构体TYPE"中的位置偏移 */
#ifndef offsetof
#   define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

// aidb文件魔法值
#define AIDB_MAGIC 0x42444941
#define AIDB_VERSION 0x01
#define AIDB_INDEX_END 0xFFFFFFFF
#define AIDB_MAX_BLOCK 0xFFFFFFFF

// aidb句柄的内部结构定义
struct aidb_t {
    // FILE*       fp;             // 已打开的文件指针
    char*       filename;       // 文件名
    uint64_t    size;           // 文件长度
    uint32_t    count;          // 记录总数
    uint8_t     key[16];        // 秘钥
};

// aidb文件头定义
typedef struct aidb_header_t {
    uint32_t    magic;          // aidb文件魔法值, 固定为"AIDB"
    uint8_t     version;        // 版本号, 最大版本号255
    uint8_t     unused[3];      // 保留值
    uint32_t    crc32;          // 文件头以外的文件内容CRC32校验值
    int64_t     updated;        // 最后更新日期, 采用time函数返回的时间
    uint8_t     pwd[16]         // 秘钥MD5校验值, password + updated生成
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
    int32_t     next;           // 下个块序号, 小于0表示是被删除的块
    _Bool       deleted;        // 删除标志, 0: 使用中, 1: 已删除
} __attribute__ ((packed)) aidb_block_t;

// AES加密向量
const static unsigned char IV[16] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
    0x02, 0x46, 0x8a, 0xce, 0x13, 0x57, 0x9b, 0xdf };

// 根据更新时间和口令，生成口令md5值
inline static void gen_pwd(const char* key, uint64_t updated, uint8_t out[16]) {
    md5_ctx_t ctx;
    md5_init(&ctx);
    md5_update(&ctx, &updated, sizeof(uint64_t));
    md5_update(&ctx, key, strlen(key));
    md5_final(&ctx, out);
}

// 校验aidb文件头部
inline static AIDB_ERROR aidb_check_header(FILE *fp, const char* key) {
    // 获取文件长度
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    aidb_header_t header;
    // 读取文件头
    if (!fread(&header, sizeof(aidb_header_t), 1, fp))
        return AIDB_ERR_TOO_SMALL;
    // 校验魔法值
    if (header.magic != AIDB_MAGIC)
        return AIDB_ERR_MAGIC;
    // 校验版本号
    if (header.version != AIDB_VERSION)
        return AIDB_ERR_VERSION;
    
    // 校验密码
    uint8_t md5[16];
    gen_pwd(key, header.updated, md5);
    if (memcmp(md5, header.pwd, 16))
        return AIDB_ERR_PWD;

    // 读取记录内容
    size_t blen = size - sizeof(aidb_header_t);
    uint8_t *buf = malloc(blen);
    if (!fread(buf, blen, 1, fp))
        return AIDB_ERR_READ;

    // 校验CRC32
    if (header.crc32 != crc32(buf, blen))
        return AIDB_ERR_CRC32;

    return AIDB_OK;
}

AIDB_ERROR aidb_check(const char* filename, const char* key) {
    // 打开文件
    FILE *fp = fopen(filename, "rb");
    if (!fp) return AIDB_ERR_OPEN;

    // 校验文件头
    AIDB_ERROR ret_code = aidb_check_header(fp, key);

    fclose(fp);

    return ret_code;
}

