// c includes
#include <stdio.h>
#include <string.h>

// windows includes
#include <windows.h>
#include <commctrl.h>
#include <winuser.h>

const char *g_class_name = "BoxCutter";
class BoxCutterWindow;
BoxCutterWindow *g_win = NULL;

const char*USAGE = "\
usage: boxcutter [OPTIONS] [OUTPUT_FILENAME]\n\
Saves a bitmap screenshot to 'OUTPUT_FILENAME' if given.  Otherwise, output\n\
is saved to 'screenshot.bmp' by default.\n\
\n\
OPTIONS\n\
  -h, --help   display help message\n\
";



// Saves a bitmap to a file
//   adopted from pywin32
bool save_bitmap_file(HBITMAP hBmp, HDC hDC, const char *filename)
{
    // data structures
    BITMAP bmp;
    PBITMAPINFO pbmi;
    WORD cClrBits;

    // Retrieve the bitmap's color format, width, and height. 
    if (!GetObject(hBmp, sizeof(BITMAP), (LPVOID) &bmp))
        // GetObject failed
        return false;
    
    // Convert the color format to a count of bits. 
    cClrBits = (WORD)(bmp.bmPlanes * bmp.bmBitsPixel); 
    if (cClrBits == 1) 
      cClrBits = 1; 
    else if (cClrBits <= 4) 
      cClrBits = 4; 
    else if (cClrBits <= 8) 
      cClrBits = 8; 
    else if (cClrBits <= 16) 
      cClrBits = 16; 
    else if (cClrBits <= 24) 
      cClrBits = 24; 
    else cClrBits = 32; 

    
    // Allocate memory for the BITMAPINFO structure. (This structure 
    // contains a BITMAPINFOHEADER structure and an array of RGBQUAD 
    // data structures.) 
    if (cClrBits != 24) 
        pbmi = (PBITMAPINFO) LocalAlloc(LPTR, 
                                        sizeof(BITMAPINFOHEADER) + 
                                        sizeof(RGBQUAD) * (1<< cClrBits)); 

    // There is no RGBQUAD array for the 24-bit-per-pixel format. 
    else
        pbmi = (PBITMAPINFO) LocalAlloc(LPTR, 
                                        sizeof(BITMAPINFOHEADER)); 
  
    // Initialize the fields in the BITMAPINFO structure. 

    pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER); 
    pbmi->bmiHeader.biWidth = bmp.bmWidth; 
    pbmi->bmiHeader.biHeight = bmp.bmHeight; 
    pbmi->bmiHeader.biPlanes = bmp.bmPlanes; 
    pbmi->bmiHeader.biBitCount = bmp.bmBitsPixel; 
    
    if (cClrBits < 24) 
        pbmi->bmiHeader.biClrUsed = (1<<cClrBits); 

    // If the bitmap is not compressed, set the BI_RGB flag. 
    pbmi->bmiHeader.biCompression = BI_RGB; 

    // Compute the number of bytes in the array of color 
    // indices and store the result in biSizeImage. 
    pbmi->bmiHeader.biSizeImage = (pbmi->bmiHeader.biWidth + 7) /8 
        * pbmi->bmiHeader.biHeight * cClrBits; 

    // Set biClrImportant to 0, indicating that all of the 
    // device colors are important. 
    pbmi->bmiHeader.biClrImportant = 0; 
  
    HANDLE hf;                  // file handle 
    BITMAPFILEHEADER hdr;       // bitmap file-header 
    PBITMAPINFOHEADER pbih;     // bitmap info-header 
    LPBYTE lpBits;              // memory pointer 
    DWORD dwTotal;              // total count of bytes 
    DWORD cb;                   // incremental count of bytes 
    BYTE *hp;                   // byte pointer 
    DWORD dwTmp; 

    pbih = (PBITMAPINFOHEADER) pbmi; 
    lpBits = (LPBYTE) GlobalAlloc(GMEM_FIXED, pbih->biSizeImage);
    
    
    
    if (!lpBits) {
        // GlobalAlloc failed
        printf("error: out of memory\n");
        return false;
    }

    // Retrieve the color table (RGBQUAD array) and the bits 
    // (array of palette indices) from the DIB. 
    if (!GetDIBits(hDC, hBmp, 0, (WORD) pbih->biHeight, lpBits, pbmi, 
                   DIB_RGB_COLORS)) 
    {
        // GetDIBits failed
        printf("error: GetDiBits failed\n");
        return false;
    }

    // Create the .BMP file. 
    hf = CreateFile(filename, 
                    GENERIC_READ | GENERIC_WRITE, 
                    (DWORD) 0, 
                    NULL, 
                    CREATE_ALWAYS, 
                    FILE_ATTRIBUTE_NORMAL, 
                    (HANDLE) NULL); 
    if (hf == INVALID_HANDLE_VALUE) {
        // create file
        printf("error: cannot create file '%s'\n", filename);
        return false;
    }
    hdr.bfType = 0x4d42;        // 0x42 = "B" 0x4d = "M" 
    // Compute the size of the entire file. 
    hdr.bfSize = (DWORD) (sizeof(BITMAPFILEHEADER) + 
                          pbih->biSize + pbih->biClrUsed 
                          * sizeof(RGBQUAD) + pbih->biSizeImage); 
    hdr.bfReserved1 = 0; 
    hdr.bfReserved2 = 0; 

    // Compute the offset to the array of color indices. 
    hdr.bfOffBits = (DWORD) sizeof(BITMAPFILEHEADER) + 
        pbih->biSize + pbih->biClrUsed * sizeof (RGBQUAD); 

    // Copy the BITMAPFILEHEADER into the .BMP file. 
    if (!WriteFile(hf, (LPVOID) &hdr, sizeof(BITMAPFILEHEADER), 
                   (LPDWORD) &dwTmp,  NULL)) 
    {
        // WriteFile failed
        printf("error: cannot write file '%s'\n", filename);
        return false;
    }

    // Copy the BITMAPINFOHEADER and RGBQUAD array into the file. 
    if (!WriteFile(hf, (LPVOID) pbih, sizeof(BITMAPINFOHEADER) 
                   + pbih->biClrUsed * sizeof (RGBQUAD), 
                   (LPDWORD) &dwTmp, ( NULL)))
    {
        // WriteFile failed
        printf("error: cannot write file '%s'\n", filename);
        return false;
    }

    
    // Copy the array of color indices into the .BMP file. 
    dwTotal = cb = pbih->biSizeImage; 
    hp = lpBits; 
    if (!WriteFile(hf, (LPSTR) hp, (int) cb, (LPDWORD) &dwTmp, NULL)) {
        // WriteFile failed
        printf("error: cannot write file '%s'\n", filename);
        return false;
    }
    
    
    // Close the .BMP file. 
    if (!CloseHandle(hf)) {
        // CloseHandle failed
        printf("error: cannot close file '%s'\n", filename);
        return false;
    }
    
    // Free memory. 
    GlobalFree((HGLOBAL)lpBits);

    return true;
}

