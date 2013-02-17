/* Sample Program to demonstrate the drawing functions in GBDK */
/* Jon Fuge jonny@q-continuum.demon.co.uk */

#include <gb/gb.h>
#include <gb/drawing.h>

void main()
{
    UBYTE  a,b,c,d,e;
    c=0;
    /* Draw many characters on the screen with different fg and bg colours */
    for (a=0; a<=15; a++) {
	for (b=0; b<=15; b++) {
	    gotogxy(b,a);
	    gprintf("%c",c++);
	} 
    }


}
