#include <windows.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <mmsystem.h>
#include "header.h"

#pragma comment(lib,"winmm.lib")

#define WM_MOVEOBJECTS WM_USER+1
#define INTROTIMER 1
#define NUMSTARS  300
#define ID_BUTTON 201
#define COMMENTDURATION 100

#define FPS 15
#define OBJSPEED (FPS/3) - 3
#define STARSPEED FPS - 13
#define MISSILESPEED (FPS/2) - 2
#define DANCERSPEED FPS - 12

MMRESULT timerID;
char lpszWindowClassName[] = "main";
const int cxSize=800, cySize=640;
int currPen, currBrush;
BOOL bIntro = TRUE;
HWND hwnd, hButton;   
HDC hdc, hdcMem;
HBITMAP hbmpRedButton;
BOOL LeftPressed = FALSE, RightPressed = FALSE, bFirePressed = FALSE, bTimerKilled = FALSE, bProcessing = FALSE;
int KeyCode = 0;
int bmpCPU2CommentCounter = 0, bmpCPU1CommentCounter = 0, bmpPlayerCommentCounter = 0;

char *sIntro = "";

typedef struct {
   HBITMAP hPic;
   HBITMAP hMask;
   int left;
   int top;
   int width;
   int height; 
   int speed;
   BOOL visible; } BMP;

typedef struct {
   int x;
   int y; } STAR;

STAR stars[NUMSTARS];

BMP bmpPlayer, bmpCPU1, bmpCPU2, bmpCpu1_Missile1, bmpCpu1_Missile2, bmpCpu1_Missile3, 
    bmpCpu2_Missile1, bmpCpu2_Missile2, bmpCpu2_Missile3, bmpPlayer_Missile1, bmpPlayer_Missile2,
    bmpPlayer_Missile3, bmpDancerLeft, bmpDancerRight, bmpCPU2Comment, bmpCPU1Comment, bmpPlayerComment;

char DebugStr[100];


void CALLBACK TimeProc(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
   if (!bProcessing)
      PostMessage(hwnd, WM_MOVEOBJECTS, 0, 0);
}

void FillStars(int cxSize, int cySize)
{
   int i;
   for(i=0; i<NUMSTARS; i++)
   {
      stars[i].x = rand() % cxSize;
      stars[i].y = rand() % cySize;
   }
}

void UpdateStars(int cySize)
{
   int i;
   for(i=0; i<NUMSTARS; i++)
   {
      if (stars[i].y >= cySize)
         stars[i].y = 1;
      else
         stars[i].y += STARSPEED;
   }
}


void MissileHitEnemy(BMP *missile, BMP enemy, BMP *comment, int *commentCounter)
{
   BMP* dancer;

   if (!missile->visible)
      return;

   if ((missile->left >= enemy.left) && (missile->left <= (enemy.left + enemy.width - 10)) &&
       (missile->top <= enemy.height))
   {
      comment->visible = TRUE;
      missile->visible = FALSE;
      *commentCounter = 0;
   }

   dancer = &bmpDancerLeft;
   if (dancer->visible && (missile->left >= dancer->left) && (missile->left <= (dancer->left + dancer->width - 10)) &&
       (missile->top <= (dancer->top + dancer->height)))
      missile->visible = FALSE;

   dancer = &bmpDancerRight;
   if (dancer->visible && (missile->left >= dancer->left) && (missile->left <= (dancer->left + dancer->width - 10)) &&
       (missile->top <= (dancer->top + dancer->height)))
      missile->visible = FALSE;
}


void MissileHitMe(BMP *missile, BMP me, BMP *comment, int *commentCounter)
{
   BMP *dancer;
   if (!missile->visible)
      return;

   if ((missile->left >= me.left) && (missile->left <= (me.left + me.width - 10)) &&
       ((missile->top + missile->height) >= me.top))
   {
      comment->visible = TRUE;
      missile->visible = FALSE;
      *commentCounter = 0;
   }

   dancer = &bmpDancerLeft;
   if (dancer->visible && (missile->left >= dancer->left) && (missile->left <= (dancer->left + dancer->width - 10)) &&
       ((missile->top + missile->height) >= dancer->top))
       missile->visible = FALSE;

   dancer = &bmpDancerRight;
   if (dancer->visible && (missile->left >= dancer->left) && (missile->left <= (dancer->left + dancer->width - 10)) &&
       ((missile->top + missile->height) >= dancer->top))
       missile->visible = FALSE;
}


