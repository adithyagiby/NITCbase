#include "BlockAccess.h"
#include <cstring>

RecId BlockAccess::linearSearch(int relId, char attrName[ATTR_SIZE],
                                union Attribute attrVal, int op)
{
    RecId prevRecId;
    RelCacheTable::getSearchIndex(relId, &prevRecId);

    int block, slot;

    if (prevRecId.block == -1 && prevRecId.slot == -1)
    {
        RelCatEntry relCatEntry;
        RelCacheTable::getRelCatEntry(relId, &relCatEntry);

        block = relCatEntry.firstBlk;
        slot = 0;
    }
    else
    {
        block = prevRecId.block;
        slot = prevRecId.slot + 1;
    }

    while (block != -1)
    {
        RecBuffer recBuffer(block);

        HeadInfo head;
        recBuffer.getHeader(&head);

        unsigned char slotMap[head.numSlots];
        recBuffer.getSlotMap(slotMap);

        if (slot >= head.numSlots)
        {
            block = head.rblock;
            slot = 0;
            continue;
        }

        if (slotMap[slot] == SLOT_UNOCCUPIED)
        {
            slot++;
            continue;
        }

        Attribute record[head.numAttrs];
        recBuffer.getRecord(record, slot);

        AttrCatEntry attrCatEntry;
        int attrStatus = AttrCacheTable::getAttrCatEntry(
            relId, attrName, &attrCatEntry
        );

        if (attrStatus != SUCCESS)
        {
            return RecId{-1, -1};
        }

        Attribute currentAttrVal = record[attrCatEntry.offset];

        int cmpVal = compareAttrs(
            currentAttrVal,
            attrVal,
            attrCatEntry.attrType
        );

        if ((op == NE && cmpVal != 0) ||
            (op == LT && cmpVal < 0) ||
            (op == LE && cmpVal <= 0) ||
            (op == EQ && cmpVal == 0) ||
            (op == GT && cmpVal > 0) ||
            (op == GE && cmpVal >= 0))
        {
            RecId searchIndex = {block, slot};

            RelCacheTable::setSearchIndex(relId, &searchIndex);

            return searchIndex;
        }

        slot++;
    }

    return RecId{-1, -1};
}


int BlockAccess::renameRelation(char oldName[ATTR_SIZE],
                                char newName[ATTR_SIZE])
{
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute newRelationName;
    strcpy(newRelationName.sVal, newName);

    char relCatAttrRelName[ATTR_SIZE];
    strcpy(relCatAttrRelName, RELCAT_ATTR_RELNAME);

    RecId recId = BlockAccess::linearSearch(
        RELCAT_RELID,
        relCatAttrRelName,
        newRelationName,
        EQ
    );

    if (recId.block != -1 && recId.slot != -1)
    {
        return E_RELEXIST;
    }

    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute oldRelationName;
    strcpy(oldRelationName.sVal, oldName);

    recId = BlockAccess::linearSearch(
        RELCAT_RELID,
        relCatAttrRelName,
        oldRelationName,
        EQ
    );

    if (recId.block == -1 && recId.slot == -1)
    {
        return E_RELNOTEXIST;
    }

    Attribute record[RELCAT_NO_ATTRS];

    RecBuffer recBuffer(RELCAT_BLOCK);
    recBuffer.getRecord(record, recId.slot);

    strcpy(record[RELCAT_REL_NAME_INDEX].sVal, newName);
    recBuffer.setRecord(record, recId.slot);

    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    int numOfAttrs = record[RELCAT_NO_ATTRIBUTES_INDEX].nVal;

    char attrCatAttrRelName[ATTR_SIZE];
    strcpy(attrCatAttrRelName, ATTRCAT_ATTR_RELNAME);

    for (int i = 0; i < numOfAttrs; i++)
    {
        RecId rec_id = BlockAccess::linearSearch(
            ATTRCAT_RELID,
            attrCatAttrRelName,
            oldRelationName,
            EQ
        );

        if (rec_id.block == -1 && rec_id.slot == -1)
        {
            break;
        }

        Attribute attrRecord[ATTRCAT_NO_ATTRS];

        RecBuffer attrBuffer(rec_id.block);
        attrBuffer.getRecord(attrRecord, rec_id.slot);

        strcpy(attrRecord[ATTRCAT_REL_NAME_INDEX].sVal, newName);
        attrBuffer.setRecord(attrRecord, rec_id.slot);
    }

    return SUCCESS;
}


