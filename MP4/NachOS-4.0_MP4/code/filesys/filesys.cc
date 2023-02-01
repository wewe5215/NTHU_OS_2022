// filesys.cc
//	Routines to manage the overall operation of the file system.
//	Implements routines to map from textual file names to files.
//
//	Each file in the file system has:
//	   A file header, stored in a sector on disk
//		(the size of the file header data structure is arranged
//		to be precisely the size of 1 disk sector)
//	   A number of data blocks
//	   An entry in the file system directory
//
// 	The file system consists of several data structures:
//	   A bitmap of free disk sectors (cf. bitmap.h)
//	   A directory of file names and file headers
//
//      Both the bitmap and the directory are represented as normal
//	files.  Their file headers are located in specific sectors
//	(sector 0 and sector 1), so that the file system can find them
//	on bootup.
//
//	The file system assumes that the bitmap and directory files are
//	kept "open" continuously while Nachos is running.
//
//	For those operations (such as Create, Remove) that modify the
//	directory and/or bitmap, if the operation succeeds, the changes
//	are written immediately back to disk (the two files are kept
//	open during all this time).  If the operation fails, and we have
//	modified part of the directory and/or bitmap, we simply discard
//	the changed version, without writing it back to disk.
//
// 	Our implementation at this point has the following restrictions:
//
//	   there is no synchronization for concurrent accesses
//	   files have a fixed size, set when the file is created
//	   files cannot be bigger than about 3KB in size
//	   there is no hierarchical directory structure, and only a limited
//	     number of files can be added to the system
//	   there is no attempt to make the system robust to failures
//	    (if Nachos exits in the middle of an operation that modifies
//	    the file system, it may corrupt the disk)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.
#ifndef FILESYS_STUB

#include "copyright.h"
#include "debug.h"
#include "disk.h"
#include "pbitmap.h"
#include "directory.h"
#include "filehdr.h"
#include "filesys.h"

// Sectors containing the file headers for the bitmap of free sectors,
// and the directory of files.  These file headers are placed in well-known
// sectors, so that they can be located on boot-up.
#define FreeMapSector 0
#define DirectorySector 1

// Initial file sizes for the bitmap and directory; until the file system
// supports extensible files, the directory size sets the maximum number
// of files that can be loaded onto the disk.
#define FreeMapFileSize (NumSectors / BitsInByte)
#define NumDirEntries 64 // 64 files/subdirectories per directory MP4 109062233
#define DirectoryFileSize (sizeof(DirectoryEntry) * NumDirEntries)

//----------------------------------------------------------------------
// FileSystem::FileSystem
// 	Initialize the file system.  If format = TRUE, the disk has
//	nothing on it, and we need to initialize the disk to contain
//	an empty directory, and a bitmap of free sectors (with almost but
//	not all of the sectors marked as free).
//
//	If format = FALSE, we just have to open the files
//	representing the bitmap and the directory.
//
//	"format" -- should we initialize the disk?
//----------------------------------------------------------------------

