#include "BlockBuffer.h"

#include <cstdlib>
#include <cstring>

BlockBuffer::BlockBuffer(int blockNum)
{
	this->blockNum = blockNum;	 // initialise this.blockNum with the argument
}

//calls the parent class constructor
RecBuffer :: RecBuffer(int blockNum) : BlockBuffer :: BlockBuffer(blockNum){}

//load the block header into argument pointer
int BlockBuffer::getHeader(struct HeadInfo* head) //tells its block buffer class
{

	unsigned char *bufferPtr;
	int ret = loadBlockAndGetBufferPtr(&bufferPtr); //instead of disk::getheader
	if (ret != SUCCESS)
	{
		return ret;
	}
	
	// // populate the numEntries, numAttrs and numSlots fields in *head
    memcpy(&head->numSlots, bufferPtr + 24, 4);
    memcpy(&head->numEntries, bufferPtr + 16, 4);
    memcpy(&head->numAttrs, bufferPtr + 20, 4);
    memcpy(&head->rblock, bufferPtr + 12, 4);
    memcpy(&head->lblock, bufferPtr + 8, 4);
//
    return SUCCESS;
}


// load the record at slotNum into the argument pointer, basically slotNum in rec
int RecBuffer :: getRecord(union Attribute *rec, int slotNum)
{

	unsigned char* bufferPtr;
	int ret = loadBlockAndGetBufferPtr(&bufferPtr);
	if (ret != SUCCESS)
	{
		return ret;
	}
	
	struct HeadInfo head;
	this->getHeader(&head);

	int attrCount = head.numAttrs;
	int slotCount = head.numSlots;
    
	int recordSize = attrCount * ATTR_SIZE; //attrSize is 16
	int offset = HEADER_SIZE + head.numSlots + (recordSize * slotNum);
	unsigned char *slotPointer = bufferPtr + offset;
//
	memcpy(rec, slotPointer, recordSize);
//
	return SUCCESS;

}

int BlockBuffer :: loadBlockAndGetBufferPtr(unsigned char **buffPtr)
{
	//check if the block is alr present in buffer, so we just return the num
	int bufferNum = StaticBuffer :: getBufferNum(this->blockNum);
	if (bufferNum == E_BLOCKNOTINBUFFER) //if not present, we assign one
	{
		bufferNum = StaticBuffer :: getFreeBuffer(this->blockNum);
		if (bufferNum == E_OUTOFBOUND)
		{
			return E_OUTOFBOUND;
		}
		Disk :: readBlock(StaticBuffer :: blocks[bufferNum], this->blockNum);
	}
	*buffPtr = StaticBuffer::blocks[bufferNum];

	return SUCCESS;
}


/* used to get the slotmap from a record block
NOTE: this function expects the caller to allocate memory for `*slotMap`
*/


int RecBuffer::getSlotMap(unsigned char *slotMap)
{
	unsigned char *bufferPtr;

	//get starting address
	int ret = loadBlockAndGetBufferPtr(&bufferPtr);
	if (ret != SUCCESS)
	{
		return ret;
	}
	
	struct HeadInfo head;
	//get header of the block
	this->getHeader(&head);

	//get number of slots
	int slotCount = head.numSlots;

	//bufferPtr is beginning address, HEADER_SIZE is 32 which is basically the header, 
	unsigned char *slotMapInBuffer = bufferPtr + HEADER_SIZE;

	//gets slot count
	memcpy(slotMap, slotMapInBuffer, slotCount);

	return SUCCESS;
}


int compareAttrs(union Attribute attr1, union Attribute attr2, int attrType)
{
	double diff;

	if (attrType == STRING)
	{
		diff = strcmp(attr1.sVal, attr2.sVal);
	}
	else
	{
		diff = attr1.nVal - attr2.nVal;
	}

	if (diff > 0)
	{
		return 1;
	}
	if (diff < 0)
	{
		return -1;
	}
	return 0;
}