// Captures a screenshot from a region of the screen
// save its to a file
bool capture_screen(const char *filename, 
                    int x, int y, int x2, int y2)
{
    // normalize coordinates
    if (x > x2) {
        int tmp = x;
        x = x2;
        x2 = tmp;
    }
    if (y > y2) {
        int tmp = y;
        y = y2;
        y2 = tmp;
    }
    int w = x2 - x;
    int h = y2 - y;

    // copy screen to bitmap
    HDC screen_dc = GetDC(0);
    HDC shot_dc = CreateCompatibleDC(screen_dc);
    HBITMAP shot_bitmap =  CreateCompatibleBitmap(screen_dc, w, h);
    HGDIOBJ old_obj = SelectObject(shot_dc, shot_bitmap);
    
    if (!BitBlt(shot_dc, 0, 0, w, h, screen_dc, x, y, SRCCOPY)) {
        return false;
    }
    
    // save bitmap to file
    bool ret = save_bitmap_file(shot_bitmap, shot_dc, filename);
    
    DeleteDC(shot_dc);
    DeleteDC(screen_dc);
    SelectObject(shot_dc, old_obj);
    
    return ret;
}
   

class BoxCutterWindow
{
public:
    BoxCutterWindow(HINSTANCE hinst, 
                    const char *title, const char* filename) :
        m_active(true),
        m_drag(false),
        m_draw(false),
        m_filename(filename)
    {
        // fill wndclass structure
        WNDCLASS wc;
        strcpy(m_class_name, g_class_name);
        wc.hInstance = hinst; 
        wc.lpszClassName = m_class_name;
        wc.lpfnWndProc = WindowProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.hbrBackground = 0; //(HBRUSH) GetStockObject(WHITE_BRUSH);
        wc.hCursor = LoadCursor(0, IDC_CROSS);
        wc.hIcon = LoadIcon(0, IDI_APPLICATION);
        
        // register class
        ATOM class_atom = RegisterClass(&wc);
        
        // determine screen dimensions
        RECT rect;
        GetWindowRect(GetDesktopWindow(), &rect);
        
        // create window
        DWORD exstyle = WS_EX_TRANSPARENT;
        DWORD style = WS_POPUP;
        
        m_handle = CreateWindowEx(exstyle,
                                  m_class_name, 
                                  title,
                                  style,
                                  // dimensions
                                  0, 0, rect.right, rect.bottom,
                                  0, // no parent
                                  0, // no menu
                                  hinst, //module_instance,
                                  NULL);
    }
    
    ~BoxCutterWindow()
    {
    }

    
    void show(bool enabled=true)
    {
        if (enabled)
            ShowWindow(m_handle, SW_SHOW);
        else
            ShowWindow(m_handle, SW_HIDE);
    }
    
    void maximize()
    {
        ShowWindow(m_handle, SW_SHOWMAXIMIZED);
        //UpdateWindow(m_handle);
    }
    
