#pragma once

#include <efi.h>
#include <efibind.h>
#include <efilib.h>
#include <efiprot.h>

#define SIZE_1MB 0x00100000
#define MAX_BUFFER_SIZE SIZE_1MB

#pragma pack(1)
typedef struct {
	CHAR8	Type[2];
	UINT32	Size;
	UINT32	Reserved;
	UINT32	Offset;
	UINT32	CoreHeaderSize;
	UINT32	Width;
	UINT32	Height;
	UINT16	Planes;
	UINT16	BitCount;
} BITMAP_FILE_HEADER;
#pragma pack()

STATIC
EFI_STATUS
LoadBitmapFile(IN CHAR16 *Path,	OUT	VOID **BmpBuffer, OUT UINTN *BmpSize){
	EFI_STATUS					Status = EFI_SUCCESS;
	EFI_FILE_IO_INTERFACE		*SimpleFile;
	EFI_GUID					SimpleFileSystemProtocolGuid = SIMPLE_FILE_SYSTEM_PROTOCOL;
	EFI_FILE					*Root, *File;
	UINTN						BufferSize;
	VOID						*Buffer = NULL;

	Status = LibLocateProtocol(&SimpleFileSystemProtocolGuid, (VOID **)&SimpleFile);
	if(EFI_ERROR(Status)){
		CHAR16 tmp[256];
		StatusToString(tmp, Status);
		Print(tmp);
		return Status;
	}

	Status = SimpleFile->OpenVolume(SimpleFile, &Root);
	if(EFI_ERROR(Status)){
		Print(L"Volume open error\n");
		return Status;
	}

	Print(L"hoge\n");
	Status = Root->Open(Root, &File, Path, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
	if(EFI_ERROR(Status)){
		Print(L"File open error\n");
		return Status;
	}

	BufferSize = MAX_BUFFER_SIZE;
	Buffer = AllocatePool(BufferSize);
	if(Buffer == NULL){
		return EFI_OUT_OF_RESOURCES;
	}
	Status = File->Read(File, &BufferSize, Buffer);
	if(BufferSize == MAX_BUFFER_SIZE){
		Print(L"Buffer is too small.\n");
		if(Buffer != NULL){
			FreePool(Buffer);
		}
		return EFI_OUT_OF_RESOURCES;
	}
	
	Buffer = ReallocatePool(Buffer, MAX_BUFFER_SIZE, BufferSize);

	*BmpBuffer = Buffer;
	*BmpSize   = BufferSize;

	return Status;
}
