/**
 * Binary File to PSF1 (PSX-EXE) Converter
 */

#define APP_NAME    "bin2psf1"
#define APP_VER     "[2015-03-15]"
#define APP_DESC	"Convert binary file into a PSF1 format"
#define APP_AUTHOR	"loveemu <http://github.com/loveemu/bin2psf1>"

#ifdef _WIN32
#define ZLIB_WINAPI
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <string>
#include <sstream>
#include <map>

#include <zlib.h>

#include "bin2psf1.h"
#include "cbyteio.h"
#include "cpath.h"
#include "PSFFile.h"

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <sys/stat.h>
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#define _chdir(s)	chdir((s))
#define _mkdir(s)	mkdir((s), 0777)
#define _rmdir(s)	rmdir((s))
#endif

#define PSF1_PSF_VERSION	0x01
#define PSF_EXE_HEADER_SIZE	0x800

#define MAX_PSX_ROM_SIZE	0x200000
#define PSX_ROM_ALIGN_SIZE	0x800

void printUsage(const char *cmd)
{
	const char *availableOptions[] = {
		"--help", "Show this help",
	};

	printf("%s %s\n", APP_NAME, APP_VER);
	printf("======================\n");
	printf("\n");
	printf("%s. Created by %s.\n", APP_DESC, APP_AUTHOR);
	printf("\n");
	printf("Usage\n");
	printf("-----\n");
	printf("\n");
	printf("Syntax: `%s <Input file> <Start address (hex)> <Initial PC (hex)> <Initial SP (hex)>`\n", cmd);
	printf("\n");
	printf("Note: Padding bytes will be added unless input file size is a multiple of 2048 bytes.\n");
	printf("\n");
	printf("### Options ###\n");
	printf("\n");

	for (int i = 0; i < sizeof(availableOptions) / sizeof(availableOptions[0]); i += 2) {
		printf("%s\n", availableOptions[i]);
		printf("  : %s\n", availableOptions[i + 1]);
		printf("\n");
	}
}

bool bin2psf1(const char * bin_path, const char * psf_path, uint32_t load_address, uint32_t initial_pc, uint32_t initial_sp, const char * region_name, std::map<std::string, std::string> tags)
{
	uint32_t load_offset = load_address & 0xffffff;

	if ((load_address >> 24) == 0) {
		load_address |= 0x80000000;
	}

	if ((load_address >> 24) != 0x80 || load_offset >= MAX_PSX_ROM_SIZE) {
		fprintf(stderr, "Error: Invalid load address 0x%08X\n", load_address);
		return false;
	}

	off_t bin_size = path_getfilesize(bin_path);
	if (bin_size == -1) {
		fprintf(stderr, "Error: File size error \"%s\"\n", bin_path);
		return false;
	}

	if (bin_size > MAX_PSX_ROM_SIZE) {
		fprintf(stderr, "Error: File too large \"%s\"\n", bin_path);
		return false;
	}

	uint32_t load_size = (uint32_t)bin_size;
	load_size = (load_size + PSX_ROM_ALIGN_SIZE - 1) / PSX_ROM_ALIGN_SIZE * PSX_ROM_ALIGN_SIZE;

	if (load_offset + load_size > MAX_PSX_ROM_SIZE) {
		fprintf(stderr, "Error: Address out of range (0x%08X + 0x%X) \"%s\"\n", load_address, load_size, bin_path);
		return false;
	}

	uint8_t * exe = new uint8_t[PSF_EXE_HEADER_SIZE + load_size];
	if (exe == NULL) {
		fprintf(stderr, "Error: Unable to allocate memory\n");
		return false;
	}
	memset(exe, 0, PSF_EXE_HEADER_SIZE + load_size);

	// Read input file
	FILE * bin_file = fopen(bin_path, "rb");
	if (bin_file == NULL) {
		fprintf(stderr, "Error: Unable to open file \"%s\"\n", bin_path);
		delete[] exe;
		return false;
	}

	if (fread(&exe[PSF_EXE_HEADER_SIZE], 1, bin_size, bin_file) != bin_size) {
		fprintf(stderr, "Error: File read error \"%s\"\n", bin_path);
		fclose(bin_file);
		delete[] exe;
		return false;
	}

	fclose(bin_file);

	// Construct PSX-EXE header
	char region_marker[256];
	if (region_name == NULL || region_name[0] == '\0') {
		region_name = "North America";
	}
	sprintf(region_marker, "Sony Computer Entertainment Inc. for %s area", region_name);

	memcpy(exe, "PS-X EXE", 8);
	mput4l(initial_pc, &exe[0x10]);
	mput4l(load_address, &exe[0x18]);
	mput4l(load_size, &exe[0x1c]);
	mput4l(initial_sp, &exe[0x30]);
	strcpy((char *)(&exe[0x4c]), region_marker);

	// Write to PSF1 file
	ZlibWriter exe_z(Z_BEST_COMPRESSION);
	exe_z.write(exe, PSF_EXE_HEADER_SIZE + load_size);
	if (!PSFFile::save(psf_path, PSF1_PSF_VERSION, NULL, 0, exe_z, tags)) {
		fprintf(stderr, "Error: File write error \"%s\"\n", psf_path);
		delete[] exe;
		return false;
	}

	delete[] exe;
	return true;
}

int main(int argc, char **argv)
{
	int argnum;
	int argi;

	unsigned long ulvalue;
	char * strtol_endp;

	char psf_path[PATH_MAX] = { '\0' };

	char region[256] = { '\0' };

	argi = 1;
	while (argi < argc && argv[argi][0] == '-') {
		if (strcmp(argv[argi], "--help") == 0) {
			printUsage(argv[0]);
			return EXIT_SUCCESS;
		}
		else {
			fprintf(stderr, "Error: Unknown option \"%s\"\n", argv[argi]);
			return EXIT_FAILURE;
		}
		argi++;
	}

	argnum = argc - argi;
	if (argnum != 4) {
		fprintf(stderr, "Error: Too few/more arguments.\n");
		fprintf(stderr, "\n");
		fprintf(stderr, "Run \"%s --help\" for help.\n", argv[0]);
		return EXIT_FAILURE;
	}

	char * bin_path = argv[argi];

	if (psf_path[0] == '\0') {
		strcpy(psf_path, bin_path);
		path_stripext(psf_path);
		strcat(psf_path, ".psf");
	}

	ulvalue = strtoul(argv[argi + 1], &strtol_endp, 16);
	if (*strtol_endp != '\0' || errno == ERANGE) {
		fprintf(stderr, "Error: Number format error \"%s\"\n", argv[argi + 1]);
		return EXIT_FAILURE;
	}
	uint32_t load_address = ulvalue;

	ulvalue = strtoul(argv[argi + 2], &strtol_endp, 16);
	if (*strtol_endp != '\0' || errno == ERANGE) {
		fprintf(stderr, "Error: Number format error \"%s\"\n", argv[argi + 2]);
		return EXIT_FAILURE;
	}
	uint32_t initial_pc = ulvalue;

	ulvalue = strtoul(argv[argi + 3], &strtol_endp, 16);
	if (*strtol_endp != '\0' || errno == ERANGE) {
		fprintf(stderr, "Error: Number format error \"%s\"\n", argv[argi + 3]);
		return EXIT_FAILURE;
	}
	uint32_t initial_sp = ulvalue;

	std::map<std::string, std::string> tags;
	if (!bin2psf1(bin_path, psf_path, load_address, initial_pc, initial_sp, region, tags)) {
		fprintf(stderr, "Error: Unable to convert to PSF1 format\n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
