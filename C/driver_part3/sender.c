#include <windows.h>
#include <stdio.h>

/*********************************************************
 *   Main Function Entry
 *
 *********************************************************/
int _cdecl main(int argc, char **argv)
{
	HANDLE hFile;
	DWORD dwReturn;
	char szTemp[256] = {0};

	sprintf(szTemp, "\\\\.\\YTDevice\\tmp_file");

	hFile = CreateFile(szTemp, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

	if(hFile == INVALID_HANDLE_VALUE) {
		printf("invalid handle\n");
		goto cleanup;
	}
	while(szTemp[0] != 'q' && szTemp[0] != 'Q') {

	    printf("Enter a string and press enter, 'Q' to quit>\n");
	    gets(szTemp);
	    if(szTemp[0] != 'q' &&  szTemp[0] != 'Q') {
		    dwReturn = 0;
			//WriteFile(hFile, szTemp, strlen(szTemp) + 1, &dwReturn, NULL);
		    printf("%d bytes written\n", dwReturn);
		}
	}

	CloseHandle(hFile);

cleanup:
	return 0;
}
