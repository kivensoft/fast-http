#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <getopt.h>
#include <time.h>
#include <assert.h>

#include "uv.h"
#include "platform.h"
#include "buffer.h"
#include "str.h"
#include "log.h"
#include "aidb.h"
#include "httpserver.h"
#include "list.h"
// #include "memwatch.h"

const char APP_NAME[] = "account info web server";
const char APP_VERSION[] = "1.02";
const char APP_COPYLEFT[] = "2019-2020 Kivensoft";

typedef struct config_t {
    char* debug;
    char* listen;
    char* make;
    char* password;
    char* username;
    char* workdir;
    char* decrypt;
} config_t;

config_t g_app_cfg = {
    .listen   = "0.0.0.0:8888",
    .username = "admin",
    .password = "password"
};

const char g_key[] = "account info web server, version 1.0";

char *g_app_name;

/** 使用帮助 */
static void usage() {
	printf("%s, version %s, copyleft by %s.\n\n", APP_NAME, APP_VERSION, APP_COPYLEFT);
	printf("Usage: %s [option]\n\n", g_app_name);
	printf("Options:\n");
	printf("    -d file         log file name\n");
	printf("    -h              show this help\n");
	printf("    -l address      listen address, default %s\n", g_app_cfg.listen);
	printf("    -m filename     encrypt xml to aidb file\n");
	printf("    -p password     login password, default %s\n", g_app_cfg.password);
	printf("    -u username     login username, default %s\n", g_app_cfg.username);
	printf("    -w dir          set work dir, default current dir\n");
	printf("    -x filename     decrypt aidb to xml file\n");
	printf("    -z              show chinese help\n");
    exit(0);
}

static void usage_chinese() {
	printf("%s, 版本 %s, 版权所有 %s.\n\n", "账户信息web服务", APP_VERSION, APP_COPYLEFT);
	printf("用法: %s [选项]\n\n", g_app_name);
	printf("选项:\n");
	printf("    -d 文件名       指定日志文件名\n");
	printf("    -h              显示帮助\n");
	printf("    -l 监听地址     指定服务监听地址, 缺省为: %s\n", g_app_cfg.listen);
	printf("    -m 文件名       加密xml文件到aidb文件\n");
	printf("    -p 口令         登录口令, 缺省为: %s\n", g_app_cfg.password);
	printf("    -u 用户名       登录用户名, 缺省为: %s\n", g_app_cfg.username);
	printf("    -w 目录         设置工作目录, 缺省为当前目录\n");
	printf("    -x 文件名       解密aidb文件到xml文件\n");
	printf("    -z              显示中文帮助信息\n");
    exit(0);
}


/** 处理命令行参数 */
void process_cmdline(int argc, char **argv) {
    int c;
	while ((c = getopt(argc, argv, "d:hl:m:p:u:w:x:z")) != -1) {
		switch (c) {
			case 'd': g_app_cfg.debug = optarg; break;
			case 'h': usage(); break;
            case 'l': g_app_cfg.listen = optarg; break;
			case 'm': g_app_cfg.make = optarg; break;
			case 'p': g_app_cfg.password = optarg; break;
			case 'u': g_app_cfg.username = optarg; break;
			case 'w': g_app_cfg.workdir = optarg; break;
			case 'x': g_app_cfg.decrypt = optarg; break;
			case 'z': usage_chinese(); break;
			default: printf("Try %s -h for more informaton.\n", g_app_name);
		}
	}
}

inline static void mk_out_name(char* out, const char* file, const char* ext) {
    strcpy(out, file);
    char *op = strrchr(out, '.');
    if (op) *op = '\0';
    strcat(op, ext);
}

int process_encrypt() {
    size_t len = strlen(g_app_cfg.make);
    char outfile[len + 8];
    mk_out_name(outfile, g_app_cfg.make, ".aidb");
    make_aidb_file(g_app_cfg.make, outfile, g_key);
    return 0;
}

int process_derypt() {
    size_t len = strlen(g_app_cfg.decrypt);
    char outfile[len + 8];
    mk_out_name(outfile, g_app_cfg.decrypt, ".xml");
    make_xml_file(g_app_cfg.decrypt, outfile, g_key);
    return 0;
}

uint32_t g_count = 0;
void on_idle(uv_idle_t *handle) {
    printf("idle call %d ok\n", ++g_count);
}

int on_http_serve(httpctx_t* pctx) {
    httpreq_t* preq = &pctx->req;
    httpres_t* pres = &pctx->res;
    mem_array_t *reqbuf = &preq->data;

    // 获取url
    char path[preq->path.len + 1];
    path[preq->path.len] = '\0';
    mem_array_read(reqbuf, preq->path.pos, preq->path.len, path);

    const char* cct = "application/json; charset=UTF-8";

    if (httpctx_path_match(preq, "/hello/")) {
        httpctx_set_content_type(pctx, "text/plain");

        httpctx_body_begin(pctx);
        httpctx_body_append(pctx, "Hello", 5);
        httpctx_body_append(pctx, " World!", 7);
        httpctx_body_end(pctx);
    } else if (httpctx_path_match(preq, "/index")) {
        httpctx_set_content_type(pctx, cct);
        httpctx_add_header(pctx, "Cookie", "Kiven");

        char buf[128];
        time_t t;
        time(&t);
        struct tm tm;
        localtime_r(&t, &tm);
        size_t c = strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);

        httpctx_set_body(pctx, buf, c);
    } else {
        pres->status = 404;
        httpctx_set_content_type(pctx, cct);

        const char* text = "{\"code\": 404, \"message\": \"未找到资源\"}";
        httpctx_set_body(pctx, text, strlen(text));
    }

    return 0;
}

/** 主函数入口 */
int main(int argc, char **argv) {
    // mwInit();
    INIT_UTF8_TERM(code_page);

    g_app_name = strrchr(argv[0], PATH_SPEC);
    if (!g_app_name)
        g_app_name = argv[0];
    else
        ++g_app_name;

    process_cmdline(argc, argv);

    // 如果是加密文件操作，转加密处理
    if (g_app_cfg.make)
        return process_encrypt();
    else if (g_app_cfg.decrypt)
        return process_derypt();
    
    // 设置日志服务
    log_start(g_app_cfg.debug, 1024 * 1024);

    // 正常web启动处理流程==========================
    uv_loop_t* ploop = uv_default_loop();
    // uv_idle_t idle;
    // uv_idle_init(ploop, &idle);
    // uv_idle_start(&idle, on_idle);
    http_server_t server;
    http_server(ploop, &server, g_app_cfg.listen, 5, on_http_serve);

    uv_run(ploop, UV_RUN_DEFAULT);
    uv_loop_close(ploop);

    UNINIT_UTF8_TERM(code_page);
    // mwTerm();
    return 0;
}
