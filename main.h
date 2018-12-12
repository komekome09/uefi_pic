#pragma once

#include <efi.h>
#include <efibind.h>
#include <efilib.h>
#include <efiprot.h>
#include "stb_image.h"

#define SIZE_1MB 0x00100000
#define MAX_BUFFER_SIZE SIZE_1MB

// chunk byte size of png
#define PNG_LENGTH_SIZE 4
#define PNG_CHUNKTYPE_SIZE 4
#define PNG_CRC_SIZE 4

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


VOID
CatenateMemory(IN void *Dest, IN const void *Src, IN UINTN Size, IN UINTN StartOffset){
    CHAR8       *dst;
    const CHAR8 *src = Src;
    dst = Dest+StartOffset;
    while(Size--){
        *(dst++) = *(src++);
    }
}

UINT32
BitFieldRead32(IN UINT32 Operand, IN UINTN StartBit, IN UINTN EndBit){
	ASSERT(EndBit < sizeof(Operand) * 8);
	ASSERT(StartBit <= EndBit);
	return (Operand & ~((unsigned int)-2 << EndBit)) >> StartBit;
}

STATIC
EFI_STATUS
LoadImageFile(IN EFI_HANDLE Handle, IN CHAR16 *Path,	OUT	void **BmpBuffer, OUT UINTN *BmpSize){
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

// Byte Swapping
UINT32 SwapBit32(UINT32 value){
    UINT32 ret;
    ret  = value              << 24;
    ret |= (value&0x0000FF00) <<  8;
    ret |= (value&0x00FF0000) >>  8;
    ret |= value              >> 24;
    return ret;
}

UINT16 SwapBit16(UINT16 value){
    UINT16 ret;
    ret  = value << 8;
    ret |= value >> 8;
    return ret;
}

STATIC
EFI_STATUS
DrawImage(IN EFI_GRAPHICS_OUTPUT_PROTOCOL *GraphicsOutput, IN void *ImgBuffer, IN UINTN ImgSize){
    EFI_STATUS      Status = EFI_SUCCESS;
    UINT8           *Pixels;
    INT32           Width = 0;
    INT32           Height = 0;
    INT32           Bpp = 0;

    if(ImgBuffer == NULL){
        return EFI_INVALID_PARAMETER;
    }

    Pixels = stbi_load_from_memory((UINT8*)ImgBuffer, ImgSize, &Width, &Height, &Bpp, 0);
    if(Pixels == NULL){
        Print(L"%s\n", (CHAR16*)stbi_failure_reason());
        return EFI_INVALID_PARAMETER;
    }

    Print(L"Width       = %d\n", Width);
    Print(L"Height      = %d\n", Height);
    Print(L"Bpp         = %d\n", Bpp);

    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *BltBuffer;
    UINT8                           *ImgIndex = Pixels;
    INTN                            BltPos = 0;

    BltBuffer = AllocateZeroPool(sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL)*Width*Height);
    if(BltBuffer == NULL){
		Print(L"BltBuffer: %r\nSize = %d\n", EFI_OUT_OF_RESOURCES, ImgSize);
		return EFI_OUT_OF_RESOURCES;
    }

    for(INTN YIndex = 0; YIndex < Height; YIndex++){
        for(INTN XIndex = 0; XIndex < Width; XIndex++){
			BltPos	= YIndex * Width + XIndex;
			switch(Bpp){
				case 4:					
                    BltBuffer[BltPos].Red           = *ImgIndex++;
                    BltBuffer[BltPos].Green         = *ImgIndex++;
                    BltBuffer[BltPos].Blue	        = *ImgIndex++;
                    BltBuffer[BltPos].Reserved      = *ImgIndex++;
					break;
				case 3:
					BltBuffer[BltPos].Red           = *ImgIndex++;
					BltBuffer[BltPos].Green         = *ImgIndex++;
					BltBuffer[BltPos].Blue	        = *ImgIndex++;
					break;
				default:
					Print(L"BitCount:: %r\n", EFI_UNSUPPORTED);
					return EFI_UNSUPPORTED;
			}
		}
	}

	Status = uefi_call_wrapper(GraphicsOutput->Blt, 10, GraphicsOutput, BltBuffer, EfiBltBufferToVideo, 0, 0, 0, 0, Width, Height, 0);

    return Status;
}

STATIC
EFI_STATUS
outb(IN EFI_PCI_IO_PROTOCOL *PciIo, UINTN Addr, UINT8 Data){
    return uefi_call_wrapper(PciIo->Io.Write, 6, PciIo, EfiPciIoWidthUint8, EFI_PCI_IO_PASS_THROUGH_BAR, Addr, 1, &Data);
}

STATIC
UINT8
inb(IN EFI_PCI_IO_PROTOCOL *PciIo, UINTN Addr){
    UINT8 Data;
    uefi_call_wrapper(PciIo->Io.Read, 6, PciIo, EfiPciIoWidthUint8, EFI_PCI_IO_PASS_THROUGH_BAR, Addr, 1, &Data);
    return Data;
}

STATIC
EFI_STATUS
Beep(IN EFI_PCI_IO_PROTOCOL *PciIo, IN UINTN freq){
    UINTN Div;
    UINT8 Tmp;
    EFI_STATUS Status;

    Div = 1193180 / freq;
    UINTN Addr = 0x43;
    UINT8 Data = 0xb6;

    Status = outb(PciIo, Addr, Data);
    if(EFI_ERROR(Status)){
        Print(L"Error in 0x43\n");
        return Status;
    }

    Addr = 0x42; Data = (UINT8)Div;
    Status = outb(PciIo, Addr, Data);
    if(EFI_ERROR(Status)){
        Print(L"Error in 0x42 Div\n");
        return Status;
    }

    Data = (UINT8)(Div >> 8);
    Status = outb(PciIo, Addr, Data);
    if(EFI_ERROR(Status)){
        Print(L"Error in 0x42 Div >> 8\n");
        return Status;
    }

    Addr = 0x61;
    Tmp = inb(PciIo, Addr);
    if(Tmp != (Tmp | 3)){
        Status = outb(PciIo, Addr, Tmp | 3);
        if(EFI_ERROR(Status)){
            Print(L"Error in 0x61\n");
            return Status;
        }
    }

    uefi_call_wrapper(BS->Stall, 1, 1e6);

    UINTN notes[11] = {277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494};
    Status = outb(PciIo, 0x43, 0xb6); 
    for(UINTN i=0; i<11; i++){
        Data = 0x1234DC / notes[i];
        Status = outb(PciIo, 0x42, Data & 0xff);
        if(EFI_ERROR(Status)){
            Print(L"Error in 0x42 Data & 0xff\n");
            return Status;
        }
        outb(PciIo, 0x42, Data >> 8);
        if(EFI_ERROR(Status)){
            Print(L"Error in 0x42 Data >> 8\n");
            return Status;
        }
        uefi_call_wrapper(BS->Stall, 1, 1e5);
    }

    Tmp = inb(PciIo, Addr) & 0xfc;
    Status = outb(PciIo, Addr, Tmp);
    if(EFI_ERROR(Status)){
        Print(L"Error in 0x61 stop\n");
        return Status;
    }

    return EFI_SUCCESS;
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