int BlockAccess::renameAttribute(char relName[ATTR_SIZE],
                                 char oldName[ATTR_SIZE],
                                 char newName[ATTR_SIZE])
{
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute relNameAttr;
    strcpy(relNameAttr.sVal, relName);

    char relCatAttrRelName[ATTR_SIZE];
    strcpy(relCatAttrRelName, RELCAT_ATTR_RELNAME);

    RecId recId = BlockAccess::linearSearch(
        RELCAT_RELID,
        relCatAttrRelName,
        relNameAttr,
        EQ
    );

    if (recId.block == -1 && recId.slot == -1)
    {
        return E_RELNOTEXIST;
    }

    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    RecId attrToRenameRecId;
    attrToRenameRecId.block = -1;
    attrToRenameRecId.slot = -1;

    Attribute attrCatEntryRecord[ATTRCAT_NO_ATTRS];

    char attrCatAttrRelName[ATTR_SIZE];
    strcpy(attrCatAttrRelName, ATTRCAT_ATTR_RELNAME);

    while (true)
    {
        RecId rec_id = BlockAccess::linearSearch(
            ATTRCAT_RELID,
            attrCatAttrRelName,
            relNameAttr,
            EQ
        );

        if (rec_id.block == -1 && rec_id.slot == -1)
        {
            break;
        }

        RecBuffer attrBuffer(rec_id.block);
        attrBuffer.getRecord(attrCatEntryRecord, rec_id.slot);

        if (strcmp(
                attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,
                oldName) == 0)
        {
            attrToRenameRecId.block = rec_id.block;
            attrToRenameRecId.slot = rec_id.slot;
        }

        if (strcmp(
                attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,
                newName) == 0)
        {
            return E_ATTREXIST;
        }
    }

    if (attrToRenameRecId.block == -1 &&
        attrToRenameRecId.slot == -1)
    {
        return E_ATTRNOTEXIST;
    }

    RecBuffer attrToRenameBuff(attrToRenameRecId.block);

    attrToRenameBuff.getRecord(
        attrCatEntryRecord,
        attrToRenameRecId.slot
    );

    strcpy(
        attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,
        newName
    );

    attrToRenameBuff.setRecord(
        attrCatEntryRecord,
        attrToRenameRecId.slot
    );

    return SUCCESS;
}


