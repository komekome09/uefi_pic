ARCH 		= x86_64
EFIROOT 	= /usr/local
EFILIB		= $(EFIROOT)/lib
HDDRROOT 	= $(EFIROOT)/include/efi
INCLUDES 	= -I. -I$(HDDRROOT) -I$(HDDRROOT)/$(ARCH) -I$(HDDRROOT)/protocol -I$(HOME)/opt/include

CRTOBJS 	= $(EFILIB)/crt0-efi-$(ARCH).o
CFLAGS 		= -O2 -std=gnu11 -fpic -Wall -fshort-wchar -fno-strict-aliasing -fno-merge-constants -mno-red-zone -ffreestanding
ifeq ($(ARCH),x86_64)
	CFLAGS += -m64 -DEFI_FUNCTION_WRAPPER
endif

CPPFLAGS 	= -DCONFIG_$(ARCH)
FORMAT		= efi-app-$(ARCH)
INSTALL		= install
LDFLAGS		= -nostdlib
LDSCRIPTS 	= -T $(EFILIB)/elf_$(ARCH)_efi.lds
LDFLAGS 	+= $(LDSCRIPTS) -shared -Bsymbolic -L$(EFILIB) $(CRTOBJS)
LOADLIBS 	= -lefi -lgnuefi $(shell $(CC) -print-libgcc-file-name)

prefix 		= #$(ARCH)-w64-mingw32-
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
	$(LD) $(LDFLAGS) $^ -o $@ $(LOADLIBS)

%.o: %.c %.h stb_image.h
	$(CC) $(INCLUDES) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

TARGETS = main.efi

all: qemu 

qemu: $(TARGETS) OVMF/OVMF.fd image/EFI/BOOT/BOOTX64.EFI
	#qemu-system-x86_64 -L OVMF/ -bios OVMF/OVMF.fd -hda fat:image -vnc 192.168.1.9:3
	qemu-system-x86_64 -L OVMF/ -bios OVMF/OVMF.fd -hda fat:image 

image/EFI/BOOT/BOOTX64.EFI:
	mkdir -p image/EFI/BOOT
	ln -sf ../../../main.efi image/EFI/BOOT/BOOTX64.EFI

OVMF/OVMF.fd:
	mkdir OVMF
	wget -nc http://downloads.sourceforge.net/project/edk2/OVMF/OVMF-X64-r15214.zip
	unzip OVMF-X64-r15214.zip OVMF.fd -d OVMF
	rm OVMF-X64-r15214.zip

clean:
	rm -f main.efi main.so main.o
