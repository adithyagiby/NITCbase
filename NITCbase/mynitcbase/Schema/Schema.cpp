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