void LaunchMissile(BMP *missile, BMP enemy)
{
   missile->visible = TRUE;
   missile->left = enemy.left + enemy.width / 2;
   missile->top = enemy.height;
}

void Attack(BMP *missile, BMP Player)
{
   missile->visible = TRUE;
   missile->left = Player.left + Player.width / 2;
   missile->top = Player.top - missile->height;
}

void UpdateObjects()
{
   static int CPU2Direction = OBJSPEED;
   static int CPU1Direction = OBJSPEED;
   static BOOL bLeftDancer = TRUE;
   static int dancercount = 0, dancecount=0;

   if (bProcessing)
      return;

   bProcessing = TRUE;

   UpdateStars(cySize);

   dancercount++;
   if (dancercount>=400)
   {
      bmpDancerLeft.left += DANCERSPEED;
      bmpDancerRight.left += DANCERSPEED;
      bmpDancerLeft.visible = bLeftDancer;
      bmpDancerRight.visible = !bLeftDancer;
      dancecount++;
      if (dancecount >= 15)
      {
         dancecount = 0;
         bLeftDancer = !bLeftDancer;
      }
   }

   if (bmpDancerLeft.left > cxSize)
   {
      dancercount = 0;
      bmpDancerLeft.visible = FALSE;
      bmpDancerRight.visible = FALSE;
      bmpDancerLeft.left = 0;
      bmpDancerRight.left = 0;
   }

   if ((bmpCPU2Comment.visible) && ((++bmpCPU2CommentCounter) >= COMMENTDURATION))
   {
      bmpCPU2CommentCounter = 0;
      bmpCPU2Comment.visible = FALSE;
   }

   if ((bmpCPU1Comment.visible) && ((++bmpCPU1CommentCounter) >= COMMENTDURATION))
   {
      bmpCPU1CommentCounter = 0;
      bmpCPU1Comment.visible = FALSE;
   }

   if ((bmpPlayerComment.visible) && ((++bmpPlayerCommentCounter) >= COMMENTDURATION))
   {
      bmpPlayerCommentCounter = 0;
      bmpPlayerComment.visible = FALSE;
   }
   
   if ((bmpCPU2.left >= (cxSize - bmpCPU2.width)))
      CPU2Direction = -OBJSPEED;
   else if (bmpCPU2.left < 0) 
      CPU2Direction = OBJSPEED;         
   else if ((rand()%100) < 1)
      CPU2Direction *= -1;         

   bmpCPU2.left += CPU2Direction;

   if ((bmpCPU1.left >= (cxSize - bmpCPU1.width))) 
      CPU1Direction = -OBJSPEED;
   else if (bmpCPU1.left < 0) 
      CPU1Direction = OBJSPEED;         
   else if ((rand()%100) < 1)
      CPU1Direction *= -1;         

   bmpCPU1.left += CPU1Direction;

   if ((rand()%100) < 3)
   {
      if (!bmpCpu1_Missile1.visible)
         LaunchMissile(&bmpCpu1_Missile1, bmpCPU2);
      else if (!bmpCpu1_Missile2.visible)
         LaunchMissile(&bmpCpu1_Missile2, bmpCPU2);
      else if (!bmpCpu1_Missile3.visible)
         LaunchMissile(&bmpCpu1_Missile3, bmpCPU2);
   }

   if ((rand()%100) < 3)
   {
      if (!bmpCpu2_Missile1.visible)
         LaunchMissile(&bmpCpu2_Missile1, bmpCPU1);
      else if (!bmpCpu2_Missile2.visible)
         LaunchMissile(&bmpCpu2_Missile2, bmpCPU1);
      else if (!bmpCpu2_Missile3.visible)
         LaunchMissile(&bmpCpu2_Missile3, bmpCPU1);
   }

   if (bmpCpu1_Missile1.visible)
      if ((bmpCpu1_Missile1.top += MISSILESPEED) >= cySize)
         bmpCpu1_Missile1.visible = FALSE;

   if (bmpCpu1_Missile2.visible)
      if ((bmpCpu1_Missile2.top += MISSILESPEED) >= cySize)
         bmpCpu1_Missile2.visible = FALSE;

   if (bmpCpu1_Missile3.visible)
      if ((bmpCpu1_Missile3.top += MISSILESPEED) >= cySize)
         bmpCpu1_Missile3.visible = FALSE;

   if (bmpCpu2_Missile1.visible)
      if ((bmpCpu2_Missile1.top += MISSILESPEED) >= cySize)
         bmpCpu2_Missile1.visible = FALSE;

   if (bmpCpu2_Missile2.visible)
      if ((bmpCpu2_Missile2.top += MISSILESPEED) >= cySize)
         bmpCpu2_Missile2.visible = FALSE;

   if (bmpCpu2_Missile3.visible)
      if ((bmpCpu2_Missile3.top += MISSILESPEED) >= cySize)
         bmpCpu2_Missile3.visible = FALSE;

   if (bmpPlayer_Missile1.visible)
      if ((bmpPlayer_Missile1.top -= MISSILESPEED) <= 0)
         bmpPlayer_Missile1.visible = FALSE;

   if (bmpPlayer_Missile2.visible)
      if ((bmpPlayer_Missile2.top -= MISSILESPEED) <= 0)
         bmpPlayer_Missile2.visible = FALSE;

   if (bmpPlayer_Missile3.visible)
      if ((bmpPlayer_Missile3.top -= MISSILESPEED) <= 0)
         bmpPlayer_Missile3.visible = FALSE;

   if (LeftPressed && (bmpPlayer.left >= 3))
      bmpPlayer.left -= (OBJSPEED + 2);

   if (RightPressed && (bmpPlayer.left <= (cxSize - bmpPlayer.width)))
         bmpPlayer.left += (OBJSPEED + 2);

   bmpCPU2Comment.left = bmpCPU2.left - bmpCPU2.width / 2;
   bmpCPU1Comment.left = bmpCPU1.left - bmpCPU1.width / 2;
   bmpPlayerComment.left = bmpPlayer.left - bmpPlayer.width / 2;

   if (bFirePressed)
   {
      if (!bmpPlayer_Missile1.visible)
         Attack(&bmpPlayer_Missile1, bmpPlayer);
      else if (!bmpPlayer_Missile2.visible)
         Attack(&bmpPlayer_Missile2, bmpPlayer);
      else if (!bmpPlayer_Missile3.visible)
         Attack(&bmpPlayer_Missile3, bmpPlayer);
      bFirePressed = FALSE;
   }

   MissileHitEnemy(&bmpPlayer_Missile1, bmpCPU2, &bmpCPU2Comment, &bmpCPU2CommentCounter);
   MissileHitEnemy(&bmpPlayer_Missile2, bmpCPU2, &bmpCPU2Comment, &bmpCPU2CommentCounter);
   MissileHitEnemy(&bmpPlayer_Missile3, bmpCPU2, &bmpCPU2Comment, &bmpCPU2CommentCounter);

   MissileHitEnemy(&bmpPlayer_Missile1, bmpCPU1, &bmpCPU1Comment, &bmpCPU1CommentCounter);
   MissileHitEnemy(&bmpPlayer_Missile2, bmpCPU1, &bmpCPU1Comment, &bmpCPU1CommentCounter);
   MissileHitEnemy(&bmpPlayer_Missile3, bmpCPU1, &bmpCPU1Comment, &bmpCPU1CommentCounter);

   MissileHitMe(&bmpCpu1_Missile1, bmpPlayer, &bmpPlayerComment, &bmpPlayerCommentCounter);
   MissileHitMe(&bmpCpu1_Missile2, bmpPlayer, &bmpPlayerComment, &bmpPlayerCommentCounter);
   MissileHitMe(&bmpCpu1_Missile3, bmpPlayer, &bmpPlayerComment, &bmpPlayerCommentCounter);

   MissileHitMe(&bmpCpu2_Missile1, bmpPlayer, &bmpPlayerComment, &bmpPlayerCommentCounter);
   MissileHitMe(&bmpCpu2_Missile2, bmpPlayer, &bmpPlayerComment, &bmpPlayerCommentCounter);
   MissileHitMe(&bmpCpu2_Missile3, bmpPlayer, &bmpPlayerComment, &bmpPlayerCommentCounter);

   InvalidateRect(hwnd, NULL, FALSE);
   bProcessing = FALSE;
}

