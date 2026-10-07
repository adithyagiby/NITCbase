#include "Schema.h"

#include <cmath>
#include <cstring>

int Schema::openRel(char relName[ATTR_SIZE])
{
	int ret = OpenRelTable::openRel(relName);
	//openRel returns relid if successfull else error, valid rel id will be bw 0 to 12

	if (ret >= 0)
	{
		return SUCCESS;
	}
	return ret;	
}

int Schema::closeRel(char relName[ATTR_SIZE])
{
	if (strcmp(relName, RELCAT_RELNAME) == 0 || strcmp(relName, ATTRCAT_RELNAME) == 0)
	{
		return E_NOTPERMITTED;
	}
	//returns relId if relation is open
	int relId = OpenRelTable::getRelId(relName);

	if (relId == E_RELNOTOPEN)
	{
		return E_RELNOTOPEN;
	}
	return OpenRelTable::closeRel(relId);
}

int Schema::renameRel(char oldRelName[ATTR_SIZE], char newRelName[ATTR_SIZE])
{
    // if the oldRelName or newRelName is either Relation Catalog or Attribute Catalog, return not permitted
	if (strcmp(oldRelName, RELCAT_RELNAME) == 0 || strcmp(newRelName, RELCAT_RELNAME) == 0 || strcmp(oldRelName,ATTRCAT_RELNAME)==0 || strcmp(newRelName, ATTRCAT_RELNAME)==0)
	{
		return E_NOTPERMITTED;	
	}

	int relId = OpenRelTable::getRelId(oldRelName);
	if (relId >= 0)
	{
		return E_RELOPEN;
	}

	int retVal = BlockAccess::renameRelation(oldRelName, newRelName);
	return retVal;
}


int Schema::renameAttr(char *relName, char *oldAttrName, char *newAttrName)
{
	if (strcmp(relName, RELCAT_RELNAME) == 0 ||  strcmp(relName,ATTRCAT_RELNAME)==0)
	{
		return E_NOTPERMITTED;	
	}
	int relId = OpenRelTable::getRelId(relName);
	if (relId >= 0)
	{
		return E_RELOPEN;
	}
	int retVal = BlockAccess::renameAttribute(relName, oldAttrName, newAttrName);
	return retVal;
	
	
}

int Schema::createRel(char relName[],int nAttrs, char attrs[][ATTR_SIZE],int attrtype[]){

    Attribute relNameAsAttribute;
    strcpy(relNameAsAttribute.sVal,relName);

    RecId targetRelId;

    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    char relCatAttrRelname[]=RELCAT_ATTR_RELNAME;
    targetRelId=BlockAccess::linearSearch(RELCAT_RELID,relCatAttrRelname,relNameAsAttribute,EQ);

    if (targetRelId.block!=-1 && targetRelId.slot!=-1){
    	return E_RELEXIST;
    }

    for (int i=0;i<nAttrs;i++){
    	for (int j=i+1;j<nAttrs;j++){
    		if (strcmp(attrs[i],attrs[j])==0){
    			return E_DUPLICATEATTR;
    		}
    	}
    }

    /* declare relCatRecord of type Attribute which will be used to store the
       record corresponding to the new relation which will be inserted
       into relation catalog */
    Attribute relCatRecord[RELCAT_NO_ATTRS];
    // fill relCatRecord fields as given below
    strcpy(relCatRecord[RELCAT_REL_NAME_INDEX].sVal,relName);
    relCatRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal=nAttrs;
    relCatRecord[RELCAT_NO_RECORDS_INDEX].nVal=0;
    relCatRecord[RELCAT_FIRST_BLOCK_INDEX].nVal=-1;
    relCatRecord[RELCAT_LAST_BLOCK_INDEX].nVal=-1;
    relCatRecord[RELCAT_NO_SLOTS_PER_BLOCK_INDEX].nVal=floor((2016/(16*nAttrs+1)));
    // (number of slots is calculated as specified in the physical layer docs)
	int ret=BlockAccess::insert(RELCAT_RELID,relCatRecord);
	if (ret!=SUCCESS){
		return ret;
	}

    for (int i=0;i<nAttrs;i++)
    {
        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        strcpy(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal,relName);
        strcpy(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,attrs[i]);
        attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal=attrtype[i];
        attrCatRecord[ATTRCAT_PRIMARY_FLAG_INDEX].nVal=-1;
        attrCatRecord[ATTRCAT_ROOT_BLOCK_INDEX].nVal=-1;
        attrCatRecord[ATTRCAT_OFFSET_INDEX].nVal=i;
        int retVal=BlockAccess::insert(ATTRCAT_RELID,attrCatRecord);
        if (retVal!=SUCCESS){
        	Schema::deleteRel(relName);
        	return E_DISKFULL;
        }
    }
    return SUCCESS;
}

int Schema::deleteRel(char *relName) {
    if (strcmp(relName,RELCAT_RELNAME)==0 || strcmp(relName,ATTRCAT_RELNAME)==0){
    	return E_NOTPERMITTED;
    }
    
    int relId=OpenRelTable::getRelId(relName);
	if (relId>=0 && relId<MAX_OPEN){
		return E_RELOPEN;
	}

    int ret=BlockAccess::deleteRelation(relName);
    return ret;
}
