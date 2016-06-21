#include "main.h"

EFI_STATUS
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable){
	EFI_GUID						EfiGraphicsOutputProtocolGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
	EFI_STATUS 						Status = EFI_SUCCESS;
	EFI_GRAPHICS_OUTPUT_PROTOCOL	*GraphicsOutput;
	VOID							*BmpBuffer = NULL;
	UINTN							BmpSize;
	EFI_INPUT_KEY					Key;
	CHAR16							FileName[] = L"ruru.bmp";

	InitializeLib(ImageHandle, SystemTable);

	LibLocateProtocol(&EfiGraphicsOutputProtocolGuid, (void **)&GraphicsOutput);
	
	Status = LoadBitmapFile(ImageHandle,  FileName, &BmpBuffer, &BmpSize);
	if(EFI_ERROR(Status)){
		if(BmpBuffer != NULL){
			FreePool(BmpBuffer);
		}
		Print(L"Load bitmap failed.\n");
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

	while ((Status = ST->ConIn->ReadKeyStroke(ST->ConIn, &Key)) == EFI_NOT_READY) ;

	return EFI_SUCCESS;
}