int BlockAccess::insert(int relId, Attribute *record)
{
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(relId, &relCatEntry);

    int blockNum = relCatEntry.firstBlk;

    RecId rec_id = {-1, -1};

    int numOfSlots = relCatEntry.numSlotsPerBlk;
    int numOfAttributes = relCatEntry.numAttrs;

    int prevBlockNum = -1;

    while (blockNum != -1)
    {
        RecBuffer recBuffer(blockNum);

        HeadInfo head;
        recBuffer.getHeader(&head);

        unsigned char slotMap[head.numSlots];
        recBuffer.getSlotMap(slotMap);

        for (int i = 0; i < head.numSlots; i++)
        {
            if (slotMap[i] == SLOT_UNOCCUPIED)
            {
                rec_id.block = blockNum;
                rec_id.slot = i;
                break;
            }
        }

        if (rec_id.block != -1 && rec_id.slot != -1)
        {
            break;
        }

        prevBlockNum = blockNum;
        blockNum = head.rblock;
    }

    if (rec_id.block == -1 && rec_id.slot == -1)
    {
        if (relId == RELCAT_RELID)
        {
            return E_MAXRELATIONS;
        }

        RecBuffer newRecBuffer;
        int ret = newRecBuffer.getBlockNum();

        if (ret < 0)
        {
            return ret;
        }

        rec_id.block = ret;
        rec_id.slot = 0;

        HeadInfo newHead;
        newHead.blockType = REC;
        newHead.lblock = prevBlockNum;
        newHead.numAttrs = numOfAttributes;
        newHead.numEntries = 0;
        newHead.numSlots = numOfSlots;
        newHead.pblock = -1;
        newHead.rblock = -1;

        newRecBuffer.setHeader(&newHead);

        unsigned char newSlotMap[numOfSlots];

        for (int i = 0; i < numOfSlots; i++)
        {
            newSlotMap[i] = SLOT_UNOCCUPIED;
        }

        newRecBuffer.setSlotMap(newSlotMap);

        if (prevBlockNum != -1)
        {
            RecBuffer oldRecBuffer(prevBlockNum);

            HeadInfo prevHead;
            oldRecBuffer.getHeader(&prevHead);

            prevHead.rblock = rec_id.block;
            oldRecBuffer.setHeader(&prevHead);

            RelCacheTable::getRelCatEntry(relId, &relCatEntry);
            relCatEntry.lastBlk = rec_id.block;
            RelCacheTable::setRelCatEntry(relId, &relCatEntry);
        }
        else
        {
            RelCacheTable::getRelCatEntry(relId, &relCatEntry);

            relCatEntry.firstBlk = rec_id.block;
            relCatEntry.lastBlk = rec_id.block;

            RelCacheTable::setRelCatEntry(relId, &relCatEntry);
        }
    }

    RecBuffer currRec(rec_id.block);
    currRec.setRecord(record, rec_id.slot);

    HeadInfo currHead;
    currRec.getHeader(&currHead);

    int currSlotNum = currHead.numSlots;
    unsigned char currSlotMap[currSlotNum];

    currRec.getSlotMap(currSlotMap);

    currSlotMap[rec_id.slot] = SLOT_OCCUPIED;
    currRec.setSlotMap(currSlotMap);

    currHead.numEntries += 1;
    currRec.setHeader(&currHead);

    RelCacheTable::getRelCatEntry(relId, &relCatEntry);
    relCatEntry.numRecs += 1;
    RelCacheTable::setRelCatEntry(relId, &relCatEntry);

    return SUCCESS;
}


// SEARCH: use linear search without an index, B+ tree search with an index.
int BlockAccess::search(int relId, Attribute *record,
                        char attrName[ATTR_SIZE],
                        Attribute attrVal, int op)
{
    RecId recId;

    AttrCatEntry attrCatEntry;

    int ret = AttrCacheTable::getAttrCatEntry(
        relId, attrName, &attrCatEntry
    );

    if (ret != SUCCESS)
    {
        return ret;
    }

    if (attrCatEntry.rootBlock == -1)
    {
        recId = BlockAccess::linearSearch(
            relId, attrName, attrVal, op
        );
    }
    else
    {
        recId = BPlusTree::bPlusSearch(
            relId, attrName, attrVal, op
        );
    }

    if (recId.block == -1 && recId.slot == -1)
    {
        return E_NOTFOUND;
    }

    RecBuffer recBuffer(recId.block);
    recBuffer.getRecord(record, recId.slot);

    return SUCCESS;
}


