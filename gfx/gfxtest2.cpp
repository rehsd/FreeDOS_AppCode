#include <iostream>
#include <stdint.h>				//uint16_t
#include <conio.h>				// x86 in/out operations
#include <i86.h>
#include <time.h>

#define VIDEO_ADDRESS_START					0xa0000000
#define BYTES_PER_ROW						2048			
#define BYTES_PER_PIXEL						2
#define VGA_REG								0x00a0
#define COLOR_RED							0xF800
#define COLOR_GREEN							0x07E0
#define COLOR_BLUE							0x001F
#define TEST_WIDTH							256
#define TEST_HEIGTH							256

using namespace std;

extern "C" uint16_t __cdecl add2Nums(uint16_t num1, uint16_t num2);
extern "C" void vga_swap_frame();
extern "C" void __cdecl vga_draw_pixel(uint16_t x, uint16_t y, uint16_t color);
extern "C" void vga_draw_pixel_fast(uint16_t color, uint16_t y, uint16_t x);
extern "C" void vga_draw_pixel_faster(uint16_t color, uint16_t y, uint16_t x);

static inline void write_word_far(uintptr_t addr, uint16_t value)
{
	*(volatile uint16_t*)addr = value;
}

void SwapFrame()
{
	vga_swap_frame();

	//loop until vsync
	do
	{
	} while (inpw(VGA_REG) & 0x8000);

}
void ClearScreen()
{
	union REGPACK regs;
	memset(&regs, 0, sizeof(union REGPACK));
	regs.w.ax = 0x0600;		// ah = 6 (scroll), al = 0 = clear screen
	intr(0x10, &regs);
}
void SetSegment(uint16_t y)
{
	uint16_t currentRegister = inpw(VGA_REG);
	uint16_t currentSegment = currentRegister & 0x000f;								// read current register from video card
	uint16_t newSegment = ((y & 0x01e0) >> 5);
	if (currentSegment != newSegment)
	{
		outpw(VGA_REG, (currentRegister & 0xfff0) | newSegment);
	}
}

void test_int()
{
	ClearScreen();
	for (uint16_t y = 0; y < TEST_HEIGTH; y++)
	{
		for (uint16_t x = 0; x < TEST_WIDTH; x++)
		{
			uint16_t color = (x + y) * 58;
			union REGPACK regs;
			memset(&regs, 0, sizeof(union REGPACK));
			regs.w.ax = 0xb300;
			regs.w.bx = color & 0x0000ffff;	// 16-bit color
			regs.w.cx = x;					// column #
			regs.w.dx = y;					// row #
			intr(0x10, &regs);
		}
	}
}
void test_asm_c()
{
	ClearScreen();
	for (uint16_t y = 0; y < TEST_HEIGTH; y++)
	{
		for (uint16_t x = 0; x < TEST_WIDTH; x++)
		{
			uint16_t color = (x + y) * 58;
			vga_draw_pixel(x, y, color);
		}
	}
}
void test_asm_watcomc()
{
	ClearScreen();
	for (uint16_t y = 0; y < TEST_HEIGTH; y++)
	{
		for (uint16_t x = 0; x < TEST_WIDTH; x++)
		{
			uint16_t color = (x + y) * 58;
			vga_draw_pixel_fast(color, x, y);
		}
	}
}
void test_asm_watcomc_nocalls()
{
	ClearScreen();
	for (uint16_t y = 0; y < TEST_HEIGTH; y++)
	{
		for (uint16_t x = 0; x < TEST_WIDTH; x++)
		{
			uint16_t color = (x + y) * 58;
			vga_draw_pixel_faster(color, x, y);
		}
	}
}
void test_direct_cpp()
{
	ClearScreen();
	for (uint16_t y = 0; y < TEST_HEIGTH; y++)
	{
		for (uint16_t x = 0; x < TEST_WIDTH; x++)
		{
			SetSegment(y);
			uint16_t color = (x + y) * 58;
			uintptr_t addr = VIDEO_ADDRESS_START + (BYTES_PER_ROW * (y)) + (BYTES_PER_PIXEL * x);
			write_word_far(addr, color);
		}
	}
}

int main() {
	clock_t start_time1, end_time1, start_time2, end_time2, start_time3, end_time3, start_time4, end_time4, start_time5, end_time5;
	
	//cout << "\n Let's add two numbers...\n";
	//cout << "Enter first number: ";
	//uint16_t num1, num2;
	//cin >> num1;
	//cout << "Enter second number: ";
	//cin >> num2;
	//int i = add2Nums(num1, num2);
	//std::cout << "Result: " << i << "\n";

	cout << "Starting benchmark...\n";

	start_time1 = clock();
	test_int();
	SwapFrame();
	end_time1 = clock();

	start_time2 = clock();
	test_asm_c();
	SwapFrame();
	end_time2 = clock();

	start_time3 = clock();
	test_asm_watcomc();
	SwapFrame();
	end_time3 = clock();

	start_time4 = clock();
	test_asm_watcomc_nocalls();
	SwapFrame();
	end_time4 = clock();

	start_time5 = clock();
	test_direct_cpp();
	SwapFrame();
	end_time5 = clock();

	ClearScreen();
	cout << "results...\n\n";

	cout << "interrupt                              >>>   duration:  " << end_time1 - start_time1 << " ms\n";
	cout << "asm c calling conv                     >>>   duration:  " << end_time2 - start_time2 << " ms\n";
	cout << "asm watc calling conv                  >>>   duration:  " << end_time3 - start_time3 << " ms\n";
	cout << "asm watc calling conv no subcalls      >>>   duration:  " << end_time4 - start_time4 << " ms\n";
	cout << "c++ direct                             >>>   duration:  " << end_time5 - start_time5 << " ms\n\n\n";

	return 0;
}