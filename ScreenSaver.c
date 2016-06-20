#include <windows.h>
#include <time.h>
#include <stdlib.h>
#include <mmsystem.h>
#pragma comment(lib,"winmm.lib")

#define ROWS 60
#define COLS 80
#define WM_LOOP WM_USER + 1

HINSTANCE gInstance;
int ScreenWidth, ScreenHeight, dx, dy, bwidth, bheight, x, y;
BOOL bProcessing = FALSE, bStartup = TRUE, bMouseMove = FALSE;
HWND hMainWindow, hDesktopWindow;
HDC hdcDesktop, hdcMem, hdcMain;
HBITMAP hbmpTemp1, hbmpOld1, hbmpTemp2, hbmpOld2;
HFONT hFont, hOldFont;
MMRESULT timerID;

char* strs[3] = {"Linux.. good", "Windows.. bad", "MacOS.. evil"};
int cnt = 3, i = 0, di = COLS * ROWS;

void CALLBACK TimeProc(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
   PostMessage(hMainWindow, WM_LOOP, 0, 0);
}


LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   HDC hdc;
   PAINTSTRUCT ps;

   switch (uMsg)
   {
      case WM_CREATE:
         bProcessing = TRUE;
         srand ((unsigned int) time(NULL) );

         hDesktopWindow = GetDesktopWindow();
         hdcDesktop = GetDC(hDesktopWindow);
         
         hdcMem = CreateCompatibleDC(hdcDesktop);
         hdcMain = CreateCompatibleDC(hdcDesktop);
         hbmpTemp1 = CreateCompatibleBitmap(hdcDesktop, ScreenWidth, ScreenHeight);
         hbmpOld1 = (HBITMAP) SelectObject(hdcMem, hbmpTemp1);
         hbmpTemp2 = CreateCompatibleBitmap(hdcDesktop, ScreenWidth, ScreenHeight);
         hbmpOld2 = (HBITMAP) SelectObject(hdcMain, hbmpTemp2);

         hFont = CreateFont(-MulDiv(96, GetDeviceCaps(hdcMem, LOGPIXELSY), 72),
                            0, 0, 0, 0, TRUE, 0, 0, 0, 0, 0, 0, 0, "Times New Roman");
         hOldFont = (HFONT) SelectObject(hdcMem, hFont);
         
         BitBlt(hdcMem,  0, 0, ScreenWidth, ScreenHeight, hdcDesktop, 0, 0, SRCCOPY);
         BitBlt(hdcMain, 0, 0, ScreenWidth, ScreenHeight, hdcDesktop, 0, 0, SRCCOPY);
         
         ReleaseDC(hDesktopWindow, hdcDesktop);

         dx = bwidth = ScreenWidth / COLS;
         dy = bheight = ScreenHeight / ROWS;
         x = -dx;
         y = -dy;
         ShowCursor(FALSE);
         timeBeginPeriod(1);
         timerID = timeSetEvent(2, 2, TimeProc, 0, TIME_PERIODIC);
      break;
      case WM_PAINT:
         bProcessing = TRUE;
         hdc = BeginPaint(hwnd, &ps);
         BitBlt(hdc, 0, 0, ScreenWidth, ScreenHeight, hdcMain, 0, 0, SRCCOPY);
         EndPaint(hwnd, &ps);
         bProcessing = FALSE;
      break;
      case WM_LOOP:
         if (!bProcessing)
         {
            bProcessing = TRUE;

            if ((x == 0) && (y == 0) && (di >= COLS * ROWS))
            {
               BitBlt(hdcMem, 0, 0, ScreenWidth, ScreenHeight, hdcMain, 0, 0, SRCCOPY);
			   TextOut(hdcMem, 30 + rand() % 200, 50 + rand() % 200, strs[i], strlen(strs[i]));
               di = 0;
               if (++i >= cnt)
                  i = 0;
            }

            x += dx;
            y += dy;
            di++;
            
            if (x > ScreenWidth)
               x += (dx = -bwidth);
            else if (x < 0)
               x += (dx = bwidth);

            if (y > ScreenHeight)
               y += (dy = -bheight);
            else if (y < 0)
               y += (dy = bheight);
            
            BitBlt(hdcMain, x, y, bwidth, bheight, hdcMem, x, y, SRCINVERT);
            InvalidateRect(hMainWindow, NULL, FALSE);
            bProcessing = FALSE;
         }
      break;
      case WM_KEYDOWN:
      case WM_LBUTTONDOWN:
      case WM_RBUTTONDOWN:
         PostMessage(hMainWindow, WM_CLOSE, 0, 0);
      break;
      //case WM_MOUSEMOVE:
      //   if (bMouseMove)
      //      PostMessage(hMainWindow, WM_CLOSE, 0, 0);
      //break;
      case WM_CLOSE:
         timeKillEvent(timerID);
         timeEndPeriod(1);
         DestroyWindow(hMainWindow);
      break;
      case WM_DESTROY:
         ShowCursor(TRUE);
         SelectObject(hdcMem, hOldFont);
         DeleteObject(hFont);
         SelectObject(hdcMem, hbmpOld1);
         DeleteObject(hbmpTemp1);
         SelectObject(hdcMain, hbmpOld2);
         DeleteObject(hbmpTemp2);
         DeleteDC(hdcMem);
         DeleteDC(hdcMain);
         PostQuitMessage(0);
      break;
      default: 
         return DefWindowProc(hwnd, uMsg, wParam, lParam);
   }
   return 0;
}


