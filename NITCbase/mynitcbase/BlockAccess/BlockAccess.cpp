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

        // Check slot BEFORE accessing the record
        if (slot >= head.numSlots)
        {
            block = head.rblock;
            slot = 0;
            continue;
        }

        Attribute record[head.numAttrs];
        recBuffer.getRecord(record, slot);

        if (slotMap[slot] == SLOT_UNOCCUPIED)
        {
            slot++;
            continue;
        }

        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);

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
    // Reset relation catalog search index
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    // New relation name
    Attribute newRelationName;
    strcpy(newRelationName.sVal, newName);

    // FIX: allocate actual memory for the string
    char relCatAttrRelName[ATTR_SIZE];
    strcpy(relCatAttrRelName, RELCAT_ATTR_RELNAME);

    // Check whether new relation name already exists
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

    // Reset search index
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    // Old relation name
    Attribute oldRelationName;
    strcpy(oldRelationName.sVal, oldName);

    // Find old relation
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

    // Get relation catalog record
    Attribute record[RELCAT_NO_ATTRS];

    RecBuffer recBuffer(RELCAT_BLOCK);
    recBuffer.getRecord(record, recId.slot);

    // Change relation name
    strcpy(
        record[RELCAT_REL_NAME_INDEX].sVal,
        newName
    );

    recBuffer.setRecord(record, recId.slot);

    // Update Attribute Catalog
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    int numOfAttrs =
        record[RELCAT_NO_ATTRIBUTES_INDEX].nVal;

    // FIX: actual character array
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

        // Safety check
        if (rec_id.block == -1 && rec_id.slot == -1)
        {
            break;
        }

        Attribute attrRecord[ATTRCAT_NO_ATTRS];

        RecBuffer attrBuffer(rec_id.block);
        attrBuffer.getRecord(attrRecord, rec_id.slot);

        // Change relation name in attribute catalog
        strcpy(
            attrRecord[ATTRCAT_REL_NAME_INDEX].sVal,
            newName
        );

        attrBuffer.setRecord(attrRecord, rec_id.slot);
    }

    return SUCCESS;
}


int BlockAccess::renameAttribute(char relName[ATTR_SIZE],
                                 char oldName[ATTR_SIZE],
                                 char newName[ATTR_SIZE])
{
    // Reset relation catalog search index
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute relNameAttr;
    strcpy(relNameAttr.sVal, relName);

    // FIX: actual character array
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

    // Reset Attribute Catalog search index
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    RecId attrToRenameRecId;
    attrToRenameRecId.block = -1;
    attrToRenameRecId.slot = -1;

    Attribute attrCatEntryRecord[ATTRCAT_NO_ATTRS];

    // FIX: actual character array
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
        attrBuffer.getRecord(
            attrCatEntryRecord,
            rec_id.slot
        );

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

    RecBuffer attrToRenameBuff(
        attrToRenameRecId.block
    );

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
    //get relation catalog entry
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(relId, &relCatEntry);

    int blockNum = relCatEntry.firstBlk;

    //store where new record will be inserted
    RecId rec_id = {-1,-1};

    int numOfSlots = relCatEntry.numSlotsPerBlk;
    int numOfAttributes = relCatEntry.numAttrs;

    int prevBlockNum = -1;
    
    //traverse the linked list of existing record block until free slot
    // or until end of list is reached

    while (blockNum != -1)
    {
        //create recbuffer objec for block num
        RecBuffer recBuffer(blockNum);
        //get header of the block using getHeader()
        struct HeadInfo head;
        recBuffer.getHeader(&head);
        //get slot map of block
        unsigned char slotMap[head.numSlots];
        recBuffer.getSlotMap(slotMap);

        for (int i=0; i<head.numSlots; i++)
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
        //if relation if relation catalog, return max, cant fit more relations only
        if (relId == RELCAT_RELID)
        {
            return E_MAXRELATIONS;
        }
        
        //else make a new rec buffer and work on it
        RecBuffer newRecBuffer;
        int ret = newRecBuffer.getBlockNum();

        if (ret == E_DISKFULL)
        {
            return E_DISKFULL;
        }

        rec_id.block = ret;
        rec_id.slot = 0;
        //update the newHead
        struct HeadInfo newHead;
        newHead.blockType = REC;
        newHead.lblock = prevBlockNum;
        newHead.numAttrs = numOfAttributes;
        newHead.numEntries = 0;
        newHead.numSlots = numOfSlots;
        newHead.pblock = -1;
        newHead.rblock = -1;
        newRecBuffer.setHeader(&newHead);

        //make all the slots of the newBlock unoccupied
        unsigned char newSlotMap[numOfSlots];
        for (int i=0; i<numOfSlots; i++)
        {
            newSlotMap[i] = SLOT_UNOCCUPIED;
        }
        newRecBuffer.setSlotMap(newSlotMap);

        //update the prev block header, right block
        if (prevBlockNum != -1)
        {
            RecBuffer oldRecBuffer(prevBlockNum);
            struct HeadInfo prevHead;
            oldRecBuffer.getHeader(&prevHead);
            prevHead.rblock = rec_id.block;
            oldRecBuffer.setHeader(&prevHead);
        }
        else
        {
            //it is the first block of the relation and update the first block
            //in relcatentry to new block

            RelCatEntry relCatEntry;
            RelCacheTable::getRelCatEntry(relId, &relCatEntry);
            relCatEntry.firstBlk = rec_id.block;
            RelCacheTable::setRelCatEntry(relId, &relCatEntry);
        }

        //create rec buffer for current block, whicj we just got
        
    }
    RecBuffer currRec(rec_id.block);
    currRec.setRecord(record, rec_id.slot);

    //update slot map marking it occupied
    struct HeadInfo currHead;
    currRec.getHeader(&currHead);
    int currSlotNum = currHead.numSlots;
    unsigned char currSlotMap[currSlotNum];
    currRec.getSlotMap(currSlotMap);
    currSlotMap[rec_id.slot] = SLOT_OCCUPIED;
    currRec.setSlotMap(currSlotMap);

        //increment numEnteries in header of block
    currHead.numEntries += 1;
    currRec.setHeader(&currHead);

        //increment number of record field in relation cache for relation
    RelCacheTable::getRelCatEntry(relId, &relCatEntry);
    relCatEntry.numRecs += 1;
    RelCacheTable::setRelCatEntry(relId, &relCatEntry);
    return SUCCESS;
}
