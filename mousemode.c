#define _POSIX_C_SOURCE 199309L
#include <stdlib.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <time.h>
#include <signal.h>

#define FREQUENCY 100

Display *dpy;
int running = 1;
int root;
char keys_return[32] = {0};
int mousespdl = 0;
int mousespdd = 0;
int mousespdu = 0;
int mousespdr = 0;
int mousepress1 = 0;
int mousepress2 = 0;
int mousepress3 = 0;
int mousescrollrate = 0;

int iskeydown(int keysym)
{
    KeyCode c = XKeysymToKeycode(dpy, keysym);
    return (keys_return[ c >> 3 ] & (1 << (c & 7)))? 1 : 0;
}

int handleKeys()
{
    /* References:
     * https://stackoverflow.com/a/49840783
     * https://pastebin.com/sk7FZ6AP
     */
    XQueryKeymap(dpy, keys_return);
    if (iskeydown(XK_Escape))
        return 0;
    if (iskeydown(XK_Super_L) || iskeydown(XK_Super_R))
        return 1;
    int isfast, isslow, isleft, isdown, isup, isright, ism1, ism2, ism3, ism4, ism5;
    isfast    = iskeydown(XK_c);
    isslow    = iskeydown(XK_f);

    /*
     *     rouge-like    scroll       Speed      Quit
     *      y  k  u        o
     *       \ | /         |       (c) faster    Esc
     *    h --   -- l      |       (f) slower
     *       / | \         |       
     *     b   j  n       e/i
     */

    isleft    = (iskeydown(XK_h)) || (iskeydown(XK_y)) ||  (iskeydown(XK_b))?  1 : 0;
    isdown    = (iskeydown(XK_j)) || (iskeydown(XK_n)) ||  (iskeydown(XK_b))?  1 : 0;
    isup      = (iskeydown(XK_k)) || (iskeydown(XK_y)) ||  (iskeydown(XK_u))?  1 : 0;
    isright   = (iskeydown(XK_l)) || (iskeydown(XK_u)) ||  (iskeydown(XK_n))?  1 : 0;

    ism1      = iskeydown(XK_a); /* left click   */
    ism2      = iskeydown(XK_s); /* middle click */
    ism3      = iskeydown(XK_d); /* right click  */
    ism4      = iskeydown(XK_o); /* up   scroll  */
    /* ism4      = (iskeydown(XK_o)) ||  (iskeydown(XK_w))?  1 : 0;  up   scroll  */
    ism5      = (iskeydown(XK_e)) ||  (iskeydown(XK_i))?  1 : 0; /* down   scroll  */

    mousespdl = isleft  * (isslow? 1 : 8) * (isfast? 2 : 1);
    mousespdd = isdown  * (isslow? 1 : 8) * (isfast? 2 : 1);
    mousespdu = isup    * (isslow? 1 : 8) * (isfast? 2 : 1);
    mousespdr = isright * (isslow? 1 : 8) * (isfast? 2 : 1);
    XWarpPointer(dpy, None, None, 0, 0, 0, 0, mousespdr - mousespdl, mousespdd - mousespdu);

    if (ism1 && !mousepress1) {
        XTestFakeButtonEvent(dpy, 1, True, CurrentTime);
        mousepress1 = 1;
    } else if (!ism1 && mousepress1) {
        XTestFakeButtonEvent(dpy, 1, False, CurrentTime);
        mousepress1 = 0;
    }
    if (ism2 && !mousepress2) {
        XTestFakeButtonEvent(dpy, 2, True, CurrentTime);
        mousepress2 = 1;
    } else if (!ism2 && mousepress2) {
        XTestFakeButtonEvent(dpy, 2, False, CurrentTime);
        mousepress2 = 0;
    }
    if (ism3 && !mousepress3) {
        XTestFakeButtonEvent(dpy, 3, True, CurrentTime);
        mousepress3 = 1;
    } else if (!ism3 && mousepress3) {
        XTestFakeButtonEvent(dpy, 3, False, CurrentTime);
        mousepress3 = 0;
    }
    if (!isslow) {
        if (ism4 && !ism5 && mousescrollrate-- <= 0) {
            XTestFakeButtonEvent(dpy, 4, True, CurrentTime);
            XTestFakeButtonEvent(dpy, 4, False, CurrentTime);
            mousescrollrate = (isfast? 1 : 2);
        } else if (!ism4 && ism5 && mousescrollrate-- <= 0) {
            XTestFakeButtonEvent(dpy, 5, True, CurrentTime);
            XTestFakeButtonEvent(dpy, 5, False, CurrentTime);
            mousescrollrate = (isfast? 1 : 2);
        } else if (!ism4 && !ism5) {
            mousescrollrate = 0;
        }
    } else {
        XTestFakeButtonEvent(dpy, 4, ism4? True : False, CurrentTime);
        XTestFakeButtonEvent(dpy, 5, ism5? True : False, CurrentTime);
    }
    XFlush(dpy);

    return 1;
}

