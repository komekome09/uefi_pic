#include "main.h"

EFI_STATUS
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable){
	EFI_GUID						EfiGraphicsOutputProtocolGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
	EFI_STATUS 						Status = EFI_SUCCESS;
	EFI_GRAPHICS_OUTPUT_PROTOCOL	*GraphicsOutput;
	VOID							*BmpBuffer = NULL;
	UINTN							BmpSize;
	VOID							*PngBuffer = NULL;
	UINTN							PngSize;
	EFI_INPUT_KEY					Key;
	CHAR16							FileName[] = L"nanagi_8.bmp";
	CHAR16							FileName2[] = L"nanagi.png";

	InitializeLib(ImageHandle, SystemTable);

	LibLocateProtocol(&EfiGraphicsOutputProtocolGuid, (void **)&GraphicsOutput);
	
	Status = LoadImageFile(ImageHandle,  FileName, &BmpBuffer, &BmpSize);
	if(EFI_ERROR(Status)){
		if(BmpBuffer != NULL){
			FreePool(BmpBuffer);
		}
		Print(L"Load Image failed.\n");
		return Status;
	}

	ShowQueryMode(GraphicsOutput);

	Status = DrawBmp(GraphicsOutput, BmpBuffer, BmpSize);
	if(EFI_ERROR(Status)){
		Print(L"Draw bmp failed.\n");
		return Status;
	}

	if(BmpBuffer != NULL){
		FreePool(BmpBuffer);
	}

    Status = LoadImageFile(ImageHandle, FileName2, &PngBuffer, &PngSize);
	if(EFI_ERROR(Status)){
		if(PngBuffer != NULL){
			FreePool(PngBuffer);
		}
		Print(L"Load Image failed.\n");
		return Status;
	}
    Status = DrawPng(GraphicsOutput, PngBuffer, PngSize);
	if(EFI_ERROR(Status)){
		Print(L"Draw Png failed.\n");
		return Status;
	}

    if(PngBuffer != NULL){
        FreePool(PngBuffer);
    }

	while ((Status = ST->ConIn->ReadKeyStroke(ST->ConIn, &Key)) == EFI_NOT_READY) ;

	return EFI_SUCCESS;
}
