//
// Created by 赵立伟 on 2018/10/20.
//

#ifndef FLYDB_EVENTDEF_H
#define FLYDB_EVENTDEF_H

#include <cstdint>
#include <sys/time.h>

enum EventStatus {
    ES_NONE = 0,
    ES_READABLE,
    ES_WRITABLE
};

class AbstractCoordinator;

const int EVENT_FILE_EVENTS = 1;
const int EVENT_TIME_EVENTS = 2;
const int EVENT_ALL_EVENTS = EVENT_FILE_EVENTS | EVENT_TIME_EVENTS;
const int EVENT_CALL_AFTER_SLEEP = 8;
const int EVENT_DONT_WAIT = 4;

typedef int timeEventProc(const AbstractCoordinator *coorinator,
                          uint64_t id,
                          void *clientData);
typedef void fileEventProc(const AbstractCoordinator *coorinator,
                           int fd,
                           void *clientdata,
                           int mask);
typedef void eventFinalizerProc(const AbstractCoordinator *coorinator,
                                void *clientData);
typedef void beforeAndAfterSleepProc(const AbstractCoordinator *coorinator);

#endif //FLYDB_EVENTDEF_H
