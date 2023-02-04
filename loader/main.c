#include <efi/efi.h>
#include <elf.h>
#include <stdint.h>

EFI_HANDLE HD;
EFI_SYSTEM_TABLE *ST;
EFI_BOOT_SERVICES *BS;

#define ALIGN_UP(addr, align) \
	(((addr) + (typeof (addr)) (align) - 1) & ~((typeof (addr)) (align) - 1))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define PAGE_SIZE (1 << 12)

extern void jump_to_kernel(UINT64);
extern void cpu_relax();

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

void
output_string(CHAR16 *str)
{
	uefi_call_wrapper(ST->ConOut->OutputString, 2, ST->ConOut, str);
}

void
output_string_ln(CHAR16 *str)
{
	uefi_call_wrapper(ST->ConOut->OutputString, 2, ST->ConOut, str);
	uefi_call_wrapper(ST->ConOut->OutputString, 2, ST->ConOut, L"\n\r");
}

/**
 * sleep some amount of ms time
 */
static void
sleep(int seconds)
{
	uefi_call_wrapper(ST->BootServices->Stall, 1, (UINTN) seconds * 1000000);
}

static void
spin(uint64_t t)
{
	uint64_t i;

	for (i = 0; i < (t << 20); ++i);
}

EFI_FILE_HANDLE
get_volume(EFI_HANDLE image)
{
	EFI_LOADED_IMAGE *loaded_image = NULL;
	EFI_GUID lip_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
	EFI_FILE_IO_INTERFACE *io_volume;
	EFI_GUID fs_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
	EFI_FILE_HANDLE volume;

	uefi_call_wrapper(BS->HandleProtocol, 3, image, &lip_guid, (void **) &loaded_image);
	uefi_call_wrapper(BS->HandleProtocol, 3, loaded_image->DeviceHandle,
		&fs_guid, (void **) &io_volume);
	uefi_call_wrapper(io_volume->OpenVolume, 2, io_volume, &volume);

	return volume;
}

void *
allocate_pool_with_type(UINTN size, EFI_MEMORY_TYPE type)
{
	EFI_STATUS status;
	void *p;

	status = uefi_call_wrapper(BS->AllocatePool, 3, type, size, &p);
	if (EFI_ERROR(status)) {
		p = NULL;
	}

	return p;
}

void *
allocate_pool(UINTN size)
{
	return allocate_pool_with_type(size, EfiBootServicesData);
}

BOOLEAN 
grow_buffer(EFI_STATUS *status, void **buffer, UINTN size)
{
	BOOLEAN try_again;

	if (!*buffer && size) {
		*status = EFI_BUFFER_TOO_SMALL;
	}

	try_again = FALSE;
	if (*status == EFI_BUFFER_TOO_SMALL) {
		if (*buffer) {
			uefi_call_wrapper(BS->FreePool, 1, buffer);
		}
		*buffer = allocate_pool(size);

		if (*buffer) {
			try_again = TRUE;
		} else {
			*status = EFI_OUT_OF_RESOURCES;
		}
	}

	if (!try_again && EFI_ERROR(*status) && *buffer) {
		uefi_call_wrapper(BS->FreePool, 1, buffer);
		*buffer = NULL;
	}

	return try_again;
}

EFI_FILE_INFO *
get_file_info(EFI_FILE_HANDLE handle)
{
	EFI_STATUS status;
	EFI_FILE_INFO *buffer;
	UINTN buffer_size;
	EFI_GUID GenericFileInfo = EFI_FILE_INFO_ID;

	status = EFI_SUCCESS;
	buffer = NULL;
	buffer_size = SIZE_OF_EFI_FILE_INFO + 200;

	while (grow_buffer(&status, (void **)&buffer, buffer_size)) {
		status = uefi_call_wrapper(handle->GetInfo, 4, handle,
			&GenericFileInfo, &buffer_size, buffer);
	}

	return buffer;
}

CHAR16 *
get_image_file_path(EFI_HANDLE image)
{
	EFI_LOADED_IMAGE *loaded_image = NULL;
	EFI_GUID lip_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
	EFI_FILE_IO_INTERFACE *io_volume;
	EFI_GUID fs_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
	EFI_FILE_HANDLE volume;

	uefi_call_wrapper(BS->HandleProtocol, 3, image, &lip_guid,
		(void **) &loaded_image);

	EFI_DEVICE_PATH *file_path = loaded_image->FilePath;
	if (file_path->Type == 4 && file_path->SubType == 4) {
		return (CHAR16 *) ((char *) file_path + 4);
	}
	return NULL;
}

CHAR16 *
get_parent_path(CHAR16 *path)
{
	int i, pos;
	CHAR16 *buffer;

	i = 0;
	pos = -1;
	while (*(path + i)) {
		if (*(path + i) == L'\\')
			pos = i;
		i++;
	}
	if (pos == -1)
		return NULL;

	buffer = allocate_pool((pos + 1) * 2);
	for (i = 0; i <= pos; ++i) {
		buffer[i] = *(path + i);
	}
	buffer[i] = 0;

	return buffer;
}

