#include <windows.h>
#include <stdint.h>
#include <stdlib.h>
#include <wchar.h>
#include "libz80/z80.h"

#define TIMER_ID 1
#define TIMER_INTERVAL 1000

#define RAM_SIZE 0x10000 // 64KB max addressable by Z80

uint8_t memory[RAM_SIZE];
Z80Context cpu;

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



LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        
        case WM_CREATE: {
            // Start a timer when the window is created
            SetTimer(hwnd, TIMER_ID, TIMER_INTERVAL, NULL);
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
            
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            // Get the client rectangle
            RECT clientRect;
            GetClientRect(hwnd, &clientRect);
            
            // Fill the background with a solid color (e.g., white)
            HBRUSH hBrush = CreateSolidBrush(RGB(155, 155, 155)); // Gray
            FillRect(hdc, &clientRect, hBrush);
            DeleteObject(hBrush);


            // Set text color and background mode
            SetTextColor(hdc, RGB(0, 0, 0));
            SetBkMode(hdc, TRANSPARENT);
            
            // printf("PC: 0x%04X\n", cpu.PC);
            LPTSTR buffer[100];
            LPTSTR* label = TEXT("PC: ");
            // Format the string with a hexadecimal number
            wsprintf(buffer, TEXT("%s 0x%04X"), label, cpu.PC);
            TextOut(hdc, 8, 8, buffer, lstrlen(buffer));
            
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
            
            // Prepare BITMAPINFO
            BITMAPINFO bmi = {0};
            bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth = 256;
            bmi.bmiHeader.biHeight = -256; // negative for top-down
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = 24;
            bmi.bmiHeader.biCompression = BI_RGB;
            
            // Blit to window
            SetDIBitsToDevice(
                hdc,
                64, 64, 256, 256, // dest x, y, width, height
                0, 0, 0, 256,   // src x, y, start scan, num scans
                framebuffer,
                &bmi,
                DIB_RGB_COLORS
            );
            
            
            EndPaint(hwnd, &ps);
            return 0;            
        }
        
        case WM_DESTROY: {
            KillTimer(hwnd, TIMER_ID);
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
    
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    
    // Initialize Z80 CPU state and memory
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
        L"Pixel Drawing Window",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 400,
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
