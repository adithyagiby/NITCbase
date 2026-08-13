#include "BlockAccess.h"

#include <cstring>

RecId BlockAccess::linearSearch(int relId, char attrName[ATTR_SIZE], union Attribute attrVal, int op)
{
	//get search index
	RecId prevRecId;
	RelCacheTable :: getSearchIndex(relId, &prevRecId);

	int block, slot;

	if (prevRecId.block == -1 && prevRecId.slot == -1)
	{
		//no search done prev, therefor start from beginning, use RelCatEntry
		RelCatEntry relCatEntry;
		RelCacheTable :: getRelCatEntry(relId, &relCatEntry);
		block = relCatEntry.firstBlk;
		slot = 0;
	}
	else
	{
		block = prevRecId.block;
		slot = prevRecId.slot+1;
	}

	//now begin linear search
	while (block != -1)
	{
		//creating a rec buffer object to get to access that block
		RecBuffer recBuffer(block);

		
		//get record with id, for that use recBuffer.getRecord
		//need header and slot map for this
		struct HeadInfo head;
		recBuffer.getHeader(&head);

		//slot map
		unsigned char slotMap[head.numSlots];
		recBuffer.getSlotMap(slotMap);
		
		//getting thr record
		Attribute record[head.numAttrs];
		recBuffer.getRecord(record, slot);

		if (slot >= head.numSlots)
		{
			block = head.rblock;
			slot = 0;
			continue;
		}

		//if slot is unoccupied dont bother with it
		if (slotMap[slot] == SLOT_UNOCCUPIED)
		{
			slot++;
			continue;
		}

		//compare record attribute value
		AttrCatEntry attrCatEntry;
		AttrCacheTable :: getAttrCatEntry(relId, attrName, &attrCatEntry);

		//the current attrbute val
		Attribute currentAttrVal = record[attrCatEntry.offset];

		int cmpVal = compareAttrs(currentAttrVal, attrVal, attrCatEntry.attrType);

		//if the cmpval and the op together works fine

		if (
            (op == NE && cmpVal != 0) ||    // if op is "not equal to"
            (op == LT && cmpVal < 0) ||     // if op is "less than"
            (op == LE && cmpVal <= 0) ||    // if op is "less than or equal to"
            (op == EQ && cmpVal == 0) ||    // if op is "equal to"
            (op == GT && cmpVal > 0) ||     // if op is "greater than"
            (op == GE && cmpVal >= 0)       // if op is "greater than or equal to"
        )
        {
        	RecId searchIndex = {block, slot};

        	RelCacheTable :: setSearchIndex(relId, &searchIndex);

        	return searchIndex;
        }

        slot++;
	}

	return RecId{-1,-1};
}
