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
        int ret = RelCacheTable::getRelCatEntry(relId, &relCatEntry);

        if (ret != SUCCESS)
            return RecId{-1, -1};

        block = relCatEntry.firstBlk;
        slot = 0;
    }
    else
    {
        block = prevRecId.block;
        slot = prevRecId.slot + 1;
    }

    AttrCatEntry attrCatEntry;
    int ret = AttrCacheTable::getAttrCatEntry(
        relId, attrName, &attrCatEntry);

    if (ret != SUCCESS)
        return RecId{-1, -1};

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

        Attribute currentAttrVal = record[attrCatEntry.offset];

        int cmpVal = compareAttrs(
            currentAttrVal,
            attrVal,
            attrCatEntry.attrType
        );

        if (
            (op == NE && cmpVal != 0) ||
            (op == LT && cmpVal < 0) ||
            (op == LE && cmpVal <= 0) ||
            (op == EQ && cmpVal == 0) ||
            (op == GT && cmpVal > 0) ||
            (op == GE && cmpVal >= 0)
        )
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
        return E_RELEXIST;

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
        return E_RELNOTEXIST;

    Attribute record[RELCAT_NO_ATTRS];

    RecBuffer recBuffer(recId.block);
    recBuffer.getRecord(record, recId.slot);

    strcpy(record[RELCAT_REL_NAME_INDEX].sVal, newName);
    recBuffer.setRecord(record, recId.slot);

    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    int numOfAttrs = record[RELCAT_NO_ATTRIBUTES_INDEX].nVal;

    char attrCatAttrRelName[ATTR_SIZE];
    strcpy(attrCatAttrRelName, ATTRCAT_ATTR_RELNAME);

    for (int i = 0; i < numOfAttrs; i++)
    {
        RecId attrRecId = BlockAccess::linearSearch(
            ATTRCAT_RELID,
            attrCatAttrRelName,
            oldRelationName,
            EQ
        );

        if (attrRecId.block == -1 && attrRecId.slot == -1)
            break;

        Attribute attrRecord[ATTRCAT_NO_ATTRS];

        RecBuffer attrBuffer(attrRecId.block);
        attrBuffer.getRecord(attrRecord, attrRecId.slot);

        strcpy(
            attrRecord[ATTRCAT_REL_NAME_INDEX].sVal,
            newName
        );

        attrBuffer.setRecord(attrRecord, attrRecId.slot);
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
        return E_RELNOTEXIST;

    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    RecId attrToRenameRecId;
    attrToRenameRecId.block = -1;
    attrToRenameRecId.slot = -1;

    Attribute attrCatEntryRecord[ATTRCAT_NO_ATTRS];

    char attrCatAttrRelName[ATTR_SIZE];
    strcpy(attrCatAttrRelName, ATTRCAT_ATTR_RELNAME);

    while (true)
    {
        RecId attrRecId = BlockAccess::linearSearch(
            ATTRCAT_RELID,
            attrCatAttrRelName,
            relNameAttr,
            EQ
        );

        if (attrRecId.block == -1 && attrRecId.slot == -1)
            break;

        RecBuffer attrBuffer(attrRecId.block);
        attrBuffer.getRecord(
            attrCatEntryRecord,
            attrRecId.slot
        );

        if (strcmp(
                attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,
                oldName) == 0)
        {
            attrToRenameRecId.block = attrRecId.block;
            attrToRenameRecId.slot = attrRecId.slot;
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

    RecBuffer attrToRenameBuffer(attrToRenameRecId.block);

    attrToRenameBuffer.getRecord(
        attrCatEntryRecord,
        attrToRenameRecId.slot
    );

    strcpy(
        attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,
        newName
    );

    attrToRenameBuffer.setRecord(
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
            break;

        prevBlockNum = blockNum;
        blockNum = head.rblock;
    }

    if (rec_id.block == -1 && rec_id.slot == -1)
    {
        if (relId == RELCAT_RELID)
            return E_MAXRELATIONS;

        RecBuffer newRecBuffer;
        int ret = newRecBuffer.getBlockNum();

        if (ret < 0)
            return ret;

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
            newSlotMap[i] = SLOT_UNOCCUPIED;

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


int BlockAccess::search(int relId, Attribute *record,
                        char attrName[ATTR_SIZE],
                        Attribute attrVal, int op)
{
    RecId recId = BlockAccess::linearSearch(
        relId,
        attrName,
        attrVal,
        op
    );

    if (recId.block == -1 && recId.slot == -1)
        return E_NOTFOUND;

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
        return E_RELNOTEXIST;

    Attribute relCatEntryRecord[RELCAT_NO_ATTRS];

    RecBuffer relCatBuffer(recId.block);
    relCatBuffer.getRecord(relCatEntryRecord, recId.slot);

    int firstBlock =
        relCatEntryRecord[RELCAT_FIRST_BLOCK_INDEX].nVal;

    int numberOfAttributes =
        relCatEntryRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal;

    // Release the relation's record blocks.
    int currBlock = firstBlock;

    while (currBlock != -1)
    {
        RecBuffer currRecBuffer(currBlock);

        HeadInfo head;
        currRecBuffer.getHeader(&head);

        int nextBlock = head.rblock;

        currRecBuffer.releaseBlock();

        currBlock = nextBlock;
    }

    // Remove the relation's attribute-catalog entries.
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

        if (attrCatRecId.block == -1 &&
            attrCatRecId.slot == -1)
        {
            break;
        }

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

        numberOfAttributesDeleted++;

        if (head.numEntries == 0)
        {
            // Remove the empty block from the attribute-catalog list.
            int leftBlock = head.lblock;
            int rightBlock = head.rblock;

            if (leftBlock != -1)
            {
                RecBuffer leftBuffer(leftBlock);

                HeadInfo leftHead;
                leftBuffer.getHeader(&leftHead);

                leftHead.rblock = rightBlock;
                leftBuffer.setHeader(&leftHead);
            }

            if (rightBlock != -1)
            {
                RecBuffer rightBuffer(rightBlock);

                HeadInfo rightHead;
                rightBuffer.getHeader(&rightHead);

                rightHead.lblock = leftBlock;
                rightBuffer.setHeader(&rightHead);
            }

            RelCatEntry attrRelCatEntry;
            RelCacheTable::getRelCatEntry(
                ATTRCAT_RELID,
                &attrRelCatEntry
            );

            if (leftBlock == -1)
                attrRelCatEntry.firstBlk = rightBlock;

            if (rightBlock == -1)
                attrRelCatEntry.lastBlk = leftBlock;

            RelCacheTable::setRelCatEntry(
                ATTRCAT_RELID,
                &attrRelCatEntry
            );

            attrBuffer.releaseBlock();
        }
    }

    // Mark the relation's entry in the relation catalog unoccupied.
    HeadInfo relCatHead;
    relCatBuffer.getHeader(&relCatHead);

    unsigned char relCatSlotMap[relCatHead.numSlots];
    relCatBuffer.getSlotMap(relCatSlotMap);

    relCatSlotMap[recId.slot] = SLOT_UNOCCUPIED;
    relCatBuffer.setSlotMap(relCatSlotMap);

    relCatHead.numEntries--;
    relCatBuffer.setHeader(&relCatHead);

    // Update the relation-catalog record count.
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(RELCAT_RELID, &relCatEntry);

    relCatEntry.numRecs--;
    RelCacheTable::setRelCatEntry(RELCAT_RELID, &relCatEntry);

    // Update the attribute-catalog record count.
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
            relId,
            &relCatEntry
        );

        if (ret != SUCCESS)
            return ret;

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
        return E_NOTFOUND;

    RecId nextRecId = {block, slot};

    RelCacheTable::setSearchIndex(relId, &nextRecId);

    RecBuffer recBuffer(nextRecId.block);
    recBuffer.getRecord(record, nextRecId.slot);

    return SUCCESS;
}