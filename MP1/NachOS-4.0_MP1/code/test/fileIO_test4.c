#include "syscall.h"

int main(void)
{
	char test[] = "abcdefghijklmnopqrstuvwxyz";
	char store[26];
	int success= Create("file1.test");
	OpenFileId fid;
	int i,count;
	if (success != 1) MSG("Failed on creating file");
	fid = Open("file1.test");
	
	if (fid < 0) MSG("Failed on opening file");
	
	for (i = 0; i < 26; ++i) {
		int count = Write(test + i, 1, fid);
		if (count != 1) MSG("Failed on writing file");
	}
	// you should run fileIO_test1 first before running this one
    fid = Close("file1.test");
    fid = Open("file1.test");
	count = Read(store, 26, fid);
	if (count != 26) MSG("Failed on reading file");
	
	for (i = 0; i < 26; ++i) {
		if (test[i] != store[i]) MSG("Failed: reading wrong result");
	}

	success = Close(fid);
	if (success != 1) MSG("Failed on closing file");

	MSG("Passed , your implementation is very good ! ^_^");
	Halt();
}