FileSystem::FileSystem(bool format)
{
    DEBUG(dbgFile, "Initializing the file system.");
    if (format)
    {
        PersistentBitmap *freeMap = new PersistentBitmap(NumSectors);
        Directory *directory = new Directory(NumDirEntries);
        FileHeader *mapHdr = new FileHeader;
        FileHeader *dirHdr = new FileHeader;

        DEBUG(dbgFile, "Formatting the file system.");

        // First, allocate space for FileHeaders for the directory and bitmap
        // (make sure no one else grabs these!)
        freeMap->Mark(FreeMapSector);
        freeMap->Mark(DirectorySector);

        // Second, allocate space for the data blocks containing the contents
        // of the directory and bitmap files.  There better be enough space!

        ASSERT(mapHdr->Allocate(freeMap, FreeMapFileSize));
        ASSERT(dirHdr->Allocate(freeMap, DirectoryFileSize));

        // Flush the bitmap and directory FileHeaders back to disk
        // We need to do this before we can "Open" the file, since open
        // reads the file header off of disk (and currently the disk has garbage
        // on it!).

        DEBUG(dbgFile, "Writing headers back to disk.");
        mapHdr->WriteBack(FreeMapSector);
        dirHdr->WriteBack(DirectorySector);

        // OK to open the bitmap and directory files now
        // The file system operations assume these two files are left open
        // while Nachos is running.

        freeMapFile = new OpenFile(FreeMapSector);
        directoryFile = new OpenFile(DirectorySector);

        // Once we have the files "open", we can write the initial version
        // of each file back to disk.  The directory at this point is completely
        // empty; but the bitmap has been changed to reflect the fact that
        // sectors on the disk have been allocated for the file headers and
        // to hold the file data for the directory and bitmap.

        DEBUG(dbgFile, "Writing bitmap and directory back to disk.");
        freeMap->WriteBack(freeMapFile); // flush changes to disk
        directory->WriteBack(directoryFile);

        if (debug->IsEnabled('f'))
        {
            freeMap->Print();
            directory->Print();
        }
        delete freeMap;
        delete directory;
        delete mapHdr;
        delete dirHdr;
    }
    else
    {
        // if we are not formatting the disk, just open the files representing
        // the bitmap and directory; these are left open while Nachos is running
        freeMapFile = new OpenFile(FreeMapSector);
        directoryFile = new OpenFile(DirectorySector);
    }
    // MP4 109062233
    fileDescriptor = NULL;
}

//----------------------------------------------------------------------
// MP4 mod tag
// FileSystem::~FileSystem
//----------------------------------------------------------------------
FileSystem::~FileSystem()
{
    delete freeMapFile;
    delete directoryFile;
}

//----------------------------------------------------------------------
// FileSystem::Create
// 	Create a file in the Nachos file system (similar to UNIX create).
//	Since we can't increase the size of files dynamically, we have
//	to give Create the initial size of the file.
//
//	The steps to create a file are:
//	  Make sure the file doesn't already exist
//        Allocate a sector for the file header
// 	  Allocate space on disk for the data blocks for the file
//	  Add the name to the directory
//	  Store the new file header on disk
//	  Flush the changes to the bitmap and the directory back to disk
//
//	Return TRUE if everything goes ok, otherwise, return FALSE.
//
// 	Create fails if:
//   		file is already in directory
//	 	no free space for file header
//	 	no free entry for file in directory
//	 	no free space for data blocks for the file
//
// 	Note that this implementation assumes there is no concurrent access
//	to the file system!
//
//	"name" -- name of file to be created
//	"initialSize" -- size of file to be created
//----------------------------------------------------------------------

