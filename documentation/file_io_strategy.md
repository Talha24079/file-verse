# File I/O Strategy

This document describes how the OFS server saves (serializes) its in-memory data structures into the single `.omni` file and loads (deserializes) them back on startup.

## 1. `.omni` File Structure

The `.omni` file is divided into five fixed-location sections for simple access:

1.  **[Header] (Block 0):** A 512-byte `OMNIHeader` struct.
2.  **[User Table]:** A fixed-size area immediately following the header. Its size is `max_users * sizeof(UserInfo)`.
3.  **[File System Tree]:** A fixed-size area immediately following the user table. Its size is `max_files * sizeof(FSTreeNode_Disk)`.
4.  **[Free Space Bitmap]:** A fixed-size area immediately following the file tree. Its size is rounded up to the nearest block.
5.  **[Data Blocks]:** The remaining space in the file, used for actual file content.

## 2. Serialization (Saving Data)

This process happens during `fs_shutdown`.

### User Table (UserAVLTree)
* **Strategy:** Convert the AVL Tree to a sorted array (using an in-order traversal).
* **Process:**
    1.  Call `userTree.listAllUsers()` to get a `std::vector<UserInfo>`.
    2.  Write this vector as a flat array directly into the **[User Table]** section of the file.

### File System Tree (FileSystemTree)
* **Strategy:** Save the tree to the file using a **Pre-Order Traversal**. We will store a simplified node, `FSTreeNode_Disk`, that uses an *index* to its first child instead of a pointer.
* **Process:**
    1.  We will write a recursive function that traverses the `FSTreeNode` tree.
    2.  As it visits each node, it writes that node's metadata to the **[File System Tree]** area.
    3.  This saves the tree's structure to disk sequentially.

### Free Space Bitmap
* **Strategy:** Save the `std::vector<bool>` directly.
* **Process:**
    1.  Get the raw data pointer from the `std::vector<bool>`.
    2.  Write the bytes of the bitmap directly into the **[Free Space Bitmap]** section.

## 3. Deserialization (Loading Data)

This process happens during `fs_init`.

### User Table (UserAVLTree)
* **Strategy:** Read the flat array and re-build the AVL tree.
* **Process:**
    1.  Seek to the **[User Table]** section.
    2.  Read the `UserInfo` structs one by one from the file.
    3.  For each user, call `userTree.insert()`. This will efficiently re-build a balanced AVL tree from the sorted data.

### File System Tree (FileSystemTree)
* **Strategy:** Read the nodes sequentially and reconstruct the tree in memory.
* **Process:**
    1.  Seek to the **[File System Tree]** section.
    2.  Read the `FSTreeNode_Disk` structs one by one.
    3.  Use the stored child/sibling indices to link the nodes together in memory, rebuilding the tree with `FSTreeNode` objects and their `ChildAVLTree`s.

### Free Space Bitmap
* **Strategy:** Read the raw bytes back into the `std::vector<bool>`.
* **Process:**
    1.  Seek to the **[Free Space Bitmap]** section.
    2.  Read the bytes from the file directly into the `bitmap.initialize()` function.