#include <stdio.h>
#include <string.h>
#include <termio.h>
#include <unistd.h>
#include "key.h"

static int stdin_fileno = -1;
static struct termios original_state;


void keyboardInit(void)
{
    struct termios term_state;


    if (stdin_fileno == -1)
    {
        stdin_fileno = fileno(stdin);
        tcgetattr(stdin_fileno, &original_state);
        memcpy(&term_state, &original_state, sizeof(term_state));
        term_state.c_lflag &= ~(ICANON|ECHO);
        tcsetattr(stdin_fileno, TCSANOW, &term_state);
        setbuf(stdin, NULL);
    }
}


unsigned int keyboardRead(void)
{
    int chars_in_buff = 0;


    ioctl(stdin_fileno, FIONREAD, &chars_in_buff);

    if (chars_in_buff)
    {
        unsigned int retVal = 0;
        int character;

        //printf("%d chars in buff:", chars_in_buff);
        while (chars_in_buff--)
        {
            character = fgetc(stdin);
            //printf(" 0x%02x", character);
            retVal += character<<(8*chars_in_buff);
        }

        //printf("\n");

        return retVal;
    }
    else  // no chars in buff
        return 0;
}

void keyboardDeinit(void)
{
    if (stdin_fileno != -1)
    {
        tcsetattr(stdin_fileno, TCSANOW, &original_state);
    }
}