// 109062233 MP4 
int FileSystem::return_sector(char* parent_path){
    Directory *directory = new Directory(NumDirEntries);
    OpenFile* openFile = NULL;
    int sector;
    directory->FetchFrom(directoryFile);
    char* temp = strtok(parent_path , "/");
    while(temp){
        sector = directory->Find(temp); 
        if(sector == -1){
            return -1;
        }
        openFile = new OpenFile(sector);
        directory->FetchFrom(openFile);
        delete openFile;
        //delete openFile;
        temp = strtok(NULL , "/");
    }
    delete directory;
    return sector;
}
//109062320 modify
// 109062233 MP4
bool FileSystem::CreateDirectory(char* name){
    Directory *directory = new Directory(NumDirEntries);
    PersistentBitmap *freeMap;
    FileHeader *hdr;
    int sector;
    bool success = true;
    DEBUG(dbgFile, "Creating a directory " << name);

    /* -------- Main code segment ---------------- */
    char* parent_path = new char[500];
    char* target_name = new char[500];
    char* temp_parent_path = new char[500];
    SplitPath(name, parent_path, target_name);
    strcpy(temp_parent_path, parent_path);
    char* temp = strtok(temp_parent_path , "/");
    /* -------- Main code segment ---------------- */

    directory->FetchFrom(directoryFile);

    if(temp != NULL ){
        if(OpenDir(parent_path) == NULL){
            success = false;
        }
        else{
            DEBUG(dbgFile, "[FileSystem::CreateDirectory] with " << parent_path);
            directory->FetchFrom(OpenDir(parent_path));
        }
    }
    else{
        DEBUG(dbgFile, "[FileSystem::CreateDirectory] Root ");
    }
    if (directory->Find(target_name) != -1){
        DEBUG(dbgFile, "[FileSystem::CreateDirectory] file is already in directory ");
        success = FALSE; // file is already in directory
    }
    if(success){
        //109062320 maybe there's some problems
        //directory->Add, if !hdr->Allocate(freeMap, DirectoryFileSize)
        //then do we need to reverse the change?
        freeMap = new PersistentBitmap(freeMapFile,NumSectors);
        sector = freeMap->FindAndSet();	// find a sector to hold the file header
        bool isAdd = directory->Add(target_name, sector, true);
        if (sector == -1 || !isAdd) // not a directory
            success = FALSE;
        else {
            hdr = new FileHeader;
            if (!hdr->Allocate(freeMap, DirectoryFileSize))
                success = FALSE;	// no space on disk for data
            else {
                success = TRUE;  // everthing worked, flush all changes back to disk
                hdr->WriteBack(sector);
                //Write any modifications to the directory back to disk
                if(!temp){
                    // root
                    directory->WriteBack(directoryFile);
                    Directory * new_dir = new Directory(NumDirEntries);
                    OpenFile* f = new OpenFile(sector); // MP4 109062233 = =
                    new_dir->WriteBack(f); // MP4 109062233
                    DEBUG(dbgFile, "[FileSystem::CreateDirectory] Root and create Entry in sector " << sector);
                }
                else{
                    DEBUG(dbgFile, "[FileSystem::CreateDirectory] Not Root and write to sector " << sector);
                    directory->WriteBack(OpenDir(parent_path));
                    Directory * new_dir = new Directory(NumDirEntries);
                    OpenFile* f = new OpenFile(sector); // MP4 109062233 = =
                    new_dir->WriteBack(f); // MP4 109062233
                }
                freeMap->WriteBack(freeMapFile);
                //109062320 modify
                //Directory * new_dir = new Directory(NumDirEntries);
                //OpenFile* f = OpenDir(parent_path); // MP4 109062233 = =
                //new_dir->WriteBack(f); // MP4 109062233
            }
            delete hdr;
        }
    }
    delete parent_path;
    delete temp_parent_path;
    delete target_name;
    delete freeMap;
    delete directory;
    return success;
}

