#include "main.h"

EFI_STATUS
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable){
	EFI_GUID						EfiGraphicsOutputProtocolGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
	EFI_STATUS 						Status = EFI_SUCCESS;
	EFI_GRAPHICS_OUTPUT_PROTOCOL	GraphicsOutput;
	VOID							*BmpBuffer = NULL;
	UINTN							BmpSize;

	InitializeLib(ImageHandle, SystemTable);

	LibLocateProtocol(&EfiGraphicsOutputProtocolGuid, (VOID **)&GraphicsOutput);
	
	Status = LoadBitmapFile(L"Logo.bmp", &BmpBuffer, &BmpSize);

	Print(L"Hello, world!!\n");

	return EFI_SUCCESS;
}
