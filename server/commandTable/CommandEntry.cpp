//
// Created by 赵立伟 on 2018/9/19.
//

#include <iostream>
#include "CommandEntry.h"
#include "../flyServer/interface/AbstractFlyServer.h"
#include "../dataStructure/skiplist/SkipList.cpp"
#include "../dataStructure/dict/Dict.cpp"

std::vector<CommandEntry> flyDBCommandTable = {
        {"version",     versionCommand,     1, "rF",  0, NULL, 1, 1, 1, 0, 0 },
        {"get",         getCommand,         2, "rF",  0, NULL, 1, 1, 1, 0, 0 },
        {"set",         setCommand,        -3, "wm",  0, NULL, 1, 1, 1, 0, 0 },
        {"expire",      expireCommand,      3, "wF",  0, NULL, 1, 1, 1, 0, 0 },
        {"expireat",    expireatCommand,    3, "wF",  0, NULL, 1, 1, 1, 0, 0 },
        {"mget",        mgetCommand,       -2, "rF",  0, NULL, 1,-1, 1, 0, 0 },
        {"rpush",       rpushCommand,      -3, "wmF", 0, NULL, 1, 1, 1, 0, 0 },
        {"lpush",       lpushCommand,      -3, "wmF", 0, NULL, 1, 1, 1, 0, 0 },
        {"sortpush",    pushSortCommand,   -3, "wmF", 0, NULL, 1, 1, 1, 0, 0 },
        {"rpop",        rpopCommand,        2, "wF",  0, NULL, 1, 1, 1, 0, 0 },
        {"lpop",        lpopCommand,        2, "wF",  0, NULL, 1, 1, 1, 0, 0 },
        {"sortPop",     popSortCommand,     2, "wF",  0, NULL, 1, 1, 1, 0, 0 },
        {"hset",        hsetCommand,       -4, "wmF", 0, NULL, 1, 1, 1, 0, 0 },
        {"hget",        hgetCommand,        3, "rF",  0, NULL, 1, 1, 1, 0, 0 },
        {"save",        saveCommand,        1, "as",  0, NULL, 0, 0, 0, 0, 0 },
        {"bgsave",      bgsaveCommand,     -1, "a",   0, NULL, 0, 0, 0, 0, 0 }
};


CommandEntry::CommandEntry() {

}

CommandEntry::CommandEntry(commandProc proc, int flag) {
    this->proc = proc;
    this->flag = flag;
}

CommandEntry::CommandEntry(char *name,
                           commandProc proc,
                           int arity,
                           const std::string &sflags,
                           int flag,
                           getKeysProc keysProc,
                           int firstKey,
                           int lastKey,
                           int keyStep,
                           uint64_t microseconds,
                           uint64_t calls) {
    this->name = name;
    this->proc = proc;
    this->arity = arity;
    this->sflags = sflags;
    this->flag = flag;
    this->keysProc = keysProc;
    this->firstKey = firstKey;
    this->lastKey = lastKey;
    this->keyStep = keyStep;
    this->microseconds = microseconds;
    this->calls = calls;
}

char *CommandEntry::getName() const {
    return this->name;
}

void CommandEntry::setName(char *name) {
    this->name = name;
}

commandProc CommandEntry::getProc() const {
    return this->proc;
}

void CommandEntry::setProc(commandProc proc) {
    this->proc = proc;
}

int CommandEntry::getArity() const {
    return this->arity;
}

void CommandEntry::setArity(int arity) {
    this->arity = arity;
}

const std::string &CommandEntry::getSflags() const {
    return this->sflags;
}

void CommandEntry::setSflags(const std::string &sflags) {
    this->sflags = sflags;
}

int CommandEntry::getFlag() const {
    return this->flag;
}

void CommandEntry::setFlag(int flag) {
    this->flag = flag;
}

void CommandEntry::addFlag(int flag) {
    this->flag |= flag;

}

void CommandEntry::setKeysProc(getKeysProc proc) {
    this->keysProc = keysProc;
}

uint64_t CommandEntry::getMicroseconds() const {
    return this->microseconds;
}

void CommandEntry::setMicroseconds(uint64_t microseconds) {
    this->microseconds = microseconds;
}

uint64_t CommandEntry::getCalls() const {
    return this->calls;
}

void CommandEntry::setCalls(uint64_t calls) {
    this->calls = calls;
}

int CommandEntry::getFirstKey() const {
    return firstKey;
}

void CommandEntry::setFirstKey(int firstKey) {
    CommandEntry::firstKey = firstKey;
}

int CommandEntry::getLastKey() const {
    return lastKey;
}

void CommandEntry::setLastKey(int lastKey) {
    CommandEntry::lastKey = lastKey;
}