// 109062233 MP4
bool FileSystem::Create(char *name, int initialSize)
{
    Directory *directory;
    PersistentBitmap *freeMap;
    FileHeader *hdr;
    int sector;
    bool success = true;
    DEBUG(dbgFile, "Creating file " << name << " size " << initialSize);
    directory = new Directory(NumDirEntries);

    /* -------- Main code segment ---------------- */
    char* parent_path = new char[500];
    char* target_name = new char[500];
    char* temp_parent_path = new char[500];
    SplitPath(name, parent_path, target_name);
    strcpy(temp_parent_path, parent_path);
    //if temp_parent_path == NULL -> temp = NULL
    //that's what strtok do
    //strtok(str , delim) delim is used to seperate string
    char* temp = strtok(temp_parent_path , "/");
    /* -------- Main code segment ---------------- */
    
    directory->FetchFrom(directoryFile);
    
    if(temp != NULL ){
        DEBUG(dbgFile, "[FileSystem::Create] Not root and path " << parent_path );
        if(OpenDir(parent_path) == NULL){
            DEBUG(dbgFile, "[FileSystem::Create] path doesn't exists");
            success = false;
        }
        else{
            directory->FetchFrom(OpenDir(parent_path));
        }
    }
    else{
        DEBUG(dbgFile, "[FileSystem::Create] Root ");
    }
    if (directory->Find(target_name) != -1){
        success = FALSE; // file is already in directory
    }
    if(success){
        DEBUG(dbgFile, "[FileSystem::Create] No name conflict ");
        freeMap = new PersistentBitmap(freeMapFile, NumSectors);
        sector = freeMap->FindAndSet(); // find a sector to hold the file header
        if (sector == -1)
            success = FALSE; // no free block for file header
        else if (!directory->Add(target_name, sector,false))
            success = FALSE; // no space in directory
        else{
            DEBUG(dbgFile, "[FileSystem::Create] enough sector & successfully add ");
            hdr = new FileHeader;
            if (!hdr->Allocate(freeMap, initialSize))
                success = FALSE; // no space on disk for data
            else{
                success = TRUE;
                // everthing worked, flush all changes back to disk
                DEBUG(dbgFile, "[FileSystem::Create] write back to sector " << sector);
                hdr->WriteBack(sector);
                // 2023 
                if(temp != NULL){
                    directory->WriteBack(OpenDir(parent_path));
                }
                else{
                    directory->WriteBack(directoryFile);
                }
                freeMap->WriteBack(freeMapFile);
            }
        delete hdr;
        }
    }
    delete parent_path;
    delete temp_parent_path;
    delete target_name;
    delete freeMap;
    delete directory;
    return success;
}
//109062320 modify
// The target is to split the /aaa/aa.txt into /aaa and aa.txt
void FileSystem::SplitPath(char* fullpath, char* parent_dir, char* target_name) {
    //when the length of src < n, dest = src + (null)*(n - src.length)
    //strncpy(char *dest, const char *src, size_t n)
    strncpy(parent_dir, fullpath, 300); 
    int idx = strlen(parent_dir) - 1;
    for(int i = idx ; i >= 0 ; i--){
        if(parent_dir[i] == '/'){
            break;
        }
        idx--;
    }
    //after split, there are two sitiation
    //1. parent_dir != empty --> to split the word after '/', 
    // the space will change to '\0' from '\'
    //2. parent_dir == empty --> still empty
    parent_dir[idx] = '\0';
    //target_name = parent_dir[idx + 1] ~ parent_dir[300]
    //aka "aa.txt"
    strncpy(target_name, parent_dir + idx + 1, 300);

    if (strlen(parent_dir) == 0) {
        strcpy(parent_dir, "/");
    }
}

// 109062233 MP4 we only consider valid path
OpenFile* FileSystem::OpenDir(char* parent_path){
    Directory *directory = new Directory(NumDirEntries);
    OpenFile* openFile = NULL;
    int sector;
    // 2023 
    char* new_path = new char[500];
    strcpy(new_path, parent_path);
    directory->FetchFrom(directoryFile);
    char* temp = strtok(new_path , "/");
    while(temp){
        sector = directory->Find(temp); 
        DEBUG(dbgFile, "[FileSystem::OpenDir] temp  " << temp << " sector " << sector);
        if(sector == -1){
            return NULL;
        }
        openFile = new OpenFile(sector);
        //recursively find the parent
        directory->FetchFrom(openFile);
        delete openFile;
        //delete openFile;
        // /a/b/c
        // first temp --> a
        // second temp --> b
        // thrid temp --> c
        temp = strtok(NULL , "/");
    }
    delete directory;
    delete new_path;
    openFile = new OpenFile(sector);
    return openFile;
}

//----------------------------------------------------------------------
// FileSystem::Open
// 	Open a file for reading and writing.
//	To open a file:
//	  Find the location of the file's header, using the directory
//	  Bring the header into memory
//
//	"name" -- the text name of the file to be opened
//----------------------------------------------------------------------

