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
        
        if (ret < 0)
        {
            return ret;
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
            HeadInfo prevHead;
            oldRecBuffer.getHeader(&prevHead);
            prevHead.rblock = rec_id.block;
            oldRecBuffer.setHeader(&prevHead);
        
            RelCatEntry relCatEntry;
            RelCacheTable::getRelCatEntry(relId, &relCatEntry);
            relCatEntry.lastBlk = rec_id.block;
            RelCacheTable::setRelCatEntry(relId, &relCatEntry);
        }
        else
        {
            RelCatEntry relCatEntry;
            RelCacheTable::getRelCatEntry(relId, &relCatEntry);
        
            relCatEntry.firstBlk = rec_id.block;
            relCatEntry.lastBlk = rec_id.block;
        
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


int BlockAccess::search(int relId, Attribute *record, char attrName[ATTR_SIZE], Attribute attrVal, int op) {
    // Declare a variable called recid to store the searched record
    RecId recId;

    /* search for the record id (recid) corresponding to the attribute with
    attribute name attrName, with value attrval and satisfying the condition op
    using linearSearch() */
	recId=BlockAccess::linearSearch(relId,attrName,attrVal,op);
    // if there's no record satisfying the given condition (recId = {-1, -1})
    //    return E_NOTFOUND;
    if (recId.block==-1 && recId.slot==-1){
    	return E_NOTFOUND;
    }
	
    /* Copy the record with record id (recId) to the record buffer (record)
       For this Instantiate a RecBuffer class object using recId and
       call the appropriate method to fetch the record
    */
    RecBuffer recBuffer(recId.block);
    recBuffer.getRecord(record,recId.slot);

    return SUCCESS;
}

int BlockAccess::deleteRelation(char relName[ATTR_SIZE]){
    // if the relation to delete is either Relation Catalog or Attribute Catalog, return E_NOTPERMITTED
    if(strcmp(relName, RELCAT_RELNAME) == 0 || strcmp(relName, ATTRCAT_RELNAME) == 0){
        return E_NOTPERMITTED;
    }

    // reset the searchIndex of the relation catalog
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute relNameAttr;
    strcpy(relNameAttr.sVal, relName);

    char *relCatAttrRelname;
    strcpy(relCatAttrRelname,RELCAT_ATTR_RELNAME);

    // linearSearch on the relation catalog for RelName = relName
    RecId recId = BlockAccess::linearSearch(RELCAT_RELID, relCatAttrRelname, relNameAttr,EQ);
    if(recId.block == -1 && recId.slot == -1){
        return E_RELNOTEXIST;
    }

    /* Store the relation catalog */
    Attribute relCatEntryRecord[RELCAT_NO_ATTRS];
    RecBuffer recBuffer(recId.block);
    recBuffer.getRecord(relCatEntryRecord,recId.slot);

    /* get the first record block and number of attributes of the relation */
    int firstBlock = relCatEntryRecord[RELCAT_FIRST_BLOCK_INDEX].nVal;  
    int numAttrs = relCatEntryRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal;

    /***  Delete all the record blocks of the relation ***/
    int currBlock = firstBlock;
    while(currBlock != -1){
        // get the block header
        // get the next block from header
        // release the current block

        struct HeadInfo head;
        RecBuffer currRecBuffer(currBlock);
        currRecBuffer.getHeader(&head);
        currBlock = head.rblock;
        currRecBuffer.releaseBlock();
    }

    /*** 
        Deleting attributes catalog entries corresponding the relation and
        index block corresponding to the relation with relName on its attributes
    ***/
    // reset searchIndex of the attribute catalog
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    int numberOfAttributesDeleted = 0;

    // NOTE: ATTRCAT_ATTR_RELNAME = RELCAT_ATTR_RELNAME = Relname
    
    while(true){
        RecId attrCatRecId = BlockAccess::linearSearch(ATTRCAT_RELID, relCatAttrRelname, relNameAttr, EQ);
        if(attrCatRecId.block == -1 && attrCatRecId.slot == -1){
            break;
        }

        numberOfAttributesDeleted++;

        // create a RecBuffer for the attrCatRecId.block and get header and record at attrCatRecId.slot
        RecBuffer recBuffer(attrCatRecId.block);
        struct HeadInfo head;
        recBuffer.getHeader(&head);
        Attribute record[head.numAttrs];
        recBuffer.getRecord(record, attrCatRecId.slot);
        
        // declare the rootBlock which will be used to store the root block field from the attribute catalog record.
        // int rootBlock = /* get root block from the record */

        // update the SlotMap for the block by setting the slot as SLOT_UNOCCUPIED
        unsigned char slotMap[head.numSlots];
        recBuffer.getSlotMap(slotMap);
        slotMap[attrCatRecId.slot] = SLOT_UNOCCUPIED;
        recBuffer.setSlotMap(slotMap);

        /*
            Decrement the numEntries in the header of the block corresponding to
            the attribute catalog entry and then set back the header.
        */
        head.numEntries--;
        recBuffer.setHeader(&head);

        /*
            If number of entries become 0, releaseBlock is called after fixing
            the linkedList
        */
        if(head.numEntries == 0){
            /*
                Standard Linked List Delete for a block:
                    Get header of left block and sets its rblock to rblock of
                    current block
            */

            RecBuffer leftRecBuffer(head.lblock);
            struct HeadInfo leftHead;
            leftRecBuffer.getHeader(&leftHead);
            leftHead.rblock = head.rblock;
            leftRecBuffer.setHeader(&leftHead);

            if(head.rblock != -1){
                /*
                    Get the header of the right blocl and set it's lblock to
                    this block's lbock
                */
                RecBuffer rightRecBuffer(head.rblock);
                struct HeadInfo rightHead;
                rightRecBuffer.getHeader(&rightHead);
                rightHead.lblock = head.lblock;
                rightRecBuffer.setHeader(&rightHead);
            }
            else{
                // so this is the last block being released of the relation
                /*
                    Update the Relation Catalog entry'a LastBlock field for this
                    relation (Attribute catalog) with the block number of the previous block
                 */
                 RelCatEntry relCatEntry;
                 RelCacheTable::getRelCatEntry(ATTRCAT_RELID, &relCatEntry);
                 relCatEntry.lastBlk = head.lblock;
                 RelCacheTable::setRelCatEntry(ATTRCAT_RELID, &relCatEntry);
            }
            
            // call releaseBlock()
            recBuffer.releaseBlock();
        }

        // next part is for later stages.
          
    }

    /*** Delete the entry corresponding to the relation from relation catalog ***/

    // Fetch header of the relcat block
    struct HeadInfo head;
    recBuffer.getHeader(&head);

    // decrement the numEntries in the header and set it back
    head.numEntries--;
    recBuffer.setHeader(&head);

    // get the slotmap of relation catalog and mark the corresponding slot unoccupied
    unsigned char slotMap[head.numSlots];
    recBuffer.getSlotMap(slotMap);
    slotMap[recId.slot] = SLOT_UNOCCUPIED;
    recBuffer.setSlotMap(slotMap);

    /*** Updating the Relation Cache Table ***/

    // update relation catalog entry [number of record decreased by 1]
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(RELCAT_RELID, &relCatEntry);
    relCatEntry.numRecs--;
    RelCacheTable::setRelCatEntry(RELCAT_RELID, &relCatEntry);

    // update attribute catalog entry [num of records decreased by numberOfDeletedAttr]
    RelCatEntry attrCatEntry;
    RelCacheTable::getRelCatEntry(ATTRCAT_RELID, &attrCatEntry);
    attrCatEntry.numRecs -= numberOfAttributesDeleted;
    RelCacheTable::setRelCatEntry(ATTRCAT_RELID, &attrCatEntry);

    return SUCCESS;
}
