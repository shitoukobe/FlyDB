//
// Created by 赵立伟 on 2018/11/3.
//

#include <sys/socket.h>
#include <sys/time.h>
#include <string>
#include <errno.h>
#include <netinet/in.h>
#include <cstdarg>
#include <cstdio>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <zconf.h>
#include <fcntl.h>
#include <array>
#include <sys/un.h>
#include <sys/stat.h>
#include "NetHandler.h"
#include "NetDef.h"

NetHandler* NetHandler::getInstance() {
    static NetHandler* instance;
    if (NULL == instance) {
        instance = new NetHandler();
    }

    return instance;
}

int NetHandler::setV6Only(char *err, int fd) {
    int yes = 1;

    if (-1 == setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &yes, sizeof(yes))) {
        setError(err, "setsockopt IPV6_V6ONLY: %s", strerror(errno));
        return -1;
    }
    return 1;
}

int NetHandler::setSendTimeout(char *err, int fd, long long ms) {
    struct timeval tv;
    tv.tv_sec = ms / 1000;
    tv.tv_usec = ms % 1000;

    if (-1 == setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv))) {
        setError(err, "setsockopt SO_SNDTIMEO: %s", strerror(errno));
        return -1;
    }
    return 1;
}

int NetHandler::setTcpNoDelay(char *err, int fd, int val) {
    if (-1 == setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val))) {
        setError(err, "setsockopt TCP_NODELAY: %s", strerror(errno));
        return -1;
    }
    return 1;
}

int NetHandler::setSendBuffer(char *err, int fd, int buffsize) {
    if (-1 == setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &buffsize, sizeof(buffsize))) {
        setError(err, "setsockopt SO_SNDBUF: %s", strerror(errno));
        return -1;
    }
    return 1;
}

int NetHandler::setTcpKeepAlive(char *err, int fd) {
    int yes;
    if (-1 == setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(yes))) {
        setError(err, "setsockopt SO_KEEPALIVE: %s", strerror(errno));
        return -1;
    }
    return 1;
}

int NetHandler::resolve(char *err, char *host, char *ipbuf, size_t ipbuf_len) {
    return genericResolve(err, host, ipbuf, ipbuf_len, NET_NONE);
}

int NetHandler::resolveIP(char *err, char *host, char *ipbuf, size_t ipbuf_len) {
    return genericResolve(err, host, ipbuf, ipbuf_len, NET_IP_ONLY);
}

int NetHandler::createSocket(char *err, int domain) {
    int sock;
    if ((sock = socket(domain, SOCK_STREAM, 0)) == -1) {
        setError(err, "creating socket: %s", strerror(errno));
        return -1;
    }

    if (-1 == setReuseAddr(err, sock)) {
        close(sock);
        return -1;
    }
    return sock;
}

int NetHandler::keepAlive(char *err, int fd, int interval) {
    int val = 1;

    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val)) == -1) {
        setError(err, "setsockopt SO_KEEPALIVE: %s", strerror(errno));
        return -1;
    }

#ifdef __linux__
    // 这些都是linux特有api，所以只有linux环境下才会执行如下步骤
    // 开始首次keepAlive探测前的TCP空闲时间
    val = interval;
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &val, sizeof(val)) < 0) {
        setError(err, "setsockopt TCP_KEEPIDLE: %s\n", strerror(errno));
        return -1;
    }

    // 两次keepAlive探测间的间隔时间
    val = interval / 3;
    if (val == 0) {
        val = 1;
    }
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &val, sizeof(val)) < 0) {
        setError(err, "setsockopt TCP_KEEPINTVL: %s\n", strerror(errno));
        return -1;
    }

    // 判定断开前的keepAlive探测次数
    val = 3;
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &val, sizeof(val)) < 0) {
        setError(err, "setsockopt TCP_KEEPCNT: %s\n", strerror(errno));
        return -1;
    }
#else
    ((void) interval); /* Avoid unused var warning for non Linux systems. */
#endif

    return 1;
}