void RegisterMyClass(HINSTANCE hinst, WNDPROC wproc, char* clsName, int bkColor)
{
   WNDCLASSEX wndclass;   

   wndclass.cbSize = sizeof(WNDCLASSEX);
   wndclass.hbrBackground = (HBRUSH) GetStockObject(bkColor);
   wndclass.hCursor = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
   wndclass.hInstance = hinst;
   wndclass.lpfnWndProc = wproc;
   wndclass.lpszClassName = clsName;
   wndclass.style = CS_HREDRAW | CS_VREDRAW;
   wndclass.cbClsExtra = 0;
   wndclass.cbWndExtra = 0;
   wndclass.hIcon = NULL;
   wndclass.hIconSm = NULL;
   wndclass.lpszMenuName = NULL;

   if (RegisterClassEx(&wndclass) == 0)
   {
      FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0, "Failed to register window class", 250, NULL);
      MessageBox(NULL, "Failed to register window class", "RegisterClassEx", MB_ICONERROR | MB_OK);
   }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
   int msgResult;
   MSG msg;
   gInstance = hInstance;
   ScreenWidth = GetSystemMetrics(SM_CXSCREEN);
   ScreenHeight = GetSystemMetrics(SM_CYSCREEN);

   RegisterMyClass(hInstance, (WNDPROC) WindowProc, "ScreenSaver", WHITE_BRUSH);   
   hMainWindow = CreateWindowEx(WS_EX_TOPMOST, "ScreenSaver", NULL, WS_VISIBLE | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                                0, 0, ScreenWidth, ScreenHeight, NULL, NULL, hInstance, NULL);

   if (hMainWindow == NULL)
   {
      FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0, "Failed to create window", 250, NULL);
      MessageBox(hMainWindow, "Failed to create window", "CreateWindowEx", MB_ICONERROR | MB_OK);
      return 0;
   }

   ShowWindow(hMainWindow, nShowCmd);
   UpdateWindow(hMainWindow);

   while ((msgResult = GetMessage(&msg, NULL, 0, 0)) != 0)
   {
      if (msgResult < 1)
      {
         FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0, "GetMessage failed", 250, NULL);
         MessageBox(NULL, "GetMessage failed", "GetMessage", MB_ICONERROR | MB_OK);
         return 0;
      }

      TranslateMessage(&msg);
      DispatchMessage(&msg);
   }

   return 0;
}



