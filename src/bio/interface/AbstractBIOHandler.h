//
// Created by 赵立伟 on 2019/1/20.
//

#ifndef FLYDB_ABSTRACTBIOHANDLER_H
#define FLYDB_ABSTRACTBIOHANDLER_H

#include <cstdint>

class AbstractBIOHandler {
public:

    virtual uint64_t getPendingJobCount(int type) = 0;

    virtual uint64_t waitStep(int type) = 0;

    virtual void createBackgroundJob(int type,
                                     void *arg1,
                                     void *arg2,
                                     void *arg3) = 0;

    virtual void killThreads(void) = 0;
};

#endif //FLYDB_ABSTRACTBIOHANDLER_H
