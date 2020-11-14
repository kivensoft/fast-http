#pragma once
#ifndef __AIDB_H__
#define __AIDB_H__

#include "sharedptr.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void make_aidb_file(const char* infile, const char* outfile, const char* key);

extern void make_xml_file(const char* infile, const char* outfile, const char* key);

extern pshared_ptr_t load_aidb(const char* infile, const char* key);

#ifdef __cplusplus
}
#endif

#endif // __AIDB_H__