// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -n -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you are using the "stub" file system, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.


#include "copyright.h"
#include "main.h"
#include "addrspace.h"
#include "machine.h"
#include "noff.h"

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
    noffH->noffMagic = WordToHost(noffH->noffMagic);
    noffH->code.size = WordToHost(noffH->code.size);
    noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
    noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
#ifdef RDATA
    noffH->readonlyData.size = WordToHost(noffH->readonlyData.size);
    noffH->readonlyData.virtualAddr = 
           WordToHost(noffH->readonlyData.virtualAddr);
    noffH->readonlyData.inFileAddr = 
           WordToHost(noffH->readonlyData.inFileAddr);
#endif 
    noffH->initData.size = WordToHost(noffH->initData.size);
    noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
    noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
    noffH->uninitData.size = WordToHost(noffH->uninitData.size);
    noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
    noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);

#ifdef RDATA
    DEBUG(dbgAddr, "code = " << noffH->code.size <<  
                   " readonly = " << noffH->readonlyData.size <<
                   " init = " << noffH->initData.size <<
                   " uninit = " << noffH->uninitData.size << "\n");
#endif
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//----------------------------------------------------------------------

AddrSpace::AddrSpace()
{
    // modified 109062233
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
    // modified by 109062233 
    for (int i = 0; i < numPages; i++) {
        kernel->UsedPhyAddr[pageTable[i].virtualPage] = false;
    }
   delete pageTable;
}


//----------------------------------------------------------------------
// AddrSpace::Load
// 	Load a user program into memory from a file.
//
//	Assumes that the page table has been initialized, and that
//	the object code file is in NOFF format.
//
//	"fileName" is the file containing the object code to load into memory
//----------------------------------------------------------------------

