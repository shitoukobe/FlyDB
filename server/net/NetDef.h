//
// Created by 赵立伟 on 2018/11/3.
//

#ifndef FLYDB_NETDEF_H
#define FLYDB_NETDEF_H

const int NET_NONE = 0;
const int NET_IP_ONLY = 1<<0;

enum NetConnectFlag {
    NET_CONNECT_NONE = 0,
    NET_CONNECT_NONBLOCK = 1,
    NET_CONNECT_BE_BINDING = 2     /* Best effort binding. */
};

const int MAX_ACCEPTS_PER_CALL = 1000;
const int NET_IP_STR_LEN = 46;

#endif //FLYDB_NETDEF_H
