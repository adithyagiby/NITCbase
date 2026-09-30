#include "StaticBuffer.h"
//buffer capacity= 32
//block size = 2048
unsigned char StaticBuffer :: blocks[BUFFER_CAPACITY][BLOCK_SIZE];
struct BufferMetaInfo StaticBuffer :: metainfo[BUFFER_CAPACITY];

StaticBuffer :: StaticBuffer() //constructor
{	
	//initialise all the blocks free
	for (int i=0; i<BUFFER_CAPACITY; i++)
	{
		metainfo[i].free = true;
		metainfo[i].dirty=false;
		metainfo[i].timeStamp=-1;
		metainfo[i].blockNum=-1;
	}
}

StaticBuffer::~StaticBuffer(){
    /*
        Iterate through all the buffer block,
        write back blocks with metainfo as free = false, dirty = true using Disk::writeBlock()
    */
    for(int bufferIndex = 0; bufferIndex < BUFFER_CAPACITY; bufferIndex++){
        if(metainfo[bufferIndex].free == false && metainfo[bufferIndex].dirty == true){
            Disk::writeBlock(StaticBuffer:: blocks[bufferIndex], metainfo[bufferIndex].blockNum);
        }
    }
}	//deconstructor

int StaticBuffer::getFreeBuffer(int blockNum){
    // Check if blockNum is valid (non zero and less than DISK_BLOCKS)
    // and return E_OUTOFBOUND if not valid.
    if (blockNum<0 || blockNum>=DISK_BLOCKS){
    	return E_OUTOFBOUND;
    }
	int bufferNum=-1,bufferWithMaxTimeStamp=-1,maxTimeStamp=0;
    // increase the timeStamp in metaInfo of all occupied buffers.


    // iterate through metainfo and check if there is any buffer free

    // if a free buffer is available, set bufferNum = index of that free buffer.

    // if a free buffer is not available,
    //     find the buffer with the largest timestamp
    //     IF IT IS DIRTY, write back to the disk using Disk::writeBlock()
    //     set bufferNum = index of this buffer
    for (int bufferIndex=0;bufferIndex<BUFFER_CAPACITY;bufferIndex++){
    	if (metainfo[bufferIndex].free==false){
    		metainfo[bufferIndex].timeStamp+=1;

    		if (maxTimeStamp<metainfo[bufferIndex].timeStamp){
    			bufferWithMaxTimeStamp=bufferIndex;
    			maxTimeStamp=metainfo[bufferIndex].timeStamp;
    		}
    	}
    	if (metainfo[bufferIndex].free==true && bufferNum==-1){
    		bufferNum=bufferIndex;
    	}
    }
    if (bufferNum==-1){
    	if (metainfo[bufferWithMaxTimeStamp].dirty==true){
    		Disk::writeBlock(StaticBuffer::blocks[bufferWithMaxTimeStamp],metainfo[bufferWithMaxTimeStamp].blockNum);
    	}
    	bufferNum=bufferWithMaxTimeStamp;
    }
    // return the bufferNum.
    metainfo[bufferNum].free=false;
    metainfo[bufferNum].dirty=false;
    metainfo[bufferNum].blockNum=blockNum;
    metainfo[bufferNum].timeStamp=0;

    return bufferNum;
}

int StaticBuffer::getBufferNum(int blockNum)
{
	if (blockNum < 0 || blockNum > DISK_BLOCKS)
	{
		return E_OUTOFBOUND;
	}

	for (int i=0; i<BUFFER_CAPACITY; i++)
	{
		if (metainfo[i].blockNum == blockNum and metainfo[i].free == false)
		{
			return i;
		}
	}
	return E_BLOCKNOTINBUFFER;


}

int StaticBuffer::setDirtyBit(int blockNum){
	int bufferNum=StaticBuffer::getBufferNum(blockNum);
	if (bufferNum==E_BLOCKNOTINBUFFER){
		return E_BLOCKNOTINBUFFER;
	}
	if (bufferNum==E_OUTOFBOUND){
		return E_OUTOFBOUND;
	}
	metainfo[bufferNum].dirty=true;
	return SUCCESS;
}