bool 
AddrSpace::Load(char *fileName) 
{
    OpenFile *executable = kernel->fileSystem->Open(fileName);
    NoffHeader noffH;
    unsigned int size;

    if (executable == NULL) {
	cerr << "Unable to open file " << fileName << "\n";
	return FALSE;
    }

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

#ifdef RDATA
// how big is address space?
    size = noffH.code.size + noffH.readonlyData.size + noffH.initData.size +
           noffH.uninitData.size + UserStackSize;	
                                                // we need to increase the size
						// to leave room for the stack
    DEBUG(dbgSys, "There is readonly data with the size "  <<  noffH.readonlyData.size << "\n"); 
#else
// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
#endif
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;
//revised by 109062320
    DEBUG(dbgSys, "we need numPage " << numPages << "here, and the physical page number is" << NumPhysPages << "\n");
    if(numPages > NumPhysPages)
            ExceptionHandler(MemoryLimitException);
    ASSERT(numPages <= NumPhysPages);		// check we're not trying
						// to run anything too big --
						// at least until we have
						// virtual memory

    DEBUG(dbgAddr, "Initializing address space: " << numPages << ", " << size);
    //109062320 revise
    // ============================ modified 109062233 ==============================================//
    pageTable = new TranslationEntry[numPages];
    // numPages
    DEBUG(dbgSys, "Initialize the addrspace \n" ); 
    DEBUG(dbgSys, "And the numPage is " << numPages); 
    unsigned int total_size = noffH.code.size + noffH.code.inFileAddr;
    for (int i = 0; i < numPages; i++) {
        for(int j = 0 ; j < NumPhysPages; j++){
            if(!kernel->UsedPhyAddr[j]){ //if find empty space
                DEBUG(dbgAddr, "The address find  " << j << "in physical mem \n" );
                kernel->UsedPhyAddr[j] = true;
                pageTable[i].virtualPage = j;	
                pageTable[i].physicalPage = j;
                pageTable[i].valid = TRUE;
                pageTable[i].use = FALSE;
                pageTable[i].dirty = FALSE;
                pageTable[i].readOnly = FALSE;
                // zero out the segment address space
                bzero(kernel->machine->mainMemory + j*PageSize, PageSize);
		DEBUG(dbgSys, "physical page used now " << j << "\n");                
                break;
            }
            //all the page tables are used
            if(j == NumPhysPages - 1 && kernel->UsedPhyAddr[j]){
                ExceptionHandler(MemoryLimitException);
            }
        }
    }
    unsigned int code_starting_page = noffH.code.virtualAddr / PageSize;
    unsigned int code_remainder = noffH.code.virtualAddr % PageSize;
    unsigned int initData_starting_page = noffH.initData.virtualAddr / PageSize;
    unsigned int initData_remainder = noffH.initData.virtualAddr % PageSize;

    unsigned int code_size = noffH.code.size;
    unsigned int code_used_page = noffH.code.size / PageSize;
    unsigned int code_size_rem = noffH.code.size % PageSize;

    unsigned int initData_size = noffH.initData.size;
    unsigned int initData_used_page = noffH.initData.size / PageSize;
    unsigned int initData_size_rem = noffH.initData.size % PageSize;
    
    bool first_time = true;
    if(code_size == 0 && initData_size == 0){
        return true;
    }
    //for code
    DEBUG(dbgSys  , "the noff file have code size of  " << noffH.code.size <<  " and initData size = "     
    << noffH.initData.size << "with the physical page "<< pageTable[noffH.code.virtualAddr / PageSize].physicalPage << " \n");
    DEBUG(dbgAddr, "Initializing code segment.");
	
    for (int i = 0; i < numPages; i++) {
        DEBUG(dbgAddr, noffH.code.virtualAddr << ", " << noffH.code.size);
        if(code_size > 0){
            if(code_size > PageSize){
                executable->ReadAt(&(kernel->machine->mainMemory[pageTable[code_starting_page].physicalPage*PageSize
                + code_remainder]), PageSize, noffH.code.inFileAddr + i*PageSize);
                code_starting_page++;
                code_size -= PageSize;
            }

            else{
                executable->ReadAt(&(kernel->machine->mainMemory[pageTable[code_starting_page].physicalPage*PageSize
                + code_remainder]), code_size, noffH.code.inFileAddr + i*PageSize);
                code_starting_page++;
                code_size -= code_size_rem;
            }
        }
        else break;
    }
    
    DEBUG(dbgAddr, "Initializing data segment.");
    //for initData
    for (int i = 0; i < numPages; i++) {
        if(initData_size > 0){
            if(initData_size > PageSize){
                executable->ReadAt(&(kernel->machine->mainMemory[pageTable[initData_starting_page].physicalPage*PageSize
                + initData_remainder]), PageSize, noffH.initData.inFileAddr + i*PageSize);
                initData_starting_page++;
                initData_size -= PageSize;
            }

            else{
                executable->ReadAt(&(kernel->machine->mainMemory[pageTable[initData_starting_page].physicalPage*PageSize
                + initData_remainder]), initData_size, noffH.initData.inFileAddr + i*PageSize);
                initData_starting_page++;
                initData_size -= PageSize;
            }
        }
        else{
            break;
        }
    }
    
// then, copy in the code and data segments into memory
// Note: this code assumes that virtual address = physical address space
    // modified 109062233 

    /*DEBUG(dbgSys  , "the noff file have code size of  " << noffH.code.size <<  " and initData size = " 
    << noffH.initData.size << "with the physical page "<< pageTable[noffH.code.virtualAddr / PageSize].physicalPage << " \n");
    if (noffH.code.size > 0) {
        DEBUG(dbgAddr, "Initializing code segment.");
	DEBUG(dbgAddr, noffH.code.virtualAddr << ", " << noffH.code.size);
        executable->ReadAt(
        &(kernel->machine->mainMemory[pageTable[noffH.code.virtualAddr / PageSize].physicalPage*PageSize
        + noffH.code.virtualAddr%PageSize]), 
			noffH.code.size, noffH.code.inFileAddr);
    }
    if (noffH.initData.size > 0) {
        DEBUG(dbgAddr, "Initializing data segment.");
	DEBUG(dbgAddr, noffH.initData.virtualAddr << ", " << noffH.initData.size);
        executable->ReadAt(
		&(kernel->machine->mainMemory[pageTable[noffH.initData.virtualAddr / PageSize].physicalPage*PageSize
        + noffH.initData.virtualAddr%PageSize]), 
			noffH.initData.size, noffH.initData.inFileAddr);
    }*/

    // ============================ modified 109062233 ==============================================//
#ifdef RDATA
    /*if (noffH.readonlyData.size > 0) {
        DEBUG(dbgAddr, "Initializing read only data segment.");
	DEBUG(dbgAddr, noffH.readonlyData.virtualAddr << ", " << noffH.readonlyData.size);
        executable->ReadAt(
		&(kernel->machine->mainMemory[noffH.readonlyData.virtualAddr]),
			noffH.readonlyData.size, noffH.readonlyData.inFileAddr);
    }*/
    unsigned int read_size = noffH.readonlyData.size;
    DEBUG(dbgSys, "There is readonly data with the size "  << read_size << "\n");
    DEBUG(dbgSys, "There is readonly data with the size "  << read_size << "\n");
    unsigned int read_starting_page = noffH.readonlyData.virtualAddr / PageSize;
    unsigned int read_remainder = noffH.readonlyData.virtualAddr % PageSize;
    for (int i = 0; i < numPages; i++) {
        if(read_size > 0){
            if(read_size > PageSize){
                executable->ReadAt(&(kernel->machine->mainMemory[pageTable[read_starting_page].physicalPage*PageSize
                + read_remainder]), PageSize, noffH.readonlyData.inFileAddr + i*PageSize);
                pageTable[read_starting_page].readOnly = true;
                DEBUG(dbgSys, "Set the page " << read_starting_page << "As " << pageTable[read_starting_page].readOnly << "\n");
                read_starting_page++;
                read_size -= PageSize;

            }
            else{
                executable->ReadAt(&(kernel->machine->mainMemory[pageTable[read_starting_page].physicalPage*PageSize
                + read_remainder]), read_size, noffH.readonlyData.inFileAddr + i*PageSize);
                pageTable[read_starting_page].readOnly = true;
                DEBUG(dbgSys, "Set the page " << read_starting_page << "As " << pageTable[read_starting_page].readOnly << "\n");
                read_starting_page++;
                read_size -= PageSize;
            }
        }
        else{
            // for const int a = 1 -> it will have a size with 0 ??? 
            pageTable[read_starting_page].readOnly = true;
            DEBUG(dbgSys, "Set the page " << read_starting_page << "As " << pageTable[read_starting_page].readOnly << "\n");
            break;
        }
    }
#endif

    delete executable;			// close file
    return TRUE;			// success
}