/* testing to see what interrupts mousemode */
int handler(Display *display, XErrorEvent *ev)
{
    fprintf(stderr, "--------------------------------\n");
    fprintf(stderr, "Error event %lu %p!\n", ev->serial, ev);
    fprintf(stderr, "  ev->type: %d\n", ev->type);
    switch (ev->error_code) {
        case Success: fprintf(stderr, "  ev->error_code: success.\n"); break;
        case BadAccess: fprintf(stderr, "  ev->error_code: BadAccess!\n"); break;
        case BadValue: fprintf(stderr, "  ev->error_code: BadValue!\n"); break;
        case BadWindow: fprintf(stderr, "  ev->error_code: BadWindow!\n"); break;
        default: fprintf(stderr, "  ev->error_code: unknown error %d!\n", ev->error_code); break;
    }
    return 0;
}



void grabkey(int keysym)
{
    KeyCode code;
    if ((code = XKeysymToKeycode(dpy, keysym))) {
        XGrabKey(dpy, code, 0, root, True, GrabModeAsync, GrabModeAsync);
        XGrabKey(dpy, code, ShiftMask, root, True, GrabModeAsync, GrabModeAsync);
        XGrabKey(dpy, code, ControlMask, root, True, GrabModeAsync, GrabModeAsync);
        XGrabKey(dpy, code, Mod1Mask, root, True, GrabModeAsync, GrabModeAsync);
        XGrabKey(dpy, code, ShiftMask|ControlMask, root, True, GrabModeAsync, GrabModeAsync);
    }
}

void termhandler(int signum)
{
    running = 0;
}

int main()
{
    signal(SIGTERM, termhandler);
    signal(SIGINT, termhandler);

    /* test to see what interrupts mousemode */
    XSetErrorHandler(handler);

    if (!(dpy = XOpenDisplay(NULL))) {
        fprintf(stderr, "mousemode: failed to open display");
        return 2;
    }

    root = RootWindow(dpy, DefaultScreen(dpy));
    {

        grabkey(XK_Escape);                          /* quit    */
        grabkey(XK_a); grabkey(XK_s); grabkey(XK_d); /* mouse   */
        grabkey(XK_e); grabkey(XK_o);                /* scrolls */
        // grabkey(XK_w); grabkey(XK_i);             /* scrolls */
        grabkey(XK_i);                               /* scrolls */
        grabkey(XK_f); grabkey(XK_c);                /* speed   */
        grabkey(XK_h); grabkey(XK_j); grabkey(XK_k); grabkey(XK_l); /* vim-only */
        grabkey(XK_y); grabkey(XK_u); grabkey(XK_b); grabkey(XK_n); /* rouge-like */
   }

    while (running) {
        const struct timespec sleep_timespec = {0, 1e9 / FREQUENCY};
        running &= handleKeys();
        nanosleep(&sleep_timespec, NULL);
    }

    XUngrabKey(dpy, AnyKey, AnyModifier, root);
    for (int i = 1; i <= 5; i++)
        XTestFakeButtonEvent(dpy, i, False, CurrentTime);
    XCloseDisplay(dpy);
    return 0;
}#define _POSIX_C_SOURCE 199309L
#include <stdlib.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <time.h>
#include <signal.h>

