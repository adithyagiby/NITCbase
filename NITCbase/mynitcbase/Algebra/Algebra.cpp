#include "Algebra.h"

#include <cstring>
#include <cstdio>
#include <cstdlib>


bool isNumber(char *str);

/* used to select all the records that satisfy a condition.
the arguments of the function are
- srcRel - the source relation we want to select from
- targetRel - the relation we want to select into. (ignore for now)
- attr - the attribute that the condition is checking
- op - the operator of the condition
- strVal - the value that we want to compare against (represented as a string)
*/
int Algebra::select(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], char attr[ATTR_SIZE], int op, char strVal[ATTR_SIZE]) 
{
	int srcRelId = OpenRelTable::getRelId(srcRel);
	if (srcRelId == E_RELNOTOPEN)
	{
		return E_RELNOTOPEN;
	}

	//get attribute entry which we need to check condiiton on
	AttrCatEntry attrCatEntry;
	if(AttrCacheTable ::getAttrCatEntry(srcRelId, attr, &attrCatEntry) == E_ATTRNOTEXIST)
	{
		return E_ATTRNOTEXIST;
	}

	//check type, convert sVal to Number or String
	int type = attrCatEntry.attrType;
	
	//Attribute type can hold both Number String wout any issues
	Attribute attrVal;
	if (type == NUMBER)
	{
		if (isNumber(strVal)) //boolean user defined function
		{
			attrVal.nVal = atof(strVal);
		}
		else
		{
			return E_ATTRTYPEMISMATCH;
		}
	}
	else if(type == STRING)
	{
		strcpy(attrVal.sVal, strVal);
	}

	//selecting records from source relation

	//1) reset the search index to begins searching from the beginning
	int resresetSearchIdx = RelCacheTable :: resetSearchIndex(srcRelId);
	if (resresetSearchIdx != SUCCESS)
	{
		return resresetSearchIdx;
	}

	RelCatEntry relCatEntry;
	RelCacheTable :: getRelCatEntry(srcRelId, &relCatEntry);

	printf("|");
	for (int i=0; i<relCatEntry.numAttrs; i++)
	{
		AttrCatEntry attrCatEntry;
		AttrCacheTable :: getAttrCatEntry(srcRelId, i, &attrCatEntry);

		printf(" %s |", attrCatEntry.attrName);
	}
	printf("\n");

	while (true)
	{
		RecId  searchRes = BlockAccess :: linearSearch(srcRelId, attr, attrVal, op);

		if (searchRes.block != -1 && searchRes.slot != -1)
		{
			RecBuffer recBuffer(searchRes.block);
			Attribute record[relCatEntry.numAttrs];
			recBuffer.getRecord(record, searchRes.slot);

			printf("|");
			for (int i=0; i<relCatEntry.numAttrs; i++)
			{
				AttrCatEntry attrCatEntry;
				AttrCacheTable :: getAttrCatEntry(srcRelId, i, &attrCatEntry);

				if(attrCatEntry.attrType == NUMBER)
				{
                    printf("%d |", (int)record[i].nVal);
                }
                else
                {
                	 printf("%s |", record[i].sVal);
                }
			}
			printf("\n");
		}
		else
		{
			break;
		}
	}

	return SUCCESS;
}

//will return ig string can be parsed as floating point num
bool isNumber(char *str)
{
	int len;
	float ignore;

	//sscanf return number of elem reached, so if %f = 0 no floating math lese 1

	int ret = sscanf(str, "%f %n", &ignore, &len);
	return ret == 1 && len == strlen(str);
} 
