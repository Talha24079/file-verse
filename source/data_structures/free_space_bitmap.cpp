#include "free_space_bitmap.hpp"

void FreeSpaceBitmap::initialize(size_t num_blocks) 
{
    bitmap.resize(num_blocks, false); 
    total_blocks = num_blocks;
}

int FreeSpaceBitmap::findFreeBlocks(size_t num_blocks_needed) 
{
    size_t consecutive_free = 0;
    for (size_t i = 0; i < total_blocks; ++i) 
    {
        if (!bitmap[i]) 
        {
            consecutive_free++;
            if (consecutive_free == num_blocks_needed) 
            {
                return i - num_blocks_needed + 1; 
            }
        } 
        else 
        {
            consecutive_free = 0; 
        }
    }
    return -1; 
}

void FreeSpaceBitmap::setBlock(size_t block_index) 
{
    if (block_index < total_blocks) 
    {
        bitmap[block_index] = true;
    }
}


void FreeSpaceBitmap::setBlocks(size_t start_index, size_t num_blocks) 
{
    for (size_t i = 0; i < num_blocks; ++i) 
    {
        setBlock(start_index + i);
    }
}

void FreeSpaceBitmap::freeBlock(size_t block_index) 
{
    if (block_index < total_blocks) 
    {
        bitmap[block_index] = false;
    }
}

void FreeSpaceBitmap::freeBlocks(size_t start_index, size_t num_blocks) 
{
    for (size_t i = 0; i < num_blocks; ++i) 
    {
        freeBlock(start_index + i);
    }
}

bool FreeSpaceBitmap::isBlockSet(size_t block_index) const
{
    if (block_index < total_blocks)
    {
        return bitmap[block_index];
    }
    return false;
}