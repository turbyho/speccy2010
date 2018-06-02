#include "commander.h"
#include "screen.h"
#include "dialog.h"
#include "viewer.h"
#include "../system.h"
#include "../system/sdram.h"
#include "../utils/cstring.h"
#include "../utils/dirent.h"
#include "../specBetadisk.h"
#include "../specConfig.h"
#include "../specKeyboard.h"
#include "../specSnapshot.h"
#include "../specTape.h"


const char *fatErrorMsg[] = {
	"FR_OK",
	"FR_DISK_ERR",
	"FR_INT_ERR",
	"FR_NOT_READY",
	"FR_NO_FILE",
	"FR_NO_PATH",
	"FR_INVALID_NAME",
	"FR_DENIED",
	"FR_EXIST",
	"FR_INVALID_OBJECT",
	"FR_WRITE_PROTECTED",
	"FR_INVALID_DRIVE",
	"FR_NOT_ENABLED",
	"FR_NO_FILESYSTEM",
	"FR_MKFS_ABORTED",
	"FR_TIMEOUT",
	"FR_LOCKED",
	"FR_NOT_ENOUGH_CORE",
};
//---------------------------------------------------------------------------------------
char destinationPath[PATH_SIZE] = "/";
//---------------------------------------------------------------------------------------
void make_short_name(char *sname, word size, const char *name)
{
	word nSize = strlen(name);

	if (nSize + 1 <= size)
		strcpy(sname, name);
	else {
		word sizeA = (size - 2) / 2;
		word sizeB = (size - 1) - (sizeA + 1);

		memcpy(sname, name, sizeA);
		sname[sizeA] = '~';
		memcpy(sname + sizeA + 1, name + nSize - sizeB, sizeB + 1);
	}
}

