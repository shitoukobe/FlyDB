//
// Created by 赵立伟 on 2019/1/20.
//

#ifndef FLYDB_BIODEF_H
#define FLYDB_BIODEF_H

#include <ctime>

/** Background job opcodes */
enum BIOJobOpCode {
    BIO_CLOSE_FILE = 0,     /** Deferred close(2) syscall. */
    BIO_AOF_FSYNC = 1,      /** Deferred AOF fsync. */
    BIO_LAZY_FREE = 2,      /** Deferred objects freeing. */
    BIO_NUM_OPS = 3
};

/**
 * Define aof_fsync to fdatasync() in Linux
 * and fsync() for all the rest
 **/
#ifdef __linux__
#define aof_fsync fdatasync
#else
#define aof_fsync fsync
#endif

const int FLYDB_THREAD_STACK_SIZE = 1024 * 1024 * 4;

#endif //FLYDB_BIODEF_H
