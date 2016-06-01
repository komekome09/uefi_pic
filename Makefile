ARCH 		= x86_64
EFIROOT 	= /usr
EFILIB		= $(EFIROOT)/lib64
HDDRROOT 	= $(EFIROOT)/include/efi
INCLUDES 	= -I. -I$(HDDRROOT) -I$(HDDRROOT)/$(ARCH) -I$(HDDRROOT)/protocol

CRTOBJS 	= $(EFILIB)/crt0-efi-$(ARCH).o
CFLAGS 		= -g -O0 -fpic -Wall -fshort-wchar -fno-strict-aliasing -fno-merge-constants -mno-red-zone -e efi_main
ifeq ($(ARCH),x86_64)
	CFLAGS += -DEFI_FUNCTION_WRAPPER
endif

CPPFLAGS 	= -DCONFIG_$(ARCH)
FORMAT		= efi-app-$(ARCH)
INSTALL		= install
LDFLAGS		= -nostdlib
LDSCRIPTS 	= -T $(EFILIB)/elf_$(ARCH)_efi.lds
LDFLAGS 	+= $(LDSCRIPTS) -shared -Bsymbolic -L$(EFILIB) $(CRTOBJS) 
LOADLIBS 	= -lefi -lgnuefi $(shell $(CC) -print-libgcc-file-name)

prefix 		= $(ARCH)-w64-mingw32-
CC 			= $(prefix)gcc
AS 			= $(prefix)as
LD 			= $(prefix)ld
AR 			= $(prefix)ar
RANLIB 		= $(prefix)ranlib
OBJCOPY 	= $(prefix)objcopy

%.efi: %.so
	$(OBJCOPY) -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel \
				   -j .rela -j .reloc --target=$(FORMAT) $*.so $@

%.so: %.o
	$(LD) -t $(LDFLAGS) $^ -o $@ $(LOADLIBS)

%.o: %.c
	$(CC) $(INCLUDES) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

TARGETS = main.efi

all: qemu 

qemu: $(TARGETS) OVMF/OVMF.fd BOOT.EFI
	qemu-system-x86_64 -nographic -L OVMF/ -bios OVMF/OVMF.fd -hda fat:image

BOOT.EFI:
	mkdir -p image/EFI/BOOT
	ln -sf ../../../main.efi image/EFI/BOOT/BOOTX64.EFI

OVMF/OVMF.fd:
	mkdir OVMF
	wget -nc http://downloads.sourceforge.net/project/edk2/OVMF/OVMF-X64-r15214.zip
	unzip OVMF-X64-r15214.zip OVMF.fd -d OVMF

clean:
	rm -f main.efi
