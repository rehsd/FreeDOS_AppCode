// References:			-Computer Graphics, C Version (2nd Ed.) -- Hearn & Baker
//	

#include <stdlib.h>
#include <stdint.h>				// uint16_t
#include <conio.h>				// x86 in/out operations
#include <i86.h>				// interrupts
#include <math.h>
#include <iostream.h>			// memset, ...
#include <stdio.h>	

#define ROUND(a) ((int)(a+0.5))
#define PRINT_CHAR_OPTION_NO_SWAP_FRAME		0x0001		// when set, print_char only prints on inactive VGA frame
#define PRINT_CHAR_OPTION_RESET				0x0000
#define	WIDTH_PIXELS						640
#define HEIGHT_PIXELS						480
#define VIDEO_ADDRESS_START					0xa0000000
#define BYTES_PER_ROW						2048			
#define BYTES_PER_PIXEL						2
#define VGA_REG								0x00a0
#define COLOR_RED							0xF800
#define COLOR_GREEN							0x07E0
#define COLOR_BLUE							0x001F

using namespace std;

extern "C" void vga_swap_frame();
extern "C" void __cdecl vga_draw_pixel(uint16_t x, uint16_t y, uint16_t color);
extern "C" void vga_draw_pixel_fast(uint16_t color, uint16_t y, uint16_t x);
extern "C" void vga_draw_pixel_faster(uint16_t x, uint16_t y, uint16_t color);

static inline void write_word_far(uintptr_t addr, uint16_t value)
{
	*(volatile uint16_t*)addr = value;
}

