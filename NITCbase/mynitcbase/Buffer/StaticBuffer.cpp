#include "StaticBuffer.h"

// Buffer capacity = 32
// Block size = 2048

unsigned char StaticBuffer::blocks[BUFFER_CAPACITY][BLOCK_SIZE];
struct BufferMetaInfo StaticBuffer::metainfo[BUFFER_CAPACITY];

// Stage 7
unsigned char StaticBuffer::blockAllocMap[DISK_BLOCKS];

StaticBuffer::StaticBuffer()
{
    // Read the block allocation map from disk
    for (int i = 0; i < 4; i++)
    {
        Disk::readBlock(blockAllocMap + i * BLOCK_SIZE, i);
    }

    // Initialize buffer metadata
    for (int i = 0; i < BUFFER_CAPACITY; i++)
    {
        metainfo[i].free = true;
        metainfo[i].dirty = false;
        metainfo[i].timeStamp = -1;
        metainfo[i].blockNum = -1;
    }
}

StaticBuffer::~StaticBuffer()
{
    // Write the block allocation map back to disk
    for (int i = 0; i < 4; i++)
    {
        Disk::writeBlock(blockAllocMap + i * BLOCK_SIZE, i);
    }

    // Write dirty buffers back to disk
    for (int i = 0; i < BUFFER_CAPACITY; i++)
    {
        if (!metainfo[i].free && metainfo[i].dirty)
        {
            Disk::writeBlock(
                StaticBuffer::blocks[i],
                metainfo[i].blockNum
            );
        }
    }
}

int StaticBuffer::getFreeBuffer(int blockNum)
{
    // Validate the block number
    if (blockNum < 0 || blockNum >= DISK_BLOCKS)
    {
        return E_OUTOFBOUND;
    }

    int bufferNum = -1;
    int maxTimeStamp = -1;

    // Increase timestamps of occupied buffers
    for (int i = 0; i < BUFFER_CAPACITY; i++)
    {
        if (!metainfo[i].free)
        {
            metainfo[i].timeStamp++;
        }
    }

    // Find a free buffer
    for (int i = 0; i < BUFFER_CAPACITY; i++)
    {
        if (metainfo[i].free)
        {
            bufferNum = i;
            break;
        }
    }

    // If no free buffer exists, use LRU replacement
    if (bufferNum == -1)
    {
        for (int i = 0; i < BUFFER_CAPACITY; i++)
        {
            if (metainfo[i].timeStamp > maxTimeStamp)
            {
                bufferNum = i;
                maxTimeStamp = metainfo[i].timeStamp;
            }
        }

        // Write the replaced buffer to disk if dirty
        if (metainfo[bufferNum].dirty)
        {
            Disk::writeBlock(
                StaticBuffer::blocks[bufferNum],
                metainfo[bufferNum].blockNum
            );
        }
    }

    // Assign the buffer to the requested block
    metainfo[bufferNum].free = false;
    metainfo[bufferNum].dirty = false;
    metainfo[bufferNum].timeStamp = 0;
    metainfo[bufferNum].blockNum = blockNum;

    return bufferNum;
}

int StaticBuffer::getBufferNum(int blockNum)
{
    // Validate the block number
    if (blockNum < 0 || blockNum >= DISK_BLOCKS)
    {
        return E_OUTOFBOUND;
    }

    // Find the buffer containing the requested block
    for (int i = 0; i < BUFFER_CAPACITY; i++)
    {
        if (metainfo[i].blockNum == blockNum &&
            metainfo[i].free == false)
        {
            return i;
        }
    }

    return E_BLOCKNOTINBUFFER;
}

int StaticBuffer::setDirtyBit(int blockNum)
{
    int bufferIdx = getBufferNum(blockNum);

    if (bufferIdx == E_BLOCKNOTINBUFFER)
    {
        return E_BLOCKNOTINBUFFER;
    }

    if (bufferIdx == E_OUTOFBOUND)
    {
        return E_OUTOFBOUND;
    }

    metainfo[bufferIdx].dirty = true;

    return SUCCESS;
}

int StaticBuffer::getStaticBlockType(int blockNum)
{
    // Validate the block number
    if (blockNum < 0 || blockNum >= DISK_BLOCKS)
    {
        return E_OUTOFBOUND;
    }

    // Return the block type
    return (int)StaticBuffer::blockAllocMap[blockNum];
}