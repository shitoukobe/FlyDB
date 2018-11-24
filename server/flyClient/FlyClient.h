//
// Created by 赵立伟 on 2018/10/18.
//

#ifndef FLYDB_FLYCLIENT_H
#define FLYDB_FLYCLIENT_H

#include <string>
#include <list>
#include "../dataStructure/flyObj/FlyObj.h"
#include "../commandTable/CommandEntry.h"

class FlyServer;

const int FLY_REPLY_CHUNK_BYTES = 16 * 1024;

class FlyClient {
public:
    FlyClient(int fd, FlyServer *flyServer);
    ~FlyClient();
    uint64_t getId() const;
    void setId(uint64_t id);
    int getFd() const;
    void setFd(int fd);
    FlyObj *getName() const;
    void setName(FlyObj *name);
    int getFlags() const;
    void setFlags(int flags);
    void addFlag(int flag);
    const std::string &getQueryBuf() const;
    void setQueryBuf(const std::string &queryBuf);
    void addToQueryBuf(const std::string &str);             // 向输入缓冲中添加数据
    void trimQueryBuf(int begin, int end);
    int getQueryBufSize() const;
    FlyObj **getArgv() const;
    void freeArgv();
    void allocArgv(int64_t count);
    int getArgc() const;
    void addArgv(FlyObj *obj);
    void setArgc(int argc);
    CommandEntry *getCmd() const;
    void setCmd(CommandEntry *cmd);
    const char *getBuf() const;
    const std::list<std::string*> &getReply() const;
    void setReply(const std::list<std::string*> &reply);
    int getAuthentiated() const;
    void setAuthentiated(int authentiated);
    time_t getCreateTime() const;
    void setCreateTime(time_t createTime);
    time_t getLastInteractionTime() const;
    void setLastInteractionTime(time_t lastInteractionTime);
    time_t getSoftLimitTime() const;
    void setSoftLimitTime(time_t softLimitTime);
    char getFirstQueryChar();
    bool isMultiBulkType();
    int32_t getMultiBulkLen() const;
    void setMultiBulkLen(int32_t multiBulkLen);
    int64_t getBulkLen() const;
    void setBulkLen(int64_t bulkLen);
    int prepareClientToWrite();
    bool hasNoPending();
    void addReply(const char *s, size_t len);
    int getReqType() const;
    void setReqType(int reqType);

private:
    int addReplyToBuffer(const char *s, size_t len);
    int addReplyToReplyList(const char *s, size_t len);

    FlyServer *flyServer;
    uint64_t id;
    int fd;                             // 套接字
    FlyObj* name;                       // client名字
    int flags;                          // 标志
    std::string queryBuf;               // 输入缓冲区
    FlyObj **argv;                      // 命令参数
    int argc;
    CommandEntry* cmd;                  // 命令实现函数
    char buf[FLY_REPLY_CHUNK_BYTES];    // 固定大小输出缓冲区
    int bufpos;
    std::list<std::string *> replies;   // 可变长度输出缓冲区
    uint64_t replyBytes;
    int authentiated;                   // 是否通过了身份验证
    time_t createTime;                  // 客户端创建事件
    time_t lastInteractionTime;         // 最后一次互动时间
    time_t softLimitTime;               // 软性限制时间
    int reqType;
    int32_t multiBulkLen;               // 剩余可读的multi bulk参数数量
    int64_t bulkLen;
};


#endif //FLYDB_FLYCLIENT_H