int CommandEntry::getKeyStep() const {
    return keyStep;
}

void CommandEntry::setKeyStep(int keyStep) {
    CommandEntry::keyStep = keyStep;
}

void versionCommand(const AbstractCoordinator* coordinator,
                    AbstractFlyClient *client) {
    if (NULL == client) {
        return;
    }

    client->addReply("FlyDB version: %s",
                     coordinator->getFlyServer()->getVersion().c_str());
}

void getCommand(const AbstractCoordinator* coordinator,
                AbstractFlyClient* flyClient) {
    if (NULL == flyClient) {
        return;
    }

    // 如果参数数量<2，直接返回
    if (flyClient->getArgc() < 2) {
        char buf[100];
        flyClient->addReply("missing parameters!");
        return;
    }

    // 获取到key
    std::string *key = reinterpret_cast<std::string*>
    (flyClient->getArgv()[1]->getPtr());

    // 查看key是否已经过期
    AbstractFlyDB *flyDB = flyClient->getFlyDB();
    uint64_t expireTime = flyDB->getExpire(key);
    if (expireTime != -1 && expireTime < time(NULL)) {
        flyDB->delKey(key);
    }

    // 返回结果
    std::string* val = reinterpret_cast<std::string*>(
            flyDB->lookupKey(key)->getPtr());
    flyClient->addReply(val->c_str(), val->length());
}

void setGenericCommand(const AbstractCoordinator *coordinator,
                       AbstractFlyClient *flyClient,
                       std::string *key,
                       FlyObj *val,
                       int64_t expireMilli) {
    char buf[1024];
    // 将key和val添加到flydb中
    if (-1 == flyClient->getFlyDB()->addExpire(key, val, expireMilli)) {
        flyClient->addReply("set error!");
        return;
    }

    coordinator->getFlyServer()->addDirty(1);
    flyClient->addReply("set OK!");
}

void setCommand(const AbstractCoordinator* coordinator,
                AbstractFlyClient* flyClient) {
    if (NULL == flyClient) {
        return;
    }

    // 如果参数数量 < 2，直接返回
    if (flyClient->getArgc() < 3) {
        flyClient->addReply("missing parameters!");
        return;
    }

    // 获取到key和val
    std::string *key = reinterpret_cast<std::string*>
    (flyClient->getArgv()[1]->getPtr());
    FlyObj *val = flyClient->getArgv()[2];

    setGenericCommand(coordinator, flyClient, key, val, -1);
}

void setExCommand(const AbstractCoordinator* coordinator,
                  AbstractFlyClient* flyClient) {
    if (NULL == flyClient) {
        return;
    }

    // 如果参数数量 < 3，直接返回
    if (flyClient->getArgc() < 4) {
        flyClient->addReply("missing parameters!");
        return;
    }

    // 获取到key和val
    std::string *key =
            reinterpret_cast<std::string*>(flyClient->getArgv()[1]->getPtr());
    FlyObj *val = flyClient->getArgv()[2];

    // 获取超时时间
    int64_t expireSeconds =
            *(reinterpret_cast<int*>(flyClient->getArgv()[3]->getPtr()));
    int64_t expireMilli = (-1 == expireSeconds ? -1 : expireSeconds * 1000);

    setGenericCommand(coordinator, flyClient, key, val, expireMilli);
}

void psetExCommand(const AbstractCoordinator* coordinator,
                   AbstractFlyClient* flyClient) {
    if (NULL == flyClient) {
        return;
    }

    // 如果参数数量 < 3，直接返回
    if (flyClient->getArgc() < 4) {
        flyClient->addReply("missing parameters!");
        return;
    }

    // 获取到key和val
    std::string *key =
            reinterpret_cast<std::string*>(flyClient->getArgv()[1]->getPtr());
    FlyObj *val = flyClient->getArgv()[2];
    int64_t expireMilli =
            *(reinterpret_cast<int*>(flyClient->getArgv()[3]->getPtr()));

    setGenericCommand(coordinator, flyClient, key, val, expireMilli);
}

void expireCommand(const AbstractCoordinator* coordinator,
                   AbstractFlyClient* flyClient) {
}

void expireatCommand(const AbstractCoordinator* coordinator,
                     AbstractFlyClient* flyClient) {

}

void mgetCommand(const AbstractCoordinator* coordinator,
                 AbstractFlyClient* flyClient) {

}

enum ListLocation {
    LIST_HEAD,
    LIST_TAIL
};

