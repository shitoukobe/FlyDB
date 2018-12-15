//
// Created by 赵立伟 on 2018/12/8.
//

#include "Coordinator.h"
#include "../net/NetHandler.h"
#include "../config/TextConfigReader.h"
#include "../config/ConfigCache.h"
#include "../fdb/FDBHandler.h"
#include "../atomic/AtomicHandler.h"
#include "../flyClient/FlyClient.h"
#include "../flyClient/FlyClientFactory.h"
#include "../flyObj/flyObjHashTable/FlyObjHashTableFactory.h"
#include "../flyObj/FlyObjInt/FlyObjIntFactory.h"
#include "../flyObj/FlyObjLinkedList/FlyObjLinkedListFactory.h"
#include "../flyObj/FlyObjSkipList/FlyObjSkipListFactory.h"
#include "../flyObj/FlyObjIntSet/FlyObjIntSetFactory.h"
#include "../flyObj/FlyObjString/FlyObjStringFactory.h"

Coordinator::Coordinator() {
    /** 加载config **/
    std::string configfile = "fly.conf";                    /** 配置文件名字 */
    AbstractConfigReader *configReader = new TextConfigReader(configfile);
    ConfigCache *configCache = configReader->loadConfig();

    /** client factory **/
    this->flyClientFactory = new FlyClientFactory();

    /** fly obj factory **/
    this->flyObjHashTableFactory = new FlyObjHashTableFactory();
    this->flyObjIntFactory = new FlyObjIntFactory();
    this->flyObjLinkedListFactory = new FlyObjLinkedListFactory();
    this->flyObjSkipListFactory = new FlyObjSkipListFactory();
    this->flyObjIntSetFactory = new FlyObjIntSetFactory();
    this->flyObjStringFactory = new FlyObjStringFactory();

    /** net handler **/
    this->netHandler = NetHandler::getInstance();

    /** logger初始化 */
    FileLogFactory::init(configCache->getLogfile(),
                         configCache->getSyslogEnabled(),
                         configCache->getVerbosity());
    if (configCache->getSyslogEnabled()) {          /** syslog */
        openlog(configCache->getSyslogIdent(),
                LOG_PID | LOG_NDELAY | LOG_NOWAIT,
                configCache->getSyslogFacility());
    }

    /** fdb handler **/
    this->fdbHandler = new FDBHandler(this,
                                      configCache->getFdbFile(),
                                      CONFIG_LOADING_INTERVAL_BYTES);

    /** event loop **/
    this->flyServer = new FlyServer(this);
    this->eventLoop =
            new EventLoop(this, flyServer->getMaxClients() + CONFIG_FDSET_INCR);

    /** flyserver初始化 **/
    this->flyServer->init(configCache);
}

Coordinator::~Coordinator() {
    delete this->flyServer;
    delete this->eventLoop;
    delete this->fdbHandler;
    delete this->netHandler;
    delete this->aofHandler;
}

AbstractNetHandler *Coordinator::getNetHandler() const {
    return this->netHandler;
}

AbstractFlyServer *Coordinator::getFlyServer() const {
    return this->flyServer;
}

AbstractEventLoop *Coordinator::getEventLoop() const {
    return this->eventLoop;
}

AbstractAOFHandler *Coordinator::getAofHandler() const {
    return this->aofHandler;
}

AbstractFDBHandler *Coordinator::getFdbHandler() const {
    return this->fdbHandler;
}

AbstractFlyClientFactory *Coordinator::getFlyClientFactory() const {
    return this->flyClientFactory;
}

AbstractFlyObjFactory *Coordinator::getFlyObjHashTableFactory() const {
    return flyObjHashTableFactory;
}

AbstractFlyObjFactory *Coordinator::getFlyObjIntFactory() const {
    return flyObjIntFactory;
}

AbstractFlyObjFactory *Coordinator::getFlyObjLinkedListFactory() const {
    return flyObjLinkedListFactory;
}

AbstractFlyObjFactory *Coordinator::getFlyObjSkipListFactory() const {
    return flyObjSkipListFactory;
}

AbstractFlyObjFactory *Coordinator::getFlyObjIntSetFactory() const {
    return flyObjIntSetFactory;
}

AbstractFlyObjFactory *Coordinator::getFlyObjStringFactory() const {
    return flyObjStringFactory;
}