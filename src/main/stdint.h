/***************************************************************************
    Data Types.

    The Boost library is only used to enforce data type size at compile
    time.

    If you're sure the sizes are correct, it can be removed for your port.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef signed long long int64_t;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef int Boolean;
enum { FALSE, TRUE };


#ifdef __GNUC__
#define STATIC_ASSERT_HELPER(expr, msg) (!!sizeof (struct { unsigned int STATIC_ASSERTION__##msg: (expr) ? 1 : -1; }))
#define STATIC_ASSERT(expr, msg) extern int (*assert_function__(void)) [STATIC_ASSERT_HELPER(expr, msg)]
#else
#define STATIC_ASSERT(expr, msg)   \
    extern char STATIC_ASSERTION__##msg[1]; \
    extern char STATIC_ASSERTION__##msg[(expr)?1:2];
#endif /* #ifdef __GNUC__ */


STATIC_ASSERT(sizeof(int8_t)   == 1, int8_t_is_not_of_the_correct_size );
STATIC_ASSERT(sizeof(int16_t)  == 2, int16_t_is_not_of_the_correct_size);
STATIC_ASSERT(sizeof(int32_t)  == 4, int32_t_is_not_of_the_correct_size);
STATIC_ASSERT(sizeof(int64_t)  == 8, int64_t_is_not_of_the_correct_size);

STATIC_ASSERT(sizeof(uint8_t)  == 1, int8_t_is_not_of_the_correct_size);
STATIC_ASSERT(sizeof(uint16_t) == 2, int16_t_is_not_of_the_correct_size);
STATIC_ASSERT(sizeof(uint32_t) == 4, int32_t_is_not_of_the_correct_size);
STATIC_ASSERT(sizeof(uint64_t) == 8, int64_t_is_not_of_the_correct_size);

