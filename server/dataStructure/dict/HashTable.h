// Created by 赵立伟 on 2018/9/20.
//

#ifndef FLYDB_HASHTABLE_H
#define FLYDB_HASHTABLE_H

#include "DictEntry.cpp"
#include "DictDef.h"
#include "../../log/interface/AbstractLogHandler.h"

template<class KEY, class VAL>
class HashTable {
 public:
    HashTable(uint32_t size);
    virtual ~HashTable();

    int addEntry(const KEY key, const VAL val);
    DictEntry<KEY, VAL>* findEntry(const KEY key);
    int deleteEntry(const KEY key);
    bool needExpand(bool canResize) const;
    bool needShrink() const;
    bool hasKey(const KEY key);
    uint32_t getIndexWithKey(const KEY key) const;
    uint32_t getIndex(uint32_t cursor) const;
    uint32_t getSize() const;
    uint32_t getUsed() const;
    bool isEmpty() const;
    DictEntry<KEY, VAL>* getEntryBy(uint32_t index) const;
    int scanEntries(uint32_t index,
                    int (*scanProc)(void* priv,
                                      const KEY key,
                                      const VAL val),
                     void* priv);
    uint32_t getMask() const;

private:
    DictEntry<KEY, VAL>** table;
    uint32_t size;
    uint32_t used;
    uint32_t mask;

    AbstractLogHandler *logHandler;
};

#endif //FLYDB_HASHTABLE_H