    void activate()
    {
        SetForegroundWindow(m_handle);
        //SwitchToThisWindow(self._handle, False)
    }
    
    
    void close()
    {
        DestroyWindow(m_handle);
    }
    
    // mouse button down callback
    void on_mouse_down()
    {
        // start draging
        m_drag = true;
        GetCursorPos(&m_start);
    }
    
    // mouse button up callback
    void on_mouse_up()
    {
        
        // if drawing has occurred, clean it up
        if (m_draw) {
            // cleanup rectangle on desktop
            m_drag = false;
            m_draw = false;
            
            HDC hdc = CreateDC("DISPLAY", NULL, NULL, NULL);
            SetROP2(hdc, R2_NOTXORPEN);
            Rectangle(hdc, m_start.x, m_start.y, m_end.x, m_end.y);
            DeleteDC(hdc);
        }

        // save bitmap
        if (!capture_screen(m_filename, m_start.x, m_start.y,
                            m_end.x, m_end.y))
        {
                MessageBox(m_handle, "Cannot save screenshot", 
                           "Error", MB_OK);
        }
        
        // close BoxCutter window
        close();
    }
    
    // callback for mouse movement
    void on_mouse_move()
    {
        // get current mouse coordinates
        POINT pos;
        GetCursorPos(&pos);
        
        // if mouse is down, process drag
        if (m_drag) {
            HDC hdc = CreateDC("DISPLAY", NULL, NULL, NULL);
            SetROP2(hdc, R2_NOTXORPEN);
            
            // erase old rectangle
            if (m_draw) {
                Rectangle(hdc, m_start.x, m_start.y, m_end.x, m_end.y);
            }
            
            // draw new rectangle
            m_draw = true;
            Rectangle(hdc, m_start.x, m_start.y, pos.x, pos.y);
            m_end = pos;
            
            DeleteDC(hdc);
        }
    }
    
    bool active()
    {
        return m_active;
    }


    LRESULT CALLBACK static WindowProc(HWND hwnd,
                                       UINT uMsg,
                                       WPARAM wParam,
                                       LPARAM lParam)
    {
        // if window is present
        if (g_win) {
            // route message
        
            switch (uMsg) {
                case WM_DESTROY: 
                    g_win->close();
                    PostQuitMessage(0);
                    return 0;
            
                case WM_MOUSEMOVE:
                    g_win->on_mouse_move();
                    return 0;
                
                case WM_LBUTTONDOWN: 
                    g_win->on_mouse_down();
                    return 0;
                
                case WM_LBUTTONUP:
                    g_win->on_mouse_up();
                    return 0;
            }
        }
        
        // perform default action
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }


private:    
    char m_class_name[101];
    HWND m_handle;
    
    bool m_active;
    bool m_drag;
    bool m_draw;
    const char *m_filename;
    POINT m_start, m_end;
};



// Returns commandline arguments: argv, argc 
char **get_args(int *argc)
{
    LPWSTR *wargv = CommandLineToArgvW(GetCommandLineW(), argc);
    
    char **argv = new char* [*argc];
    
    // convert args to char*
    for (int i=0; i<*argc; i++) {
        int len = wcslen(wargv[i]);
        argv[i] = new char [len+1];
        snprintf(argv[i], len, "%ws", wargv[1]);
    }
    
    return argv;
}

// Deletes arguments
void delete_args(int argc, char **argv)
{
    for (int i=0; i<argc; i++)
        delete [] argv[i];
    delete [] argv;
}

void usage()
{
    printf(USAGE);
}


//=============================================================================
// main function
int WINAPI WinMain (HINSTANCE hInstance, 
                    HINSTANCE hPrevInstance, 
                    PSTR cmdline, 
                    int iCmdShow) 
{
    InitCommonControls();
    
    // default screenshot filename
    char *filename = "screenshot.bmp";
    
    // parse command line
    int argc;
    char **argv = get_args(&argc);
    int i;

    // parse options
    for (i=0; i<argc; i++) {
        if (argv[i][0] != '-')
            // argument is not an option
            break;
        
        else if (strcmp(argv[i], "-h") == 0 ||
                 strcmp(argv[i], "--help") == 0)
        {
            // display help info
            usage();
            return 1;
        }

        else {
            printf("error: unknown option '%s'\n", argv[i]);
            usage();
            return 1;
        }
    }
    
    // argument after options is a filename
    if (i < argc)
        filename = argv[i];
    
    
    
    // create screenshot window
    BoxCutterWindow win(hInstance, "BoxCutter", filename);
    win.show();
    win.maximize();
    win.activate();
    g_win = &win;
    
    printf("save to file: '%s'\n", filename);

    
    // main loop: retrieve all messages for this process
    int ret;
    MSG msg; // message structure
    while ((ret = GetMessage(&msg, 0, 0, 0)) != 0)
    {
        if (ret == -1) {
            // error occurred
            break;
        }
        
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    delete_args(argc, argv);
    
    return msg.wParam;
}

