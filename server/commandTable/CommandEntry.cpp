//
// Created by 赵立伟 on 2018/9/19.
//

#include <iostream>
#include "CommandEntry.h"
#include "../flyServer/interface/AbstractFlyServer.h"

void versionCommand(AbstractFlyServer* server, AbstractFlyClient *client) {
    if (NULL == server || NULL == client) {
        return;
    }

    char buf[1024];
    snprintf(buf, sizeof(buf), "FlyDB version: %s",
             server->getVersion().c_str());
    client->addReply(buf, strlen(buf));
}

void getCommand(AbstractFlyServer* flyServer,
                AbstractFlyClient* flyClient) {
    if (NULL == flyClient) {
        return;
    }

    // 如果参数数量<2，直接返回
    if (flyClient->getArgc() < 2) {
        char buf[100];
        snprintf(buf, sizeof(buf), "missing parameters!");
        flyClient->addReply(buf, strlen(buf));
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

void setGenericCommand(AbstractFlyClient *flyClient,
                       std::string *key,
                       FlyObj *val,
                       int64_t expireMilli) {
    // 将key和val添加到flydb中
    flyClient->getFlyDB()->addExpire(key, val, expireMilli);

    char buf[1024];
    snprintf(buf, sizeof(buf), "set OK!");
    flyClient->addReply(buf, strlen(buf));
}

void setCommand(AbstractFlyServer* flyServer,
                AbstractFlyClient* flyClient) {
    if (NULL == flyClient) {
        return;
    }

    // 如果参数数量 < 2，直接返回
    if (flyClient->getArgc() < 3) {
        char buf[100];
        snprintf(buf, sizeof(buf), "missing parameters!");
        flyClient->addReply(buf, strlen(buf));
        return;
    }

    // 获取到key和val
    std::string *key = reinterpret_cast<std::string*>
    (flyClient->getArgv()[1]->getPtr());
    FlyObj *val = flyClient->getArgv()[2];

    setGenericCommand(flyClient, key, val, -1);
}

void setExCommand(AbstractFlyServer* flyServer,
                AbstractFlyClient* flyClient) {
    if (NULL == flyClient) {
        return;
    }

    // 如果参数数量 < 3，直接返回
    if (flyClient->getArgc() < 4) {
        char buf[100];
        snprintf(buf, sizeof(buf), "missing parameters!");
        flyClient->addReply(buf, strlen(buf));
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

    setGenericCommand(flyClient, key, val, expireMilli);
}

void psetExCommand(AbstractFlyServer* flyServer,
                  AbstractFlyClient* flyClient) {
    if (NULL == flyClient) {
        return;
    }

    // 如果参数数量 < 3，直接返回
    if (flyClient->getArgc() < 4) {
        char buf[100];
        snprintf(buf, sizeof(buf), "missing parameters!");
        flyClient->addReply(buf, strlen(buf));
        return;
    }

    // 获取到key和val
    std::string *key =
            reinterpret_cast<std::string*>(flyClient->getArgv()[1]->getPtr());
    FlyObj *val = flyClient->getArgv()[2];
    int64_t expireMilli =
            *(reinterpret_cast<int*>(flyClient->getArgv()[3]->getPtr()));

    setGenericCommand(flyClient, key, val, expireMilli);
}

void expireCommand(AbstractFlyServer* flyServer,
                   AbstractFlyClient* flyClient) {
}

void expireatCommand(AbstractFlyServer* flyServer,
                     AbstractFlyClient* flyClient) {

}

void mgetCommand(AbstractFlyServer* flyServer,
                 AbstractFlyClient* flyClient) {

}

enum PushLocation {
    LIST_HEAD,
    LIST_TAIL
};

void pushGenericCommand(AbstractFlyServer* flyServer,
                        AbstractFlyClient* flyClient,
                        PushLocation location) {
    if (NULL == flyClient) {
        return;
    }

    // 如果参数数量 < 2，直接返回
    if (flyClient->getArgc() < 3) {
        char buf[100];
        snprintf(buf, sizeof(buf), "missing parameters!");
        flyClient->addReply(buf, strlen(buf));
        return;
    }

    // 获取到list
    std::string *key =
            reinterpret_cast<std::string*>(flyClient->getArgv()[1]->getPtr());
    AbstractFlyDB *flyDB = flyClient->getFlyDB();
    std::list<std::string*> *list = reinterpret_cast<std::list<std::string*> *>
    (flyDB->lookupKey(key)->getPtr());
    if (NULL == list) {
        list = new std::list<std::string*>();
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

    char buf[1024];
    snprintf(buf, sizeof(buf), "push OK!");
    flyClient->addReply(buf, strlen(buf));
}

void rpushCommand(AbstractFlyServer* flyServer,
                  AbstractFlyClient* flyClient) {
}

void lpushCommand(AbstractFlyServer* flyServer,
                  AbstractFlyClient* flyClient) {

}

void lpushxCommand(AbstractFlyServer* flyServer,
                   AbstractFlyClient* flyClient) {

}

void linsertCommand(AbstractFlyServer* flyServer,
                    AbstractFlyClient* flyClient) {

}

void rpopCommand(AbstractFlyServer* flyServer,
                 AbstractFlyClient* flyClient) {

}

void lpopCommand(AbstractFlyServer* flyServer,
                 AbstractFlyClient* flyClient) {

}

void brpopCommand(AbstractFlyServer* flyServer,
                  AbstractFlyClient* flyClient) {

}

void hsetCommand(AbstractFlyServer* flyServer,
                 AbstractFlyClient* flyClient) {

}

void hmgetCommand(AbstractFlyServer* flyServer,
                  AbstractFlyClient* flyClient) {

}

void saveCommand(AbstractFlyServer* flyServer,
                 AbstractFlyClient* flyClient) {

}

void bgsaveCommand(AbstractFlyServer* flyServer,
                   AbstractFlyClient* flyClient) {

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

void CommandEntry::setKeysProc(getKeysProc proc) {
    this->keysProc = keysProc;
}

bool CommandEntry::isFirstKey() const {
    return this->firstKey;
}

void CommandEntry::setFirstKey(bool firstKey) {
    this->firstKey = firstKey;
}

bool CommandEntry::isLastKey() const {
    return this->lastKey;
}

void CommandEntry::setLastKey(bool lastKey) {
    this->lastKey = lastKey;
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
