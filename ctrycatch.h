#pragma once
#ifndef __CTRYCATCH_H__
#define __CTRYCATCH_H__

#include <setjmp.h>
#include <stdbool.h>

// Some macro magic
#define CTRYCATCH_CAT(a, ...) CTRYCATCH_PRIMITIVE_CAT(a, __VA_ARGS__)
#define CTRYCATCH_PRIMITIVE_CAT(a, ...) a ## __VA_ARGS__

#define CTRYCATCH_NAME(X) CTRYCATCH_CAT(__ctrycatch_, X)

// New block arguments
#define try \
    if(!(CTRYCATCH_NAME(exception_type) = setjmp(CTRYCATCH_NAME(exception_env))))

#define catch(X) \
    else if((X +0) == 0 || CTRYCATCH_NAME(exception_type) == (X +0))

#define finally

#define throw(X,...) \
    CTRYCATCH_NAME(exception_message) = (__VA_ARGS__  +0), longjmp(CTRYCATCH_NAME(exception_env), (X))

// Exception type
typedef int CTRYCATCH_NAME(exception_types);

// Global variables to store exception details
extern jmp_buf CTRYCATCH_NAME(exception_env);
extern CTRYCATCH_NAME(exception_types) CTRYCATCH_NAME(exception_type);
extern char *CTRYCATCH_NAME(exception_message);

// Helper functions
#define __ctrycatch_exception_message_exists (bool)CTRYCATCH_NAME(exception_message)

// Exception types
#ifdef Exception
#undef Exception
#endif

enum exception_type {
    Exception, // Caution: 0 **IS** defined as "no error" to make it work. DO NOT modify this line. 
    ArgumentException,
    ArgumentNullException,
    IndexOutOfRangeException,
    InvalidOperationException,
    IOException,
};

#define Exception 0

/*
try {
	// do something
	throw(ArgumentsException);
	// or
	throw(ArgumentsException, "description");
} catch(ArgumentsException) {
	// error handling
} catch() {
	// error handling for default case
} finally {
	// cleanup
}
*/
#endif // __CTRYCATCH_H__
