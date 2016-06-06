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

UINT32
BitFieldRead32(IN UINT32 Operand, IN UINTN StartBit, IN UINTN EndBit){
	ASSERT(EndBit < sizeof(Operand) * 8);
	ASSERT(StartBit <= EndBit);
	return (Operand & ~((unsigned int)-2 << EndBit)) >> StartBit;
}

STATIC
EFI_STATUS
LoadBitmapFile(IN EFI_HANDLE Handle, IN CHAR16 *Path,	OUT	void **BmpBuffer, OUT UINTN *BmpSize){
	EFI_STATUS					Status = EFI_SUCCESS;
	EFI_FILE_IO_INTERFACE		*SimpleFile;
	EFI_GUID					SimpleFileSystemProtocolGuid = SIMPLE_FILE_SYSTEM_PROTOCOL;
	EFI_FILE					*Root, *File;
	UINTN						BufferSize;
	void						*Buffer = NULL;

	Status = uefi_call_wrapper(BS->LocateProtocol, 3, &SimpleFileSystemProtocolGuid, NULL, &SimpleFile);
	if(EFI_ERROR(Status)){
		Print(L"Locate protocol error.\n");
		return Status;
	}

	Print(L"hoge\n");
	Status = uefi_call_wrapper(SimpleFile->OpenVolume, 2, SimpleFile, &Root);
	if(EFI_ERROR(Status)){
		Print(L"Volume open error\n");
		return Status;
	}

	Print(L"hoge\n");
	Status = uefi_call_wrapper(Root->Open, 5, Root, &File, Path, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
	if(EFI_ERROR(Status)){
		Print(L"File open error\n");
		return Status;
	}

	BufferSize = MAX_BUFFER_SIZE;
	Buffer = AllocatePool(BufferSize);
	if(Buffer == NULL){
		return EFI_OUT_OF_RESOURCES;
	}
	Status = uefi_call_wrapper(File->Read, 3, File, &BufferSize, Buffer);
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

STATIC
EFI_STATUS
DrawBmp(IN EFI_GRAPHICS_OUTPUT_PROTOCOL *GraphicsOutput, IN void *BmpBuffer, IN UINTN BmpSize){
	EFI_STATUS						Status = EFI_SUCCESS;
	BITMAP_FILE_HEADER				*BitmapHeader;
	UINT8							*BitmapData;
	UINT32							*Palette;
	UINTN							Pixels;
	UINTN							XIndex;
	UINTN							YIndex;
	UINTN							Pos;
	UINTN							BltPos;
	EFI_GRAPHICS_OUTPUT_BLT_PIXEL	*BltBuffer;

	BitmapHeader = (BITMAP_FILE_HEADER*)BmpBuffer;
	Print(L"%d x %d\n", BitmapHeader->Width, BitmapHeader->Height);

	if(BitmapHeader->CoreHeaderSize != 40){
		return EFI_UNSUPPORTED;
	}
	if(BitmapHeader->BitCount != 8){
		return EFI_UNSUPPORTED;
	}

	BitmapData 	= (UINT8*)BmpBuffer + BitmapHeader->Offset;
	Palette		= (UINT32*)((UINT8*)BmpBuffer + 0x36); // 決め打ち
	Pixels		= BitmapHeader->Width * BitmapHeader->Height;
	BltBuffer	= AllocateZeroPool(sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL) * Pixels);
	if(BltBuffer == NULL){
		return EFI_OUT_OF_RESOURCES;
	}

	for(YIndex = BitmapHeader->Height; YIndex > 0; YIndex--){
		for(XIndex = 0; XIndex < BitmapHeader->Width; XIndex++){
			Pos		= (YIndex - 1) * ((BitmapHeader->Width + 3) / 4) * 4 + XIndex;
			BltPos	= (BitmapHeader->Height - YIndex) * BitmapHeader->Width + XIndex;
			BltBuffer[BltPos].Blue		= (UINT8) BitFieldRead32 (Palette[BitmapData[Pos]], 0 , 7 );
			BltBuffer[BltPos].Green    = (UINT8) BitFieldRead32 (Palette[BitmapData[Pos]], 8 , 15);
			BltBuffer[BltPos].Red      = (UINT8) BitFieldRead32 (Palette[BitmapData[Pos]], 16, 23);
			BltBuffer[BltPos].Reserved = (UINT8) BitFieldRead32 (Palette[BitmapData[Pos]], 24, 31);
		}
	}

	Status = uefi_call_wrapper(GraphicsOutput->Blt, 10, GraphicsOutput, BltBuffer, EfiBltBufferToVideo, 0, 0, 200, 200, BitmapHeader->Width, BitmapHeader->Height, 0);
	if(EFI_ERROR(Status)){
		Print(L"Can not draw picture.\n");
		return Status;
	}
	FreePool(BltBuffer);

	return Status;
}
