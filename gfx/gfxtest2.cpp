#include <iostream>
#include <stdint.h>				// uint16_t
#include <conio.h>				// x86 in/out operations
#include <i86.h>				// interrupts
#include <time.h>
#include "gfxlib.cpp"

#define TEST_WIDTH							64
#define TEST_HEIGTH							64

using namespace std;

extern "C" uint16_t __cdecl add2Nums(uint16_t num1, uint16_t num2);
Graphics gfx;

void test_int()
{
	gfx.ClearScreen();
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
	gfx.ClearScreen();
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
	gfx.ClearScreen();
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
	gfx.ClearScreen();
	for (uint16_t y = 0; y < TEST_HEIGTH; y++)
	{
		for (uint16_t x = 0; x < TEST_WIDTH; x++)
		{
			uint16_t color = (x + y) * 58;
			vga_draw_pixel_faster(x, y, color);
		}
	}
}
void test_direct_cpp()
{
	gfx.ClearScreen();
	for (uint16_t y = 0; y < TEST_HEIGTH; y++)
	{
		for (uint16_t x = 0; x < TEST_WIDTH; x++)
		{
			gfx.SetSegment(y);
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
	gfx.SwapFrame();
	end_time1 = clock();

	start_time2 = clock();
	test_asm_c();
	gfx.SwapFrame();
	end_time2 = clock();

	start_time3 = clock();
	test_asm_watcomc();
	gfx.SwapFrame();
	end_time3 = clock();

	start_time4 = clock();
	test_asm_watcomc_nocalls();
	gfx.SwapFrame();
	end_time4 = clock();

	start_time5 = clock();
	test_direct_cpp();
	gfx.SwapFrame();
	end_time5 = clock();

	gfx.ClearScreen();
	cout << "results...\n\n";

	cout << "interrupt                              >>>   duration:  " << end_time1 - start_time1 << " ms\n";
	cout << "asm c calling conv                     >>>   duration:  " << end_time2 - start_time2 << " ms\n";
	cout << "asm watc calling conv                  >>>   duration:  " << end_time3 - start_time3 << " ms\n";
	cout << "asm watc calling conv no subcalls      >>>   duration:  " << end_time4 - start_time4 << " ms\n";
	cout << "c++ direct                             >>>   duration:  " << end_time5 - start_time5 << " ms\n\n";


	gfx.DisableKeyboardCursor();

	uint16_t startY = 90;
	for (int i = 20; i < WIDTH_PIXELS; i += 10)
	{
		gfx.DrawRectangle(i, startY, i + 5, startY + i / 10, i * 100);
		gfx.SwapFrame();
		gfx.DrawRectangle(i, startY, i + 5, startY + i / 10, i * 100);
	}

	startY += 50;
	for (int i = 20; i < WIDTH_PIXELS; i += 10)
	{
		gfx.DrawRectangleFilled(i, startY, i + 5, startY + i / 10, i * 100);
		gfx.SwapFrame();
		gfx.DrawRectangleFilled(i, startY, i + 5, startY + i / 10, i * 100);
	}

	startY += 80;
	for (int i = 20; i < WIDTH_PIXELS; i += 10)
	{
		gfx.DrawCircle(i, startY, i / 30, i * 100);
		gfx.SwapFrame();
		gfx.DrawCircle(i, startY, i / 30, i * 100);
	}

	startY += 30;
	for (int i = 20; i < WIDTH_PIXELS; i += 10)
	{
		gfx.DrawCircleFilled(i, startY, i / 30, i * 100);
		gfx.SwapFrame();
		gfx.DrawCircleFilled(i, startY, i / 30, i * 100);
	}

	startY += 50;
	for (int i = 20; i < WIDTH_PIXELS; i += 20)
	{
		gfx.DrawEllipse(i, startY, i / 30, i / 20, i * 50);
		gfx.SwapFrame();
		gfx.DrawEllipse(i, startY, i / 30, i / 20, i * 50);
	}

	for (int i = 5; i < WIDTH_PIXELS; i += 10)
	{
		gfx.DrawLineBres(i, 340, WIDTH_PIXELS - i, 440, i * 100);
		gfx.SwapFrame();
		gfx.DrawLineBres(i, 340, WIDTH_PIXELS - i, 440, i * 100);
	}

	gfx.SetCursorPosition(0, 450);

	return 0;
}