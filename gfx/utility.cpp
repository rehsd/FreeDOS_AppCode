bool IsKeyPressed()
{
	union REGPACK regs;
	memset(&regs, 0, sizeof(union REGPACK));
	regs.w.ax = 0x0100;				// AH = 0x01		get keystroke status
	intr(0x16, &regs);
	if (regs.w.ax)
	{
		return true;
	}
	else
	{
		return false;
	}
}