#define FREQUENCY 100

Display *dpy;
int running = 1;
int root;
char keys_return[32] = {0};
int mousespdl = 0;
int mousespdd = 0;
int mousespdu = 0;
int mousespdr = 0;
int mousepress1 = 0;
int mousepress2 = 0;
int mousepress3 = 0;
int mousescrollrate = 0;

int iskeydown(int keysym)
{
    KeyCode c = XKeysymToKeycode(dpy, keysym);
    return (keys_return[ c >> 3 ] & (1 << (c & 7)))? 1 : 0;
}

int handleKeys()
{
    /* References:
     * https://stackoverflow.com/a/49840783
     * https://pastebin.com/sk7FZ6AP
     */
    XQueryKeymap(dpy, keys_return);
    if (iskeydown(XK_Escape))
        return 0;
    if (iskeydown(XK_Super_L) || iskeydown(XK_Super_R))
        return 1;
    int isfast, isslow, isleft, isdown, isup, isright, ism1, ism2, ism3, ism4, ism5;
    isfast    = iskeydown(XK_c);
    isslow    = iskeydown(XK_f);

    /*
     *     rouge-like    scroll       Speed      Quit
     *      y  k  u        o
     *       \ | /         |       (c) faster    Esc
     *    h --   -- l      |       (f) slower
     *       / | \         |       
     *     b   j  n       e/i
     */

    isleft    = (iskeydown(XK_h)) || (iskeydown(XK_y)) ||  (iskeydown(XK_b))?  1 : 0;
    isdown    = (iskeydown(XK_j)) || (iskeydown(XK_n)) ||  (iskeydown(XK_b))?  1 : 0;
    isup      = (iskeydown(XK_k)) || (iskeydown(XK_y)) ||  (iskeydown(XK_u))?  1 : 0;
    isright   = (iskeydown(XK_l)) || (iskeydown(XK_u)) ||  (iskeydown(XK_n))?  1 : 0;

    ism1      = iskeydown(XK_a); /* left click   */
    ism2      = iskeydown(XK_s); /* middle click */
    ism3      = iskeydown(XK_d); /* right click  */
    ism4      = iskeydown(XK_o); /* up   scroll  */
    /* ism4      = (iskeydown(XK_o)) ||  (iskeydown(XK_w))?  1 : 0;  up   scroll  */
    ism5      = (iskeydown(XK_e)) ||  (iskeydown(XK_i))?  1 : 0; /* down   scroll  */

    mousespdl = isleft  * (isslow? 1 : 8) * (isfast? 2 : 1);
    mousespdd = isdown  * (isslow? 1 : 8) * (isfast? 2 : 1);
    mousespdu = isup    * (isslow? 1 : 8) * (isfast? 2 : 1);
    mousespdr = isright * (isslow? 1 : 8) * (isfast? 2 : 1);
    XWarpPointer(dpy, None, None, 0, 0, 0, 0, mousespdr - mousespdl, mousespdd - mousespdu);

    if (ism1 && !mousepress1) {
        XTestFakeButtonEvent(dpy, 1, True, CurrentTime);
        mousepress1 = 1;
    } else if (!ism1 && mousepress1) {
        XTestFakeButtonEvent(dpy, 1, False, CurrentTime);
        mousepress1 = 0;
    }
    if (ism2 && !mousepress2) {
        XTestFakeButtonEvent(dpy, 2, True, CurrentTime);
        mousepress2 = 1;
    } else if (!ism2 && mousepress2) {
        XTestFakeButtonEvent(dpy, 2, False, CurrentTime);
        mousepress2 = 0;
    }
    if (ism3 && !mousepress3) {
        XTestFakeButtonEvent(dpy, 3, True, CurrentTime);
        mousepress3 = 1;
    } else if (!ism3 && mousepress3) {
        XTestFakeButtonEvent(dpy, 3, False, CurrentTime);
        mousepress3 = 0;
    }
    if (!isslow) {
        if (ism4 && !ism5 && mousescrollrate-- <= 0) {
            XTestFakeButtonEvent(dpy, 4, True, CurrentTime);
            XTestFakeButtonEvent(dpy, 4, False, CurrentTime);
            mousescrollrate = (isfast? 1 : 2);
        } else if (!ism4 && ism5 && mousescrollrate-- <= 0) {
            XTestFakeButtonEvent(dpy, 5, True, CurrentTime);
            XTestFakeButtonEvent(dpy, 5, False, CurrentTime);
            mousescrollrate = (isfast? 1 : 2);
        } else if (!ism4 && !ism5) {
            mousescrollrate = 0;
        }
    } else {
        XTestFakeButtonEvent(dpy, 4, ism4? True : False, CurrentTime);
        XTestFakeButtonEvent(dpy, 5, ism5? True : False, CurrentTime);
    }
    XFlush(dpy);

    return 1;
}

