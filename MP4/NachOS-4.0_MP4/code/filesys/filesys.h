// filesys.h
//	Data structures to represent the Nachos file system.
//
//	A file system is a set of files stored on disk, organized
//	into directories.  Operations on the file system have to
//	do with "naming" -- creating, opening, and deleting files,
//	given a textual file name.  Operations on an individual
//	"open" file (read, write, close) are to be found in the OpenFile
//	class (openfile.h).
//
//	We define two separate implementations of the file system.
//	The "STUB" version just re-defines the Nachos file system
//	operations as operations on the native UNIX file system on the machine
//	running the Nachos simulation.
//
//	The other version is a "real" file system, built on top of
//	a disk simulator.  The disk is simulated using the native UNIX
//	file system (in a file named "DISK").
//
//	In the "real" implementation, there are two key data structures used
//	in the file system.  There is a single "root" directory, listing
//	all of the files in the file system; unlike UNIX, the baseline
//	system does not provide a hierarchical directory structure.
//	In addition, there is a bitmap for allocating
//	disk sectors.  Both the root directory and the bitmap are themselves
//	stored as files in the Nachos file system -- this causes an interesting
//	bootstrap problem when the simulated disk is initialized.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#ifndef FS_H
#define FS_H

#include "copyright.h"
#include "sysdep.h"
#include "openfile.h"

typedef int OpenFileId;

#ifdef FILESYS_STUB // Temporarily implement file system calls as
// calls to UNIX, until the real file system
// implementation is available
class FileSystem
{
public:
	FileSystem()
	{
		for (int i = 0; i < 20; i++)
			fileDescriptorTable[i] = NULL;
	}

	bool Create(char *name)
	{
		int fileDescriptor = OpenForWrite(name);

		if (fileDescriptor == -1)
			return FALSE;
		Close(fileDescriptor);
		return TRUE;
	}

	OpenFile *Open(char *name)
	{
		int fileDescriptor = OpenForReadWrite(name, FALSE);

		if (fileDescriptor == -1)
			return NULL;
		return new OpenFile(fileDescriptor);
	}

	bool Remove(char *name) { return Unlink(name) == 0; }

	OpenFile *fileDescriptorTable[20];
};

#else // FILESYS
class FileSystem
{
public:
	FileSystem(bool format); // Initialize the file system.
							 // Must be called *after* "synchDisk"
							 // has been initialized.
							 // If "format", there is nothing on
							 // the disk, so initialize the directory
							 // and the bitmap of free blocks.
	// MP4 mod tag
	~FileSystem();

	bool Create(char *name, int initialSize);
	// Create a file (UNIX creat)
	// 109062233 MP4 Add begin
	bool CreateDirectory(char *name);
	
	int return_sector(char* name);

	// 109062233 MP4 Add end

	OpenFile *Open(char *name); // Open a file (UNIX open)

	bool Remove(char *name); // Delete a file (UNIX unlink)
	// modify 109062233 MP4
	void List(char* name , bool flag); // List all the files in the file system
	// add 109062233 MP4
	void recursiveList(char* name , int layer);
	void Print(); // List all the files and their contents


	// MP4 109062233 Begin adding 

	int OpenAFile(char* name);

	int Read(char* buffer,int  size, int id);
	
    int Write(char* buffer, int size, int id);

    int Close(int id);

	OpenFile* OpenDir(char* parent_path);
	// MP4 109062233 Begin end adding
	// 109062320 modify
	void SplitPath(char* fullpath, char* parent_dir, char* target_name); 
private:
	OpenFile *freeMapFile;	 // Bit map of free disk blocks,
							 // represented as a file
	OpenFile *directoryFile; // "Root" directory -- list of
							 // file names, represented as a file
	OpenFile* fileDescriptor; // MP4 109062233 , store opened table
	// Add notation from spec 109062233 
	/*Only at most one file will be opened at the same time . 
	Here, any OpenFileId larger than 0 is considered a successful open.
	(You do not need to maintain OpenFileTable in this assignment)
	*/
};

#endif // FILESYS

#endif // FS_H