int NetHandler::tcpConnect(char *err, char *addr, int port) {
    return tcpGenericConnect(err, addr, port, NULL, NET_CONNECT_NONE);
}

int NetHandler::tcpNonBlockConnect(char *err, char *addr, int port) {
    return tcpGenericConnect(err, addr, port, NULL, NET_CONNECT_NONBLOCK);
}

int NetHandler::tcpNonBlockBindConnect(char *err, char *addr, int port, char *source_addr) {
    return tcpGenericConnect(err, addr, port, source_addr, NET_CONNECT_NONBLOCK);
}

int NetHandler::tcpNonBlockBestEffortBindConnect(char *err, char *addr, int port, char *source_addr) {
    return tcpGenericConnect(err, addr, port, source_addr, NET_CONNECT_NONBLOCK | NET_CONNECT_BE_BINDING);
}

int NetHandler::genericResolve(char *err, char *host, char *ipbuf, size_t ipbuf_len, int flags) {
    struct addrinfo hints, *info;
    int rv;

    memset(&hints,0,sizeof(hints));
    if (flags & NET_IP_ONLY) hints.ai_flags = AI_NUMERICHOST;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;  /* specify socktype to avoid dups */

    if ((rv = getaddrinfo(host, NULL, &hints, &info)) != 0) {
        setError(err, "%s", gai_strerror(rv));
        return -1;
    }
    if (info->ai_family == AF_INET) {
        struct sockaddr_in *sa = (struct sockaddr_in *)info->ai_addr;
        inet_ntop(AF_INET, &(sa->sin_addr), ipbuf, ipbuf_len);
    } else {
        struct sockaddr_in6 *sa = (struct sockaddr_in6 *)info->ai_addr;
        inet_ntop(AF_INET6, &(sa->sin6_addr), ipbuf, ipbuf_len);
    }

    freeaddrinfo(info);
    return 1;
}

int NetHandler::setReuseAddr(char *err, int fd) {
    int yes = 1;

    if (-1 == setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes))) {
        setError(err, "setsockopt SO_REUSEADDR: %s", strerror(errno));
        return -1;
    }
    return 1;
}

int NetHandler::setListen(char *err, int s, struct sockaddr *sa, socklen_t len, int backlog) {
    if (bind(s, sa, len) == -1) {
        setError(err, "bind: %s", strerror(errno));
        close(s);
        return -1;
    }

    if (listen(s, backlog) == -1) {
        setError(err, "listen: %s", strerror(errno));
        close(s);
        return -1;
    }
    return 1;
}

int NetHandler::tcpGenericConnect(char *err, char *addr, int port, char *source_addr, int flags) {
    int s = -1, rv;
    char portstr[6];  /* strlen("65535") + 1; */
    struct addrinfo hints, *servinfo, *bservinfo, *p, *b;

    snprintf(portstr, sizeof(portstr), "%d", port);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(addr, portstr, &hints, &servinfo)) != 0) {
        setError(err, "%s", gai_strerror(rv));
        return -1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        /**
         * 创建并connect socket, 如果这个servinfo失败了，则尝试下一个
         **/
        if (-1 == (s = socket(p->ai_family, p->ai_socktype, p->ai_protocol))) {
            continue;
        }

        // reuse address
        if (-1 == setReuseAddr(err,s)) {
            goto error;

        }
        // set block
        if (flags & NET_CONNECT_NONBLOCK && -1 == setBlock(err, s, 0)) {
            goto error;
        }

        if (NULL != source_addr) {
            int bound = 0;
            // 获得source地址
            if ((rv = getaddrinfo(source_addr, NULL, &hints, &bservinfo)) != 0) {
                setError(err, "%s", gai_strerror(rv));
                goto error;
            }

            // 绑定地址
            for (b = bservinfo; b != NULL; b = b->ai_next) {
                if (bind(s, b->ai_addr, b->ai_addrlen) != -1) {
                    bound = 1;
                    break;
                }
            }
            freeaddrinfo(bservinfo);
            if (!bound) {
                setError(err, "bind: %s", strerror(errno));
                goto error;
            }
        }
        if (connect(s, p->ai_addr, p->ai_addrlen) == -1) {
            /* If the socket is non-blocking, it is ok for connect() to
             * return an EINPROGRESS error here. */
            if (errno == EINPROGRESS && flags & NET_CONNECT_NONBLOCK) {
                goto end;
            }
            close(s);
            s = -1;
            continue;
        }

        // conect成功
        goto end;
    }
    if (p == NULL) {
        setError(err, "creating socket: %s", strerror(errno));
    }

    error:
    if (s != -1) {
        close(s);
        s = -1;
    }

    end:
    freeaddrinfo(servinfo);

    // 处理best effort binding: 如果传递了source_addr, 但是创建socket失败，则使用不传递source_addr的方式重试一次
    if (-1 == s && source_addr && (flags & NET_CONNECT_BE_BINDING)) {
        return tcpGenericConnect(err, addr, port, NULL, flags);
    } else {
        return s;
    }
}

