#include <efi/efi.h>

EFI_HANDLE HD;
EFI_SYSTEM_TABLE *ST;

static CHAR16 *
a2u(char *str)
{
	static CHAR16 mem[2048];
	int i;

	for (i = 0; str[i]; ++i)
		mem[i] = (CHAR16) str[i];
	mem[i] = 0;
	return mem;
}

static CHAR16 *
i2u(UINT64 x)
{
	static CHAR16 mem[32];
	int i, j;

	i = 0;
	do {
		CHAR16 r = (CHAR16) (x % 10);
		x = x / 10;
		mem[i++] = r + 48;
	} while (x > 0);
	mem[i] = (CHAR16) 0;

	for (i--, j = 0; j < i; ++j, --i) {
		CHAR16 tmp = mem[i];
		mem[i] = mem[j];
		mem[j] = tmp;
	}

	return mem;
}

static CHAR16 *
i2h(UINT64 x)
{
	static CHAR16 mem[32];
	int i, j;

	i = 0;
	do {
		CHAR16 r = (CHAR16) (x % 16);
		x = x / 16;
		mem[i++] = (r < 10) ? r + 48 : r + 55;
	} while (x > 0);
	mem[i] = (CHAR16) 0;

	for (i--, j = 0; j < i; ++j, --i) {
		CHAR16 tmp = mem[i];
		mem[i] = mem[j];
		mem[j] = tmp;
	}

	return mem;
}

/**
 * sleep some amount of time
 */
static void
sleep(int seconds)
{
	uefi_call_wrapper(ST->BootServices->Stall, 1, (UINTN) seconds * 1000000);
}

void
peek_memory_map()
{
	SIMPLE_TEXT_OUTPUT_INTERFACE *ConOut;
	UINTN mKey;
	UINTN dSize;
	UINT32 dVersion;
	UINTN mmSize;
	EFI_MEMORY_DESCRIPTOR *mm;
	EFI_STATUS status;

	ConOut = ST->ConOut;
	char mem[8192];
	mmSize = 8192;
	mm = (EFI_MEMORY_DESCRIPTOR *) mem;

	status = uefi_call_wrapper(ST->BootServices->GetMemoryMap, 5,
		&mmSize, mm, &mKey, &dSize, &dVersion);
	if (status == EFI_BUFFER_TOO_SMALL) {
		uefi_call_wrapper(ConOut->OutputString, 2, ConOut, a2u("Buffer too small\n\r"));
		return;
	}

	for (int i = 0; i < mmSize / dSize; ++i) {
		if (i != 0 && i % 24 == 0) {
			sleep(5);
		}
		EFI_MEMORY_DESCRIPTOR *d = (EFI_MEMORY_DESCRIPTOR*) ((char *) mm + (dSize * i));

		uefi_call_wrapper(ConOut->OutputString, 2, ConOut, a2u("Type:\t"));
		switch (d->Type) {
		case EfiReservedMemoryType:
			uefi_call_wrapper(ConOut->OutputString, 2, ConOut, a2u("ReservedMemory         \t"));
			break;
		case EfiLoaderCode:
			uefi_call_wrapper(ConOut->OutputString, 2, ConOut, a2u("LoaderCode             \t"));
			break;
		case EfiLoaderData:
			uefi_call_wrapper(ConOut->OutputString, 2, ConOut, a2u("LoaderData             \t"));
			break;
		case EfiBootServicesCode:
			uefi_call_wrapper(ConOut->OutputString, 2, ConOut, a2u("BootServicesCode       \t"));
			break;
		case EfiBootServicesData:
			uefi_call_wrapper(ConOut->OutputString, 2, ConOut, a2u("BootServicesData       \t"));
			break;
		case EfiRuntimeServicesCode:
			uefi_call_wrapper(ConOut->OutputString, 2, ConOut, a2u("RuntimeServicesCode    \t"));
			break;
		case EfiRuntimeServicesData:
			uefi_call_wrapper(ConOut->OutputString, 2, ConOut, a2u("RuntimeServicesData    \t"));
			break;
		case EfiConventionalMemory:
			uefi_call_wrapper(ConOut->OutputString, 2, ConOut, a2u("Conventional           \t"));
			break;
		case EfiUnusableMemory:
			uefi_call_wrapper(ConOut->OutputString, 2, ConOut, a2u("Unusable               \t"));
			break;
		case EfiACPIReclaimMemory:
			uefi_call_wrapper(ConOut->OutputString, 2, ConOut, a2u("ACPIReclaim            \t"));
			break;
		case EfiACPIMemoryNVS:
			uefi_call_wrapper(ConOut->OutputString, 2, ConOut, a2u("ACPIMemoryNVS          \t"));
			break;
		case EfiMemoryMappedIO:
			uefi_call_wrapper(ConOut->OutputString, 2, ConOut, a2u("MemoryMappedIO         \t"));
			break;
		case EfiMemoryMappedIOPortSpace:
			uefi_call_wrapper(ConOut->OutputString, 2, ConOut, a2u("MemoryMappedIOPortSpace\t"));
			break;
		case EfiPalCode:
			uefi_call_wrapper(ConOut->OutputString, 2, ConOut, a2u("PalCode                \t"));
			break;
		default:
			uefi_call_wrapper(ConOut->OutputString, 2, ConOut, a2u("UnkownType             \t"));
			break;
		}

		uefi_call_wrapper(ConOut->OutputString, 2, ConOut, i2u(d->PhysicalStart));
		uefi_call_wrapper(ConOut->OutputString, 2, ConOut, a2u("\t"));
		uefi_call_wrapper(ConOut->OutputString, 2, ConOut, i2u(d->VirtualStart));
		uefi_call_wrapper(ConOut->OutputString, 2, ConOut, a2u("\t"));
		uefi_call_wrapper(ConOut->OutputString, 2, ConOut, i2u(d->NumberOfPages));
		uefi_call_wrapper(ConOut->OutputString, 2, ConOut, a2u("\t"));
		uefi_call_wrapper(ConOut->OutputString, 2, ConOut, i2h(d->Attribute));
		uefi_call_wrapper(ConOut->OutputString, 2, ConOut, a2u("\n\r"));
	}
}

