#include "OpenRelTable.h"
#include<iostream>
#include <cstring>

OpenRelTableMetaInfo OpenRelTable::tableMetaInfo[MAX_OPEN];

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


    // // Setting up Student Catalog relation in the Relation Cache Table
    // relCatBlock.getRecord(relCatRecord, 2);
    // // set up the relation cache entry for the attribute catalog similarly
    // // from the record at RELCAT_SLOTNUM_FOR_ATTRCAT
    // RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
    // relCacheEntry.recId.block = RELCAT_BLOCK; //block number 4 (relational catalog block)
    // relCacheEntry.recId.slot = 2; // 1
    // RelCacheTable::relCache[2] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
    // *(RelCacheTable::relCache[2]) = relCacheEntry;



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
    // for(int i = 0; i<relCatRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal; i++){
        // attrCatBlock.getRecord(attrCatRecord,12+i);
        // struct AttrCacheEntry* attrCacheEntry = (struct AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
        // AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &attrCacheEntry->attrCatEntry);
        // attrCacheEntry->recId.block = ATTRCAT_BLOCK;
        // attrCacheEntry->recId.slot = i+12;
        // if(i == 0){
            // head = attrCacheEntry;
            // last = attrCacheEntry;
        // }
        // else{
            // last->next = attrCacheEntry;
            // last = attrCacheEntry;
        // }
    // }
    // last->next = nullptr;
    // AttrCacheTable::attrCache[2] = head;


    //Table Meta Info entry
    for (int i=0; i<MAX_OPEN; i++)
    {
    	if (i == RELCAT_RELID)
    	{	 
		    tableMetaInfo[i].free = false;
		    strcpy(tableMetaInfo[i].relName,RELCAT_RELNAME);
    	}
   		else if (i == ATTRCAT_RELID)
   		{
	   		tableMetaInfo[i].free = false;
	    	strcpy(tableMetaInfo[i].relName, ATTRCAT_RELNAME);
   		}
		else
		{
			tableMetaInfo[i].free = true;
		}
   
    
    

	}
}