class Graphics
{
public:
	Graphics()
	{

	}
	void SetCursorPosition(int x, int y)
	{
		union REGPACK regs;
		memset(&regs, 0, sizeof(union REGPACK));
		regs.w.ax = 0x0200;				// AH = 0x02
		regs.w.bx = x;					// column position in pixels
		regs.w.cx = y;					// row  position in pixels
		intr(0x10, &regs);
	}
	void DisableKeyboardCursor()
	{
		union REGPACK regs;
		memset(&regs, 0, sizeof(union REGPACK));
		regs.w.ax = 0x0100;				// AH = 0x01		setcursortype
		regs.w.cx = 0;					// ch, cl = 0
		intr(0x10, &regs);
	}
	void SetPrintCharOptions(int options)
	{
		union REGPACK regs;
		memset(&regs, 0, sizeof(union REGPACK));
		regs.w.ax = 0xb100;				// AH = 0x02
		regs.w.bx = options & 0x0000ffff;			// print char options
		intr(0x10, &regs);
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

	void DrawRectangle(uint16_t start_x, uint16_t start_y, uint16_t end_x, uint16_t end_y, uint16_t color)
	{
		for (uint16_t x = start_x; x <= end_x; x++)
		{
			vga_draw_pixel_faster(x, start_y, color);
			vga_draw_pixel_faster(x, end_y, color);

			//DrawPixel(x, start_y, color, false);
			//DrawPixel(x, end_y, color, false);
		}
		for (uint16_t y = start_y; y <= end_y; y++)
		{
			vga_draw_pixel_faster(start_x, y, color);
			vga_draw_pixel_faster(end_x, y, color);
			//DrawPixel(start_x, y, color, false);
			//DrawPixel(end_x, y, color, false);
		}
	}
	void DrawRectangleFilled(uint16_t start_x, uint16_t start_y, uint16_t end_x, uint16_t end_y, uint16_t color)
	{
		for (uint16_t x = start_x; x <= end_x; x++)
		{
			for (uint16_t y = start_y; y <= end_y; y++)
			{
				vga_draw_pixel_faster(x, y, color);
				//DrawPixel(x, y, color, false);
			}
		}
	}
	bool IsValidXY(uint16_t x, uint16_t y)
	{
		if (x >= WIDTH_PIXELS || y >= HEIGHT_PIXELS)
		{
			return false;
		}
		else
		{
			return true;
		}
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
	void DrawLine(int xa, int ya, int xb, int yb, uint16_t color)
	{
		//souce: Computer Graphics, Hearn & Baker
		int dx = xb - xa, dy = yb - ya, steps, k;
		float xIncrement, yIncrement, x = xa, y = ya;
		if (abs(dx) > abs(dy))
		{
			steps = abs(dx);
		}
		else
		{
			steps = abs(dy);
		}
		xIncrement = dx / (float)steps;
		yIncrement = dy / (float)steps;

		vga_draw_pixel_faster(ROUND(x), ROUND(y), color);
		//DrawPixel(ROUND(x), ROUND(y), color, false);
		for (k = 0; k < steps; k++)
		{
			x += xIncrement;
			y += yIncrement;
			vga_draw_pixel_faster(ROUND(x), ROUND(y), color);
			//DrawPixel(ROUND(x), ROUND(y), color, false);
		}
	}
	void DrawLineBres(int x1, int y1, int x2, int y2, uint16_t color)
	{
		int x, y, dx, dy, dx1, dy1, px, py, xe, ye, i;
		dx = x2 - x1;
		dy = y2 - y1;
		dx1 = fabs(dx);
		dy1 = fabs(dy);
		px = 2 * dy1 - dx1;
		py = 2 * dx1 - dy1;
		if (dy1 <= dx1)
		{
			if (dx >= 0)
			{
				x = x1;
				y = y1;
				xe = x2;
			}
			else
			{
				x = x2;
				y = y2;
				xe = x1;
			}
			vga_draw_pixel_faster((uint16_t)x, (uint16_t)y, color);
			//DrawPixel((uint16_t)x, (uint16_t)y, color, false);
			for (i = 0; x < xe; i++)
			{
				x = x + 1;
				if (px < 0)
				{
					px = px + 2 * dy1;
				}
				else
				{
					if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0))
					{
						y = y + 1;
					}
					else
					{
						y = y - 1;
					}
					px = px + 2 * (dy1 - dx1);
				}
				vga_draw_pixel_faster((uint16_t)x, (uint16_t)y, color);
				//DrawPixel((uint16_t)x, (uint16_t)y, color, false);
			}
		}
		else
		{
			if (dy >= 0)
			{
				x = x1;
				y = y1;
				ye = y2;
			}
			else
			{
				x = x2;
				y = y2;
				ye = y1;
			}
			vga_draw_pixel_faster((uint16_t)x, (uint16_t)y, color);
			//DrawPixel((uint16_t)x, (uint16_t)y, color, false);
			for (i = 0; y < ye; i++)
			{
				y = y + 1;
				if (py <= 0)
				{
					py = py + 2 * dx1;
				}
				else
				{
					if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0))
					{
						x = x + 1;
					}
					else
					{
						x = x - 1;
					}
					py = py + 2 * (dx1 - dy1);
				}
				vga_draw_pixel_faster((uint16_t)x, (uint16_t)y, color);
				//DrawPixel((uint16_t)x, (uint16_t)y, color, false);
			}
		}
	}
	void DrawCircle(int xCenter, int yCenter, int radius, uint16_t color)
	{
		int x = 0;
		int y = radius;
		int p = 1 - radius;
		//void CirclePlotPoints(int, int, int, int);

		//Plot first set of points
		CirclePlotPoints(xCenter, yCenter, x, y, color);

		while (x < y)
		{
			x++;
			if (p < 0)
			{
				p += 2 * x + 1;
			}
			else
			{
				y--;
				p += 2 * (x - y) + 1;
			}
			CirclePlotPoints(xCenter, yCenter, x, y, color);
		}
	}
	void CirclePlotPoints(int xCenter, int yCenter, int x, int y, uint16_t color)
	{

		vga_draw_pixel_faster(xCenter + x, yCenter + y, color);
		vga_draw_pixel_faster(xCenter - x, yCenter + y, color);
		vga_draw_pixel_faster(xCenter + x, yCenter - y, color);
		vga_draw_pixel_faster(xCenter - x, yCenter - y, color);
		vga_draw_pixel_faster(xCenter + y, yCenter + x, color);
		vga_draw_pixel_faster(xCenter - y, yCenter + x, color);
		vga_draw_pixel_faster(xCenter + y, yCenter - x, color);
		vga_draw_pixel_faster(xCenter - y, yCenter - x, color);

		//DrawPixel(xCenter + x, yCenter + y, color, false);
		//DrawPixel(xCenter - x, yCenter + y, color, false);
		//DrawPixel(xCenter + x, yCenter - y, color, false);
		//DrawPixel(xCenter - x, yCenter - y, color, false);
		//DrawPixel(xCenter + y, yCenter + x, color, false);
		//DrawPixel(xCenter - y, yCenter + x, color, false);
		//DrawPixel(xCenter + y, yCenter - x, color, false);
		//DrawPixel(xCenter - y, yCenter - x, color, false);
	}
	void DrawCircleFilled(int xCenter, int yCenter, int radius, uint16_t color)
	{
		// Likely, there's a faster way to do this
		int height;
		for (int x = -radius; x < radius; x++)
		{
			height = (int)sqrt(radius * radius - x * x);

			for (int y = -height; y < height; y++)
			{
				vga_draw_pixel_faster(x + xCenter, y + yCenter, color);
				//color,DrawPixel(x + xCenter, y + yCenter, color, false);
			}
		}
	}
	void DrawEllipse(int xCenter, int yCenter, int Rx, int Ry, uint16_t color)
	{
		int Rx2 = Rx * Rx;
		int Ry2 = Ry * Ry;
		int twoRx2 = 2 * Rx2;
		int twoRy2 = 2 * Ry2;
		int p;
		int x = 0;
		int y = Ry;
		int px = 0;
		int py = twoRx2 * y;

		//void ellipsePlotPoints(int,int,int,int);

		//Plot the first set of points
		EllipsePlotPoints(xCenter, yCenter, x, y, color);

		//Region 1
		p = ROUND(Ry2 - (Rx2 * Ry) + (0.25 * Rx2));
		while (px < py)
		{
			x++;
			px += twoRy2;
			if (p < 0)
			{
				p += Ry2 + px;
			}
			else
			{
				y--;
				py -= twoRx2;
				p += Ry2 + px - py;
			}
			EllipsePlotPoints(xCenter, yCenter, x, y, color);
		}

		//Reigon 2
		p = ROUND(Ry2 * (x + 0.5) * (x + 0.5) + Rx2 * (y - 1) * (y - 1) - Rx2 * Ry2);
		while (y > 0)
		{
			y--;
			py -= twoRx2;
			if (p > 0)
			{
				p += Rx2 - py;
			}
			else
			{
				x++;
				px += twoRy2;
				p += Rx2 - py + px;
			}
			EllipsePlotPoints(xCenter, yCenter, x, y, color);
		}
	}
	void EllipsePlotPoints(int xCenter, int yCenter, int x, int y, uint16_t color)
	{
		vga_draw_pixel_faster(xCenter + x, yCenter + y, color);
		vga_draw_pixel_faster(xCenter - x, yCenter + y, color);
		vga_draw_pixel_faster(xCenter + x, yCenter - y, color);
		vga_draw_pixel_faster(xCenter - x, yCenter - y, color);
		 
		//DrawPixel(xCenter + x, yCenter + y, color, false);
		//DrawPixel(xCenter - x, yCenter + y, color, false);
		//DrawPixel(xCenter + x, yCenter - y, color, false);
		//DrawPixel(xCenter - x, yCenter - y, color, false);
	}

	//The following are unused, from Computer Graphics,	Hearn & Baker
	typedef struct tEdge
	{
		int yUpper;
		float xIntersect, dxPerScan;
		struct tEdge* next;
	} Edge;
	typedef struct tdcPt		//?
	{
		int x;
		int y;
	}dcPt;
	void insertEdge(Edge* list, Edge* edge)
	{
		//Inserts edge into list in order of increasing xIntersect field
		Edge* p, * q = list;
		p = q->next;
		while (p != NULL)
		{
			if (edge->xIntersect < p->xIntersect)
			{
				p = NULL;
			}
			else
			{
				q = p;
				p = p->next;
			}
		}
		edge->next = q->next;
		q->next = edge;
	}
	int yNext(int k, int cnt, dcPt* pts)
	{
		//For an index, return y-coordinate of next nonhorizonal line
		int j;
		if ((k + 1) > (cnt - 1))
		{
			j = 0;
		}
		else
		{
			j = k + 1;
		}
		while (pts[k].y == pts[j].y)
		{
			if ((j + 1) > (cnt - 1))
			{
				j = 0;
			}
			else
			{
				j++;
			}
		}
		return (pts[j].y);
	}
	void makeEdgeRec(dcPt lower, dcPt upper, int yComp, Edge* edge, Edge* edges[])
	{
		/* Store lower-y coordinate and inverse slope for each edge. Adjust
		*  and store upper-y coordinate for edges that are the lower member
		* of a monotonically increasing or decreasing pair of edges 	*/
		edge->dxPerScan = (float)(upper.x - lower.x) / (upper.y - lower.y);
		edge->xIntersect = lower.x;
		if (upper.y < yComp)
		{
			edge->yUpper = upper.y - 1;
		}
		else
		{
			edge->yUpper = upper.y;
		}
		insertEdge(edges[lower.y], edge);
	}
	void buildEdgeList(int cnt, dcPt* pts, Edge* edges[])
	{
		Edge* edge;
		dcPt v1, v2;
		int i, yPrev = pts[cnt - 2].y;
		v1.x = pts[cnt - 1].x;
		v1.y = pts[cnt - 1].y;
		for (i = 0; i < cnt; i++)
		{
			v2 = pts[i];
			if (v1.y != v2.y)		//non-horizontal line
			{
				edge = (Edge*)malloc(sizeof(Edge));
				if (v1.y < v2.y)
				{
					makeEdgeRec(v1, v2, yNext(i, cnt, pts), edge, edges);
				}
				else
				{
					makeEdgeRec(v2, v1, yPrev, edge, edges);
				}
			}
			yPrev = v1.y;
			v1 = v2;
		}
	}
	void buildActiveList(int scan, Edge* active, Edge* edges[])
	{
		Edge* p, * q;
		p = edges[scan]->next;
		while (p)
		{
			q = p->next;
			insertEdge(active, p);
			p = q;
		}
	}
	void fillScan(int scan, Edge* active, uint16_t color)
	{
		Edge* p1, * p2;
		int i;
		p1 = active->next;
		while (p1)
		{
			p2 = p1->next;
			for (i = p1->xIntersect; i < p2->xIntersect; i++)
			{
				//DrawPixel((int)i, scan, color, false);
			}
		}
	}
	void deleteAfter(Edge* q)
	{
		Edge* p = q->next;
		q->next = p->next;
		free(p);
	}
	void updateActiveList(int scan, Edge* active)
	{
		//Delete copmleted edges. Update xIntersect field for others
		Edge* q = active, * p = active->next;
		while (p)
		{
			if (scan >= p->yUpper)
			{
				p = p->next;
				deleteAfter(q);
			}
			else
			{
				p->xIntersect = p->xIntersect + p->dxPerScan;
				q = p;
				p = p->next;
			}
		}
	}
	void resortActiveList(Edge* active)
	{
		Edge* q, * p = active->next;
		active->next = NULL;
		while (p)
		{
			q = p->next;
			insertEdge(active, p);
			p = q;
		}
	}
	void scanFill(int cnt, dcPt* pts, uint16_t color)
	{
		Edge* edges[HEIGHT_PIXELS], * active;
		int i, scan;
		for (i = 0; i < HEIGHT_PIXELS; i++)
		{
			edges[i] = (Edge*)malloc(sizeof(Edge));
			edges[i]->next = NULL;
		}
		buildEdgeList(cnt, pts, edges);
		active = (Edge*)malloc(sizeof(Edge));
		active->next = NULL;
		for (scan = 0; scan < HEIGHT_PIXELS; scan++)
		{
			buildActiveList(scan, active, edges);
			if (active->next)
			{
				fillScan(scan, active, color);
				updateActiveList(scan, active);
				resortActiveList(active);
			}
		}
		//Free edge records that have been alloc'ed ...
	}
};