CHAR16 *
concat(CHAR16 *a, CHAR16 *b)
{
	int i, j, la, lb;
	CHAR16 *buffer;

	if (a == NULL || b == NULL)
		return NULL;
	
	la = 0;
	lb = 0;
	while (*(a + la++));
	while (*(b + lb++));

	buffer = allocate_pool(la + lb - 1);
	for (i = 0; i < la - 1; ++i) {
		buffer[i] = a[i];
	}
	for (j = 0; j < lb; ++i, ++j) {
		buffer[i] = b[j];
	}

	return buffer;
}

int
assert_elf64_hdr(char *hdr)
{
	int i;

	if (!hdr)
		return 0;
	
	char valid[] = {0x7f, 'E', 'L', 'F', 0x02, 0x01, 0x01, 0x00};
	for (i = 0; i < 8; ++i) {
		if (hdr[i] != valid[i])
			return 0;
	}

	return 1;
}

void *
allocate_pages_with_type(UINTN pages, EFI_MEMORY_TYPE type)
{
	EFI_PHYSICAL_ADDRESS address;
	EFI_STATUS status;

	status = uefi_call_wrapper(BS->AllocatePages, 4, AllocateAnyPages,
		type, pages, &address);

	if (status != EFI_SUCCESS) {
		return NULL;
	}
	return (void *) address;
}