void make_short_name(CString &str, word size, const char *name)
{
	word nSize = strlen(name);

	str = name;

	if (nSize + 1 > size) {
		str.Delete((size - 2) / 2, 2 + nSize - size);
		str.Insert((size - 2) / 2, "~");
	}
}
//------------------------------------------------------------------
void display_path(const char *str, int col, int row, byte max_sz)
{
	char path_buff[33] = "/";
	char *path_short = (char *) str;

	if (strlen(str) > max_sz) {
		while (strlen(path_short) + 2 > max_sz) {
			path_short++;
			while (*path_short != '/')
				path_short++;
		}

		strcpy(path_buff, "...");
	}

	strcat(path_buff, path_short);
	WriteStr(col, row, path_buff, max_sz);
}
//---------------------------------------------------------------------------------------
byte get_sel_attr(FRECORD &fr)
{
	byte result = 007;

	if ((fr.attr & AM_DIR) == 0) {
		strlwr(fr.name);

		char *ext = fr.name + strlen(fr.name);
		while (ext > fr.name && *ext != '.')
			ext--;

		if (strcmp(ext, ".trd") == 0 || strcmp(ext, ".fdi") == 0 || strcmp(ext, ".scl") == 0)
			result = 006;
		else if (strcmp(ext, ".tap") == 0 || strcmp(ext, ".tzx") == 0)
			result = 004;
		else if (strcmp(ext, ".sna") == 0)
			result = 0103;
		else if (strcmp(ext, ".scr") == 0)
			result = 0102;
		else
			result = 005;
	}

	if (fr.sel)
		result = (result & 077) | 010;

	return result;
}
//---------------------------------------------------------------------------------------
void show_table()
{
	display_path(get_current_dir(), 0, FILES_PER_ROW + 3, 32);

	FRECORD fr;

	for (int i = 0; i < FILES_PER_ROW; i++) {
		for (int j = 0; j < 2; j++) {
			int col = j * 16;
			int row = i + 2;
			int pos = i + j * FILES_PER_ROW + files_table_start;

			Read(&fr, table_buffer, pos);

			char sname[16];
			make_short_name(sname, sizeof(sname), fr.name);

			//if( ( fr.attr & AM_DIR ) == 0 ) strlwr( sname );
			//else strupr( sname );

			if (pos < files_size) {
				WriteAttr(col, row, get_sel_attr(fr), 16);

				if (fr.sel)
					WriteChar(col, row, 0x95);
				else
					WriteChar(col, row, ' ');

				WriteStr(col + 1, row, sname, 15);
			}
			else {
				WriteAttr(col, row, 0, 16);
				WriteStr(col, row, "", 16);
			}
		}
	}

	if (too_many_files) {
		WriteStr(3, 5, "too many files (>9999) !", 0);
		WriteAttr(3, 5, 0102, 24);
	}
	else if (files_size == 0) {
		WriteStr(10, 5, "no files !", 0);
		WriteAttr(10, 5, 0102, 10);
	}
}
//---------------------------------------------------------------------------------------
void hide_sel()
{
	FRECORD fr;
	Read(&fr, table_buffer, files_sel);

	if (files_size != 0) {
		WriteAttr(files_sel_pos_x * 16, 2 + files_sel_pos_y, get_sel_attr(fr), 16);

		if (fr.sel)
			WriteChar(files_sel_pos_x * 16, 2 + files_sel_pos_y, 0x95);
		else
			WriteChar(files_sel_pos_x * 16, 2 + files_sel_pos_y, ' ');
	}

	WriteStr(0, FILES_PER_ROW + 4, "", 32);
	WriteStr(0, FILES_PER_ROW + 5, "", 32);
}
//---------------------------------------------------------------------------------------
void show_sel(bool redraw = false)
{
	if (calc_sel() || redraw)
		show_table();

	if (files_size != 0) {
		FRECORD fr;
		Read(&fr, table_buffer, files_sel);

		WriteAttr(files_sel_pos_x * 16, 2 + files_sel_pos_y, 071, 16);

		if (fr.sel)
			WriteChar(files_sel_pos_x * 16, 2 + files_sel_pos_y, 0x95);
		else
			WriteChar(files_sel_pos_x * 16, 2 + files_sel_pos_y, ' ');

		char sname[PATH_SIZE];
		make_short_name(sname, 33, fr.name);
		WriteStr(0, FILES_PER_ROW + 4, sname, 32);

		if (files_sel_number > 0) {
			sniprintf(sname, sizeof(sname), "selected %u item%s", files_sel_number, files_sel_number > 1 ? "s" : "");
			WriteStr(0, FILES_PER_ROW + 5, sname, 32);
		}
		else {
			if (fr.date == 0) {
				WriteStr(0, FILES_PER_ROW + 5, "", 15);
			}
			else {
				sniprintf(sname, sizeof(sname), "%.2u.%.2u.%.2u  %.2u:%.2u", fr.date & 0x1f,
					(fr.date >> 5) & 0x0f,
					(80 + (fr.date >> 9)) % 100,
					(fr.time >> 11) & 0x1f,
					(fr.time >> 5) & 0x3f);
				WriteStr(0, FILES_PER_ROW + 5, sname, 15);
			}

			if ((fr.attr & AM_DIR) != 0)
				sniprintf(sname, sizeof(sname), "      Folder");
			else if (fr.size < 9999)
				sniprintf(sname, sizeof(sname), "%10u B", fr.size);
			else if (fr.size < 0x100000)
				sniprintf(sname, sizeof(sname), "%6u.%.2u kB", fr.size >> 10, ((fr.size & 0x3ff) * 100) >> 10);
			else
				sniprintf(sname, sizeof(sname), "%6u.%.2u MB", fr.size >> 20, ((fr.size & 0xfffff) * 100) >> 20);

			WriteStr(20, FILES_PER_ROW + 5, sname, 12);
		}
	}
}
//---------------------------------------------------------------------------------------
void init_screen()
{
	byte attr = 0x07;
	ClrScr(attr);

	SystemBus_Write(0xc00021, 0x8000 | VIDEO_PAGE); // Enable shell videopage
	SystemBus_Write(0xc00022, 0x8000 | ((attr >> 3) & 0x03)); // Enable shell border

	char str[33];
	sniprintf(str, sizeof(str), "    -= Speccy2010, v" VERSION " =-    ");

	WriteStr(0, 0, str);
	WriteAttr(0, 0, 0x44, strlen(str));

	WriteAttr(0, 1, 0x06, 32);
	WriteLine(1, 3);
	WriteLine(1, 5);

	WriteAttr(0, FILES_PER_ROW + 2, 0x06, 32);
	WriteLine(FILES_PER_ROW + 2, 3);
	WriteLine(FILES_PER_ROW + 2, 5);
}
//---------------------------------------------------------------------------------------
//- COMMANDER ACTION HANDLERS -----------------------------------------------------------
//---------------------------------------------------------------------------------------
bool Shell_CopyItem(const char *srcName, const char *dstName, bool move = false)
{
	int res;

	const char *sname = &dstName[strlen(dstName)];
	while (sname != dstName && sname[-1] != '/')
		sname--;

	char shortName[20];
	make_short_name(shortName, sizeof(shortName), sname);

	show_table();
	Shell_MessageBox("Processing", shortName, "", "", MB_NO, 050);

	if (move) {
		res = f_rename(srcName, dstName);
	}
	else {
		FILINFO finfo;
		char lfn[1];
		finfo.lfname = lfn;
		finfo.lfsize = 0;

		FIL src, dst;
		UINT size;

		byte buff[0x100];

		res = f_open(&src, srcName, FA_READ | FA_OPEN_ALWAYS);
		if (res != FR_OK)
			goto copyExit1;

		if (f_stat(dstName, &finfo) == FR_OK) {
			res = FR_EXIST;
			goto copyExit2;
		}

		res = f_open(&dst, dstName, FA_WRITE | FA_CREATE_ALWAYS);
		if (res != FR_OK)
			goto copyExit2;

		size = src.fsize;
		while (size > 0) {
			UINT currentSize = min(size, sizeof(buff));
			UINT r;

			res = f_read(&src, buff, currentSize, &r);
			if (res != FR_OK)
				break;

			res = f_write(&dst, buff, currentSize, &r);
			if (res != FR_OK)
				break;

			size -= currentSize;
			if ((src.fptr & 0x300) == 0)
				CycleMark(0, FILES_PER_ROW + 4);

			if (GetKey(false) == K_ESC) {
				res = FR_DISK_ERR;
				break;
			}
		}

		f_close(&dst);
		if (res != FR_OK)
			f_unlink(dstName);

copyExit2:
		f_close(&src);

copyExit1:;
	}

	if (res == FR_OK)
		return true;

	if (move) {
		Shell_MessageBox("Error",
			(move ? "Cannot move/rename to" : "Cannot copy to"),
			shortName, fatErrorMsg[res]);
	}

	show_table();

	return false;
}
//---------------------------------------------------------------------------------------
bool Shell_Copy(const char *_name, bool move)
{
	CString name;

	if ((ReadKeyFlags() & fKeyShift) != 0)
		name = _name;
	else
		name = destinationPath;

	const char *title = move ? "Move/Rename" : "Copy";
	if (!Shell_InputBox(title, "Enter new name/path :", name))
		return false;

	bool newPath = false;

	char pname[PATH_SIZE];
	char pnameNew[PATH_SIZE];

	if (name.GetSymbol(0) == '/') {
		if (name.GetSymbol(name.Length() - 1) != '/')
			name += '/';
		newPath = true;
	}

	if (files_sel_number == 0 || !newPath) {
		sniprintf(pname, sizeof(pname), "%s%s", get_current_dir(), _name);

		if (newPath)
			sniprintf(pnameNew, sizeof(pnameNew), "%s%s", name.String() + 1, _name);
		else
			sniprintf(pnameNew, sizeof(pnameNew), "%s%s", get_current_dir(), name.String());

		if (Shell_CopyItem(pname, pnameNew, move) && !newPath && move) {
			sniprintf(files_lastName, sizeof(files_lastName), "%s", name.String());
		}
	}
	else {
		FRECORD fr;

		for (int i = 0; i < files_size; i++) {
			Read(&fr, table_buffer, i);

			if (fr.sel) {
				sniprintf(pname, sizeof(pname), "%s%s", get_current_dir(), fr.name);
				sniprintf(pnameNew, sizeof(pnameNew), "%s%s", name.String() + 1, fr.name);

				if (!Shell_CopyItem(pname, pnameNew, move))
					break;

				fr.sel = false;
				files_sel_number--;

				Write(&fr, table_buffer, i);
			}

			if (GetKey(false) == K_ESC)
				break;
		}
	}

	return !newPath || move;
}
//---------------------------------------------------------------------------------------
bool Shell_CreateFolder()
{
	CString name = "";
	if (!Shell_InputBox("Create folder", "Enter name :", name))
		return false;

	char pname[PATH_SIZE];
	sniprintf(pname, sizeof(pname), "%s%s", get_current_dir(), name.String());

	int res = f_mkdir(pname);
	if (res == FR_OK) {
		sniprintf(files_lastName, sizeof(files_lastName), "%s", name.String());
		return true;
	}

	char shortName[20];
	make_short_name(shortName, sizeof(shortName), name);

	Shell_MessageBox("Error", "Cannot create the folder", shortName, fatErrorMsg[res]);
	show_table();

	return false;
}
//---------------------------------------------------------------------------------------
bool Shell_DeleteItem(const char *name)
{
	int res = f_unlink(name);
	if (res == FR_OK)
		return true;

	const char *sname = &name[strlen(name)];
	while (sname != name && sname[-1] != '/')
		sname--;

	char shortName[20];
	make_short_name(shortName, sizeof(shortName), sname);

	Shell_MessageBox("Error", "Cannot delete the item", shortName, fatErrorMsg[res]);
	show_table();

	return false;
}
//---------------------------------------------------------------------------------------
bool Shell_Delete(const char *name)
{
	if (files_sel_number == 0 && strcmp(name, "..") == 0)
		return false;

	char sname[PATH_SIZE];

	if (files_sel_number > 0)
		sniprintf(sname, sizeof(sname), "selected %u item%s", files_sel_number, files_sel_number > 1 ? "s" : "");
	else
		make_short_name(sname, 21, name);

	strcat(sname, "?");

	if (!Shell_MessageBox("Delete", "Do you wish to delete", sname, "", MB_YESNO, 0050, 0151))
		return false;

	if (files_sel_number == 0) {
		sniprintf(sname, sizeof(sname), "%s%s", get_current_dir(), name);
		Shell_DeleteItem(sname);
	}
	else {
		FRECORD fr;

		for (int i = 0; i < files_size; i++) {
			Read(&fr, table_buffer, i);

			if (fr.sel) {
				sniprintf(sname, sizeof(sname), "%s%s", get_current_dir(), fr.name);
				if (!Shell_DeleteItem(sname))
					break;
			}
		}
	}

	return true;
}
//---------------------------------------------------------------------------------------
bool Shell_EmptyTrd(const char *_name)
{
	char name[PATH_SIZE];
	bool result = false;

	if (strlen(_name) == 0) {
		CString newName = "empty.trd";
		if (!Shell_InputBox("Format", "Enter name :", newName))
			return false;

		show_table();
		sniprintf(name, sizeof(name), "%s", newName.String());
		result = true;

		char *ext = name + strlen(name);
		while (ext > name && *ext != '.')
			ext--;
		strlwr(ext);

		if (strcmp(ext, ".trd") != 0)
			sniprintf(name, sizeof(name), "%s.trd", newName.String());
		else
			sniprintf(name, sizeof(name), "%s", newName.String());
	}
	else {
		sniprintf(name, sizeof(name), "%s", _name);

		char *ext = name + strlen(name);
		while (ext > name && *ext != '.')
			ext--;
		strlwr(ext);

		if (strcmp(ext, ".trd") != 0)
			return false;

		make_short_name(name, 21, _name);
		strcat(name, "?");

		if (!Shell_MessageBox("Format", "Do you wish to format", name, "", MB_YESNO, 0050, 0151))
			return false;
		show_table();

		sniprintf(name, sizeof(name), "%s", _name);
	}

	CString label = name;
	label.TrimRight(4);
	if (label.Length() > 8)
		label.TrimRight(label.Length() - 8);

	if (!Shell_InputBox("Format", "Enter disk label :", label))
		return false;
	show_table();

	char pname[PATH_SIZE];
	sniprintf(pname, sizeof(pname), "%s%s", get_current_dir(), name);

	const byte zero[0x10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	const byte sysArea[] = { 0x00, 0x00, 0x01, 0x16, 0x00, 0xf0, 0x09, 0x10 };
	char sysLabel[8];
	strncpy(sysLabel, label.String(), sizeof(sysLabel));

	int res;
	UINT r;
	FIL dst;

	{
		res = f_open(&dst, pname, FA_WRITE | FA_OPEN_ALWAYS);
		if (res != FR_OK)
			goto formatExit1;

		for (int i = 0; i < 256 * 16; i += sizeof(zero)) {
			res = f_write(&dst, zero, sizeof(zero), &r);
			if (res != FR_OK)
				break;
		}
		if (res != FR_OK)
			goto formatExit2;

		res = f_lseek(&dst, 0x8e0);
		if (res != FR_OK)
			goto formatExit2;

		res = f_write(&dst, sysArea, sizeof(sysArea), &r);
		if (res != FR_OK)
			goto formatExit2;

		res = f_lseek(&dst, 0x8f5);
		if (res != FR_OK)
			goto formatExit2;

		res = f_write(&dst, sysLabel, sizeof(sysLabel), &r);
		if (res != FR_OK)
			goto formatExit2;

		res = f_lseek(&dst, 256 * 16 * 2 * 80 - sizeof(zero));
		if (res != FR_OK)
			goto formatExit2;

		res = f_write(&dst, zero, sizeof(zero), &r);
		if (res != FR_OK)
			goto formatExit2;

	formatExit2:
		f_close(&dst);

	formatExit1:;
	}

	if (res != FR_OK) {
		Shell_MessageBox("Error", "Cannot format", name, fatErrorMsg[res]);
	}

	return result;
}
//---------------------------------------------------------------------------------------
// COMMANDER CORE -----------------------------------------------------------------------
//---------------------------------------------------------------------------------------
void Shell_Commander()
{
	CPU_Stop();

	SystemBus_Write(0xc00020, 0); // use bank 0

	init_screen();

	read_dir();
	show_sel(true);

	while (true) {
		byte key = GetKey();

		FRECORD fr;
		Read(&fr, table_buffer, files_sel);

		sniprintf(files_lastName, sizeof(files_lastName), "%s", fr.name);

		if ((key == K_UP || key == K_DOWN || key == K_LEFT || key == K_RIGHT) && files_size > 0) {
			hide_sel();

			if (key == K_LEFT)
				files_sel -= FILES_PER_ROW;
			else if (key == K_RIGHT)
				files_sel += FILES_PER_ROW;
			else if (key == K_UP)
				files_sel--;
			else if (key == K_DOWN)
				files_sel++;

			show_sel();
		}
		else if (key == ' ' && files_size > 0) {
			if (strcmp(fr.name, "..") != 0) {
				if (fr.sel)
					files_sel_number--;
				else
					files_sel_number++;

				fr.sel = (fr.sel + 1) % 2;
				Write(&fr, table_buffer, files_sel);
			}

			hide_sel();
			files_sel++;
			show_sel();
		}
		else if ((key == '+' || key == '=' || key == '-' || key == '/' || key == '\\') && files_size > 0) {
			hide_sel();
			files_sel_number = 0;

			for (int i = 0; i < files_size; i++) {
				Read(&fr, table_buffer, i);

				if (strcmp(fr.name, "..") != 0) {
					if (key == '+' || key == '=')
						fr.sel = 1;
					else if (key == '-')
						fr.sel = 0;
					else if (key == '/' || key == '\\')
						fr.sel = (fr.sel + 1) % 2;

					if (fr.sel)
						files_sel_number++;
					Write(&fr, table_buffer, i);
				}

				if ((i & 0x3f) == 0)
					CycleMark(0, FILES_PER_ROW + 4);
			}

			show_sel(true);
		}
		else if (key == K_ESC)
			break;
		else if (key == '[' || key == ']') {
			static byte testVideo = 0;

			if (key == '[')
				testVideo++;
			else
				testVideo--;

			SystemBus_Write(0xc00049, testVideo & 0x07);
		}
		else if (key >= '1' && key <= '4') {
			if ((fr.attr & AM_DIR) == 0) {
				char fullName[PATH_SIZE];
				sniprintf(fullName, sizeof(fullName), "%s%s", get_current_dir(), fr.name);

				strlwr(fr.name);

				char *ext = fr.name + strlen(fr.name);
				while (ext > fr.name && *ext != '.')
					ext--;

				if (strcmp(ext, ".trd") == 0 || strcmp(ext, ".fdi") == 0 || strcmp(ext, ".scl") == 0) {
					int i = key - '1';

					if (fdc_open_image(i, fullName)) {
						floppy_disk_wp(i, &specConfig.specImages[i].readOnly);

						strcpy(specConfig.specImages[key - '1'].name, fullName);
						SaveConfig();
					}
				}
			}
		}
		else if (key == K_BACKSPACE) {
			hide_sel();
			leave_dir();
			show_table();
			show_sel();
		}
		else if (key == K_F1) {
			WriteStr(0, FILES_PER_ROW + 4, "speccy2010.hlp", 32);
			Shell_Viewer("speccy2010.hlp");

			show_sel(true);
		}
		else if (key == K_F2) {
			sniprintf(destinationPath, sizeof(destinationPath), "/%s", get_current_dir());
		}
		else if (key == K_F3) {
			if ((fr.attr & AM_DIR) == 0) {
				char fullName[PATH_SIZE];
				sniprintf(fullName, sizeof(fullName), "%s%s", get_current_dir(), fr.name);
				Shell_Viewer(fullName);

				show_sel(true);
			}
		}
		else if (key == K_F4) {
			hide_sel();
			if (Shell_EmptyTrd(""))
				read_dir();
			;
			show_sel(true);
		}
		else if (key == K_F5) {
			hide_sel();
			if (Shell_Copy(fr.name, false))
				read_dir();
			show_sel(true);
		}
		else if (key == K_F6) {
			hide_sel();
			if (Shell_Copy(fr.name, true))
				read_dir();
			show_sel(true);
		}
		else if (key == K_F7) {
			hide_sel();
			if (Shell_CreateFolder())
				read_dir();
			show_sel(true);
		}
		else if (key == K_F8) {
			hide_sel();

			if (Shell_Delete(fr.name)) {
				int last_files_sel = files_sel;
				read_dir();
				files_sel = last_files_sel;
			}

			show_sel(true);
		}
		else if (key == K_F9) {
			hide_sel();
			if (Shell_EmptyTrd(fr.name))
				read_dir();

			show_sel(true);
		}
		else if (key == K_RETURN) {
			if ((fr.attr & AM_DIR) != 0) {
				hide_sel();
				enter_dir(fr.name);
				show_table();
				show_sel();
			}
			else {
				char fullName[PATH_SIZE];
				sniprintf(fullName, sizeof(fullName), "%s%s", get_current_dir(), fr.name);

				strlwr(fr.name);

				char *ext = fr.name + strlen(fr.name);
				while (ext > fr.name && *ext != '.')
					ext--;

				if (strcmp(ext, ".rbf") == 0) {
					sniprintf(specConfig.fpgaConfigName, sizeof(specConfig.fpgaConfigName), "%s", fullName);
					SaveConfig();

					SystemBus_Write(0xc00021, 0); // Enable Video
					SystemBus_Write(0xc00022, 0);
					CPU_Start();

					FPGA_Config();

					return;
				}
				else if (strcmp(ext, ".tap") == 0 || strcmp(ext, ".tzx") == 0) {
					sniprintf(specConfig.snaName, sizeof(specConfig.snaName), "/%s.00.sna", fullName);

					Tape_SelectFile(fullName);
					break;
				}
				else if (strcmp(ext, ".sna") == 0) {
					sniprintf(specConfig.snaName, sizeof(specConfig.snaName), "/%s", fullName);

					if (LoadSnapshot(fullName)) {
						break;
					}
				}
				else if (strcmp(ext, ".trd") == 0 || strcmp(ext, ".fdi") == 0 || strcmp(ext, ".scl") == 0) {
					if (fdc_open_image(0, fullName)) {
						sniprintf(specConfig.snaName, sizeof(specConfig.snaName), "/%s.00.sna", fullName);

						floppy_disk_wp(0, &specConfig.specImages[0].readOnly);

						strcpy(specConfig.specImages[0].name, fullName);
						SaveConfig();

						CPU_Start();
						CPU_Reset(true);
						CPU_Reset(false);
						CPU_Stop();

						CPU_ModifyPC(0, 0);

						if (specConfig.specRom == SpecRom_Classic48)
							SystemBus_Write(0xc00017, (1 << 4) | (1 << 5));
						else
							SystemBus_Write(0xc00017, (1 << 4));

						SystemBus_Write(0xc00018, 0x0001);

						break;
					}
				}
				else if (strcmp(ext, ".scr") == 0) {
					while (true) {
						FIL image;
						if (f_open(&image, fullName, FA_READ) == FR_OK) {
							dword addr = VIDEO_PAGE_PTR;

							for (dword pos = 0; pos < 0x1b00; pos++) {
								byte data;

								UINT res;
								if (f_read(&image, &data, 1, &res) != FR_OK)
									break;
								if (res == 0)
									break;

								SystemBus_Write(addr++, data);
							}
						}
						else
							break;

						byte key = GetKey();

						if (key == ' ') {
							while (true) {
								files_sel++;
								if (files_sel >= files_size)
									files_sel = 0;

								Read(&fr, table_buffer, files_sel);
								strlwr(fr.name);

								ext = fr.name + strlen(fr.name);
								while (ext > fr.name && *ext != '.')
									ext--;

								if (strcmp(ext, ".scr") == 0)
									break;
							}

							sniprintf(fullName, sizeof(fullName), "%s%s", get_current_dir(), fr.name);
						}
						else if (key != 0)
							break;
					}

					init_screen();
					show_table();
					show_sel();
				}
			}
		}
	}

	SystemBus_Write(0xc00021, 0); // Enable Video
	SystemBus_Write(0xc00022, 0);

	CPU_Start();
}
//---------------------------------------------------------------------------------------
