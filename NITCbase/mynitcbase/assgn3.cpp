#include "OpenRelTable.h"
#include<iostream>
#include <cstring>

OpenRelTable:: OpenRelTable() {
    // initialize relCache and attrCache with nullptr
    for(int i = 0; i< MAX_OPEN; i++){
        RelCacheTable::relCache[i] = nullptr;
        AttrCacheTable::attrCache[i] = nullptr;
    }


    /******* Setting up Relation Cache entries *********/
    // we need to populate relation cache with entries for the relation and attribute catalog.

    // Setting up Relation Catalog relation in the Relation Cache Table
    RecBuffer relCatBlock(RELCAT_BLOCK);
    Attribute relCatRecord[RELCAT_NO_ATTRS];

    relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_RELCAT);
    struct RelCacheEntry relCacheEntry;
    RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
    relCacheEntry.recId.block = RELCAT_BLOCK;
    relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_RELCAT; // 0
    // allocate this on the heap beacuse we want it to persist outside this function
    RelCacheTable::relCache[RELCAT_RELID] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
    *(RelCacheTable::relCache[RELCAT_RELID]) = relCacheEntry;



    // Setting up Attribute Catalog relation in the Relation Cache Table
    relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_ATTRCAT);
    // set up the relation cache entry for the attribute catalog similarly
    // from the record at RELCAT_SLOTNUM_FOR_ATTRCAT
    RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
    relCacheEntry.recId.block = RELCAT_BLOCK; //block number 4 (relational catalog block)
    relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_ATTRCAT; // 1
    RelCacheTable::relCache[ATTRCAT_RELID] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
    *(RelCacheTable::relCache[ATTRCAT_RELID]) = relCacheEntry;


    // Setting up Student Catalog relation in the Relation Cache Table
    relCatBlock.getRecord(relCatRecord, 2);
    // set up the relation cache entry for the attribute catalog similarly
    // from the record at RELCAT_SLOTNUM_FOR_ATTRCAT
    RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
    relCacheEntry.recId.block = RELCAT_BLOCK; //block number 4 (relational catalog block)
    relCacheEntry.recId.slot = 2; // 1
    RelCacheTable::relCache[2] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
    *(RelCacheTable::relCache[2]) = relCacheEntry;



    /******** Setting up Attribute Cache Entries *********/
    // Setting up Relation Catalog relation in the Attribute Cache Table
    RecBuffer attrCatBlock(ATTRCAT_BLOCK);
    Attribute attrCatRecord[ATTRCAT_NO_ATTRS];

    // iterate through all the attributes of the relation catalog and create a linked list
    // NOTE: allocate each entry dynamically using malloc
    struct AttrCacheEntry *head, *last;
    for(int i = 0; i<6; i++){
        attrCatBlock.getRecord(attrCatRecord,i);
        struct AttrCacheEntry* attrCacheEntry = (struct AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &attrCacheEntry->attrCatEntry);
        attrCacheEntry->recId.block = ATTRCAT_BLOCK;
        attrCacheEntry->recId.slot = i;
        if(i == 0){
            head = attrCacheEntry;
            last = attrCacheEntry;
        }
        else{
            last->next = attrCacheEntry;
            last = last->next;
        }
    }
    last->next = nullptr;
    AttrCacheTable::attrCache[RELCAT_RELID] = head;


    // Setting up Attribute Catalog relation in the Attribute Cache Table
    for(int i = 6; i<12; i++){
        attrCatBlock.getRecord(attrCatRecord,i);
        struct AttrCacheEntry* attrCacheEntry = (struct AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &attrCacheEntry->attrCatEntry);
        attrCacheEntry->recId.block = ATTRCAT_BLOCK;
        attrCacheEntry->recId.slot = i;
        if(i == 6){
            head = attrCacheEntry;
            last = attrCacheEntry;
        }
        else{
            last->next = attrCacheEntry;
            last = attrCacheEntry;
        }
    }
    last->next = nullptr;
    AttrCacheTable::attrCache[ATTRCAT_RELID] = head;



    // Setting up Student relation in the Attribute Cache Table
    for(int i = 12; i<18; i++){
        attrCatBlock.getRecord(attrCatRecord,i);
        struct AttrCacheEntry* attrCacheEntry = (struct AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &attrCacheEntry->attrCatEntry);
        attrCacheEntry->recId.block = ATTRCAT_BLOCK;
        attrCacheEntry->recId.slot = i;
        if(i == 12){
            head = attrCacheEntry;
            last = attrCacheEntry;
        }
        else{
            last->next = attrCacheEntry;
            last = attrCacheEntry;
        }
    }
    last->next = nullptr;
    AttrCacheTable::attrCache[2] = head;

}


OpenRelTable::~OpenRelTable(){
    // free all the memories that is allocted in the constructor
    for(int i = 0; i<MAX_OPEN; i++){
        if(RelCacheTable::relCache[i] != nullptr){
            free(RelCacheTable::relCache[i]);
            RelCacheTable::relCache[i] = nullptr;
        }
        if(AttrCacheTable::attrCache[i] != nullptr){
            struct AttrCacheEntry* attrCacheEntry = AttrCacheTable::attrCache[i];
            while(attrCacheEntry != nullptr){
                struct AttrCacheEntry* tempCacheEntry = attrCacheEntry;
                attrCacheEntry = attrCacheEntry->next;
                free(tempCacheEntry);
            }
            AttrCacheTable::attrCache[i] = nullptr;
        }
    }
}
