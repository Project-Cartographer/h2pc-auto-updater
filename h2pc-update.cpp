// h2pc-update.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <windows.h>

//Pauses the output console such that the user can keep up with the output for example.
void UserDelay() {
	printf("Press ENTER to continue...");
	int c;
	do {
		c = getchar();
	} while (c != '\n');
}

void PrintHelp() {
	printf("Enter the source then destination of any set of files one after the next.\n");
	printf("The following commands can be placed anywhere in the input arguments: \n");
	printf("\"-t <milliseconds>\" to delay any actions until the given number of milliseconds has elapsed.\n");
	printf("\"-p <PID>\" to delay any actions until the process of given ID has closed.\n");
}

static const wchar_t H2UpdateLocationsStr[3][10] = { L"[Temp]", L"[Game]", L"[AppData]" };

int main()
{
	int ArgCnt = 0;
	LPWSTR* ArgList = CommandLineToArgvW(GetCommandLineW(), &ArgCnt);

	wchar_t** file_locations = (wchar_t**)malloc(sizeof(wchar_t*) * ArgCnt);
	int recorded_locations = 0;

	int max_attempts = 120;

	if (ArgList && ArgCnt > 1) {
		for (int i = 1; i < ArgCnt; i++) {
			if (ArgList[i][0] == '-') {
				if (ArgList[i][1] == 'p') {
					i++;
					int tempint1 = 0;
					if (swscanf_s(ArgList[i], L"%d", &tempint1) == 1) {
						printf("Waiting for PID:%d to end...\n", tempint1);
						HANDLE phandle = OpenProcess(PROCESS_ALL_ACCESS, TRUE, tempint1);
						DWORD exit_code = 0;
						//GetExitCodeProcess will not work if process exits with code 259 (STILL_ACTIVE).
						while (GetExitCodeProcess(phandle, &exit_code), exit_code == STILL_ACTIVE && --max_attempts > 0) {
							Sleep(500L);
						}
					}
				}
				else if (ArgList[i][1] == 't') {
					i++;
					long templong1 = 0;
					if (swscanf_s(ArgList[i], L"%ld", &templong1) == 1) {
						printf("Waiting %ld milliseconds...\n", templong1);
						Sleep(templong1);
					}
				}
			}
			else {
				file_locations[recorded_locations++] = ArgList[i];
			}

			//wprintf(L"%ws\n", ArgList[i]);
		}
	}

	if (max_attempts <= 0) {
		printf("Error. Process never closed. Timed out!\n");
	}
	else if (recorded_locations <= 0) {
		printf("No input file arguments!\n");
		PrintHelp();
	}
	else if (recorded_locations % 2 != 0) {
		printf("Invalid input argument set!\n");
		PrintHelp();
	}
	else {

		wchar_t* dir_temp;
		size_t buflen;
		_wdupenv_s(&dir_temp, &buflen, L"TEMP");

		wchar_t dir_temp_h2[1024];
		swprintf(dir_temp_h2, 1024, L"%ws\\Halo2\\", dir_temp);
		CreateDirectoryW(dir_temp_h2, NULL);

		wchar_t dir_update[1024];
		swprintf(dir_update, 1024, L"%ws\\Halo2\\Update\\", dir_temp);
		CreateDirectoryW(dir_update, NULL);

		wchar_t dir_update_old[1024];
		swprintf(dir_update_old, 1024, L"%ws\\Halo2\\UpdateOld\\", dir_temp);
		CreateDirectoryW(dir_update_old, NULL);

		for (int j = 0; j < 3; j++) {
			wchar_t dir_update_mk[1024];
			swprintf(dir_update_mk, 1024, L"%ws\\Halo2\\Update\\%ws\\", dir_temp, H2UpdateLocationsStr[j]);
			CreateDirectoryW(dir_update_mk, NULL);

			swprintf(dir_update_mk, 1024, L"%ws\\Halo2\\UpdateOld\\%ws\\", dir_temp, H2UpdateLocationsStr[j]);
			CreateDirectoryW(dir_update_mk, NULL);
		}

		for (int i = 1; i < recorded_locations; i += 2) {

			//Sort out the old removed location
			wchar_t* pos_file = 0;
			int location_id = -1;
			for (int j = 0; j < 3; j++) {
				pos_file = wcsstr(file_locations[i], H2UpdateLocationsStr[j]);
				if (pos_file) {
					location_id = j;
					break;
				}
			}

			pos_file = wcsrchr(file_locations[i + 1], L'\\');
			if (!pos_file) {
				pos_file = wcsrchr(file_locations[i + 1], L'/');
			}
			if (pos_file)
				pos_file++;
			if (!pos_file) {
				pos_file = file_locations[i + 1];
			}

			int old_rem_file_buflen = 1 + wcslen(dir_update_old) + (location_id < 0 ? 0 : wcslen(H2UpdateLocationsStr[location_id]) + 1) + wcslen(pos_file);
			wchar_t* old_rem_file = (wchar_t*)malloc(sizeof(wchar_t) * old_rem_file_buflen);

			if (location_id < 0) {
				swprintf(old_rem_file, old_rem_file_buflen, L"%ws%ws", dir_update_old, pos_file);
			}
			else {
				swprintf(old_rem_file, old_rem_file_buflen, L"%ws%ws\\%ws", dir_update_old, H2UpdateLocationsStr[location_id], pos_file);
			}

			_wremove(old_rem_file);


			//get path of new file.
			wchar_t* new_file = 0;

			if (location_id == 0) {
				int new_file_buflen = 1 + wcslen(dir_update) + wcslen(pos_file);
				new_file = (wchar_t*)malloc(sizeof(wchar_t) * new_file_buflen);
				swprintf(new_file, new_file_buflen, L"%ws%ws", dir_update, pos_file);
			}
			else if (location_id > 0 && location_id < 3) {
				int new_file_buflen = 1 + wcslen(dir_update) + wcslen(H2UpdateLocationsStr[location_id]) + 1 + wcslen(pos_file);
				new_file = (wchar_t*)malloc(sizeof(wchar_t) * new_file_buflen);
				swprintf(new_file, new_file_buflen, L"%ws%ws\\%ws", dir_update, H2UpdateLocationsStr[location_id], pos_file);
			}
			else {
				new_file = file_locations[i];
			}


			//path of old file is given at i+1
			wchar_t* old_file = file_locations[i + 1];

			//Move/Copy the files.
			wprintf(L"New File: \"%ws\"\n", new_file);
			wprintf(L"Old File: \"%ws\"\n", old_file);
			wprintf(L"Old File Moving to: \"%ws\"\n", old_rem_file);

			DWORD last_err = 0;
			if (MoveFile(old_file, old_rem_file) || (last_err = GetLastError()) == ENOENT) {
				if (CopyFile(new_file, old_file, false)) {
					printf("--- SUCCESS! ---\n\n");
				}
				else {
					printf("Failed to copy in new file!\n");
					if (!last_err) {
						printf("Puting back old file.\n");
						MoveFile(old_rem_file, old_file);
					}
				}
			}
			else {
				printf("Failed to move out old file!\n");
			}


			//Clean up.
			if (location_id >= 0) {
				free(new_file);
			}
			free(old_rem_file);
		}
	}
	free(file_locations);

	UserDelay();

    return 0;
}

