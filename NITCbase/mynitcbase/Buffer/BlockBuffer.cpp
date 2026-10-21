#include "BlockBuffer.h"

#include <cstdlib>
#include <cstring>

int comparisoncount=0;

BlockBuffer::BlockBuffer(char blockType)
{
    int intBlockType;

    if (blockType == 'R')
    {
        intBlockType = REC;
    }
    else if (blockType == 'I')
    {
        intBlockType = IND_INTERNAL;
    }
    else
    {
        intBlockType = IND_LEAF;
    }

    int ret = BlockBuffer::getFreeBlock(intBlockType);

    this->blockNum = ret;
}

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

	struct HeadInfo head;
	this->getHeader(&head);

	int attrCount = head.numAttrs;
	int slotCount = head.numSlots;

	if (slotNum < 0 || slotNum >= head.numSlots)
	{
	    return E_OUTOFBOUND;
	}
    

	unsigned char* bufferPtr;
	int ret = loadBlockAndGetBufferPtr(&bufferPtr);
	if (ret != SUCCESS)
	{
		return ret;
	}
	

	int recordSize = attrCount * ATTR_SIZE; //attrSize is 16
	int offset = HEADER_SIZE + head.numSlots + (recordSize * slotNum);
	unsigned char *slotPointer = bufferPtr + offset;
//
	memcpy(rec, slotPointer, recordSize);
//
	return SUCCESS;

}



