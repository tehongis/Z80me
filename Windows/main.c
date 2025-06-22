#include <windows.h>
#include <stdint.h>
#include <stdlib.h>
#include <wchar.h>
#include "../libz80/z80.h"

#define TIMER_ID 1
#define TIMER_INTERVAL 100

#define RAM_SIZE 0x10000 // 64KB max addressable by Z80

uint8_t memory[RAM_SIZE];
Z80Context cpu;

static int width = 1280, height=1080;

static byte context_mem_read_callback(size_t param, ushort address) {
    byte data = memory[address];
    //printf("memR %04x %02x\n", address, data);
    return data;
}

static void context_mem_write_callback(size_t param, ushort address, byte data) {
    //printf("memW %04x %02x\n", address, data);
    memory[address] = data;
}

static byte context_io_read_callback(size_t param, ushort address) {
    byte data = address >> 8;
    //printf("ioR %04x %02x\n", address, data);
    return data;
}

static void context_io_write_callback(size_t param, ushort address, byte data) {
    //printf("ioW %04x %02x\n", address, data);
}


// Add this global buffer for the bitmap (24-bit RGB)
uint8_t framebuffer[RAM_SIZE * 3];

int load_rom(const char *filename, unsigned int loadAddress) {
    
    FILE *file;
    
    fopen_s(&file, filename, "rb");
    if (file == NULL) {
        MessageBox(NULL, L"Could not open the file.", L"File Error", MB_ICONERROR);
        return ERROR_FILE_NOT_FOUND;
    }
    
    // Read file into memory
    size_t bytesRead = fread(memory+(loadAddress&0xffff), 1, RAM_SIZE-1, file);
    
    fclose(file);
    return 0;
}


LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HDC hdcMem = NULL;
    static HBITMAP hbmMem = NULL;
    static HBITMAP hbmOld = NULL;
    static HBRUSH hBrush = NULL;

    switch (uMsg) {
        case WM_CREATE: {
            // Start a timer when the window is created
            SetTimer(hwnd, TIMER_ID, TIMER_INTERVAL, NULL);

            // Set cleanup brush
            hBrush = CreateSolidBrush(RGB(155, 155, 155)); // Gray


            HDC hdcWindow = GetDC(hwnd);
            hdcMem = CreateCompatibleDC(hdcWindow);
            hbmMem = CreateCompatibleBitmap(hdcWindow, width, height);
            hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);
            ReleaseDC(hwnd, hdcWindow);

            return 0;            
        }
        
        case WM_TIMER: {
            if (wParam == TIMER_ID) {
                Z80Execute(&cpu); // Run Z80 on timer tick
                InvalidateRect(hwnd, NULL, FALSE); // Request repaint
            }
            return 0;
        }
        
        case WM_PAINT: {

            // Begin painting            
            PAINTSTRUCT ps;
            RECT clientRect;

            HDC hdc = BeginPaint(hwnd, &ps);

            // Get the client rectangle
            GetClientRect(hwnd, &clientRect);

            // Fill the background with a solid color (e.g., white)
            FillRect(hdcMem, &clientRect, hBrush);
            
            // Set text color and background mode
            SetTextColor(hdcMem, RGB(0, 0, 0));
            SetBkMode(hdcMem, TRANSPARENT);
            
            // printf("PC: 0x%04X\n", cpu.PC);
            LPTSTR buffer[100];
            // Format the string with a hexadecimal number
            wsprintf(buffer, TEXT("PC: 0x%04X"), cpu.PC);
            TextOut(hdcMem, 1050, 20, buffer, lstrlen(buffer));

            wsprintf(buffer, TEXT("AF: 0x%04X"), cpu.R1.wr.AF);
            TextOut(hdcMem, 1050, 40, buffer, lstrlen(buffer));

            wsprintf(buffer, TEXT("BC: 0x%04X"), cpu.R1.wr.BC);
            TextOut(hdcMem, 1150, 40, buffer, lstrlen(buffer));

            wsprintf(buffer, TEXT("DE: 0x%04X"), cpu.R1.wr.DE);
            TextOut(hdcMem, 1050, 60, buffer, lstrlen(buffer));

            wsprintf(buffer, TEXT("HL: 0x%04X"), cpu.R1.wr.HL);
            TextOut(hdcMem, 1150, 60, buffer, lstrlen(buffer));
            
            wsprintf(buffer, TEXT("IX: 0x%04X"), cpu.R1.wr.IX);
            TextOut(hdcMem, 1050, 80, buffer, lstrlen(buffer));

            wsprintf(buffer, TEXT("IY: 0x%04X"), cpu.R1.wr.IY);
            TextOut(hdcMem, 1150, 80, buffer, lstrlen(buffer));

            wsprintf(buffer, TEXT("SP: 0x%04X"), cpu.R1.wr.SP);
            TextOut(hdcMem, 1050, 100, buffer, lstrlen(buffer));
            
            // Fill framebuffer from Z80 memory (grayscale)
            for (int address = 0; address < RAM_SIZE; address++) {
                uint8_t val = memory[address];
                
                // Extract raw values
                uint8_t red_raw   =  val       & 0b00000111;        // bits 0–2
                uint8_t green_raw = (val >> 3) & 0b00000111;        // bits 3–5
                uint8_t blue_raw  = (val >> 6) & 0b00000011;        // bits 6–7
                
                // Scale to 0–255 range (optional)
                uint8_t red   = (red_raw   * 255) / 7;
                uint8_t green = (green_raw * 255) / 7;
                uint8_t blue  = (blue_raw  * 255) / 3;            
                
                int idx = address * 3;
                framebuffer[idx + 0] = blue;
                framebuffer[idx + 1] = green;
                framebuffer[idx + 2] = red;
            }
            
            // Highlight the PC in the framebuffer
            int idx = cpu.PC * 3;
            framebuffer[idx + 0] = 0xff ;
            framebuffer[idx + 1] = 0xff;
            framebuffer[idx + 2] = 0xff;
            
            // Prepare BITMAPINFO
            BITMAPINFO bmi = {0};
            bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth = 256;
            bmi.bmiHeader.biHeight = -256; // negative for top-down
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = 24;
            bmi.bmiHeader.biCompression = BI_RGB;
            
            StretchDIBits(
                hdcMem,               // destination DC
                8, 8,      // destination x, y
                256 * 4,         // scaled width
                256 * 4,        // scaled height
                0, 0,              // source x, y
                256, 256,     // source width, height
                framebuffer,           // pointer to bitmap bits
                &bmi,              // BITMAPINFO structure
                DIB_RGB_COLORS,    // color usage
                SRCCOPY            // raster operation
            );            

            // Blit the framebuffer to the window
            BitBlt(hdc, 0, 0, width, height, hdcMem, 0, 0, SRCCOPY);
            EndPaint(hwnd, &ps);

            return 0;            
        }
        
        case WM_DESTROY: {
            KillTimer(hwnd, TIMER_ID);

            // Clean up
            DeleteObject(hBrush);

            SelectObject(hdcMem, hbmOld);
            DeleteObject(hbmMem);
            DeleteDC(hdcMem);            
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
    
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    
    // Initialize Z80 CPU state and memory
    // Load roms

    load_rom("BIOS.bin",0x0000);
    load_rom("CPM.bin",0xdc00);
    
    // Initialize Z80 CPU
    Z80RESET(&cpu);
    
    cpu.memRead = context_mem_read_callback;
    cpu.memWrite = context_mem_write_callback;
    cpu.ioRead = context_io_read_callback;
    cpu.ioWrite = context_io_write_callback;
    
    const char CLASS_NAME[] = "SampleWindowClass";
    
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    
    RegisterClass(&wc);
    
    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Z80 Emulator",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        NULL, NULL, hInstance, NULL
    );
    
    if (hwnd == NULL) return 0;
    
    ShowWindow(hwnd, nCmdShow);
    
    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return 0;
}
