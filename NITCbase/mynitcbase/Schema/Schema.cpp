#include "Schema.h"

#include <cmath>
#include <cstring>

int Schema::openRel(char relName[ATTR_SIZE]){
	int ret=OpenRelTable::openRel(relName);
	// OpenRelTable::openRel() will send rel-id [0,12].
	 //If relation is not open, any error code will return negative integer.
	if (ret>=0){
		return SUCCESS;
	}
	return ret;
}

int Schema::closeRel(char relName[ATTR_SIZE]) {
  if (strcmp(relName,RELCAT_RELNAME)==0 || strcmp(relName,ATTRCAT_RELNAME)==0) {
    return E_NOTPERMITTED;
  }

  // this function returns the rel-id of a relation if it is open or
  // E_RELNOTOPEN if it is not. we will implement this later.
  int relId = OpenRelTable::getRelId(relName);
  if (relId==E_RELNOTOPEN){
  	return E_RELNOTOPEN;
  }

  return OpenRelTable::closeRel(relId);
}

int Schema::renameRel(char oldRelName[ATTR_SIZE],char newRelName[ATTR_SIZE]){
	// if the oldRelName or newRelName is either Relation Catalog or Attribute Catalog,
    if (strcmp(oldRelName,RELCAT_RELNAME)==0 || strcmp(newRelName,RELCAT_RELNAME)==0 || strcmp(oldRelName,ATTRCAT_RELNAME)==0 || strcmp(newRelName,ATTRCAT_RELNAME)==0){
    	return E_NOTPERMITTED;
    }

    // if the relation is open
    //    (check if OpenRelTable::getRelId() returns E_RELNOTOPEN)
    int relId=OpenRelTable::getRelId(oldRelName);
    if (relId>=0){
    	return E_RELOPEN;
    }

    int retVal=BlockAccess::renameRelation(oldRelName,newRelName);
    return retVal;
}


int Schema::renameAttr(char *relName, char *oldAttrName, char *newAttrName) {
    // if the relName is either Relation Catalog or Attribute Catalog,
    if (strcmp(relName,RELCAT_RELNAME)==0 || strcmp(relName,ATTRCAT_RELNAME)==0){
    	return E_NOTPERMITTED;
    }
    // if the relation is open
    int relId=OpenRelTable::getRelId(relName);
    if (relId>=0){
    	return E_RELOPEN;
    }

    // Call BlockAccess::renameAttribute with appropriate arguments.
	int retVal=BlockAccess::renameAttribute(relName,oldAttrName,newAttrName);
	return retVal;
    // return the value returned by the above renameAttribute() call
}