//will not checkl if block is init or not, it will copy wtv content is there in diskblock to buffr
int BlockBuffer :: loadBlockAndGetBufferPtr(unsigned char **buffPtr)
{
	//check if the block is alr present in buffer, so we just return the num
	int bufferNum = StaticBuffer :: getBufferNum(this->blockNum);

	if (bufferNum != E_BLOCKNOTINBUFFER)
	{
		if (bufferNum == E_OUTOFBOUND)
		{
			return E_OUTOFBOUND;
		}
		for (int i=0 ; i<BUFFER_CAPACITY; i++)
		{
			if (i == bufferNum)
			{
				StaticBuffer::metainfo[i].timeStamp=0;
			}
			else
			{
				StaticBuffer::metainfo[i].timeStamp+=1;
			}
		}
	}
	else
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
	comparisoncount++;
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


int RecBuffer::setRecord(union Attribute *rec, int slotNum)
{
	unsigned char* bufferPtr;
	int res = BlockBuffer::loadBlockAndGetBufferPtr(&bufferPtr);

	if (res != SUCCESS)
	{
		return res;
	}


	//get header of the block
	struct HeadInfo head;
	this->getHeader(&head);

	//get number of attributes
	int attrCount = head.numAttrs;

	//get slot count
	int slotCount = head.numSlots;

	//if input slot num is out of range
	if (slotNum < 0 ||slotNum >= slotCount)
	{
		return E_OUTOFBOUND;
	}

	int recordSize = attrCount*16;
	unsigned char* slotPointer = bufferPtr + HEADER_SIZE + slotCount + slotNum * recordSize;
	memcpy(slotPointer, rec, recordSize);

	// update the dirty bit using setDirtyBit()
    int retVal = StaticBuffer::setDirtyBit(this->blockNum);

    if (retVal != SUCCESS)
    {
    	return retVal;
    }
    return SUCCESS;
}


//_____Stage-7_________________

int BlockBuffer::setHeader(struct HeadInfo *head)
{
	unsigned char *bufferPtr;
	int res = loadBlockAndGetBufferPtr(&bufferPtr);
	if (res != SUCCESS)
	{
		return res;
	}

	//cast buffer ptr to type headinfo*
	struct HeadInfo *bufferHeader = (struct HeadInfo*)bufferPtr;

	//copy field info of head to head
	bufferHeader->blockType = head->blockType;
	bufferHeader->lblock = head->lblock;
	bufferHeader->numAttrs = head->numAttrs;
	bufferHeader->numEntries = head->numEntries;
	bufferHeader->numSlots = head->numSlots;
	bufferHeader->pblock = head->pblock;
	bufferHeader->rblock = head->rblock;

	res = StaticBuffer::setDirtyBit(this->blockNum);
	return res;
}

int BlockBuffer::setBlockType(int blockType)
{
	unsigned char *bufferPtr;
	//getting starting adress of the buffer containing the block
	int res = loadBlockAndGetBufferPtr(&bufferPtr);
	if (res != SUCCESS)
	{
		return res;
	}

	*((int32_t*) bufferPtr) = blockType;

	//updating bmap to tell what type
	StaticBuffer::blockAllocMap[this->blockNum] = blockType;

	res = StaticBuffer::setDirtyBit(this->blockNum);
	return res;
}
int BlockBuffer::getFreeBlock(int blockType)
{
    int freeBlock = -1;

    for (int i = 0; i < DISK_BLOCKS; i++)
    {
        if (StaticBuffer::blockAllocMap[i] == UNUSED_BLK)
        {
            freeBlock = i;
            break;
        }
    }

    if (freeBlock == -1)
    {
        return E_DISKFULL;
    }

    this->blockNum = freeBlock;

    struct HeadInfo head;

    head.blockType = blockType;
    head.pblock = -1;
    head.rblock = -1;
    head.lblock = -1;
    head.numAttrs = 0;
    head.numEntries = 0;
    head.numSlots = 0;

    int ret = BlockBuffer::setHeader(&head);

    if (ret != SUCCESS)
    {
        return ret;
    }

    StaticBuffer::blockAllocMap[freeBlock] = blockType;

    ret = StaticBuffer::setDirtyBit(this->blockNum);

    if (ret != SUCCESS)
    {
        return ret;
    }

    return freeBlock;
}


RecBuffer::RecBuffer() : BlockBuffer('R'){}

int BlockBuffer::getBlockNum()
{
	return this->blockNum;
}


int RecBuffer::setSlotMap(unsigned char *slotMap)
{
	unsigned char *bufferPtr;
	//get starting address of the buffer continaitn the block
	int ret = loadBlockAndGetBufferPtr(&bufferPtr);
	if (ret != SUCCESS)
	{
		return ret;
	}

	//get headinfo using getHead
	struct HeadInfo head;
	this->getHeader(&head);

	int numSlots = head.numSlots;

	//slotmap starts at bufferPtr + headerSize
	//copy the content of slotmap to buffer replaxinv the current slotmap
	//size of slotmap is numSlots
	memcpy(bufferPtr+HEADER_SIZE, slotMap, numSlots);

	ret = StaticBuffer::setDirtyBit(this->blockNum);
	return ret;
}

void BlockBuffer::releaseBlock()
{
    if (this->blockNum != INVALID_BLOCKNUM)
    {
        int bufferNum = StaticBuffer::getBufferNum(this->blockNum);

        if (bufferNum != E_BLOCKNOTINBUFFER)
        {
            StaticBuffer::metainfo[bufferNum].free = true;
        }

        this->blockNum = INVALID_BLOCKNUM;
    }
}

IndBuffer::IndBuffer(char blockType) : BlockBuffer(blockType){}

IndBuffer::IndBuffer(int blockNum) : BlockBuffer(blockNum){}

// IndInternal() [constructor 1]
IndInternal::IndInternal() : IndBuffer('I'){}

// IndInternal(int blockNum) [constructor 2]
IndInternal::IndInternal(int blockNum) : IndBuffer(blockNum){}

IndLeaf::IndLeaf() : IndBuffer('L'){} 
IndLeaf::IndLeaf(int blockNum) : IndBuffer(blockNum){}

int IndInternal::getEntry(void *ptr, int indexNum){
    
    // check if indexNum is within range
    if(indexNum < 0 || indexNum >= MAX_KEYS_INTERNAL){
        return E_OUTOFBOUND;
    }

    unsigned char *bufferPtr;
    int res = BlockBuffer::loadBlockAndGetBufferPtr(&bufferPtr);
    if(res != SUCCESS){
        return res;
    }

    // typecaste the void pointer to an internal entry pointer
    struct InternalEntry *internalEntry = (struct InternalEntry *)ptr;

    // Copy the indexNum'th entry to the *internalEntry
    unsigned char *entryPtr = bufferPtr + HEADER_SIZE + (indexNum * 20);
    memcpy(&(internalEntry->lChild), entryPtr, sizeof(int32_t));
    memcpy(&(internalEntry->attrVal), entryPtr + 4, sizeof(Attribute));
    memcpy(&(internalEntry->rChild),entryPtr + 20, sizeof(int32_t));

    return SUCCESS;
}

int IndLeaf::getEntry(void *ptr, int indexNum){

    // check if indexNum is within range
    if(indexNum < 0 || indexNum >= MAX_KEYS_INTERNAL){
        return E_OUTOFBOUND;
    }

    unsigned char *bufferPtr;
    int res = BlockBuffer::loadBlockAndGetBufferPtr(&bufferPtr);
    if(res != SUCCESS){
        return res;
    }

    // copy the indexNum'th Index entry in buffer to memory ptr using memcpy
    unsigned char *entryPtr = bufferPtr + HEADER_SIZE + (indexNum * LEAF_ENTRY_SIZE);
    memcpy((struct Index *)ptr, entryPtr, LEAF_ENTRY_SIZE);

    return SUCCESS;
}

int IndInternal::setEntry(void *ptr, int indexNum) {
  if (indexNum<0 || indexNum>=MAX_KEYS_INTERNAL){
	return E_OUTOFBOUND;
  }
  unsigned char *bufferPtr;
  int ret=BlockBuffer::loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret!=SUCCESS){
	return ret;
  }
  struct InternalEntry *internalEntry=(struct InternalEntry *)ptr;
  unsigned char *entryPtr=bufferPtr+HEADER_SIZE+(indexNum*20);

  memcpy(entryPtr,&(internalEntry->lChild),4);
  memcpy(entryPtr+4,&(internalEntry->attrVal),ATTR_SIZE);
  memcpy(entryPtr+20,&(internalEntry->rChild),4);

  ret=StaticBuffer::setDirtyBit(this->blockNum);
  return ret;
}

int IndLeaf::setEntry(void *ptr, int indexNum) {
	if (indexNum<0 || indexNum>=MAX_KEYS_LEAF){
		return E_OUTOFBOUND;
	}
	unsigned char *bufferPtr;
	int ret=BlockBuffer::loadBlockAndGetBufferPtr(&bufferPtr);
	if (ret !=SUCCESS){
		return ret;
	}
	unsigned char *entryPtr=bufferPtr+HEADER_SIZE+(indexNum*LEAF_ENTRY_SIZE);
	memcpy(entryPtr,(struct Index *)ptr,LEAF_ENTRY_SIZE);

	ret=StaticBuffer::setDirtyBit(this->blockNum);
	return ret;
}