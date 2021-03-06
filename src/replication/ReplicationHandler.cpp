//
// Created by 赵立伟 on 2019/3/6.
//

#include <cassert>
#include "ReplicationHandler.h"
#include "../flyClient/ClientDef.h"
#include "../io/StringFio.h"

ReplicationHandler::ReplicationHandler(AbstractCoordinator *coordinator) {
    this->coordinator = coordinator;
    this->logHandler = logFactory->getLogger();
}

void ReplicationHandler::unsetMaster() {
    if (!this->haveMasterhost()) {
        return;
    }

    /** shift replication id */
    this->shiftReplicationId();

    /** 删除master并取消与master的replication握手 */
    this->cancelHandShake();

    /** 删除cached master */
    this->discardCachedMaster();

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

void ReplicationHandler::setMaster(std::string ip, int port) {
    /** 如果this->master是空，则代表本机曾经是主 */
    int wasMaster = this->masterhost.empty();
    this->masterhost = ip;
    this->masterport = port;

    /** 删除原先的master */
    if (NULL != this->master) {
        this->master = NULL;
    }

    /** 令所有slave重新连接 */
    this->disconnectWithSlaves();

    /** 取消与master的握手 */
    this->cancelHandShake();

    /** 缓存master */
    if (wasMaster) {
        this->cacheMasterUsingMyself();
    }

    this->state = REPL_STATE_CONNECT;
    this->masterDownSince = 0;
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

void ReplicationHandler::cron() {
    time_t nowt = coordinator->getFlyServer()->getNowt();

    /** 握手或者连接中，且超时 */
    if (this->haveMasterhost() &&
        (REPL_STATE_CONNECTING == this->state || this->inHandshakeState())
        && nowt - this->transferLastIO > this->timeout) {
        this->cancelHandShake();
    }

    /** 正在处于fdb文件传输阶段并超时 */
    if (this->haveMasterhost()
        && REPL_STATE_TRANSFER == this->state
        && nowt - this->transferLastIO > this->timeout) {
        this->cancelHandShake();
    }

    /** 已连接阶段，并超时*/
    if (this->haveMasterhost()
        && REPL_STATE_CONNECTED == this->state
        && nowt - this->lastInteraction > this->timeout) {
        coordinator->getFlyServer()->freeClient(this->master);
    }

    /** 等待连接状态 */
    if (REPL_STATE_CONNECT == this->state) {
        logHandler->logNotice("Connecting to MASTER %s:%d",
                              masterhost.c_str(),
                              masterport);
        if (1 == this->connectWithMaster()) {
            logHandler->logNotice("MASTER <--> SLAVE sync started!");
        }
    }

    /** 心跳检测：周期性（每秒一次）回复master */
    if (!this->masterhost.empty() && NULL != this->master) {
        sendAck();
    }

    // todo:
}


void ReplicationHandler::syncWithMasterStatic(
        const AbstractCoordinator *coorinator,
        int fd,
        std::shared_ptr<AbstractFlyClient> flyClient,
        int mask) {
    coorinator->getReplicationHandler()->syncWithMaster(fd, flyClient, mask);
}

void ReplicationHandler::syncWithMaster(
        int fd,
        std::shared_ptr<AbstractFlyClient> flyClient,
        int mask) {

}

void ReplicationHandler::sendAck() {
    std::shared_ptr<AbstractFlyClient> flyClient = this->master;
    if (NULL == flyClient) {
        return;
    }

    flyClient->addFlag(CLIENT_MASTER_FORCE_REPLY);
    flyClient->addReplyBulkCount(3);
    flyClient->addReplyBulkString("REPLCONF");
    flyClient->addReplyBulkString("ACK");
    flyClient->addReplyBulkString(std::to_string(this->offset));
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

/**
 * 当一个服务器由master切换成slave时调用
 *  为了创建一个cached master，将其用于在晋升为master时与slave做PSYNC用
 * */
void ReplicationHandler::cacheMasterUsingMyself() {
    AbstractFlyServer *flyServer = coordinator->getFlyServer();

    flyServer->unlinkClient(this->master);
    this->master = NULL;
    this->cachedMaster = this->master;
}

int ReplicationHandler::connectWithMaster() {
    AbstractFlyServer* flyServer = coordinator->getFlyServer();

    /** 创建与master的连接 */
    int fd = coordinator->getNetHandler()->tcpNonBlockBestEffortBindConnect(
            NULL,
            this->masterhost.c_str(),
            this->masterport,
            flyServer->getFirstBindAddr()->c_str());
    if (-1 == fd) {
        logHandler->logWarning("Unable to connect to MASTER: %S",
                               strerror(errno));
        return -1;
    }

    /** 创建网络事件, 用于和master通信 */
    if (-1 == coordinator->getEventLoop()->createFileEvent(
            fd,
            ES_READABLE | ES_WRITABLE,
            syncWithMasterStatic,
            NULL)) {
        close(fd);
        logHandler->logWarning("Can`t create readable event for SYNC");
        return -1;
    }

    this->transferLastIO = flyServer->getNowt();
    this->transferSocket = fd;
    this->state = REPL_STATE_CONNECTING;
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
    this->slaves.clear();
}

void ReplicationHandler::discardCachedMaster() {
    logHandler->logWarning("Discarding previously cached master state");
    this->cachedMaster = NULL;
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