void DrawSky(HDC hdc_)
{
   int i;
   HDC hdcBuffer;
   HBITMAP hbmpOld, hbmpTemp;

   hdcBuffer = CreateCompatibleDC(hdc_);
   hbmpTemp = CreateCompatibleBitmap(hdcBuffer, cxSize, cySize);
   hbmpOld = (HBITMAP) SelectObject(hdcBuffer, hbmpTemp);
   FloodFill(hdcBuffer, 0, 0, RGB(0, 0, 0));

   for(i=0; i<NUMSTARS; i++)
      SetPixel(hdcBuffer, stars[i].x, stars[i].y, RGB(255, 255, 255));

   BitBlt(hdc_, 0, 0, cxSize, cySize, hdcBuffer, 0, 0, SRCCOPY);

   SelectObject(hdcBuffer, hbmpOld);
   DeleteObject(hbmpTemp);
   DeleteDC(hdcBuffer);
}

void LoadTransparentBitmap(BMP *bmp, int bmpResource, int bmpResourceMask, HWND hwndMessage)
{     
   BITMAP bitmap;
   
   bmp->hPic = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(bmpResource));
   if(bmp->hPic == NULL)
      MessageBox(hwndMessage, MAKEINTRESOURCE(bmpResource), "Error Loading Resource ID:", MB_OK | MB_ICONEXCLAMATION);

   bmp->hMask = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(bmpResourceMask));
   if(bmp->hMask == NULL)
      MessageBox(hwndMessage, MAKEINTRESOURCE(bmpResourceMask), "Error Loading Resource ID:", MB_OK | MB_ICONEXCLAMATION);

   if (GetObject(bmp->hPic, sizeof(BITMAP), &bitmap) != 0)
   {
      bmp->width = bitmap.bmWidth;
      bmp->height = bitmap.bmHeight;
   }

   bmp->visible = TRUE;
}


