#include <iostream>
#include <unistd.h>
#include <X11/cursorfont.h>
#include <X11/Xatom.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xmu/WinUtil.h>

static int intarg() { return atoi(optarg); } // XXX: use strtol and invoke usage()

static Atom
net_move_resize(Display *display) {
    return XInternAtom(display, "_NET_MOVERESIZE_WINDOW", False);
}

struct Size {
    unsigned width;
    unsigned height;
};

struct Geometry {
    Size size;
    int x;
    int y;
};

static Geometry
getGeometry(Display *display, Window w, Window *root) {
    Geometry rv;
    unsigned int borderWidth;
    unsigned int depth;
    Status s = XGetGeometry(display, w, root,  &rv.x, &rv.y,
            &rv.size.width, &rv.size.height, &borderWidth, &depth);
    if (!s)
        throw "Can't get window geometry";
    return rv;
}

static void
setGeometry(Display *display,  Window win, Window root, const Geometry &geom) {
    // Tell the WM where to put it.
    XEvent e;
    XClientMessageEvent &ec = e.xclient;
    ec.type = ClientMessage;
    ec.serial = 1;
    ec.send_event = True;
    ec.message_type = net_move_resize(display);
    ec.window = win;
    ec.format = 32;
    ec.data.l[0] = 0xf0a;
    ec.data.l[1] = geom.x;
    ec.data.l[2] = geom.y;
    ec.data.l[3] = geom.size.width;
    ec.data.l[4] = geom.size.height;
    XSendEvent(display, root, False, SubstructureRedirectMask|SubstructureNotifyMask, &e);
    XSync(display, False);
}

static Window
pick(Display *display, Window root) {
    Window w = root;
    Cursor c = XCreateFontCursor(display, XC_tcross);
    if (XGrabPointer(display, root, False, ButtonPressMask|ButtonReleaseMask,
            GrabModeSync, GrabModeAsync, None, c, CurrentTime) != GrabSuccess) {
        throw "can't grab pointer";
    }
    for (bool done = false; !done;) {
        XEvent event;
        XAllowEvents(display, SyncPointer, CurrentTime);
        XWindowEvent(display, root, ButtonPressMask|ButtonReleaseMask, &event);
        switch (event.type) {
            case ButtonPress:
                if (event.xbutton.button == 1 && event.xbutton.subwindow != None)
                    w = event.xbutton.subwindow;
                break;
            case ButtonRelease:
                done = true;
                break;
        }
    }
    XUngrabPointer(display, CurrentTime);
    XFreeCursor(display, c);
    return XmuClientWindow(display, w);
}

int
main(int argc, char *argv[]) {
    int c;
    Window win = 0;
    Display *display = XOpenDisplay(0);
    auto root = XDefaultRootWindow(display);
    if (display == 0) {
        std::clog << "failed to open display: set DISPLAY environment variable" << std::endl;
        return 1;
    }
    while ((c = getopt(argc, argv, "o:s:w:W:abfghimnpuvx_O:")) != -1) {
        switch (c) {
            case 'p':
                win = pick(display, root);
                break;
            case 'w':
               win = intarg();
               break;
        }
    }
    if (win == 0) {
        std::cerr << "no window selected\n";
        return 0;
    }
    auto geo = getGeometry(display, win, &root);
    auto small = geo;
    auto smaller = geo;
    small.size.width /= 2;
    small.size.height /= 2;
    smaller.size.width /= 4;
    smaller.size.height /= 4;
    std::clog << "existing size: " << geo.size.width << "x" << geo.size.height << std::endl;
    for (int i = 0; i < 100; ++i) {
        setGeometry(display, win, root, small);
        setGeometry(display, win, root, smaller);
        usleep(1000);
    }
    setGeometry(display, win, root, geo);
    return 0;
}
