#pragma once

#include <efi.h>
#include <efibind.h>
#include <efilib.h>
#include <efiprot.h>

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

typedef struct {
    CHAR8   Signature[8];
} PNG_SIGNATURE;

typedef struct {
    UINT32 Length;
    CHAR8 Type[4];
} PNG_CHUNK_HEADER;

typedef struct {
    UINT32 Length;
    CHAR8 Type[4];
    UINT32 Width;
    UINT32 Height;
    UINT8 Depth;
    UINT8 ColorType;
    UINT8 CompressionType;
    UINT8 FilterType;
    UINT8 InterraceType;
    CHAR8 Crc[4];
} PNG_CHUNK_IHDR;
#pragma pack()

CHAR8 PNG_CHUNKHEX[][5] = {
    {0x49, 0x48, 0x44, 0x52}, // IHDR
    {0x50, 0x4C, 0x54, 0x45}, // PLTE
    {0x49, 0x44, 0x41, 0x54}, // IDAT
    {0x49, 0x45, 0x4E, 0x44}  // IEND
};

typedef enum {
    PNG_CHUNKTYPE_IHDR = 1,
    PNG_CHUNKTYPE_PLTE,
    PNG_CHUNKTYPE_IDAT,
    PNG_CHUNKTYPE_IEND,
    PNG_CHUNKTYPE_UNKNOWN = 99
} PNG_CHUNKTYPE;

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
PNG_CHUNKTYPE
CheckChunkType(CHAR8* ChunkName){
    UINTN hit = 0;
    for(UINTN i = 0; i<4; i++){
        hit = 0;
        for(UINTN j = 0; j<4; j++){
            if(PNG_CHUNKHEX[i][j] == ChunkName[j]) hit++;
        }
        if(hit == 4){
            switch(i){
                case 0:
                    Print(L"PNG_CHUNKTYPE_IHDR\n");
                    return PNG_CHUNKTYPE_IHDR;
                case 1:
                    Print(L"PNG_CHUNKTYPE_PLTE\n");
                    return PNG_CHUNKTYPE_PLTE;
                case 2:
                    Print(L"PNG_CHUNKTYPE_IDAT\n");
                    return PNG_CHUNKTYPE_IDAT;
                case 3:
                    Print(L"PNG_CHUNKTYPE_IEND\n");
                    return PNG_CHUNKTYPE_IEND;
            }
        }
        Print(L"\n");
    }
    for(UINTN i = 0; i<4; i++){
        Print(L"%c", ChunkName[i]);
    }
    Print(L"\n");
    return PNG_CHUNKTYPE_UNKNOWN;
}