//----------------------------------------------------------------------
// AddrSpace::Execute
// 	Run a user program using the current thread
//
//      The program is assumed to have already been loaded into
//      the address space
//
//----------------------------------------------------------------------

void 
AddrSpace::Execute(char* fileName) 
{

    kernel->currentThread->space = this;

    this->InitRegisters();		// set the initial register values
    this->RestoreState();		// load page table register

    kernel->machine->Run();		// jump to the user progam

    ASSERTNOTREACHED();			// machine->Run never returns;
					// the address space exits
					// by doing the syscall "exit"
}


//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    Machine *machine = kernel->machine;
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start", which
    //  is assumed to be virtual address zero
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    // Since instructions occupy four bytes each, the next instruction
    // after start will be at virtual address four.
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG(dbgAddr, "Initializing stack pointer: " << numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, don't need to save anything!
//----------------------------------------------------------------------
//modified by 109062320
void AddrSpace::SaveState() 
{
    pageTable = kernel->machine->pageTable;
    numPages = kernel->machine->pageTableSize;
}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    kernel->machine->pageTable = pageTable;
    kernel->machine->pageTableSize = numPages;
}


//----------------------------------------------------------------------
// AddrSpace::Translate
//  Translate the virtual address in _vaddr_ to a physical address
//  and store the physical address in _paddr_.
//  The flag _isReadWrite_ is false (0) for read-only access; true (1)
//  for read-write access.
//  Return any exceptions caused by the address translation.
//----------------------------------------------------------------------
ExceptionType
AddrSpace::Translate(unsigned int vaddr, unsigned int *paddr, int isReadWrite)
{
    TranslationEntry *pte;
    int               pfn;
    unsigned int      vpn    = vaddr / PageSize;
    unsigned int      offset = vaddr % PageSize;

    if(vpn >= numPages) {
        return AddressErrorException;
    }

    pte = &pageTable[vpn];

    if(isReadWrite && pte->readOnly) {
        return ReadOnlyException;
    }

    pfn = pte->physicalPage;

    // if the pageFrame is too big, there is something really wrong!
    // An invalid translation was loaded into the page table or TLB.
    if (pfn >= NumPhysPages) {
        DEBUG(dbgAddr, "Illegal physical page " << pfn);
        return BusErrorException;
    }

    pte->use = TRUE;          // set the use, dirty bits

    if(isReadWrite)
        pte->dirty = TRUE;

    *paddr = pfn*PageSize + offset;

    ASSERT((*paddr < MemorySize));

    //cerr << " -- AddrSpace::Translate(): vaddr: " << vaddr <<
    //  ", paddr: " << *paddr << "\n";

    return NoException;
}