void
load_kernel(EFI_FILE_HANDLE volume, uint64_t *addr, uint64_t *entry)
{
	CHAR16 *kernel;
	EFI_FILE_HANDLE handle;
	uint64_t kernel_size;
	char *buffer;

	Elf64_Ehdr *ehdr;
	Elf64_Phdr *phdr;
	Elf64_Shdr *shdr;
	uint64_t load_size;
	char *kmem;
	uint64_t i, j, p, q;

	*addr = 0;
	*entry = 0;
	CHAR16 *image_path = get_image_file_path(HD);
	if (!image_path) {
		output_string_ln(L"unable to locate kernel image");
		return;
	}

	kernel = concat(get_parent_path(image_path), L"kernel.img");
	output_string(L"loading kernel ");
	output_string(kernel);
	output_string_ln(L" ...");

	uefi_call_wrapper(volume->Open, 5, volume, &handle, kernel, EFI_FILE_MODE_READ,
		EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
	kernel_size = get_file_info(handle)->FileSize;
	output_string(L"kernel size: ");
	output_string_ln(i2u(kernel_size));

	buffer = allocate_pool_with_type(kernel_size, EfiLoaderData);
	uefi_call_wrapper(handle->Read, 3, handle, &kernel_size, buffer);

	ehdr = (Elf64_Ehdr *) buffer;
	phdr = (Elf64_Phdr *) (buffer + ehdr->e_phoff);
	if (!assert_elf64_hdr(ehdr->e_ident)) {
		output_string_ln(L"invalid kernel.img header");
		return;
	}

	load_size = 0;
	if (ehdr->e_phnum <= 0) {
		output_string_ln(L"no program section to load");
		return;
	}
	for (i = 0; i < ehdr->e_phnum; ++i) {
		phdr = (Elf64_Phdr *) ((void *) phdr + i * ehdr->e_phentsize);
		if (phdr->p_type == PT_LOAD) {
			load_size = MAX(load_size, phdr->p_vaddr + phdr->p_memsz);
		}
	}
	load_size = ALIGN_UP(load_size, PAGE_SIZE);
	kmem = allocate_pages_with_type(load_size / PAGE_SIZE, EfiLoaderCode);
	output_string(L"load size: ");
	output_string_ln(i2u(load_size));
	if (!kmem) {
		output_string_ln(L"unable to allocate memory to load the kernel");
		return;
	}
	for (i = 0; i < load_size / 8; ++i) {
		*((uint64_t *) kmem + i) = 0;
	}

	shdr = (Elf64_Shdr *) (buffer + ehdr->e_shoff);
	for (i = 0; i < ehdr->e_shnum; ++i) {
		shdr = (Elf64_Shdr *) ((void *) shdr + ehdr->e_shentsize);
		if ((shdr->sh_flags & SHF_ALLOC) && (shdr->sh_type == SHT_PROGBITS)) {
			output_string(L"copy size: ");
			output_string_ln(i2u(shdr->sh_size));
			for (j = 0, p = shdr->sh_offset, q = shdr->sh_addr;
				j < shdr->sh_size; j++, p++, q++) {
				*(kmem + q) = *(buffer + p);
			}
		}
	}

	*addr = (uint64_t) kmem;
	*entry = (uint64_t) (kmem + ehdr->e_entry);
}

void
efi_exit_boot_services()
{
	UINTN m_key, buffer_size, dsc_size;
	EFI_MEMORY_DESCRIPTOR *buffer;
	UINT32 dsc_version;
	EFI_STATUS status, status2;

	buffer_size = 1;
	buffer = allocate_pool_with_type(buffer_size, EfiLoaderData);
	while (status2 != EFI_SUCCESS) {
		while (grow_buffer(&status, (void **) &buffer, buffer_size)) {
			status = uefi_call_wrapper(BS->GetMemoryMap, 5,
				&buffer_size, buffer, &m_key, &dsc_size, &dsc_version);
		}
		status2 = uefi_call_wrapper(BS->ExitBootServices, 2, HD, m_key);
	}
}

EFI_GRAPHICS_OUTPUT_PROTOCOL *
video_setup()
{
	SIMPLE_TEXT_OUTPUT_INTERFACE *ConOut;
	EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
	UINTN sizeOfInfo, numModes, nativeMode, mode;
	EFI_STATUS status;
	UINT32 preferredMode;

	ConOut = ST->ConOut;

	EFI_GUID gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
	status = uefi_call_wrapper(ST->BootServices->LocateProtocol, 3,
		&gopGuid, NULL, (void **) &gop);
	if (EFI_ERROR(status)) {
		output_string_ln(L"unable to locate GOP\n\r");
		return NULL;
	}

	status = uefi_call_wrapper(gop->QueryMode, 4, gop,
		gop->Mode == NULL ? 0: gop->Mode->Mode, &sizeOfInfo, &info);
	if (status == EFI_NOT_STARTED) {
		output_string_ln(L"setting to mode 0\n\r");
		status = uefi_call_wrapper(gop->SetMode, 2, gop, 0);
	}
	if (EFI_ERROR(status)) {
		output_string_ln(L"unable to get native mode\n\r");
		return NULL;
	}
	nativeMode = gop->Mode->Mode;
	numModes = gop->Mode->MaxMode;

	/* setting up the prefered mode */
	preferredMode = nativeMode;
	for (mode = 0; mode < numModes; ++mode) {
		status = uefi_call_wrapper(gop->QueryMode, 4,
			gop, mode, &sizeOfInfo, &info);
		if (EFI_ERROR(status)) {
			output_string(L"unable to query mode: ");
			output_string_ln(i2u(mode));
			return NULL;
		}

		if (info->HorizontalResolution == 1920 &&
			info->VerticalResolution == 1080) {
			/* 1080p is good */
			preferredMode = mode;
		}
		if (info->HorizontalResolution == 3840 &&
			info->VerticalResolution == 2160) {
			/* 4k is better */
			preferredMode = mode;
			break;
		}
	}
	status = uefi_call_wrapper(gop->SetMode, 2, gop, preferredMode);

	return gop;
}

void
fill_video_params(void *kmem, EFI_GRAPHICS_OUTPUT_PROTOCOL *gop)
{
	uint64_t *head = (uint64_t *) kmem;

	head[0] = gop->Mode->FrameBufferBase;
	head[1] = gop->Mode->FrameBufferSize;
	head[2] = gop->Mode->Info->HorizontalResolution;
	head[3] = gop->Mode->Info->VerticalResolution;
	head[4] = gop->Mode->Info->PixelsPerScanLine;
}

int struct_cmp(void *a, void *b, int size) {
	int i;
	char *x, *y;

	x = (char *) a;
	y = (char *) b;
	for (i = 0; i < size; ++i) {
		if (x[i] == y[i]) {
			continue;
		}
		return x[i] - y[i];
	}

	return 0;
}

void
fill_acpi_params(void *kmem)
{
	uint64_t *head = (uint64_t *) kmem;
	EFI_GUID acpi_1 = ACPI_TABLE_GUID;
	EFI_GUID acpi_2 = ACPI_20_TABLE_GUID;
	int i;

	head[5] = 0;
	for (i = 0; i < ST->NumberOfTableEntries; ++i) {
		EFI_GUID guid = ST->ConfigurationTable[i].VendorGuid;
		if (!struct_cmp(&guid, &acpi_1, sizeof(EFI_GUID))) {
			head[5] = 1;
			head[6] = (uint64_t) (ST->ConfigurationTable[i].VendorTable);
		}
		if (!struct_cmp(&guid, &acpi_2, sizeof(EFI_GUID))) {
			head[5] = 2;
			head[6] = (uint64_t) (ST->ConfigurationTable[i].VendorTable);
			break;
		}
	}

	if (head[5] == 0) {
		output_string_ln(L"Unable to locate acpi table rsdp\n\r");
		cpu_relax();
	}
}

void
efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *efi_system_table)
{
	HD = image_handle;
	ST = efi_system_table;
	BS = ST->BootServices;

	EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
	EFI_FILE_HANDLE volume;
	UINT64 kernel_address;
	UINT64 kernel_entry;
	int i, j;

	uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);
	gop = video_setup();
	if (!gop) {
		output_string_ln(L"error setting up the video mode");
		return;
	}

	volume = get_volume(image_handle);
	load_kernel(volume, &kernel_address, &kernel_entry);
	if (!kernel_address) {
		output_string_ln(L"error loading kernel image");
		return;
	}

	output_string(L"framebuffer base: ");
	output_string_ln(i2h(gop->Mode->FrameBufferBase));

	output_string(L"kernel_address: ");
	output_string_ln(i2h(kernel_address));

	fill_video_params((void *) kernel_address, gop);
	fill_acpi_params((void *) kernel_address);

	output_string_ln(L"jumping to kernel ...");

	efi_exit_boot_services();
	jump_to_kernel(kernel_entry);
}
