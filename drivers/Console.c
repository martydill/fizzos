
/*
* Console.c
* Console text mode video driver
* References: http://my.execpc.com/~geezer/osd/cons/index.htm
*/

#include <kernel.h>
#include <io.h>
#include <console.h>
#include <device.h>

Console console[NUM_CONSOLES];
unsigned int activeConsole;
Console* pActiveConsole;

Device consoleDevice;


STATUS ConsoleDeviceWrite(char* buffer, int numBytes)
{
    return S_OK;
}


/* Initialize console driver */
STATUS ConInit(void)
{
    int i;

    for(i = 0; i < NUM_CONSOLES; ++i)
    {
        console[i].buffer = (unsigned char*)(CONSOLE_BASE + (CONSOLE_SIZE * i));
        console[i].cursorX = 0;
        console[i].cursorY = 0;
        console[i].color = 0x0F;
    }

    activeConsole = 0;
    pActiveConsole = &console[0];

    consoleDevice.Name = "Console";
    consoleDevice.Write = ConsoleDeviceWrite;
    consoleDevice.Status = 0;
    consoleDevice.Status |= DEVICE_CAN_WRITE;

    DeviceRegister(&consoleDevice);
    return S_OK;
}


/* Switch to the specified console */
STATUS ConActivateConsole(unsigned int number)
{
    if(number >= NUM_CONSOLES)
        return S_FAIL;

    activeConsole = number;
    pActiveConsole = &console[activeConsole];
    return S_OK;
}

static int offset = 0;

/* Updates the position of the hardware cursor. Private. */
STATUS ConpUpdateHardwareCursor(void)
{
    WORD position = pActiveConsole->cursorY * CONSOLE_WIDTH + pActiveConsole->cursorX + offset;

    IoWritePortByte(0x3d4, 0x0e);
    IoWritePortByte(0x3d5, position >> 8);

    IoWritePortByte(0x3d4, 0x0f);
    IoWritePortByte(0x3d5, position);

    return S_OK;
}

void ScrollDown()
{
    offset += CONSOLE_WIDTH;
    IoWritePortByte(0x3d4, 12);
    IoWritePortByte(0x3d5, offset >> 8);
    IoWritePortByte(0x3d4, 13);
    IoWritePortByte(0x3d5, offset & 0xff);
    ConpUpdateHardwareCursor();
}

STATUS ConGetCursorPosition(POINT* point)
{
    if(point == NULL)
        return S_FAIL;

    point->X = pActiveConsole->cursorX;
    point->Y = pActiveConsole->cursorY;
    return S_OK;
}

/* Moves the console's cursor to a new location */
STATUS ConMoveCursor(WORD newX, WORD newY)
{
    if(newX >= CONSOLE_WIDTH || newY >= CONSOLE_HEIGHT)
        return S_FAIL;

    pActiveConsole->cursorX = newX;
    pActiveConsole->cursorY = newY;

    ConpUpdateHardwareCursor();
    return S_OK;
}

// Moves the cursor one space back
STATUS ConBackspace(void)
{
    if(pActiveConsole->cursorX == 0)
    {
        if(pActiveConsole->cursorY == 0)
            return S_FAIL;

        pActiveConsole->cursorX = CONSOLE_WIDTH - 1;
        pActiveConsole->cursorY = pActiveConsole->cursorY - 1;
    }

    pActiveConsole->cursorX--;
    pActiveConsole->buffer[(pActiveConsole->cursorY * CONSOLE_WIDTH + pActiveConsole->cursorX) * 2] = ' ';

    ConpUpdateHardwareCursor();
    return S_OK;
}

/* Clears the console's screen */
STATUS ConClearScreen(void)
{
    int i;

    /* FIXME: use memset */
    for(i = 0; i < CONSOLE_WIDTH * CONSOLE_HEIGHT * 2; i += 2)
    {
        pActiveConsole->buffer[i] = ' ';
        pActiveConsole->buffer[i + 1] = pActiveConsole->color;
    }

    return S_OK;
}


/* Displays the requested string at the requested location */
STATUS ConDisplayString(const char* str, WORD x, WORD y)
{
    int loc;
    int newX, newY;
    loc = (y * CONSOLE_WIDTH + x) * 2;

    /*	Assert(str != NULL);
    	Assert(x > 0);
    	Assert(y > 0);*/

    if(str == NULL)
        return S_FAIL;

    while(*str != '\0')
    {
        pActiveConsole->buffer[loc++] = *str++;
        pActiveConsole->buffer[loc++] = pActiveConsole->color;
    }

    newX = (loc % (CONSOLE_WIDTH * 2)) / 2;
    newY = loc / (CONSOLE_WIDTH * 2);
    ConMoveCursor(newX, newY);

    return S_OK;
}


/* Destroy console driver */
STATUS ConDestroy(void)
{
    return S_OK;
}

