#include "main.h"

EFI_STATUS
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable){
	EFI_GUID						EfiGraphicsOutputProtocolGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
	EFI_STATUS 						Status = EFI_SUCCESS;
	EFI_GRAPHICS_OUTPUT_PROTOCOL	*GraphicsOutput;
	VOID							*PicBuffer = NULL;
	UINTN							PicSize;
	EFI_INPUT_KEY					Key;
	CHAR16							FileName[] = L"nanagi.png";

	InitializeLib(ImageHandle, SystemTable);

	LibLocateProtocol(&EfiGraphicsOutputProtocolGuid, (void **)&GraphicsOutput);
	
	Status = LoadPicFile(ImageHandle,  FileName, &PicBuffer, &PicSize);
	if(EFI_ERROR(Status)){
		if(PicBuffer != NULL){
			FreePool(PicBuffer);
		}
		Print(L"Load bitmap failed.\n");
		return Status;
	}

	ShowQueryMode(GraphicsOutput);

	Status = DrawPng(GraphicsOutput, PicBuffer, PicSize);
	//Status = DrawBmp(GraphicsOutput, PicBuffer, PicSize);
	if(EFI_ERROR(Status)){
		Print(L"Draw picture failed.\n");
		return Status;
	}

	if(PicBuffer != NULL){
		FreePool(PicBuffer);
	}

	while ((Status = ST->ConIn->ReadKeyStroke(ST->ConIn, &Key)) == EFI_NOT_READY) ;

	return EFI_SUCCESS;
}
