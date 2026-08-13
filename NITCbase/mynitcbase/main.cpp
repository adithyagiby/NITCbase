#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include <cstring>
#include <iostream>

int main(int argc, char* argv[])
{
	Disk disk_run;
	StaticBuffer buffer;
	OpenRelTable cache;

	return FrontendInterface::handleFrontend(argc, argv);
}

//insteaf of idx 0,1,2 you can use the global constant also, but too complicated other wise
