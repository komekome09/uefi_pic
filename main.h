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
	UINT32	Compression;
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
LoadPngFile(IN EFI_HANDLE Handle, IN CHAR16 *Path, OUT void **PngBuffer, OUT UINTN *BmpSize){
    EFI_STATUS                  Status = EFI_SUCCESS;
    EFI_FILE_IO_INTERFACE       *SimpleFile;
    EFI_GUID                    SimpleFileSystemProtocolGuid = SIMPLE_FILE_SYSTEM_PROTOCOL;
    EFI_FILE                    *Root, *File;
    UINTN                       BufferSize;
    void                        *Buffer = NULL;
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
		Print(L"LocateProtocol: %r\n", Status);
		return Status;
	}

	Status = uefi_call_wrapper(SimpleFile->OpenVolume, 2, SimpleFile, &Root);
	if(EFI_ERROR(Status)){
		Print(L"VolumrOpen: %r\n", Status);
		return Status;
	}

	Status = uefi_call_wrapper(Root->Open, 5, Root, &File, Path, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
	if(EFI_ERROR(Status)){
		Print(L"FileOpen: %r\n", Status);
		return Status;
	}

	BufferSize = MAX_BUFFER_SIZE;
	Buffer = AllocatePool(BufferSize);
	if(Buffer == NULL){
		Print(L"Buffer: %r\n", EFI_OUT_OF_RESOURCES);
		return EFI_OUT_OF_RESOURCES;
	}
	Status = uefi_call_wrapper(File->Read, 3, File, &BufferSize, Buffer);
	if(BufferSize == MAX_BUFFER_SIZE){
		Print(L"BufferSize: %r\n", EFI_OUT_OF_RESOURCES);
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
	UINT8							*BitmapIndex;
	UINT8							*BitmapHead;
	UINT32							*Palette;
	UINTN							Pixels;
	UINTN							Size;
	UINTN							XIndex;
	UINTN							YIndex;
	UINTN							BltPos;
	UINTN							BitmapWidth;
	UINTN							BitmapHeight;
	EFI_GRAPHICS_OUTPUT_BLT_PIXEL	*BltBuffer;

	BitmapHeader = (BITMAP_FILE_HEADER*)BmpBuffer;
	BitmapWidth  = BitmapHeader->Width;
	BitmapWidth	 = ((INT32)BitmapWidth < 0) ? -(INT32)(BitmapWidth) : BitmapWidth;
	BitmapHeight = BitmapHeader->Height;
	BitmapHeight = ((INT32)BitmapHeight < 0) ? -(INT32)(BitmapHeight) : BitmapHeight;

	Print(L"Bitmap: %d x %d\n", BitmapHeader->Width, BitmapHeader->Height);
	Print(L"Bitmap: %d x %d\n", BitmapWidth, BitmapHeight);

	if(BitmapHeader->CoreHeaderSize != 40){
		Print(L"CoreHeaderSize: %r\n", EFI_UNSUPPORTED);
		return EFI_UNSUPPORTED;
	}
	if(BitmapHeader->Compression != 0){
		Print(L"Compression: %r\n", EFI_UNSUPPORTED);
		return EFI_UNSUPPORTED;
	}
	if(BitmapHeader->BitCount != 8 && BitmapHeader->BitCount != 24){
		Print(L"BitCount: %r\n", EFI_UNSUPPORTED);
		return EFI_UNSUPPORTED;
	}

	BitmapHead	 = (UINT8*)BmpBuffer + BitmapHeader->Offset;
	BitmapIndex  = BitmapHead;
	Palette		 = (UINT32*)((UINT8*)BmpBuffer + 0x36); // 決め打ち
	Pixels		 = BitmapWidth * BitmapHeight;
	Size		 = sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL) * Pixels;

	BltBuffer	 = AllocateZeroPool(Size);
	if(BltBuffer == NULL){
		Print(L"BltBuffer: %r\nSize = %d\n", EFI_OUT_OF_RESOURCES, Size);
		return EFI_OUT_OF_RESOURCES;
	}

	for(YIndex = 0; YIndex < BitmapHeight; YIndex++){
		for(XIndex = 0; XIndex < BitmapWidth; XIndex++, BitmapIndex++){
			BltPos	= (BitmapHeight - YIndex) * BitmapWidth + XIndex;
			switch(BitmapHeader->BitCount){
				case 8:
					BltBuffer[BltPos].Blue		= (UINT8) BitFieldRead32 (Palette[*BitmapIndex], 0 , 7 );
					BltBuffer[BltPos].Green    = (UINT8) BitFieldRead32 (Palette[*BitmapIndex], 8 , 15);
					BltBuffer[BltPos].Red      = (UINT8) BitFieldRead32 (Palette[*BitmapIndex], 16, 23);
					break;
				case 24:
					BltBuffer[BltPos].Blue		= *BitmapIndex++;
					BltBuffer[BltPos].Green    = *BitmapIndex++;
					BltBuffer[BltPos].Red      = *BitmapIndex;
					break;
				default:
					Print(L"BitCount:: %r\n", EFI_UNSUPPORTED);
					return EFI_UNSUPPORTED;
			}
		}
		if((BitmapIndex - BitmapHead) % 4 != 0){
			BitmapIndex = BitmapIndex + (4 - ((BitmapIndex - BitmapHead) % 4));
		}
	}

	Status = uefi_call_wrapper(GraphicsOutput->Blt, 10, GraphicsOutput, BltBuffer, EfiBltBufferToVideo, 0, 0, 200, 200, BitmapWidth, BitmapHeight, 0);
	if(EFI_ERROR(Status)){
		Print(L"Blt: %r\n", Status);
		return Status;
	}
	FreePool(BltBuffer);

	return Status;
}

VOID
ShowQueryMode(IN EFI_GRAPHICS_OUTPUT_PROTOCOL *GraphicsOutput){
	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION	*GraphicsInfo;
	UINTN									Size;
	EFI_STATUS								Status;

	Print(L"Current Mode: %d\n", GraphicsOutput->Mode->Mode);
	for(int ModeNumber = 0; ModeNumber < GraphicsOutput->Mode->MaxMode; ModeNumber++){
		Status = uefi_call_wrapper(GraphicsOutput->QueryMode, 4, GraphicsOutput, 0, &Size, &GraphicsInfo);
		if(EFI_ERROR(Status)){
			Print(L"QueryMode: %r\n", Status);
			return;
		}
		Print(L"Mode %d - Display: %d x %d\n", ModeNumber, GraphicsInfo->HorizontalResolution, GraphicsInfo->VerticalResolution);
	}
}