// MP4 109062233
OpenFile * FileSystem::Open(char *name)
{
    Directory *directory = new Directory(NumDirEntries);
    OpenFile *openFile = NULL;
    int sector;
    DEBUG(dbgFile, "Opening file" << name);

    /* -------- Main code segment ---------------- */
    char* parent_path = new char[500];
    char* target_name = new char[500];
    directory->FetchFrom(directoryFile);
    char* temp_parent_path = new char[500];
    SplitPath(name, parent_path, target_name);
    strcpy(temp_parent_path, parent_path);
    char* temp = strtok(temp_parent_path , "/");
    /* -------- Main code segment ---------------- */

    if(temp == NULL){ // root 
    DEBUG(dbgFile, "[FileSystem::Open] root ");
        sector = directory->Find(target_name); // 2023
        DEBUG(dbgFile, "[FileSystem::Open] Find " << target_name << " in " << sector);
        if (sector >= 0){
            openFile = new OpenFile(sector); // name was found in directory
        }
        else{
            openFile = NULL;
        }
    }
    else{
        //109062320 modified
        if(OpenDir(parent_path) != NULL){
            directory->FetchFrom(OpenDir(parent_path));
            sector = directory->Find(target_name);
            openFile = new OpenFile(sector);
            
        }
        else{
            openFile = NULL;
        }
    }
    // 2023
    this->fileDescriptor = openFile;
    delete directory;
    delete parent_path;
    delete temp_parent_path;
    delete target_name;
    return openFile;
}

// 109062233 MP4  BEGIN ADDING
int FileSystem::OpenAFile(char* filename){
    OpenFile* openfile = this->Open(filename);
    DEBUG(dbgSys, "Opening A file" << filename);
    if(openfile == NULL){
        return 0;
    }
    return 1;
}

int FileSystem::Read(char *buffer, int size, int id){
    DEBUG(dbgFile, "FileSystem::Read with file size = " << size);
    if(id < 0 || size < 0 ) return -1;
    else{
        if(!this->fileDescriptor){
            return -1;
        }
        else{
            return fileDescriptor->Read(buffer, size);
        }
    }
}

int FileSystem::Write(char *buffer, int size, int id){
    if(id < 0 && size < 0  ) return -1;
    else{
        if(!this->fileDescriptor){
            DEBUG(dbgFile, "File Not Found " << size);
            return -1;
        }
        return fileDescriptor->Write(buffer, size);
    }
}

int FileSystem::Close(int id){
    /* Close the file by id . Here, this operation will always succeed and return 1. */
    delete fileDescriptor;
    fileDescriptor = NULL;
    return 1;
}


// 109062233 MP4 END ADDING

//----------------------------------------------------------------------
// FileSystem::Remove
// 	Delete a file from the file system.  This requires:
//	    Remove it from the directory
//	    Delete the space for its header
//	    Delete the space for its data blocks
//	    Write changes to directory, bitmap back to disk
//
//	Return TRUE if the file was deleted, FALSE if the file wasn't
//	in the file system.
//
//	"name" -- the text name of the file to be removed
//----------------------------------------------------------------------


// Modify MP4 109062233
bool FileSystem::Remove(char *name)
{
    Directory *directory;
    PersistentBitmap *freeMap;
    FileHeader *fileHdr;
    int sector;
    directory = new Directory(NumDirEntries);
    bool success = true;
    /* -------- Main code segment ---------------- */
    char* parent_path = new char[500];
    char* target_name = new char[500];

    directory->FetchFrom(directoryFile);

    char* temp_parent_path = new char[500];
    SplitPath(name, parent_path, target_name);
    strcpy(temp_parent_path, parent_path);
    //e.g. temp_parent_path = a/b/c
    //-->  temp = a/b
    char* temp = strtok(temp_parent_path , "/");

    /* -------- Main code segment ---------------- */
    if(temp != NULL ){
        if(OpenDir(parent_path) == NULL){
            success = false;
        }
        else{
            directory->FetchFrom(OpenDir(parent_path));
        }
    }
    if (directory->Find(target_name) == -1){
        success = FALSE; // file not in directory
    }
    if(success){    
        sector = directory->Find(target_name);
        fileHdr = new FileHeader;
        fileHdr->FetchFrom(sector);
        freeMap = new PersistentBitmap(freeMapFile, NumSectors);
        // remove until the next Sector is NULL
        fileHdr->Deallocate(freeMap); // remove data blocks
        freeMap->Clear(sector);       // remove header block
        directory->Remove(target_name);
        freeMap->WriteBack(freeMapFile);     // flush to disk
        directory->WriteBack(OpenDir(parent_path)); // flush to disk
    }
    
    delete parent_path;
    delete target_name;
    delete temp_parent_path;
    delete fileHdr;
    delete directory;
    delete freeMap;
    return TRUE;
}

