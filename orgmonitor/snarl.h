/*
 * Snarl Interface
 * Very stripped version from original,
 * for my personal usage in C
 */

#include <windows.h>

#define SNARL_STRING_LENGTH 1024

enum M_RESULT {
	M_ABORTED         = 0x80000007,
	M_ACCESS_DENIED   = 0x80000009,
	M_ALREADY_EXISTS  = 0x8000000C,
	M_BAD_HANDLE      = 0x80000006,
	M_BAD_POINTER     = 0x80000005,
	M_FAILED          = 0x80000008,
	M_INVALID_ARGS    = 0x80000003,
	M_NO_INTERFACE    = 0x80000004,
	M_NOT_FOUND       = 0x8000000B,
	M_NOT_IMPLEMENTED = 0x80000001,
	M_OK              = 0x00000000,
	M_OUT_OF_MEMORY   = 0x80000002,
	M_TIMED_OUT       = 0x8000000A
};

typedef struct SNARLSTRUCT {
	enum Cmd {
		SNARL_SHOW = 1,
		SNARL_HIDE,
		SNARL_UPDATE,
		SNARL_IS_VISIBLE,
		SNARL_GET_VERSION,
		SNARL_REGISTER_CONFIG_WINDOW,
		SNARL_REVOKE_CONFIG_WINDOW,

		SNARL_REGISTER_ALERT,
		SNARL_REVOKE_ALERT,   // for future use
		SNARL_REGISTER_CONFIG_WINDOW_2,
		SNARL_GET_VERSION_EX,
		SNARL_SET_TIMEOUT,

		SNARL_SET_CLASS_DEFAULT,
		SNARL_CHANGE_ATTR,
		SNARL_REGISTER_APP,
		SNARL_UNREGISTER_APP,
		SNARL_ADD_CLASS,

		SNARL_EX_SHOW = 0x20,
		SNARL_SHOW_NOTIFICATION                // V39
	} Cmd;

	LONG32 Id;
	LONG32 Timeout;
	LONG32 LngData2;

	char Title[SNARL_STRING_LENGTH];
	char Text[SNARL_STRING_LENGTH];
	char Icon[SNARL_STRING_LENGTH];
} SNARLSTRUCT;

LONG32 ShowMessage(LPCSTR szTitle, LPCSTR szText, LONG32 timeout, LPCSTR szIconPath, HWND hWndReply, WPARAM uReplyMsg)
{
	SNARLSTRUCT ss;
	ZeroMemory((void*)&ss, sizeof(ss));

	ss.Cmd = SNARL_SHOW;
	strncpy(ss.Title, szTitle, SNARL_STRING_LENGTH);
	strncpy(ss.Text, szText, SNARL_STRING_LENGTH);
	strncpy(ss.Icon, szIconPath, SNARL_STRING_LENGTH);
	ss.Timeout = timeout;

	ss.LngData2 = (LONG32)hWndReply;
	ss.Id = (LONG32)uReplyMsg;

	DWORD_PTR nReturn = M_FAILED;

	HWND hWnd = FindWindow(NULL, "Snarl");
	if (IsWindow(hWnd))
	{
		COPYDATASTRUCT cds;
		cds.dwData = 2;
		cds.cbData = sizeof(ss);
		cds.lpData = &ss;

		if (SendMessageTimeout(hWnd, WM_COPYDATA, 0, (LPARAM)&cds, SMTO_ABORTIFHUNG, 1000, &nReturn) == 0)
		{
			if (GetLastError() == ERROR_TIMEOUT)
				nReturn = M_TIMED_OUT;
		}
	}

	return nReturn;
}