OpenRelTable::~OpenRelTable(){

	for (int i=0; i<MAX_OPEN; i++)
	{
		if (!tableMetaInfo[i].free)
		{
			OpenRelTable::closeRel(i);
		}
	}
	
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

//Get Free Open Rel Table Entry
//return index of unoccupied entry in open relation table

int OpenRelTable::getFreeOpenRelTableEntry()
{
	for (int i=0; i<MAX_OPEN; i++)
	{
		if (tableMetaInfo[i].free)
		{
			return i;
		}
	}
	return E_CACHEFULL;
}



int OpenRelTable :: getRelId(char relName[ATTR_SIZE])
{

	for (int i=0 ; i<MAX_OPEN; i++)
	{
		if (tableMetaInfo[i].free == false && strcmp(tableMetaInfo[i].relName, relName) == 0)
		{
			return i;
		}
	}

	
	return E_RELNOTOPEN;
}


int OpenRelTable::openRel(char relName[ATTR_SIZE])
{
	int relId = getRelId(relName);
	if (relId != E_RELNOTOPEN)	//check first if relation is already opened
	{
		return relId;
	}

	//get free entry if somethings free
	relId = getFreeOpenRelTableEntry();
	if (relId < 0)
	{
		return E_CACHEFULL;
	}

	//setting up relation cache entry for the relation
	Attribute relationName;
	strcpy(relationName.sVal, relName);
	RelCacheTable::resetSearchIndex(RELCAT_RELID); //make it -1,-1 in the relation cache entry
	char relCatAttrRelName[ATTR_SIZE];
	strcpy(relCatAttrRelName, RELCAT_ATTR_RELNAME);

	//relCatrecid stores record id after linear search of relation catalog
	RecId relCatRecId = BlockAccess::linearSearch(RELCAT_RELID, relCatAttrRelName, relationName, EQ);

	if (relCatRecId.block == -1 && relCatRecId.slot == -1)
	{
		return E_RELNOTEXIST;
	}

	//reading record entry and making relCache Entry using getRecord and recordtoRelCatEntry
	RecBuffer relCatBlock(relCatRecId.block); //can also use relCat_RELID its the same block 4 only
	Attribute relCatRecord[RELCAT_NO_ATTRS];
	relCatBlock.getRecord(relCatRecord, relCatRecId.slot); //the first slot where conditions satisifed
	struct RelCacheEntry relCacheEntry;
	RelCacheTable :: recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
	relCacheEntry.recId = relCatRecId; 
	RelCacheTable::relCache[relId] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
    *(RelCacheTable::relCache[relId]) = relCacheEntry;

    //setting up attricbute cache entry
    //listhead hole head of linked list
    AttrCacheEntry *listHead, *current;
    //iterate over all entries in attribute catalog, and for eacj attr do a liner search
    RecId attrCatRecord;

    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
    for (int i=0; i<relCacheEntry.relCatEntry.numAttrs; i++)
    {
    	//let attrcatRecId store a valid record id an entry of the relation, relName,in the Attribute Catalog.

		RecId attrcatRecId = BlockAccess::linearSearch(ATTRCAT_RELID, relCatAttrRelName, relationName, EQ);
		//read the record entry and create attribut cache entry usinf recBuffer get record
		RecBuffer attrCatBlock(attrcatRecId.block);
		Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
		attrCatBlock.getRecord(attrCatRecord, attrcatRecId.slot);
		struct AttrCacheEntry* attrCacheEntry = (struct AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
		AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &attrCacheEntry->attrCatEntry);
		attrCacheEntry->recId = attrcatRecId;

		if(i==0){
	        listHead = attrCacheEntry;
	        current = attrCacheEntry;
	      }
	      else{
	        current->next = attrCacheEntry;
	        current = attrCacheEntry;
	      }
    }
    current->next = nullptr;

	// Store the linked list in the attribute cache
	AttrCacheTable::attrCache[relId] = listHead;

    //setting up metadata
    OpenRelTable::tableMetaInfo[relId].free = false;
    strcpy(OpenRelTable::tableMetaInfo[relId].relName, relName);

    return relId;
}


int OpenRelTable::closeRel(int relId)
{
	if (relId == RELCAT_RELID || relId == ATTRCAT_RELID)
	{
		return E_NOTPERMITTED;
	}

	if (relId < 0 || relId >= MAX_OPEN)
	{
		return E_OUTOFBOUND;		
	}

	if (tableMetaInfo[relId].free)
	{
		return E_RELNOTOPEN;
	}
    
    //releasing relation cache entry of relation

    //if the relcat entry is been modified
    if (RelCacheTable::relCache[relId]->dirty)
    {
        //get the relcat entry and convert to record
        Attribute relCatRecord[RELCAT_NO_ATTRS];
        RelCacheTable::relCatEntryToRecord(&RelCacheTable::relCache[relId]->relCatEntry, relCatRecord);

        //declare an object of recbuffer to write back
        RecBuffer relCatBlock(RelCacheTable::relCache[relId]->recId.block);

        //use set record and update slot
        relCatBlock.setRecord(relCatRecord, RelCacheTable::relCache[relId]->recId.slot);
    }

    //releasing attribute cache entry of relation
	free(RelCacheTable::relCache[relId]);
	AttrCacheEntry *entry, *temp;
	entry = AttrCacheTable::attrCache[relId];
	while (entry != nullptr)
	{
        if (entry->dirty)
        {
            Attribute record[ATTRCAT_NO_ATTRS];
            AttrCacheTable::attrCatEntryToRecord(&entry->attrCatEntry, record);
            RecBuffer attrCatBlock(entry->recId.block);
            attrCatBlock.setRecord(record, entry->recId.slot);
        }
		temp = entry;
		entry = entry->next;
		free(temp);
	}

	tableMetaInfo[relId].free = true;

	RelCacheTable::relCache[relId] = nullptr;
    AttrCacheTable::attrCache[relId] = nullptr;

    return SUCCESS;
}
	
	