//----------------------------------------------------------------------
// FileSystem::List
// 	List all the files in the file system directory.
//----------------------------------------------------------------------

void FileSystem::List(char* name , bool recursive)
{
    /*MP4 SPEC says Recursively list the file/directory in a directory. The Directory always exists.*/
    Directory *directory = new Directory(NumDirEntries);
    /* -------- Main code segment ---------------- */
    char* parent_path = new char[500];
    char* target_name = new char[500];
    //root
    directory->FetchFrom(directoryFile);
    char* temp_parent_path = new char[500];
    SplitPath(name,parent_path,target_name);
    strcpy(temp_parent_path, parent_path);
    char* temp = strtok(temp_parent_path , "/");
    /* -------- Main code segment ---------------- */
    //non-root
    if(temp){
        directory->FetchFrom(OpenDir(parent_path));
    }
    //modified 109062320
    directory->List();
    
    delete directory;
    delete parent_path;
    delete target_name;
    delete temp_parent_path;
}

void FileSystem::recursiveList(char* name , int layer){
    /*MP4 SPEC says Recursively list the file/directory in a directory. The Directory always exists.*/
    Directory *directory = new Directory(NumDirEntries);
    /* -------- Main code segment ---------------- */
    char* parent_path = new char[500];
    char* target_name = new char[500];
    directory->FetchFrom(directoryFile);
    char* temp_parent_path = new char[500];
    SplitPath(name, parent_path, target_name);
    strcpy(temp_parent_path, parent_path);
    char* temp = strtok(temp_parent_path , "/");
    /* -------- Main code segment ---------------- */
    if(temp){
        directory->FetchFrom(OpenDir(parent_path));
    }
    DirectoryEntry* table = directory->GetTable();
    for(int i = 0 ; i < NumDirEntries ; i++){
        if (table[i].inUse){
            for(int j = 0 ; j < layer * 2; j++){
                printf("  ");
            }
            if(table[i].isDir){
                printf("[D] %s\n", table[i].name);
                char* new_path = new char[500];
                //absolute path --> new_path = name/table[i].name
                //name and table[i].name is splited in line599
                strcpy(new_path , name);
                //strcat(str1, str2) = str1 + str2
                // 2023 
                if(layer != 0){
                    strcat(new_path , "/");
                }
                strcat(new_path , table[i].name);
                strcat(new_path , "/");
                recursiveList(new_path , layer + 1);
                delete new_path;
            }
            else{
                printf("[F] %s\n", table[i].name);
            }
        }
    }
    delete directory;
    delete parent_path;
    delete target_name;
    delete temp_parent_path;
}

//----------------------------------------------------------------------
// FileSystem::Print
// 	Print everything about the file system:
//	  the contents of the bitmap
//	  the contents of the directory
//	  for each file in the directory,
//	      the contents of the file header
//	      the data in the file
//----------------------------------------------------------------------

void FileSystem::Print()
{
    FileHeader *bitHdr = new FileHeader;
    FileHeader *dirHdr = new FileHeader;
    PersistentBitmap *freeMap = new PersistentBitmap(freeMapFile, NumSectors);
    Directory *directory = new Directory(NumDirEntries);

    printf("Bit map file header:\n");
    bitHdr->FetchFrom(FreeMapSector);
    bitHdr->Print();

    printf("Directory file header:\n");
    dirHdr->FetchFrom(DirectorySector);
    dirHdr->Print();

    freeMap->Print();

    directory->FetchFrom(directoryFile);
    directory->Print();

    delete bitHdr;
    delete dirHdr;
    delete freeMap;
    delete directory;
}


#endif // FILESYS_STUB
