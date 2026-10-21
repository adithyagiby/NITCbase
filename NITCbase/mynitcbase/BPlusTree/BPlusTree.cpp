#include "BPlusTree.h"

#include <cstring>



RecId BPlusTree::bPlusSearch(int relId, char attrName[ATTR_SIZE],
                             Attribute attrVal, int op) {

    // Get the search index for the attribute
    IndexId searchIndex;
    AttrCacheTable::getSearchIndex(relId, attrName, &searchIndex);

    // Get the attribute catalog entry
    AttrCatEntry attrCatEntry;
    AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);

    // Variables used during searching
    int block, index;

    if (searchIndex.block == -1 && searchIndex.index == -1) {

        // Search is being performed for the first time
        block = attrCatEntry.rootBlock;
        index = 0;

        // The attribute does not have a B+ tree
        if (block == -1) {
            return RecId{-1, -1};
        }

    } else {

        // Resume searching from the next entry
        block = searchIndex.block;
        index = searchIndex.index + 1;

        // Load the current leaf block
        IndLeaf leaf(block);

        HeadInfo leafHead;
        leaf.getHeader(&leafHead);

        // If the current leaf has been completely searched
        if (index >= leafHead.numEntries) {

            // Move to the next leaf block
            block = leafHead.rblock;
            index = 0;

            // No more leaf blocks
            if (block == -1) {
                return RecId{-1, -1};
            }
        }
    }

    /*
       Traverse internal nodes until a leaf node is reached.
    */

    while (StaticBuffer::getStaticBlockType(block) == IND_INTERNAL) {

        IndInternal internalBlk(block);

        HeadInfo intHead;
        internalBlk.getHeader(&intHead);

        InternalEntry intEntry;

        if (op == NE || op == LT || op == LE) {

            /*
               NE, LT and LE start searching from the
               leftmost leaf.
            */

            int ret = internalBlk.getEntry(&intEntry, 0);

            if (ret != SUCCESS) {
                return RecId{-1, -1};
            }

            block = intEntry.lChild;

        } else {

            /*
               EQ and GE:
               Find the first entry whose value >= attrVal.

               GT:
               Find the first entry whose value > attrVal.
            */

            bool found = false;

            for (int i = 0; i < intHead.numEntries; i++) {

                int ret = internalBlk.getEntry(&intEntry, i);

                if (ret != SUCCESS) {
                    return RecId{-1, -1};
                }

                int compareVal = compareAttrs(
                    intEntry.attrVal,
                    attrVal,
                    attrCatEntry.attrType
                );

                if ((op == EQ && compareVal >= 0) ||
                    (op == GE && compareVal >= 0) ||
                    (op == GT && compareVal > 0)) {

                    found = true;
                    break;
                }
            }

            if (found) {

                // Move to the left child of the matching entry
                block = intEntry.lChild;

            } else {

                /*
                   No matching separator was found.
                   Move to the right child of the last entry.
                */

                if (intHead.numEntries == 0) {
                    return RecId{-1, -1};
                }

                block = intEntry.rChild;
            }
        }

        // The next iteration examines the selected child block
    }

    /*
       Search the leaf index blocks for an entry satisfying
       the specified comparison operator.
    */

    while (block != -1) {

        IndLeaf leafBlk(block);

        HeadInfo leafHead;
        leafBlk.getHeader(&leafHead);

        Index leafEntry;

        while (index < leafHead.numEntries) {

            int ret = leafBlk.getEntry(&leafEntry, index);

            if (ret != SUCCESS) {
                return RecId{-1, -1};
            }

            int cmpVal = compareAttrs(
                leafEntry.attrVal,
                attrVal,
                attrCatEntry.attrType
            );

            // Check whether this entry satisfies the condition
            if ((op == EQ && cmpVal == 0) ||
                (op == LE && cmpVal <= 0) ||
                (op == LT && cmpVal < 0) ||
                (op == GT && cmpVal > 0) ||
                (op == GE && cmpVal >= 0) ||
                (op == NE && cmpVal != 0)) {

                // Save the search position
                searchIndex.block = block;
                searchIndex.index = index;

                AttrCacheTable::setSearchIndex(
                    relId, attrName, &searchIndex
                );

                // Return the record location
                return RecId{leafEntry.block, leafEntry.slot};
            }

            /*
               Since leaf values are sorted, if the current value
               exceeds attrVal, EQ, LT and LE cannot match later.
            */

            if ((op == EQ || op == LT || op == LE) &&
                cmpVal > 0) {

                return RecId{-1, -1};
            }

            // Examine the next entry
            index++;
        }

        /*
           NE may require searching subsequent leaf blocks.
           Other operators can stop after the current leaf.
        */

        if (op != NE) {
            break;
        }

        block = leafHead.rblock;
        index = 0;
    }

    // No matching record was found
    return RecId{-1, -1};
}