int BlockAccess::deleteRelation(char relName[ATTR_SIZE])
{
    if (strcmp(relName, RELCAT_RELNAME) == 0 ||
        strcmp(relName, ATTRCAT_RELNAME) == 0)
    {
        return E_NOTPERMITTED;
    }

    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute relNameAttr;
    strcpy(relNameAttr.sVal, relName);

    char relCatAttrRelname[ATTR_SIZE];
    strcpy(relCatAttrRelname, RELCAT_ATTR_RELNAME);

    RecId recId = BlockAccess::linearSearch(
        RELCAT_RELID,
        relCatAttrRelname,
        relNameAttr,
        EQ
    );

    if (recId.block == -1 && recId.slot == -1)
    {
        return E_RELNOTEXIST;
    }

    Attribute relCatEntryRecord[RELCAT_NO_ATTRS];

    RecBuffer recBuffer(recId.block);
    recBuffer.getRecord(relCatEntryRecord, recId.slot);

    int firstBlock = relCatEntryRecord[RELCAT_FIRST_BLOCK_INDEX].nVal;

    int currBlock = firstBlock;

    while (currBlock != -1)
    {
        HeadInfo head;
        RecBuffer currRecBuffer(currBlock);

        currRecBuffer.getHeader(&head);

        currBlock = head.rblock;
        currRecBuffer.releaseBlock();
    }

    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    int numberOfAttributesDeleted = 0;

    while (true)
    {
        RecId attrCatRecId = BlockAccess::linearSearch(
            ATTRCAT_RELID,
            relCatAttrRelname,
            relNameAttr,
            EQ
        );

        if (attrCatRecId.block == -1 && attrCatRecId.slot == -1)
        {
            break;
        }

        numberOfAttributesDeleted++;

        RecBuffer attrBuffer(attrCatRecId.block);

        HeadInfo head;
        attrBuffer.getHeader(&head);

        Attribute attrRecord[head.numAttrs];
        attrBuffer.getRecord(attrRecord, attrCatRecId.slot);

        unsigned char slotMap[head.numSlots];
        attrBuffer.getSlotMap(slotMap);

        slotMap[attrCatRecId.slot] = SLOT_UNOCCUPIED;
        attrBuffer.setSlotMap(slotMap);

        head.numEntries--;
        attrBuffer.setHeader(&head);

        if (head.numEntries == 0)
        {
            RelCatEntry attrRelCatEntry;
            RelCacheTable::getRelCatEntry(
                ATTRCAT_RELID, &attrRelCatEntry
            );

            if (head.lblock != -1)
            {
                RecBuffer leftRecBuffer(head.lblock);

                HeadInfo leftHead;
                leftRecBuffer.getHeader(&leftHead);

                leftHead.rblock = head.rblock;
                leftRecBuffer.setHeader(&leftHead);
            }
            else
            {
                attrRelCatEntry.firstBlk = head.rblock;
            }

            if (head.rblock != -1)
            {
                RecBuffer rightRecBuffer(head.rblock);

                HeadInfo rightHead;
                rightRecBuffer.getHeader(&rightHead);

                rightHead.lblock = head.lblock;
                rightRecBuffer.setHeader(&rightHead);
            }
            else
            {
                attrRelCatEntry.lastBlk = head.lblock;
            }

            RelCacheTable::setRelCatEntry(
                ATTRCAT_RELID, &attrRelCatEntry
            );

            attrBuffer.releaseBlock();
        }
    }

    HeadInfo head;
    recBuffer.getHeader(&head);

    head.numEntries--;
    recBuffer.setHeader(&head);

    unsigned char slotMap[head.numSlots];
    recBuffer.getSlotMap(slotMap);

    slotMap[recId.slot] = SLOT_UNOCCUPIED;
    recBuffer.setSlotMap(slotMap);

    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(RELCAT_RELID, &relCatEntry);

    relCatEntry.numRecs--;
    RelCacheTable::setRelCatEntry(RELCAT_RELID, &relCatEntry);

    RelCatEntry attrCatEntry;
    RelCacheTable::getRelCatEntry(ATTRCAT_RELID, &attrCatEntry);

    attrCatEntry.numRecs -= numberOfAttributesDeleted;
    RelCacheTable::setRelCatEntry(ATTRCAT_RELID, &attrCatEntry);

    return SUCCESS;
}


int BlockAccess::project(int relId, Attribute *record)
{
    RecId prevRecId;
    RelCacheTable::getSearchIndex(relId, &prevRecId);

    int block, slot;

    if (prevRecId.block == -1 && prevRecId.slot == -1)
    {
        RelCatEntry relCatEntry;

        int ret = RelCacheTable::getRelCatEntry(
            relId, &relCatEntry
        );

        if (ret != SUCCESS)
        {
            return ret;
        }

        block = relCatEntry.firstBlk;
        slot = 0;
    }
    else
    {
        block = prevRecId.block;
        slot = prevRecId.slot + 1;
    }

    while (block != -1)
    {
        RecBuffer recBuffer(block);

        HeadInfo head;
        recBuffer.getHeader(&head);

        unsigned char slotMap[head.numSlots];
        recBuffer.getSlotMap(slotMap);

        if (slot >= head.numSlots)
        {
            block = head.rblock;
            slot = 0;
        }
        else if (slotMap[slot] == SLOT_UNOCCUPIED)
        {
            slot++;
        }
        else
        {
            break;
        }
    }

    if (block == -1)
    {
        return E_NOTFOUND;
    }

    RecId nextRecId{block, slot};

    RelCacheTable::setSearchIndex(relId, &nextRecId);

    RecBuffer recBuffer(nextRecId.block);
    recBuffer.getRecord(record, nextRecId.slot);

    return SUCCESS;
}