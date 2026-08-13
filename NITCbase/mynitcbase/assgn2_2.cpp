#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include <cstring>
#include <iostream>

int main(int argc, char* argv[])
{
	Disk disk_run;
	// Rec buffer takes input block num if block alr allocated
	//declaring object of RecBuffer class, for the specific vlbovl
	RecBuffer relCatBuffer(RELCAT_BLOCK); //relcat_block is basically block num 4
	RecBuffer attrCatBuffer(ATTRCAT_BLOCK); //attrcat_block is bl
	
	
	HeadInfo relCatHeader;
	HeadInfo attrCatHeader;
	HeadInfo attrCatHeader2; //for the Rblock
		
	//setHeader and getHeader take pointer HeadInfo as argument
	// HeadInfo [BlockType, pblock, lblock, rblock, #entries, #attr, #slot, reserved[4]]

	relCatBuffer.getHeader(&relCatHeader);
	attrCatBuffer.getHeader(&attrCatHeader);


	RecBuffer attrCatBuffer2(attrCatHeader.rblock);
	attrCatBuffer2.getHeader(&attrCatHeader2);

	
	
	//1st loop to print block 1
	for (int i=0; i<relCatHeader.numEntries; i++)
	{
		Attribute relCatRecord[RELCAT_NO_ATTRS]; // Number of attributes present in one entry / record of the Relation Catalog

		relCatBuffer.getRecord(relCatRecord, i);

		printf("Relation: %s\n", relCatRecord[0].sVal); //idx 0 stands for relName in RC

		for (int j=0; j<attrCatHeader.numEntries; j++)
		{
			Attribute attrCatRecord[ATTRCAT_NO_ATTRS]; //Number of attributes in one entry still 6
			attrCatBuffer.getRecord(attrCatRecord, j);


			if (strcmp(attrCatRecord[0].sVal, "Students") == 0)
			{
				if (strcmp(attrCatRecord[1].sVal, "Class") == 0)
				{
					char unsigned buffer[BLOCK_SIZE];
					Disk :: readBlock(buffer, ATTRCAT_BLOCK);
					int recordSize = ATTR_SIZE * ATTRCAT_NO_ATTRS; //16*6
					int  offset = HEADER_SIZE + attrCatHeader.numSlots + recordSize*j + ATTRCAT_ATTR_NAME_INDEX*ATTR_SIZE;
					unsigned char* newAttrName = buffer + offset;
					memcpy(newAttrName,"BATCH", ATTR_SIZE);
					Disk :: writeBlock(buffer, ATTRCAT_BLOCK);
					
				}
			}

			if (strcmp(attrCatRecord[0].sVal,relCatRecord[0].sVal) == 0) //works only if the names are same
			{		
				const char *attrType = attrCatRecord[2].nVal == NUMBER ? "NUM" : "STR";
				printf(" %s: %s\n", attrCatRecord[1].sVal, attrType);
			}
			
		}

		for (int j=0; j<attrCatHeader2.numEntries; j++)
		{
			Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
			attrCatBuffer2.getRecord(attrCatRecord, j);
			if (strcmp(attrCatRecord[0].sVal,relCatRecord[0].sVal) == 0) //works only if the names are same
			{
				const char *attrType = attrCatRecord[2].nVal == NUMBER ? "NUM" : "STR";
				printf(" %s: %s\n", attrCatRecord[1].sVal, attrType);
			}
		}
		
		printf("\n");
		
	}

	//2nd loop to print 2nd block
	

	
	return 0;
	
}

//insteaf of idx 0,1,2 you can use the global constant also, but too complicated other wise