void pushGenericCommand(const AbstractCoordinator* coordinator,
                        AbstractFlyClient* flyClient,
                        ListLocation location) {
    if (NULL == flyClient) {
        return;
    }

    // 如果参数数量 < 2，直接返回
    if (flyClient->getArgc() < 3) {
        flyClient->addReply("missing parameters!");
        return;
    }

    // 获取到list
    std::string *key =
            reinterpret_cast<std::string*>(flyClient->getArgv()[1]->getPtr());
    AbstractFlyDB *flyDB = flyClient->getFlyDB();
    std::list<std::string*> *list = reinterpret_cast<std::list<std::string*> *>
    (flyDB->lookupKey(key)->getPtr());
    if (NULL == list) {
        FlyObj *obj = coordinator->getFlyObjLinkedListFactory()->getObject();
        flyDB->add(key, obj);
    }

    for (int j = 2; j < flyClient->getArgc(); j++) {
        if (LIST_HEAD == location) {
            list->push_front(reinterpret_cast<std::string*>
                             (flyClient->getArgv()[1]->getPtr()));
        } else {
            list->push_back(reinterpret_cast<std::string*>
                            (flyClient->getArgv()[1]->getPtr()));
        }
    }

    coordinator->getFlyServer()->addDirty(flyClient->getArgc() - 2);
    flyClient->addReply("push OK!");
}

void rpushCommand(const AbstractCoordinator* coordinator,
                  AbstractFlyClient* flyClient) {
    pushGenericCommand(coordinator, flyClient, LIST_HEAD);
}

void lpushCommand(const AbstractCoordinator* coordinator,
                  AbstractFlyClient* flyClient) {
    pushGenericCommand(coordinator, flyClient, LIST_TAIL);
}

void pushSortCommand(const AbstractCoordinator* coordinator,
                     AbstractFlyClient *flyClient) {
    if (NULL == flyClient) {
        return;
    }

    // 如果参数数量 < 2，直接返回
    if (flyClient->getArgc() < 3) {
        flyClient->addReply("missing parameters!");
        return;
    }

    // 获取到list
    std::string *key =
            reinterpret_cast<std::string*>(flyClient->getArgv()[1]->getPtr());
    AbstractFlyDB *flyDB = flyClient->getFlyDB();
    SkipList<std::string> *list = reinterpret_cast<SkipList<std::string> *>
    (flyDB->lookupKey(key)->getPtr());
    if (NULL == list) {
        FlyObj *obj = coordinator->getFlyObjLinkedListFactory()
                ->getObject(list = new SkipList<std::string>());
        flyDB->add(key, obj);
    }

    for (int j = 2; j < flyClient->getArgc(); j++) {
        list->insertNode(reinterpret_cast<std::string*>
                         (flyClient->getArgv()[1]->getPtr()));
    }

    coordinator->getFlyServer()->addDirty(flyClient->getArgc() - 2);
    flyClient->addReply("push OK!");
}

void popSortCommand(const AbstractCoordinator* coordinator,
                    AbstractFlyClient *flyClient) {
    if (NULL == flyClient) {
        return;
    }

    // 如果参数数量 < 2，直接返回
    if (flyClient->getArgc() < 3) {
        flyClient->addReply("missing parameters!");
        return;
    }

    char buf[100];
    std::string *key =
            reinterpret_cast<std::string *>(flyClient->getArgv()[1]->getPtr());
    FlyObj *val = flyClient->getFlyDB()->lookupKey(key);
    if (NULL == val) {
        flyClient->addReply("don`t have key: %s", key);
    }

    SkipList<std::string> *list =
            reinterpret_cast<SkipList<std::string> *>(val->getPtr());
    list->deleteNode(
            reinterpret_cast<std::string *>(flyClient->getArgv()[3]->getPtr()));

    coordinator->getFlyServer()->addDirty(1);
    flyClient->addReply("status OK!");
}

void popGenericCommand(const AbstractCoordinator *coordinator,
                       AbstractFlyClient *flyClient,
                       ListLocation location) {
    if (NULL == flyClient) {
        return;
    }

    // 如果参数数量 < 2，直接返回
    if (flyClient->getArgc() < 2) {
        flyClient->addReply("missing parameters!");
        return;
    }

    char buf[100];
    std::string *key =
            reinterpret_cast<std::string *>(flyClient->getArgv()[1]->getPtr());
    FlyObj *val = flyClient->getFlyDB()->lookupKey(key);
    if (NULL == val) {
        flyClient->addReply("don`t have key: %s", key);
    }

    std::list<std::string *> *list =
            reinterpret_cast<std::list<std::string *> *>(val->getPtr());
    if (LIST_HEAD == location) {
        list->pop_front();
    } else {
        list->pop_back();
    }

    coordinator->getFlyServer()->addDirty(1);
    flyClient->addReply("status OK!");
}