int NetHandler::tcpServer(char *err, int port, const char *bindaddr, int backlog) {
    return tcpGenericServer(err, port, bindaddr, AF_INET, backlog);
}

int NetHandler::tcp6Server(char *err, int port, const char *bindaddr, int backlog) {
    return tcpGenericServer(err, port, bindaddr, AF_INET6, backlog);
}

int NetHandler::tcpGenericServer(char *err, int port, const char *bindaddr, int af, int backlog) {
    int s = -1, rv;
    char _port[6];  /* strlen("65535") */
    struct addrinfo hints, *servinfo, *p;

    snprintf(_port, 6, "%d", port);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = af;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(bindaddr, _port, &hints, &servinfo)) != 0) {
        setError(err, "%s", gai_strerror(rv));
        return -1;
    }
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((s = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
            continue;

        // set ipv6 only
        if (af == AF_INET6 && -1 == setV6Only(err, s)) {
            dealError(s, servinfo);
            return -1;
        }

        // reuse address
        if (-1 == setReuseAddr(err, s)) {
            dealError(s, servinfo);
            return -1;
        }

        // listen to s
        if (-1 == setListen(err, s, p->ai_addr, p->ai_addrlen, backlog)) {
            dealError(s, servinfo);
            return -1;
        }

        // 执行成功
        freeaddrinfo(servinfo);
        return s;
    }

    // 如果所有地址都执行失败，运行错误
    if (NULL == p) {
        setError(err, "unable to bind socket, errno: %d", errno);
        dealError(s, servinfo);
        return -1;
    }
}

int NetHandler::unixServer(char *err, const char *path, mode_t perm, int backlog) {
    int s;
    struct sockaddr_un sa;

    if (-1 == (s = createSocket(err, AF_LOCAL))) {
        return -1;
    }

    memset(&sa, 0, sizeof(sa));
    sa.sun_family = AF_LOCAL;
    strncpy(sa.sun_path, path, sizeof(sa.sun_path) - 1);
    if (-1 == setListen(err, s, (struct sockaddr*)&sa, sizeof(sa), backlog)) {
        return -1;
    }
    if (perm) {
        chmod(sa.sun_path, perm);
    }

    return s;
}

int NetHandler::setBlock(char *err, int fd, int block) {
    int flags;

    if ((flags = fcntl(fd, F_GETFL)) == -1) {
        setError(err, "fcntl(F_GETFL): %s", strerror(errno));
        return -1;
    }

    if (block) {
        flags &= ~O_NONBLOCK;

    } else {
        flags |= O_NONBLOCK;
    }

    if (fcntl(fd, F_SETFL, flags) == -1) {
        setError(err, "fcntl(F_SETFL,O_NONBLOCK): %s", strerror(errno));
        return -1;
    }
    return 1;
}

void NetHandler::setError(char *err, const char *fmt, ...) {
    if (!err) {
        return;
    }

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(err, NET_ERR_LEN, fmt, ap);
    va_end(ap);
}

void NetHandler::dealError(int fd, struct addrinfo *servinfo) {
    if (fd != -1) {
        close(fd);
    }

    freeaddrinfo(servinfo);
}

