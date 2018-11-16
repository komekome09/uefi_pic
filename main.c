#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#include "main.h"

EFI_STATUS
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable){
	EFI_GUID						EfiGraphicsOutputProtocolGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
	EFI_STATUS 						Status = EFI_SUCCESS;
	EFI_GRAPHICS_OUTPUT_PROTOCOL	*GraphicsOutput;

	CHAR16	FileName[] = L"nanagi_8.bmp";
	CHAR16	FileName2[] = L"nanagi.png";
	CHAR16	FileName3[] = L"ruru.jpg";

	InitializeLib(ImageHandle, SystemTable);

	LibLocateProtocol(&EfiGraphicsOutputProtocolGuid, (void **)&GraphicsOutput);

	VOID	*BmpBuffer = NULL;
	UINTN	BmpSize;
	Status = LoadImageFile(ImageHandle,  FileName, &BmpBuffer, &BmpSize);
	if(EFI_ERROR(Status)){
		if(BmpBuffer != NULL){
			FreePool(BmpBuffer);
		}
		Print(L"Load Image failed.\n");
		return Status;
	}

	ShowQueryMode(GraphicsOutput);

	Status = DrawImage(GraphicsOutput, BmpBuffer, BmpSize);
	if(EFI_ERROR(Status)){
		Print(L"Draw bmp failed.\n");
		return Status;
	}

	if(BmpBuffer != NULL){
		FreePool(BmpBuffer);
	}

	VOID	*PngBuffer = NULL;
	UINTN	PngSize;
    Status = LoadImageFile(ImageHandle, FileName2, &PngBuffer, &PngSize);
	if(EFI_ERROR(Status)){
		if(PngBuffer != NULL){
			FreePool(PngBuffer);
		}
		Print(L"Load Image failed.\n");
		return Status;
	}
    Status = DrawImage(GraphicsOutput, PngBuffer, PngSize);
	if(EFI_ERROR(Status)){
		Print(L"Draw Png failed. %r(%d)\n", Status, Status);
	}

    if(PngBuffer != NULL){
        FreePool(PngBuffer);
    }
	
	VOID	*JpgBuffer = NULL;
	UINTN	JpgSize;
	Status = LoadImageFile(ImageHandle, FileName3, &JpgBuffer, &JpgSize);
	if(EFI_ERROR(Status)){
		if(JpgBuffer != NULL) FreePool(JpgBuffer);
		Print(L"Load Image failed\n");
		return Status;
	}
	Status = DrawImage(GraphicsOutput, JpgBuffer, JpgSize);
	if(EFI_ERROR(Status)){
		Print(L"Draw jpg failed. %r(%d)\n", Status, Status);
	}

	if(JpgBuffer != NULL) FreePool(JpgBuffer);

    while(1){
        WaitForSingleEvent(ST->ConIn->WaitForKey, 10000000);
    }

	return EFI_SUCCESS;
}