void DrawTransparent(HDC hdc_, BMP bmp)
{
   HBITMAP hbmpOld;
   HDC hdcBuffer;
   hdcBuffer = CreateCompatibleDC(hdc_);
   hbmpOld = (HBITMAP) SelectObject(hdcBuffer, bmp.hMask);
   BitBlt(hdc_, bmp.left, bmp.top, bmp.width, bmp.height, hdcBuffer, 0, 0, SRCAND);
   SelectObject(hdcBuffer, hbmpOld);
   DeleteDC(hdcBuffer);

   hdcBuffer = CreateCompatibleDC(hdc_);
   hbmpOld = (HBITMAP) SelectObject(hdcBuffer, bmp.hPic);
   BitBlt(hdc_, bmp.left, bmp.top, bmp.width, bmp.height, hdcBuffer, 0, 0, SRCPAINT);
   SelectObject(hdcBuffer, hbmpOld);
   DeleteDC(hdcBuffer);
}



void CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
   WNDCLASSEX wndclass;
   MSG msg;

   wndclass.cbClsExtra = 0;
   wndclass.cbSize = sizeof(WNDCLASSEX);
   wndclass.cbWndExtra = 0;
   wndclass.hbrBackground = NULL;
   wndclass.hCursor = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
   wndclass.hIcon = NULL;
   wndclass.hInstance = hInstance;
   wndclass.lpfnWndProc = (WNDPROC) WndProc;
   wndclass.lpszClassName = lpszWindowClassName;
   wndclass.lpszMenuName = NULL;
   wndclass.style = WS_OVERLAPPED;
   wndclass.hIconSm = NULL;

   RegisterClassEx(&wndclass);
   hwnd = CreateWindow(lpszWindowClassName, "X by 2 Space Invaders", WS_BORDER | WS_CAPTION | WS_VISIBLE | WS_BORDER | WS_SYSMENU, 
                       0, 0, cxSize, cySize, NULL, NULL, hInstance, 0);

   ShowWindow(hwnd, nShowCmd);
   UpdateWindow(hwnd);

   while (GetMessage(&msg, hwnd, 0, 0) > 0)
   {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
   }

   return 0;
}


void CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   PAINTSTRUCT ps;
   HBITMAP hbmpTemp, hbmpOld;
   int scrWidth, scrHeight;
   static int currMessage = 0;
   
   switch (uMsg)
   {
   case WM_CREATE: 
      srand ( (int) time(NULL) );
      FillStars(cxSize, cySize);

      currPen = WHITE_PEN;
      currBrush = WHITE_BRUSH;

      hdc = GetDC(hwnd);
      scrWidth = GetDeviceCaps(hdc, HORZRES);
      scrHeight = GetDeviceCaps(hdc, VERTRES);
      ReleaseDC(hwnd, hdc);

      SetWindowPos(hwnd, HWND_TOP, (scrWidth / 2) - (cxSize / 2), (scrHeight / 2) - (cySize / 2), 
                   0, 0, SWP_NOSIZE);

      LoadTransparentBitmap(&bmpPlayer, IDB_PLAYER, IDB_PLAYER_MASK, hwnd);
      LoadTransparentBitmap(&bmpCPU1, IDB_CPU1, IDB_CPU1_MASK, hwnd);
      LoadTransparentBitmap(&bmpCPU2, IDB_CPU2, IDB_CPU2_MASK, hwnd);
      LoadTransparentBitmap(&bmpCpu1_Missile1, IDB_MISSILEDOWN, IDB_MISSILEDOWN_MASK, hwnd);
      LoadTransparentBitmap(&bmpCpu1_Missile2, IDB_MISSILEDOWN, IDB_MISSILEDOWN_MASK, hwnd);
      LoadTransparentBitmap(&bmpCpu1_Missile3, IDB_MISSILEDOWN, IDB_MISSILEDOWN_MASK, hwnd);
      LoadTransparentBitmap(&bmpCpu2_Missile1, IDB_MISSILEDOWN, IDB_MISSILEDOWN_MASK, hwnd);
      LoadTransparentBitmap(&bmpCpu2_Missile2, IDB_MISSILEDOWN, IDB_MISSILEDOWN_MASK, hwnd);
      LoadTransparentBitmap(&bmpCpu2_Missile3, IDB_MISSILEDOWN, IDB_MISSILEDOWN_MASK, hwnd);
      LoadTransparentBitmap(&bmpPlayer_Missile1, IDB_MISSILEUP, IDB_MISSILEUP_MASK, hwnd);
      LoadTransparentBitmap(&bmpPlayer_Missile2, IDB_MISSILEUP, IDB_MISSILEUP_MASK, hwnd);
      LoadTransparentBitmap(&bmpPlayer_Missile3, IDB_MISSILEUP, IDB_MISSILEUP_MASK, hwnd);
      LoadTransparentBitmap(&bmpCPU2Comment, IDB_COMMENT2, IDB_COMMENT2_MASK, hwnd);
      LoadTransparentBitmap(&bmpCPU1Comment, IDB_COMMENT1, IDB_COMMENT1_MASK, hwnd);
      LoadTransparentBitmap(&bmpPlayerComment, IDB_PCOMMENT, IDB_PCOMMENT_MASK, hwnd);
      bmpCPU2Comment.visible = FALSE;
      bmpCPU1Comment.visible = FALSE;
      bmpPlayerComment.visible = FALSE;

      LoadTransparentBitmap(&bmpDancerRight, IDB_DANCERRIGHT, IDB_DANCERRIGHT_MASK, hwnd);
      LoadTransparentBitmap(&bmpDancerLeft, IDB_DANCERLEFT, IDB_DANCERLEFT_MASK, hwnd);

      bmpDancerRight.visible = FALSE;
      bmpDancerLeft.visible = FALSE;

      bmpDancerRight.top = (cySize / 2) - (bmpDancerRight.height / 2) - 20;
      bmpDancerLeft.top = (cySize / 2) - (bmpDancerLeft.height / 2) - 20;

      hbmpRedButton = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_REDBUTTON));
      if(hbmpRedButton == NULL)
         MessageBox(hwnd, "Could not load IDB_REDBUTTON!", "Error", MB_OK | MB_ICONEXCLAMATION);

      bmpCPU2.left = rand() % cxSize;
      bmpCPU1.left = rand() % cxSize;
      bmpPlayer.top = cySize - bmpPlayer.height - 20;
      bmpPlayer.left = cxSize / 2;
      bmpCPU2Comment.top = bmpCPU2.height;
      bmpCPU1Comment.top = bmpCPU1.height;
      bmpPlayerComment.top = bmpPlayer.top - bmpPlayerComment.height;
      
      hButton = CreateWindow("BUTTON", NULL, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_BITMAP, 
                  (cxSize / 2) - 200, (cySize / 2) - 100, 398, 207, hwnd, (HMENU)ID_BUTTON, GetModuleHandle(NULL), NULL);
      
      ShowWindow(hButton, SW_SHOW);
      SendMessage(hButton, BM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) hbmpRedButton);

      timeBeginPeriod(1);
   break;
   case WM_KEYDOWN:
      KeyCode = (int) wParam;
      switch (KeyCode)
      {
         case 37: LeftPressed = TRUE; break;
         case 38: bFirePressed = TRUE; break;
         case 39: RightPressed = TRUE; break;
      }
   break;
   case WM_KEYUP:
      KeyCode = (int) wParam;
         switch (KeyCode)
         {
            case 37: LeftPressed = FALSE; break;
            case 38: break; /* bFirePressed handled elsewhere */
            case 39: RightPressed = FALSE; break;
         }
   break;
   case WM_COMMAND:
      SetFocus(hwnd);
      EnableWindow(hButton, FALSE);
      DestroyWindow(hButton);
      timerID = timeSetEvent(FPS, 0, TimeProc, 0, TIME_PERIODIC);
      bIntro = FALSE;
   break;
   case WM_TIMER:

   break;
   case WM_MOVEOBJECTS:
      UpdateObjects();
   break;
   case WM_PAINT:
      if (bIntro)
      {
         hdc = BeginPaint(hwnd, &ps);
         Rectangle(hdc, 0, 0, cxSize, cySize);
         TextOut(hdc, (cxSize / 2) - 200, (cySize / 2) - 130, sIntro, strlen(sIntro));
         TextOut(hdc, (cxSize / 2) - 200, (cySize / 2) + 110, "LEFT/RIGHT arrows to move, UP to fire.",  38);
         EndPaint(hwnd, &ps);
      }
      else
      {
         /* Set up memory hdc */
         hdc = BeginPaint(hwnd, &ps);
         hbmpTemp = CreateCompatibleBitmap(hdc, cxSize, cySize);
         hdcMem = CreateCompatibleDC(hdc);
         hbmpOld = (HBITMAP) SelectObject(hdcMem, hbmpTemp);

         /* Draw whatever we need on memory hdc*/
         DrawSky(hdcMem);

         if (bmpCPU1.visible)
            DrawTransparent(hdcMem, bmpCPU1);

         if (bmpCPU2.visible)
            DrawTransparent(hdcMem, bmpCPU2);

         if (bmpPlayer.visible)
               DrawTransparent(hdcMem, bmpPlayer);

         if (bmpCpu1_Missile1.visible)
               DrawTransparent(hdcMem, bmpCpu1_Missile1);
         if (bmpCpu1_Missile2.visible)
               DrawTransparent(hdcMem, bmpCpu1_Missile2);
         if (bmpCpu1_Missile3.visible)
               DrawTransparent(hdcMem, bmpCpu1_Missile3);

         if (bmpCpu2_Missile1.visible)
               DrawTransparent(hdcMem, bmpCpu2_Missile1);
         if (bmpCpu2_Missile2.visible)
               DrawTransparent(hdcMem, bmpCpu2_Missile2);
         if (bmpCpu2_Missile3.visible)
               DrawTransparent(hdcMem, bmpCpu2_Missile3);

         if (bmpPlayer_Missile1.visible)
               DrawTransparent(hdcMem, bmpPlayer_Missile1);
         if (bmpPlayer_Missile2.visible)
               DrawTransparent(hdcMem, bmpPlayer_Missile2);
         if (bmpPlayer_Missile3.visible)
               DrawTransparent(hdcMem, bmpPlayer_Missile3);

         if (bmpCPU2Comment.visible)
            DrawTransparent(hdcMem, bmpCPU2Comment);
         if (bmpCPU1Comment.visible)
            DrawTransparent(hdcMem, bmpCPU1Comment);
         if (bmpPlayerComment.visible)
            DrawTransparent(hdcMem, bmpPlayerComment);

         if (bmpDancerLeft.visible)
            DrawTransparent(hdcMem, bmpDancerLeft);
         if (bmpDancerRight.visible)
            DrawTransparent(hdcMem, bmpDancerRight);

         /* Copy memory hdc onto main hdc*/
         BitBlt(hdc, 0, 0, cxSize, cySize, hdcMem, 0, 0, SRCCOPY);
         
         /* Clean up*/
         SelectObject(hdcMem, hbmpOld);
         DeleteDC(hdcMem);
         DeleteObject(hbmpTemp);
         EndPaint(hwnd, &ps);
      }
   break;
   case WM_DESTROY:
      timeKillEvent(timerID);
      timeEndPeriod(1);

      DeleteObject(bmpPlayer.hPic);
      DeleteObject(bmpCPU1.hPic);
      DeleteObject(bmpCPU2.hPic);
      DeleteObject(bmpCpu1_Missile1.hPic);
      DeleteObject(bmpCpu1_Missile2.hPic);
      DeleteObject(bmpCpu1_Missile3.hPic);
      DeleteObject(bmpCpu2_Missile1.hPic);
      DeleteObject(bmpCpu2_Missile2.hPic);
      DeleteObject(bmpCpu2_Missile3.hPic);
      DeleteObject(bmpPlayer_Missile1.hPic);
      DeleteObject(bmpPlayer_Missile2.hPic);
      DeleteObject(bmpPlayer_Missile3.hPic);

      DeleteObject(bmpPlayer.hMask);
      DeleteObject(bmpCPU1.hMask);
      DeleteObject(bmpCPU2.hMask);
      DeleteObject(bmpCpu1_Missile1.hMask);
      DeleteObject(bmpCpu1_Missile2.hMask);
      DeleteObject(bmpCpu1_Missile3.hMask);
      DeleteObject(bmpCpu2_Missile1.hMask);
      DeleteObject(bmpCpu2_Missile2.hMask);
      DeleteObject(bmpCpu2_Missile3.hMask);
      DeleteObject(bmpPlayer_Missile1.hMask);
      DeleteObject(bmpPlayer_Missile2.hMask);
      DeleteObject(bmpPlayer_Missile3.hMask);

      DeleteObject(bmpCPU2Comment.hPic);
      DeleteObject(bmpCPU2Comment.hMask);
      DeleteObject(bmpCPU1Comment.hPic);
      DeleteObject(bmpCPU1Comment.hMask);
      DeleteObject(bmpPlayerComment.hPic);
      DeleteObject(bmpPlayerComment.hMask);

      DeleteObject(bmpDancerLeft.hPic);
      DeleteObject(bmpDancerRight.hPic);

      DeleteObject(bmpDancerLeft.hMask);
      DeleteObject(bmpDancerRight.hMask);
      DeleteObject(hbmpRedButton);
   break;
   default: DefWindowProc(hwnd, uMsg, wParam, lParam);
   }
}