void rpopCommand(const AbstractCoordinator* coordinator,
                 AbstractFlyClient* flyClient) {
    popGenericCommand(coordinator, flyClient, LIST_HEAD);
}

void lpopCommand(const AbstractCoordinator* coordinator,
                 AbstractFlyClient* flyClient) {
    popGenericCommand(coordinator, flyClient, LIST_TAIL);
}

void hsetCommand(const AbstractCoordinator* coordinator,
                 AbstractFlyClient* flyClient) {
    if (NULL == flyClient) {
        return;
    }

    // 如果参数数量 != 2n，直接返回
    uint8_t argc = flyClient->getArgc();
    if (argc < 2 || argc % 2 == 1) {
        flyClient->addReply("parameters error!");
        return;
    }

    std::string *table =
            reinterpret_cast<std::string *>(flyClient->getArgv()[1]->getPtr());
    FlyObj *val = flyClient->getFlyDB()->lookupKey(table);
    if (NULL == val) {
        val = coordinator->getFlyObjHashTableFactory()->getObject();
    }

    Dict<std::string, std::string> *dict =
            reinterpret_cast<Dict<std::string, std::string> *>(val->getPtr());
    for (int i = 2; i < argc; i = i + 2) {
        std::string *key = reinterpret_cast<std::string *>
        (flyClient->getArgv()[i]->getPtr());
        std::string *val = reinterpret_cast<std::string *>
        (flyClient->getArgv()[i + 1]->getPtr());
        dict->addEntry(key, val);
    }

    coordinator->getFlyServer()->addDirty(argc / 2 - 1);
    flyClient->addReply("status OK!");
}

void hgetCommand(const AbstractCoordinator* coordinator,
                 AbstractFlyClient* flyClient) {
    if (NULL == flyClient) {
        return;
    }

    // 如果参数数量 < 2，直接返回
    char buf[100];
    if (flyClient->getArgc() < 3) {
        flyClient->addReply("missing parameters!");
        return;
    }

    std::string *tableName =
            reinterpret_cast<std::string *>(flyClient->getArgv()[1]->getPtr());
    FlyObj *table = flyClient->getFlyDB()->lookupKey(tableName);
    if (NULL == table) {
        flyClient->addReply("Don`t have this talbe: %s", tableName);
        return;
    }

    Dict<std::string, std::string> *dict =
            reinterpret_cast<Dict<std::string, std::string> *>(table->getPtr());
    std::string *key =
            reinterpret_cast<std::string *>(flyClient->getArgv()[2]->getPtr());
    std::string *val = dict->fetchValue(key);
    if (NULL == val) {
        flyClient->addReply("Don`t have key : %s", key);
        return;
    }

    flyClient->addReply("value: %s", val);
    return;
}

void saveCommand(const AbstractCoordinator* coordinator,
                 AbstractFlyClient* flyClient) {
    AbstractFDBHandler *fdbHandler = coordinator->getFdbHandler();
    fdbHandler->save();
}

/** BGSAVE [SCHEDULE] */
void bgsaveCommand(const AbstractCoordinator* coordinator,
                   AbstractFlyClient* flyClient) {
    if (NULL == flyClient) {
        return;
    }

    AbstractFlyServer *flyServer = coordinator->getFlyServer();

    // 如果参数数量 < 2，直接返回
    bool schedule = false;
    if (2 == flyClient->getArgc()) {
        std::string *argv1 = reinterpret_cast<std::string *>(
                flyClient->getArgv()[1]->getPtr());
        if (0 == argv1->compare("schedule")) {
            schedule = true;
        } else {
            flyClient->addReply("wrong parameters");
            return;
        }
    }

    /** 如果有fdb持久化子进程存在，则说明处于fdb过程中，不允许再次执行fdb */
    if (flyServer->haveFdbChildPid()) {
        flyClient->addReply("Background save already in progress");
        return;
    }

    /**
     * 如果有aof持久化子进程存在，则说明处于fdb过程中，
     * 如果是执行fdb子进程调度，则标记schdule flag, 以便于后续在serverCron函数中执行fdb操作
     * 否则，如果是直接进行fdb, 则不允许执行
     **/
    if (flyServer->haveAofChildPid()) {
        if (!schedule) {
            flyClient->addReply("An AOF log rewriting in progress: can't BGSAVE"
                                " right now. Use BGSAVE SCHEDULE in order to "
                                "schedule a BGSAVE whenever possible.");
            return;
        } else {
            flyServer->setFdbBGSaveScheduled(true);
            flyClient->addReply("Background saving scheduled");
            return;
        }
    }

    if (coordinator->getFdbHandler()->backgroundSave() > 0) {
        flyClient->addReply("Background saving started");
    } else {
        flyClient->addReply("error to do fdb");
    }

    return;
}