void
efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *efi_system_table)
{
	SIMPLE_TEXT_OUTPUT_INTERFACE *ConOut;
	EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
	UINTN sizeOfInfo, numModes, nativeMode, mode;
	EFI_STATUS status;

	HD = image_handle;
	ST = efi_system_table;
	ConOut = ST->ConOut;
	uefi_call_wrapper(ConOut->ClearScreen, 1, ConOut);

	EFI_GUID gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
	status = uefi_call_wrapper(ST->BootServices->LocateProtocol, 3, &gopGuid, NULL, (void **) &gop);
	if (EFI_ERROR(status)) {
		uefi_call_wrapper(ConOut->OutputString, 2, ConOut, a2u("Unable to locate GOP\n\r"));
		return;
	}

	status = uefi_call_wrapper(gop->QueryMode, 4, gop, gop->Mode == NULL ? 0: gop->Mode->Mode, &sizeOfInfo, &info);
	if (status == EFI_NOT_STARTED) {
		uefi_call_wrapper(ConOut->OutputString, 2, ConOut, a2u("Setting to mode 0\n\r"));
		status = uefi_call_wrapper(gop->SetMode, 2, gop, 0);
	}
	if (EFI_ERROR(status)) {
		uefi_call_wrapper(ConOut->OutputString, 2, ConOut, a2u("Unable to get native mode\n\r"));
		return;
	}
	nativeMode = gop->Mode->Mode;
	numModes = gop->Mode->MaxMode;
	uefi_call_wrapper(ConOut->OutputString, 2, ConOut, a2u("Native mode: "));
	uefi_call_wrapper(ConOut->OutputString, 2, ConOut, i2u(nativeMode));
	uefi_call_wrapper(ConOut->OutputString, 2, ConOut, a2u("\n\r"));
	uefi_call_wrapper(ConOut->OutputString, 2, ConOut, a2u("Num modes: "));
	uefi_call_wrapper(ConOut->OutputString, 2, ConOut, i2u(numModes));
	uefi_call_wrapper(ConOut->OutputString, 2, ConOut, a2u("\n\r"));

	for (mode = 0; 1; mode = ++mode % numModes) {
		uefi_call_wrapper(ConOut->ClearScreen, 1, ConOut);
		status = uefi_call_wrapper(gop->SetMode, 2, gop, mode);
		if (EFI_ERROR(status)) {
			uefi_call_wrapper(ConOut->OutputString, 2, ConOut, a2u("Unable to set mode: "));
			uefi_call_wrapper(ConOut->OutputString, 2, ConOut, i2u(mode));
			uefi_call_wrapper(ConOut->OutputString, 2, ConOut, a2u("\n\r"));
			return;
		}
		uefi_call_wrapper(ConOut->OutputString, 2, ConOut, a2u("Mode: "));
		uefi_call_wrapper(ConOut->OutputString, 2, ConOut, i2u(mode));
		uefi_call_wrapper(ConOut->OutputString, 2, ConOut, a2u("\n\r"));
		uefi_call_wrapper(ConOut->OutputString, 2, ConOut, a2u("Framebuffer address: "));
		uefi_call_wrapper(ConOut->OutputString, 2, ConOut, i2h(gop->Mode->FrameBufferBase));
		uefi_call_wrapper(ConOut->OutputString, 2, ConOut, a2u("\n\r"));
		uefi_call_wrapper(ConOut->OutputString, 2, ConOut, a2u("Framebuffer size: "));
		uefi_call_wrapper(ConOut->OutputString, 2, ConOut, i2u(gop->Mode->FrameBufferSize));
		uefi_call_wrapper(ConOut->OutputString, 2, ConOut, a2u("\n\r"));
		uefi_call_wrapper(ConOut->OutputString, 2, ConOut, a2u("HorizontalResolution: "));
		uefi_call_wrapper(ConOut->OutputString, 2, ConOut, i2u(gop->Mode->Info->HorizontalResolution));
		uefi_call_wrapper(ConOut->OutputString, 2, ConOut, a2u("\n\r"));
		uefi_call_wrapper(ConOut->OutputString, 2, ConOut, a2u("VerticalResolution: "));
		uefi_call_wrapper(ConOut->OutputString, 2, ConOut, i2u(gop->Mode->Info->VerticalResolution));
		uefi_call_wrapper(ConOut->OutputString, 2, ConOut, a2u("\n\r"));
		sleep(2);
	}

	return;
}
