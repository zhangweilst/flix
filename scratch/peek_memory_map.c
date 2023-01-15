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
			sleep(5 * 1000);
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