STATIC
EFI_STATUS
DrawPng(IN EFI_GRAPHICS_OUTPUT_PROTOCOL *GraphicsOutput, IN void *PngBuffer, IN UINTN PngSize){
    CHAR8                           PNG_FILE_SIGNATURE[] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};
	EFI_STATUS						Status = EFI_SUCCESS;
	EFI_GRAPHICS_OUTPUT_BLT_PIXEL	*BltBuffer;
    PNG_SIGNATURE                   *PngSignature;
    INTN                            PngIdx = 0;
    void                            *png_idat = AllocateZeroPool(MAX_BUFFER_SIZE);

    // Check Sigunature
    PngSignature = (PNG_SIGNATURE*)PngBuffer;

    for(UINTN i = 0; i < 8; i++){
        if(PngSignature->Signature[i] != PNG_FILE_SIGNATURE[i]){
            Print(L"PNG signature is invalid %d %x %x\n", i, PNG_FILE_SIGNATURE[i], PngSignature->Signature[i]);
            return EFI_UNSUPPORTED;
        }
        Print(L"%x(%x) ", PNG_FILE_SIGNATURE[i], PngSignature->Signature[i]);
    }
    Print(L"\n");
    PngIdx += 8;

    UINTN PngIdatOffset = 0;
    UINTN PngIdatStart = 0;
    UINTN Width = 0;
    
    while(PngIdx >= 0){
        PNG_CHUNK_HEADER *ChunkHeader = (PNG_CHUNK_HEADER*)(PngBuffer+PngIdx);
        PNG_CHUNKTYPE ChunkType = CheckChunkType(ChunkHeader->Type);

        UINT32 PngDataLength = SwapBit32(ChunkHeader->Length);
        Print(L"%x %d\n", PngDataLength, PngDataLength);

        PNG_CHUNK_IHDR *PngChunkIhdr;
        switch(ChunkType){
            case PNG_CHUNKTYPE_IHDR:
                PngChunkIhdr = (PNG_CHUNK_IHDR*)(PngBuffer+PngIdx);
                Width = SwapBit32(PngChunkIhdr->Width);
                Print(L"IHDR length: %d\n", SwapBit32(PngChunkIhdr->Length));
                Print(L"IHDR size: %d x %d\n", SwapBit32(PngChunkIhdr->Width), SwapBit32(PngChunkIhdr->Height));
                Print(L"IHDR depth: %d\n", PngChunkIhdr->Depth);
                Print(L"IHDR color type: %d\n", PngChunkIhdr->ColorType);
                Print(L"IHDR Compression type: %d\n", PngChunkIhdr->CompressionType);
                Print(L"IHDR filter type: %d\n", PngChunkIhdr->FilterType);
                Print(L"IHDR interrace type: %d\n", PngChunkIhdr->InterraceType);
                Print(L"IHDR crc: %x%x%x%x\n", PngChunkIhdr->Crc[0], PngChunkIhdr->Crc[0], PngChunkIhdr->Crc[2], PngChunkIhdr->Crc[3]);
                //UINT32 Crc;
                //Print(L"%r\n", uefi_call_wrapper(BS->CalculateCrc32, 3, PngBuffer+PngIdx+PNG_LENGTH_SIZE, PngChunkIhdr->Length+PNG_CHUNKTYPE_SIZE, &Crc));
                //Print(L"IHDR calc crc: %x\n", Crc);
                break;
            case PNG_CHUNKTYPE_PLTE:
                break;
            case PNG_CHUNKTYPE_IDAT: 
                if(PngIdatStart == 0){
                    PngIdatStart = PngIdx;
                }
                void *tmp = AllocatePool(PngDataLength);
                CopyMem(tmp, PngBuffer+PngIdx+PNG_LENGTH_SIZE, PngDataLength);
                CatenateMemory(png_idat, PngBuffer, PngDataLength, PngIdatOffset);
                FreePool(tmp);
                PngIdatOffset += PngDataLength;
                break;
            case PNG_CHUNKTYPE_IEND:
                PngIdx = -1;
                break;
            default:
                Print(L"Invalid chunk type or unsupported chunk\n");
                //return EFI_UNSUPPORTED;
        }
        if(PngIdx >= 0){
            PngIdx += PNG_LENGTH_SIZE + PNG_CHUNKTYPE_SIZE + PngDataLength + PNG_CRC_SIZE;
            Print(L"PngIdx = %d, PngDataLength = %d", PngIdx, PngDataLength);
        }
    
        EFI_INPUT_KEY Key;
        while ((Status = uefi_call_wrapper(ST->ConIn->ReadKeyStroke, 2, ST->ConIn, &Key)) == EFI_NOT_READY) ;
    }

    Width = 16;
    for(UINTN i = 0; i<MAX_BUFFER_SIZE; i++){
        CHAR8 *tmp = png_idat+i;
        UINTN t = i%Width;
        if(i/Width <= 10){
            if(i%Width==0 && i!=0){
                Print(L"\n%02x ", *tmp, *tmp);
            }else{
                Print(L"%02x ", *tmp, *tmp);
            }
        }
    }
    Print(L"\n\n");
    for(UINTN i = PngIdatStart; i<MAX_BUFFER_SIZE; i++){
        CHAR8 *tmp = png_idat+i;
        UINTN t = i%Width;
        if(i/Width <= 10+(PngIdatStart/Width)){
            if(i%Width==0 && i!=0){
                Print(L"\n%02x ", *tmp, *tmp);
            }else{
                Print(L"%02x ", *tmp, *tmp);
            }
        }
    }
    FreePool(png_idat);

    EFI_INPUT_KEY Key;
    while ((Status = uefi_call_wrapper(ST->ConIn->ReadKeyStroke, 2, ST->ConIn, &Key)) == EFI_NOT_READY) ;

    return Status;
}

STATIC
EFI_STATUS
DeflateDecompress(IN void *DeflateBuffer){
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

	//Status = uefi_call_wrapper(GraphicsOutput->Blt, 10, GraphicsOutput, BltBuffer, EfiBltBufferToVideo, 0, 0, 200, 200, BitmapWidth, BitmapHeight, 0);
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
