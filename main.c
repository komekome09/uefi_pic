#include "main.h"

EFI_STATUS
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable){
	EFI_GUID						EfiGraphicsOutputProtocolGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
	EFI_STATUS 						Status = EFI_SUCCESS;
	EFI_GRAPHICS_OUTPUT_PROTOCOL	*GraphicsOutput;
	VOID							*BmpBuffer = NULL;
	UINTN							BmpSize;
	EFI_INPUT_KEY					Key;

	InitializeLib(ImageHandle, SystemTable);

	LibLocateProtocol(&EfiGraphicsOutputProtocolGuid, (void **)&GraphicsOutput);
	
	Status = LoadBitmapFile(ImageHandle,  L"Logo.bmp", &BmpBuffer, &BmpSize);
	if(EFI_ERROR(Status)){
		if(BmpBuffer != NULL){
			FreePool(BmpBuffer);
		}
		Print(L"Load bitmap failed.\n");
		return Status;
	}

	Status = DrawBmp(GraphicsOutput, BmpBuffer, BmpSize);
	if(EFI_ERROR(Status)){
		Print(L"Draw bmp failed.\n");
		return Status;
	}

	if(BmpBuffer != NULL){
		FreePool(BmpBuffer);
	}

	Print(L"Hello, world!!\n");
	while ((Status = ST->ConIn->ReadKeyStroke(ST->ConIn, &Key)) == EFI_NOT_READY) ;

	return EFI_SUCCESS;
}
