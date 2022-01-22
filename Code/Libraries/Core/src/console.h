#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdio.h>
#include <Windows.h>

class Console
{
private:
	Console();
	~Console();

	static Console* m_Instance;

public:
	static Console*	GetInstance();
	static void		DeleteInstance();
	static bool		IsOpen();
	static void		Toggle();

	HWND			GetHWnd();
	void			SetPos( const int X, const int Y ) const;
};

#endif // CONSOLE_H
