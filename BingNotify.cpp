#define APP_NAME "Bing Notify"
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <stdio.h>
#include <wchar.h>
#include <wininet.h>
#include <stdlib.h>
#include <tchar.h>

#define ERROR(message, ...) \
        if (verbose){\
        MessageBoxA( NULL, message, APP_NAME, MB_ICONERROR | MB_OK ); \
        }
#define ERRORF(...) \
        if (verbose){\
        char buffer[100];\
        sprintf(buffer,__VA_ARGS__); \
        MessageBoxA( NULL, buffer, APP_NAME, MB_ICONERROR | MB_OK ); \
        }

HINTERNET hRootHandle;
bool verbose=false;

wchar_t bingHP[70]=L"http://www.bing.com/HPImageArchive.aspx?format=js&n=1";

int setWall(wchar_t*  path)
{
    //LPWSTR  path

    return SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, (LPWSTR)path, SPIF_UPDATEINIFILE);

}

wchar_t* getValue(wchar_t* source, const wchar_t* objName, wchar_t* value) {
	wchar_t* begin=wcsstr(source,objName)+wcslen(objName)+1;
	wchar_t* end=begin=wcschr(begin, L'"')+1; 
	do
		end=wcschr(end, L'"');
	while(end!=NULL && end[-1] == L'\\');
	value[0]=L'\0'; //wcsncat needs a null terminating array.
	wcsncat(value, begin, end-begin);
	return value;
}

bool unofficial=false;
int getImgInfo(wchar_t* imgURL, wchar_t* title, wchar_t* copyright) {
	HINTERNET OpenAddress = InternetOpenUrlW(hRootHandle, bingHP, NULL, 0, INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_HYPERLINK, 0);

   if ( !OpenAddress )
   {
      DWORD ErrorNum = GetLastError();
      ERRORF("Failed to open URL \nError No: %d",ErrorNum);
      return 0;
   }

   wchar_t receivedData[4096];
   char mbreceivedData[4096];
   wchar_t* temp = receivedData;
   DWORD dwBytesRead = 0;
   //while(InternetReadFile(OpenAddress, receivedData, 4096, &dwBytesRead) && dwBytesRead > 0 )
   InternetReadFile(OpenAddress, (LPVOID)mbreceivedData, 4096, &dwBytesRead);
   {
		temp += dwBytesRead;
   }
   *temp= L'\0';

    int len = MultiByteToWideChar(CP_UTF8, 0, mbreceivedData, dwBytesRead, 0, 0); 
    //wchar_t* buf = new wchar_t[len];
    MultiByteToWideChar(CP_UTF8, 0, mbreceivedData, dwBytesRead, receivedData, len);

//    mbstowcs (receivedData, mbreceivedData, sizeof(mbreceivedData));
//wprintf(L"--%s\n",receivedData); printf("\n--%s\n",mbreceivedData);
   //printf("%s",receivedData);

   InternetCloseHandle(OpenAddress);

   //wchar_t* hsh=strstr(receivedData,"\"hsh\":\"")+7;
   wcscpy(imgURL,L"http://www.bing.com/hpwp/");
   wchar_t hsh[32]; 
   getValue(receivedData, L"\"hsh\"",hsh); 
   wcscat(imgURL,hsh);
   if (unofficial){
       wcscpy(imgURL,L"http://www.bing.com");
       getValue(receivedData, L"\"url\"",imgURL+(wcslen(imgURL)));
   }
   //*strchr(url,'"')='\0';
   //strcat(imgURL,url);
   getValue(receivedData, L"\"title\"",title);
   getValue(receivedData, L"\"copyright\"",copyright);
   return 1;
}



int downloadFile(wchar_t* url, wchar_t *path) {
	HINTERNET hUrl = NULL;
	HANDLE hOut = NULL;
	DWORD dwBytesRead = 0;
	DWORD dwBytesWritten = 0;
	wchar_t lpBuffer[4096];
	DWORD statusCode ;
	DWORD statusBufferLength = sizeof(statusCode);

	hUrl = InternetOpenUrlW(hRootHandle, url, NULL, 0, /*INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE*/0 , 0);

	if ( !hUrl ) {
      DWORD ErrorNum = GetLastError();
      ERRORF("Failed to open URL \nError No: %d",ErrorNum);
      return 0;
   	}

	HttpQueryInfo(hUrl,HTTP_QUERY_STATUS_CODE| HTTP_QUERY_FLAG_NUMBER, (LPVOID)&statusCode,&statusBufferLength,NULL);

	if ( statusCode!=200 ) {
      printf("Sorry, This image is not available for download!");
      return 2;
   	}

	hOut = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hOut == INVALID_HANDLE_VALUE) {
		ERROR("Create file error!");
		InternetCloseHandle(hUrl);
		return 0;
	}


	while (InternetReadFile(hUrl, (LPVOID)lpBuffer, 4096, &dwBytesRead) && dwBytesRead>0) {
		//ZeroMemory(lpBuffer, 4096);
		WriteFile(hOut, &lpBuffer[0], dwBytesRead, &dwBytesWritten, NULL);
		//lpBuffer = NULL;
	}
	free(lpBuffer);

	CloseHandle(hOut);
	InternetCloseHandle(hUrl);
	return 1;
}


