# OFS Project: Phase 1 Design Choices

## 1. User Management (Challenge 1)

**Requirement:** The system must support fast user lookup for `user_login` (ideally $O(log~n)$), iteration for `user_list`, and dynamic updates for `user_create`/`user_delete`.

**Structure Chosen:** Self-Implemented **AVL Tree** (`UserAVLTree`).

**Justification:**
* A `std::vector` with linear search would be $O(n)$, which is too slow and does not meet the project requirements.
* A simple Binary Search Tree (BST) has a worst-case lookup of $O(n)$ if users are inserted in alphabetical order (a degenerate tree).
* An **AVL Tree** is a self-balancing BST. It **guarantees $O(log~n)$** performance for all critical operations: `insert` (`user_create`), `delete` (`user_delete`), and `find` (`user_login`).
* Iteration for `user_list` is handled with an efficient $O(n)$ in-order traversal, which returns the users in sorted order.

## 2. File & Directory Indexing (Challenge 2)

**Requirement:** The system must represent the directory hierarchy in memory to allow for "fast path lookup" (e.g., `/reports/daily.txt`) and efficient directory listing (`dir_list`).

**Structure Chosen:** A General (N-ary) Tree, where each `FSTreeNode` represents a file or directory. To manage children, each directory node contains its own **self-implemented AVL Tree** (`ChildAVLTree`).

**Justification:**
* A tree is the natural structure for a hierarchical file system.
* Using a simple `std::vector` for children would make finding a child $O(n)$, slowing down path traversal.
* By using a `ChildAVLTree` inside each directory, finding a specific child (e.g., "reports" in "/") is a fast **$O(log~k)$** operation, where *k* is the number of files in that directory.
* This makes path traversal (e.g., `/reports/daily.txt`) very efficient, as each step of the path is an $O(log~k)$ lookup.

## 3. Fast File Lookup (Challenge 3)

**Requirement:** The system must quickly map a file path to its physical data block locations in the `.omni` file.

**Structure Chosen:** A **`std::vector<int>`** (named `data_blocks`) stored within each `FSTreeNode`.

**Justification:**
* This solution combines with Challenge 2. First, we use the `FileSystemTree` to find the correct `FSTreeNode` for a path, which is a fast, logarithmic operation.
* Once the file's node is found, the `data_blocks` vector on that node is accessed in $O(1)$ time.
* This vector holds the list of block indices (e.g., `[2, 3]`) that the file occupies, as determined by the `FreeSpaceBitmap` during `file_create`. The system then knows exactly which blocks to read from disk.

## 4. Free Space Management (Challenge 4)

**Requirement:** The system must track all free blocks, handle fragmentation, and quickly find `N` consecutive free blocks for `file_create`.

**Structure Chosen:** A **Bitmap** (`FreeSpaceBitmap`), implemented using a **`std::vector<bool>`**.

**Justification:**
* A `std::vector<bool>` is highly memory-efficient, as C++ optimizes it to use only one bit per block, minimizing in-memory overhead.
* It provides a simple, direct 1-to-1 mapping: `bitmap[i] == false` means block `i` is free.
* Finding `N` consecutive blocks is achieved with a single $O(n)$ scan over this vector (where *n* is the total number of blocks). This is fast, simple to implement, and directly fulfills the requirement.