/* testing to see what interrupts mousemode */
int handler(Display *display, XErrorEvent *ev)
{
    fprintf(stderr, "--------------------------------\n");
    fprintf(stderr, "Error event %lu %p!\n", ev->serial, ev);
    fprintf(stderr, "  ev->type: %d\n", ev->type);
    switch (ev->error_code) {
        case Success: fprintf(stderr, "  ev->error_code: success.\n"); break;
        case BadAccess: fprintf(stderr, "  ev->error_code: BadAccess!\n"); break;
        case BadValue: fprintf(stderr, "  ev->error_code: BadValue!\n"); break;
        case BadWindow: fprintf(stderr, "  ev->error_code: BadWindow!\n"); break;
        default: fprintf(stderr, "  ev->error_code: unknown error %d!\n", ev->error_code); break;
    }
    return 0;
}



void grabkey(int keysym)
{
    KeyCode code;
    if ((code = XKeysymToKeycode(dpy, keysym))) {
        XGrabKey(dpy, code, 0, root, True, GrabModeAsync, GrabModeAsync);
        XGrabKey(dpy, code, ShiftMask, root, True, GrabModeAsync, GrabModeAsync);
        XGrabKey(dpy, code, ControlMask, root, True, GrabModeAsync, GrabModeAsync);
        XGrabKey(dpy, code, Mod1Mask, root, True, GrabModeAsync, GrabModeAsync);
        XGrabKey(dpy, code, ShiftMask|ControlMask, root, True, GrabModeAsync, GrabModeAsync);
    }
}

void termhandler(int signum)
{
    running = 0;
}

int main()
{
    signal(SIGTERM, termhandler);
    signal(SIGINT, termhandler);

    /* test to see what interrupts mousemode */
    XSetErrorHandler(handler);

    if (!(dpy = XOpenDisplay(NULL))) {
        fprintf(stderr, "mousemode: failed to open display");
        return 2;
    }

    root = RootWindow(dpy, DefaultScreen(dpy));
    {

        grabkey(XK_Escape);                          /* quit    */
        grabkey(XK_a); grabkey(XK_s); grabkey(XK_d); /* mouse   */
        grabkey(XK_e); grabkey(XK_o);                /* scrolls */
        // grabkey(XK_w); grabkey(XK_i);             /* scrolls */
        grabkey(XK_i);                               /* scrolls */
        grabkey(XK_f); grabkey(XK_c);                /* speed   */
        grabkey(XK_h); grabkey(XK_j); grabkey(XK_k); grabkey(XK_l); /* vim-only */
        grabkey(XK_y); grabkey(XK_u); grabkey(XK_b); grabkey(XK_n); /* rouge-like */
   }

    while (running) {
        const struct timespec sleep_timespec = {0, 1e9 / FREQUENCY};
        running &= handleKeys();
        nanosleep(&sleep_timespec, NULL);
    }

    XUngrabKey(dpy, AnyKey, AnyModifier, root);
    for (int i = 1; i <= 5; i++)
        XTestFakeButtonEvent(dpy, i, False, CurrentTime);
    XCloseDisplay(dpy);
    return 0;
}
