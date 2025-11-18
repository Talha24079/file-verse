#pragma once
#include <vector>
#include <cstddef> 

using namespace std;

class FreeSpaceBitmap 
{
private:
    vector<bool> bitmap;
    size_t total_blocks;

public:
    FreeSpaceBitmap() : total_blocks(0) {}
    void initialize(size_t num_blocks);
    int findFreeBlocks(size_t num_blocks_needed);
    void setBlock(size_t block_index);
    void setBlocks(size_t start_index, size_t num_blocks);
    void freeBlock(size_t block_index);
    void freeBlocks(size_t start_index, size_t num_blocks);
    bool isBlockSet(size_t block_index) const;
    size_t size() const {
        return total_blocks;
    }
};