//
// Created by 赵立伟 on 2019/3/6.
//

#include <cassert>
#include "ReplicationHandler.h"

ReplicationHandler::ReplicationHandler(AbstractCoordinator *coordinator) {
    this->coordinator = coordinator;
}

void ReplicationHandler::unsetMaster() {
    if (!this->haveMasterhost()) {
        return;
    }

    /** 清理masterhost */
    this->masterhost.clear();

    /** shift replication id */
    this->shiftReplicationId();

    /** 删除master */
    coordinator->getFlyClientFactory()->deleteFlyClient(&this->master);

    /** 删除cached master */
    this->discardCachedMaster();

    /** 取消与master的replication握手 */
    this->cancelHandShake();

    /**
     * 用以通知所有的slave, 本机的replication id改变了，
     * slave会向本机发送psync，这样便可以重新建立连接
     **/
    this->disconnectWithSlaves();

    /** replication state */
    this->state = REPL_STATE_NONE;

    /**
     * 这里将slaveSelDB设置为1，为了在下次full resync时强加一个SELECT命令
     **/
    this->slaveSelDB = -1;
}

const std::string &ReplicationHandler::getMasterhost() const {
    return this->masterhost;
}

int ReplicationHandler::getMasterport() const {
    return this->masterport;
}

bool ReplicationHandler::haveMasterhost() const {
    return !this->masterhost.empty();
}

int ReplicationHandler::cancelHandShake() {
    if (REPL_STATE_TRANSFER == this->state) {
        /** 如果处于正在传输fdb文件的阶段 */
        this->abortSyncTransfer();
        this->state = REPL_STATE_CONNECT;
    } else if (REPL_STATE_CONNECTING == this->state
               || this->inHandshakeState()) {
        /** 如果正处于连接或者握手阶段 */
        this->disconnectWithMaster();
        this->state = REPL_STATE_CONNECT;
    } else {
        /** 否则，处于完成或者NONE状态 */
        return 0;
    }

    return 1;
}

bool ReplicationHandler::abortSyncTransfer() {
    assert(REPL_STATE_TRANSFER == this->state);
    this->disconnectWithMaster();
    close(this->transferSocket);
    unlink(this->transferTempFile.c_str());
    this->transferTempFile.clear();
}

bool ReplicationHandler::inHandshakeState() {
    return this->state >= REPL_STATE_RECEIVE_PONG &&
           this->state <= REPL_STATE_RECEIVE_PSYNC;
}

void ReplicationHandler::disconnectWithMaster() {
    int fd = this->transferSocket;
    coordinator->getEventLoop()->deleteFileEvent(fd, ES_READABLE | ES_WRITABLE);
    close(fd);
    this->transferSocket = -1;
}

void ReplicationHandler::disconnectWithSlaves() {
    for (auto item : this->slaves) {
        coordinator->getFlyClientFactory()->deleteFlyClient(&item);
    }
}

void ReplicationHandler::discardCachedMaster() {
    coordinator->getLogHandler()->logWarning(
            "Discarding previously cached master state");
    coordinator->getFlyClientFactory()->deleteFlyClient(&cachedMaster);
}

/**
 * 本机从主切换到从，replication id切换：
 *  1.将replication id/offset存入replication id2/offset2
 *  2.replid2 = replid + 1，这样在切换成slave时，slave取其已经拥有的字节的下一个字节
 *  3.重新生成replication id
 **/
void ReplicationHandler::shiftReplicationId() {
    memcpy(this->replid2, this->replid, sizeof(this->replid));
    this->secondReplidOffset = this->masterReplOffset + 1;
    this->randomReplicationId();
}

void ReplicationHandler::randomReplicationId() {
    uint8_t len = sizeof(this->replid);
    miscTool->getRandomHexChars(this->replid, len);
    this->replid[len] = '\0';
}