int notification(const wchar_t* tip,const wchar_t* title,const wchar_t* info){

	NOTIFYICONDATAW  nid;

    //memset( &nid, 0, sizeof( nid ) );

    nid.cbSize              = 	NOTIFYICONDATA_V2_SIZE; //sizeof( nid );
    //nid.hWnd                = NULL;
    //nid.uID                 = NULL;

    nid.uFlags              = NIF_INFO | NIF_ICON | NIF_MESSAGE | NIF_TIP;
 nid.dwInfoFlags=NIIF_INFO;

wcscpy(nid.szTip,tip);
wcscpy(nid.szInfoTitle,title);
wcscpy(nid.szInfo,info);
    // Make sure we avoid conflict with other messages
    //nid.uCallbackMessage = WM_APP + 0xDDC;
    //nid.uCallbackMessage    =  0;
    //  Uncomment this if you've got your own icon.  GetModuleHandle( NULL )
    //  gives us our HINSTANCE.  I hate globals.
//  nid.hIcon               = LoadSmallIcon( GetModuleHandle( NULL ), uIcon );

    //  Comment this if you've got your own icon.
    {
    wchar_t    szIconFile[512];

    GetSystemDirectoryW( szIconFile, sizeof( szIconFile ) );
    if ( szIconFile[ wcslen( szIconFile ) - 1 ] != '\\' )
        wcscat( szIconFile, L"\\" );
    wcscat( szIconFile, L"shell32.dll" );
    //  Icon #23 (0-indexed) in shell32.dll is a "help" icon.
    ExtractIconExW( szIconFile, 49,&(nid.hIcon), NULL,  1 );

    }

HWND hWnd =CreateWindowExW(0, L"STATIC", NULL, 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
//if (!hWnd) return FALSE;
nid.hWnd=hWnd;

return Shell_NotifyIconW( NIM_ADD, &nid );
}


int wmain(int argc, wchar_t *argv[]){
    enum { NOTIFY, MESSAGEBOX, SILENT } mode = NOTIFY;
    wchar_t index[7]=L"&idx=0";
    wchar_t region[7]=L"&cc=";
    size_t optind;
    for (optind = 1; optind < argc && argv[optind][0] == '-' ; optind++) {
        for (int i=1;char c=argv[optind][i];i++) {
            switch(c){
        case 's': mode=SILENT; break;
        case 'm': mode = MESSAGEBOX; break;
        case 'i': 
            if (optind+1<argc) {
            index[5]=argv[optind+1][0];
            wcscat(bingHP,index);
            optind++; i--;
             }
            break;
        case 'r': 
            if (optind+1<argc) {
            wcsncat(region,argv[optind+1],2);
            wcscat(bingHP,region);
            optind=optind+1;
             }
            break;
        case 'v': verbose = true; break;    
        case 'u': unofficial = true; break;
        default:
            //AttachConsole(-1);
            char usage[400];
            sprintf(usage, "Usage: %s [option...]\n\n\
Options:\n\
-s \t Silent mode. Don't show any info about the image.\n\
-m \t Show a messagebox instead of system notification.\n\
-i <index number> \t Use image of <index> days ago.\n\
-r <region code> \t Use the specified region code. e.g.: us\n\
-v \t Show error messages.  ", argv[0]);
            MessageBoxA(
            NULL,
            usage,
            APP_NAME,
            MB_ICONWARNING | MB_OK
            );
            exit(EXIT_FAILURE);
        }  } 
    }


hRootHandle = InternetOpenW(L"MyBrowser",INTERNET_OPEN_TYPE_PRECONFIG,NULL, NULL, 0);
wchar_t imgURL[100],title[100],copyright[256];
HANDLE hFile     = INVALID_HANDLE_VALUE;
DWORD dwRetVal = 0;
wchar_t lpTempPathBuffer[MAX_PATH];


	if(!getImgInfo(imgURL,title,copyright)){ 
		InternetCloseHandle(hRootHandle);
		return EXIT_FAILURE;
		} 

	 //  Gets the temp path env string (no guarantee it's a valid path).
    dwRetVal = GetTempPathW(MAX_PATH,          // length of the buffer
                           lpTempPathBuffer); // buffer for path
    if (dwRetVal > MAX_PATH || (dwRetVal == 0))
    {
        ERROR("GetTempPath failed");
        CloseHandle(hFile);
        return EXIT_FAILURE;
    }
    
	wcscat(lpTempPathBuffer,L"bing-image.jpg");
    int result=downloadFile(imgURL,lpTempPathBuffer);
	if(!result){
		InternetCloseHandle(hRootHandle);
        if (!verbose)
            return EXIT_FAILURE;
        if(result==2)
            wcscat(copyright,L"\nSorry, This image is not available for download!");
		}

	InternetCloseHandle(hRootHandle);
	if(!setWall(lpTempPathBuffer)){
        ERROR("Can't set the wallpaper.\n Restarting the app as administrator might elevate the issue.");
		return EXIT_FAILURE;
    }

//gcc bingNotify-unicode.cpp -lwininet -D"UNICODE"
    if (mode==NOTIFY)
        {notification(L"Bing Wallpaper",title,copyright); Sleep(15000);}
    else if (mode==MESSAGEBOX)
        MessageBoxW(
            NULL,
            copyright,
            title,
            MB_ICONINFORMATION | MB_OK
        );
    
	return 0;
    }
