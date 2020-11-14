#include "sharedptr.h"
#include "md5.h"
#include "aes.h"
#include "aidb.h"

#include <stdio.h>

const unsigned char IV[16] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
    0x02, 0x46, 0x8a, 0xce, 0x13, 0x57, 0x9b, 0xdf };

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