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
	}
}

StaticBuffer :: ~StaticBuffer(){}	//deconstructor

int StaticBuffer::getFreeBuffer(int blockNum)
{
	if (blockNum < 0 || blockNum > DISK_BLOCKS) //if its a valid block num
	{
		return E_OUTOFBOUND;
	}

	int allocatedBuffer;

	for (int i=0; i<BUFFER_CAPACITY; i++)
	{
		if (metainfo[i].free)
		{
			allocatedBuffer = i;
			break;
		}
	}
	metainfo[allocatedBuffer].free = false;
	metainfo[allocatedBuffer].blockNum = blockNum;

	return allocatedBuffer;
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
