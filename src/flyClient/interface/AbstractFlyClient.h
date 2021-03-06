//
// Created by 赵立伟 on 2018/12/3.
//

#ifndef FLYDB_ABSTRACTFLYCLIENT_H
#define FLYDB_ABSTRACTFLYCLIENT_H

#include <ctime>
#include <cstdint>
#include <string>
#include <list>
#include "../../flyObj/FlyObj.h"

class AbstractFlyDB;

class AbstractFlyClient {
public:
    ~AbstractFlyClient() {}

    virtual uint64_t getId() const = 0;

    virtual void setId(uint64_t id) = 0;

    virtual int getFd() const = 0;

    virtual void setFd(int fd) = 0;

    virtual std::shared_ptr<FlyObj> getName() const = 0;

    virtual void setName(std::shared_ptr<FlyObj> name) = 0;

    virtual int getFlags() const = 0;

    virtual void setFlags(int flags) = 0;

    virtual void addFlag(int flag) = 0;

    virtual void delFlag(int flag) = 0;

    virtual const std::string &getQueryBuf() const = 0;

    virtual void setQueryBuf(const std::string &queryBuf) = 0;

    /** 向输入缓冲中添加数据 */
    virtual void addToQueryBuf(const std::string &str) = 0;

    virtual void trimQueryBuf(int begin, int end) = 0;

    virtual int getQueryBufSize() const = 0;

    virtual std::shared_ptr<FlyObj> *getArgv() const = 0;

    virtual void freeArgv() = 0;

    virtual void allocArgv(int64_t count) = 0;

    virtual int getArgc() const = 0;

    virtual void addArgv(std::shared_ptr<FlyObj> obj) = 0;

    virtual void setArgc(int argc) = 0;

    virtual const char *getBuf() const = 0;

    virtual bool bufSendOver() = 0;

    virtual bool IsPendingWrite() const = 0;

    virtual int getBufpos() const = 0;

    virtual const std::list<std::string*> &getReply() const = 0;

    virtual void replyPopFront() = 0;

    virtual void setReply(const std::list<std::string*> &reply) = 0;

    virtual int getAuthentiated() const = 0;

    virtual void setAuthentiated(int authentiated) = 0;

    virtual time_t getCreateTime() const = 0;

    virtual void setCreateTime(time_t createTime) = 0;

    virtual time_t getLastInteractionTime() const = 0;

    virtual void setLastInteractionTime(time_t lastInteractionTime) = 0;

    virtual time_t getSoftLimitTime() const = 0;

    virtual void setSoftLimitTime(time_t softLimitTime) = 0;

    virtual char getFirstQueryChar() = 0;

    virtual bool isMultiBulkType() = 0;

    virtual int32_t getMultiBulkLen() const = 0;

    virtual void setMultiBulkLen(int32_t multiBulkLen) = 0;

    virtual int64_t getBulkLen() const = 0;

    virtual void setBulkLen(int64_t bulkLen) = 0;

    virtual int prepareClientToWrite() = 0;

    virtual bool hasNoPending() = 0;

    virtual void addReply(const char *s, size_t len) = 0;

    virtual void addReply(const char *fmt, ...) = 0;

    virtual void addReplyErrorFormat(const char *fmt, ...) = 0;

    virtual void addReplyError(const char *err) = 0;

    virtual void addReplyBulkCount(int count) = 0;

    virtual void addReplyBulkString(std::string str) = 0;

    virtual void clearBuf() = 0;

    virtual int getReqType() const = 0;

    virtual void setReqType(int reqType) = 0;

    virtual size_t getSendLen() const = 0;

    virtual void setSendLen(size_t sentLen) = 0;

    virtual void addSendLen(size_t sentLen) = 0;

    virtual AbstractFlyDB *getFlyDB() const = 0;

    virtual void setFlyDB(AbstractFlyDB *flyDB) = 0;
};

#endif //FLYDB_ABSTRACTFLYCLIENT_H
