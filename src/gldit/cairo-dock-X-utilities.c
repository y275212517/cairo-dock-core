/**
* This file is a part of the Cairo-Dock project
*
* Copyright : (C) see the 'copyright' file.
* E-mail    : see the 'copyright' file.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 3
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#if (GTK_MAJOR_VERSION >= 3)
#include <cairo/cairo-xlib.h>  // needed for cairo_xlib_surface_create
#endif

#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include "gldi-config.h"
#ifdef HAVE_XEXTEND
#include <X11/extensions/Xcomposite.h>
//#include <X11/extensions/Xdamage.h>
#include <X11/extensions/XTest.h>
#include <X11/extensions/Xinerama.h>
#include <X11/extensions/shape.h>
#endif

#include "cairo-dock-container.h"  // CAIRO_DOCK_HORIZONTAL
#include "cairo-dock-log.h"
#include "cairo-dock-X-manager.h"
#include "cairo-dock-X-utilities.h"

extern CairoDockDesktopGeometry g_desktopGeometry;

static gboolean s_bUseXComposite = TRUE;
static gboolean s_bUseXTest = TRUE;
static gboolean s_bUseXinerama = TRUE;
//extern int g_iDamageEvent;

static Display *s_XDisplay = NULL;
// Atoms pour le bureau
static Atom s_aNetWmWindowType;
static Atom s_aNetWmWindowTypeNormal;
static Atom s_aNetWmWindowTypeUtility;
static Atom s_aNetWmWindowTypeDock;
static Atom s_aNetWmIconGeometry;
static Atom s_aNetCurrentDesktop;
static Atom s_aNetDesktopViewport;
static Atom s_aNetDesktopGeometry;
static Atom s_aNetNbDesktops;
static Atom s_aRootMapID;
// Atoms pour les fenetres
static Atom s_aNetClientList;  // z-order
static Atom s_aNetClientListStacking;  // age-order
static Atom s_aNetActiveWindow;
static Atom s_aNetWmState;
static Atom s_aNetWmBelow;
static Atom s_aNetWmAbove;
static Atom s_aNetWmHidden;
static Atom s_aNetWmFullScreen;
static Atom s_aNetWmSkipTaskbar;
static Atom s_aNetWmMaximizedHoriz;
static Atom s_aNetWmMaximizedVert;
static Atom s_aNetWmDemandsAttention;
static Atom s_aNetWmDesktop;
static Atom s_aNetWmName;
static Atom s_aWmName;
static Atom s_aUtf8String;
static Atom s_aString;

static int _cairo_dock_xerror_handler (Display * pDisplay, XErrorEvent *pXError)
{
	cd_debug ("Error (%d, %d, %d) during an X request on %d", pXError->error_code, pXError->request_code, pXError->minor_code, pXError->resourceid);
	return 0;
}
Display *cairo_dock_initialize_X_desktop_support (void)
{
	if (s_XDisplay != NULL)
		return s_XDisplay;
	s_XDisplay = XOpenDisplay (0);
	g_return_val_if_fail (s_XDisplay != NULL, NULL);
	
	XSetErrorHandler (_cairo_dock_xerror_handler);
	
	cairo_dock_support_X_extension ();
	
	s_aNetWmWindowType		= XInternAtom (s_XDisplay, "_NET_WM_WINDOW_TYPE", False);
	s_aNetWmWindowTypeNormal	= XInternAtom (s_XDisplay, "_NET_WM_WINDOW_TYPE_NORMAL", False);
	s_aNetWmWindowTypeUtility	= XInternAtom (s_XDisplay, "_NET_WM_WINDOW_TYPE_UTILITY", False);
	s_aNetWmWindowTypeDock		= XInternAtom (s_XDisplay, "_NET_WM_WINDOW_TYPE_DOCK", False);
	s_aNetWmIconGeometry		= XInternAtom (s_XDisplay, "_NET_WM_ICON_GEOMETRY", False);
	s_aNetCurrentDesktop		= XInternAtom (s_XDisplay, "_NET_CURRENT_DESKTOP", False);
	s_aNetDesktopViewport		= XInternAtom (s_XDisplay, "_NET_DESKTOP_VIEWPORT", False);
	s_aNetDesktopGeometry		= XInternAtom (s_XDisplay, "_NET_DESKTOP_GEOMETRY", False);
	s_aNetNbDesktops			= XInternAtom (s_XDisplay, "_NET_NUMBER_OF_DESKTOPS", False);
	s_aRootMapID			= XInternAtom (s_XDisplay, "_XROOTPMAP_ID", False);
	
	s_aNetClientListStacking	= XInternAtom (s_XDisplay, "_NET_CLIENT_LIST_STACKING", False);
	s_aNetClientList			= XInternAtom (s_XDisplay, "_NET_CLIENT_LIST", False);
	s_aNetActiveWindow		= XInternAtom (s_XDisplay, "_NET_ACTIVE_WINDOW", False);
	s_aNetWmState			= XInternAtom (s_XDisplay, "_NET_WM_STATE", False);
	s_aNetWmFullScreen		= XInternAtom (s_XDisplay, "_NET_WM_STATE_FULLSCREEN", False);
	s_aNetWmAbove			= XInternAtom (s_XDisplay, "_NET_WM_STATE_ABOVE", False);
	s_aNetWmBelow			= XInternAtom (s_XDisplay, "_NET_WM_STATE_BELOW", False);
	s_aNetWmHidden			= XInternAtom (s_XDisplay, "_NET_WM_STATE_HIDDEN", False);
	s_aNetWmSkipTaskbar 		= XInternAtom (s_XDisplay, "_NET_WM_STATE_SKIP_TASKBAR", False);
	s_aNetWmMaximizedHoriz		= XInternAtom (s_XDisplay, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
	s_aNetWmMaximizedVert		= XInternAtom (s_XDisplay, "_NET_WM_STATE_MAXIMIZED_VERT", False);
	s_aNetWmDemandsAttention	= XInternAtom (s_XDisplay, "_NET_WM_STATE_DEMANDS_ATTENTION", False);
	s_aNetWmDesktop			= XInternAtom (s_XDisplay, "_NET_WM_DESKTOP", False);
	s_aNetWmName 			= XInternAtom (s_XDisplay, "_NET_WM_NAME", False);
	s_aWmName 				= XInternAtom (s_XDisplay, "WM_NAME", False);
	s_aUtf8String 			= XInternAtom (s_XDisplay, "UTF8_STRING", False);
	s_aString 				= XInternAtom (s_XDisplay, "STRING", False);
	
	Screen *XScreen = XDefaultScreenOfDisplay (s_XDisplay);
	g_desktopGeometry.iXScreenWidth[CAIRO_DOCK_HORIZONTAL] = WidthOfScreen (XScreen);
	g_desktopGeometry.iXScreenHeight[CAIRO_DOCK_HORIZONTAL] = HeightOfScreen (XScreen);
	g_desktopGeometry.iXScreenWidth[CAIRO_DOCK_VERTICAL] = g_desktopGeometry.iXScreenHeight[CAIRO_DOCK_HORIZONTAL];
	g_desktopGeometry.iXScreenHeight[CAIRO_DOCK_VERTICAL] = g_desktopGeometry.iXScreenWidth[CAIRO_DOCK_HORIZONTAL];
	
	g_desktopGeometry.iScreenWidth[CAIRO_DOCK_HORIZONTAL] = WidthOfScreen (XScreen);
	g_desktopGeometry.iScreenHeight[CAIRO_DOCK_HORIZONTAL] = HeightOfScreen (XScreen);
	g_desktopGeometry.iScreenWidth[CAIRO_DOCK_VERTICAL] = g_desktopGeometry.iScreenHeight[CAIRO_DOCK_HORIZONTAL];
	g_desktopGeometry.iScreenHeight[CAIRO_DOCK_VERTICAL] = g_desktopGeometry.iScreenWidth[CAIRO_DOCK_HORIZONTAL];
	
	return s_XDisplay;
}

Display *cairo_dock_get_Xdisplay (void)
{
	return s_XDisplay;
}

guint cairo_dock_get_root_id (void)
{
	return DefaultRootWindow (s_XDisplay);
}


gboolean cairo_dock_update_screen_geometry (void)
{
	Window root = DefaultRootWindow (s_XDisplay);
	Window root_return;
	int x_return=1, y_return=1;
	unsigned int width_return, height_return, border_width_return, depth_return;
	XGetGeometry (s_XDisplay, root,
		&root_return,
		&x_return, &y_return,
		&width_return, &height_return,
		&border_width_return, &depth_return);
	cd_debug (">>>>>   screen resolution: %dx%d -> %dx%d", g_desktopGeometry.iXScreenWidth[CAIRO_DOCK_HORIZONTAL], g_desktopGeometry.iXScreenHeight[CAIRO_DOCK_HORIZONTAL], width_return, height_return);
	if ((int)width_return != g_desktopGeometry.iXScreenWidth[CAIRO_DOCK_HORIZONTAL] || (int)height_return != g_desktopGeometry.iXScreenHeight[CAIRO_DOCK_HORIZONTAL])  // on n'utilise pas WidthOfScreen() et HeightOfScreen() car leurs valeurs ne sont pas mises a jour immediatement apres les changements de resolution.
	{
		g_desktopGeometry.iXScreenWidth[CAIRO_DOCK_HORIZONTAL] = width_return;
		g_desktopGeometry.iXScreenHeight[CAIRO_DOCK_HORIZONTAL] = height_return;
		g_desktopGeometry.iXScreenWidth[CAIRO_DOCK_VERTICAL] = g_desktopGeometry.iXScreenHeight[CAIRO_DOCK_HORIZONTAL];
		g_desktopGeometry.iXScreenHeight[CAIRO_DOCK_VERTICAL] = g_desktopGeometry.iXScreenWidth[CAIRO_DOCK_HORIZONTAL];
		
		g_desktopGeometry.iScreenWidth[CAIRO_DOCK_HORIZONTAL] = g_desktopGeometry.iXScreenWidth[CAIRO_DOCK_HORIZONTAL];
		g_desktopGeometry.iScreenHeight[CAIRO_DOCK_HORIZONTAL] = g_desktopGeometry.iXScreenHeight[CAIRO_DOCK_HORIZONTAL];
		g_desktopGeometry.iScreenWidth[CAIRO_DOCK_VERTICAL] = g_desktopGeometry.iScreenHeight[CAIRO_DOCK_HORIZONTAL];
		g_desktopGeometry.iScreenHeight[CAIRO_DOCK_VERTICAL] = g_desktopGeometry.iScreenWidth[CAIRO_DOCK_HORIZONTAL];  // si on utilise Xinerama, on mettra les valeurs correctes plus tard.
		
		cd_debug ("new screen size : %dx%d", g_desktopGeometry.iScreenWidth[CAIRO_DOCK_HORIZONTAL], g_desktopGeometry.iScreenHeight[CAIRO_DOCK_HORIZONTAL]);
		return TRUE;
	}
	else
		return FALSE;
	/*Atom aNetWorkArea = XInternAtom (s_XDisplay, "_NET_WORKAREA", False);
	iBufferNbElements = 0;
	gulong *pXWorkArea = NULL;
	XGetWindowProperty (s_XDisplay, root, aNetWorkArea, 0, G_MAXULONG, False, XA_CARDINAL, &aReturnedType, &aReturnedFormat, &iBufferNbElements, &iLeftBytes, (guchar **)&pXWorkArea);
	int i;
	for (i = 0; i < iBufferNbElements/4; i ++)
	{
		cd_message ("work area : (%d;%d) %dx%d\n", pXWorkArea[4*i], pXWorkArea[4*i+1], pXWorkArea[4*i+2], pXWorkArea[4*i+3]);
	}
	XFree (pXWorkArea);*/
}


gboolean cairo_dock_property_is_present_on_root (const gchar *cPropertyName)
{
	g_return_val_if_fail (s_XDisplay != NULL, FALSE);
	Atom atom = XInternAtom (s_XDisplay, cPropertyName, False);
	Window root = DefaultRootWindow (s_XDisplay);
	int iNbProperties;
	Atom *pAtomList = XListProperties (s_XDisplay, root, &iNbProperties);
	int i;
	for (i = 0; i < iNbProperties; i ++)
	{
		if (pAtomList[i] == atom)
			break;
	}
	XFree (pAtomList);
	return (i != iNbProperties);
}


int cairo_dock_get_current_desktop (void)
{
	Window root = DefaultRootWindow (s_XDisplay);
	Atom aReturnedType = 0;
	int aReturnedFormat = 0;
	unsigned long iLeftBytes, iBufferNbElements = 0;
	gulong *pXDesktopNumberBuffer = NULL;
	XGetWindowProperty (s_XDisplay, root, s_aNetCurrentDesktop, 0, G_MAXULONG, False, XA_CARDINAL, &aReturnedType, &aReturnedFormat, &iBufferNbElements, &iLeftBytes, (guchar **)&pXDesktopNumberBuffer);

	int iDesktopNumber;
	if (iBufferNbElements > 0)
		iDesktopNumber = *pXDesktopNumberBuffer;
	else
		iDesktopNumber = 0;
	
	XFree (pXDesktopNumberBuffer);
	return iDesktopNumber;
}

void cairo_dock_get_current_viewport (int *iCurrentViewPortX, int *iCurrentViewPortY)
{
	Window root = DefaultRootWindow (s_XDisplay);
	
	Window root_return;
	int x_return=1, y_return=1;
	unsigned int width_return, height_return, border_width_return, depth_return;
	XGetGeometry (s_XDisplay, root,
		&root_return,
		&x_return, &y_return,
		&width_return, &height_return,
		&border_width_return, &depth_return);
	*iCurrentViewPortX = x_return;
	*iCurrentViewPortY = y_return;
	
	Atom aReturnedType = 0;
	int aReturnedFormat = 0;
	unsigned long iLeftBytes, iBufferNbElements = 0;
	gulong *pViewportsXY = NULL;
	XGetWindowProperty (s_XDisplay, root, s_aNetDesktopViewport, 0, G_MAXULONG, False, XA_CARDINAL, &aReturnedType, &aReturnedFormat, &iBufferNbElements, &iLeftBytes, (guchar **)&pViewportsXY);
	if (iBufferNbElements > 0)
	{
		*iCurrentViewPortX = pViewportsXY[0];
		*iCurrentViewPortY = pViewportsXY[1];
		XFree (pViewportsXY);
	}
	
}

int cairo_dock_get_nb_desktops (void)
{
	Window root = DefaultRootWindow (s_XDisplay);
	Atom aReturnedType = 0;
	int aReturnedFormat = 0;
	unsigned long iLeftBytes, iBufferNbElements = 0;
	gulong *pXDesktopNumberBuffer = NULL;
	XGetWindowProperty (s_XDisplay, root, s_aNetNbDesktops, 0, G_MAXULONG, False, XA_CARDINAL, &aReturnedType, &aReturnedFormat, &iBufferNbElements, &iLeftBytes, (guchar **)&pXDesktopNumberBuffer);
	
	int iNumberOfDesktops;
	if (iBufferNbElements > 0)
		iNumberOfDesktops = *pXDesktopNumberBuffer;
	else
		iNumberOfDesktops = 0;
	
	return iNumberOfDesktops;
}

void cairo_dock_get_nb_viewports (int *iNbViewportX, int *iNbViewportY)
{
	Window root = DefaultRootWindow (s_XDisplay);
	Atom aReturnedType = 0;
	int aReturnedFormat = 0;
	unsigned long iLeftBytes, iBufferNbElements = 0;
	gulong *pVirtualScreenSizeBuffer = NULL;
	XGetWindowProperty (s_XDisplay, root, s_aNetDesktopGeometry, 0, G_MAXULONG, False, XA_CARDINAL, &aReturnedType, &aReturnedFormat, &iBufferNbElements, &iLeftBytes, (guchar **)&pVirtualScreenSizeBuffer);
	if (iBufferNbElements > 0)
	{
		Screen *scr = XDefaultScreenOfDisplay (s_XDisplay);
		cd_debug ("pVirtualScreenSizeBuffer : %dx%d ; screen : %dx%d", pVirtualScreenSizeBuffer[0], pVirtualScreenSizeBuffer[1], g_desktopGeometry.iXScreenWidth[CAIRO_DOCK_HORIZONTAL], g_desktopGeometry.iXScreenHeight[CAIRO_DOCK_HORIZONTAL]);
		*iNbViewportX = pVirtualScreenSizeBuffer[0] / g_desktopGeometry.iXScreenWidth[CAIRO_DOCK_HORIZONTAL];
		*iNbViewportY = pVirtualScreenSizeBuffer[1] / g_desktopGeometry.iXScreenHeight[CAIRO_DOCK_HORIZONTAL];
		XFree (pVirtualScreenSizeBuffer);
	}
}



gboolean cairo_dock_desktop_is_visible (void)
{
	Atom aNetShowingDesktop = XInternAtom (s_XDisplay, "_NET_SHOWING_DESKTOP", False);
	gulong iLeftBytes, iBufferNbElements = 0;
	Atom aReturnedType = 0;
	int aReturnedFormat = 0;
	gulong *pXBuffer = NULL;
	Window root = DefaultRootWindow (s_XDisplay);
	XGetWindowProperty (s_XDisplay, root, aNetShowingDesktop, 0, G_MAXULONG, False, XA_CARDINAL, &aReturnedType, &aReturnedFormat, &iBufferNbElements, &iLeftBytes, (guchar **)&pXBuffer);

	gboolean bDesktopIsShown = (iBufferNbElements > 0 && pXBuffer != NULL ? *pXBuffer : FALSE);
	XFree (pXBuffer);
	return bDesktopIsShown;
}

void cairo_dock_show_hide_desktop (gboolean bShow)
{
	XEvent xClientMessage;
	Window root = DefaultRootWindow (s_XDisplay);

	xClientMessage.xclient.type = ClientMessage;
	xClientMessage.xclient.serial = 0;
	xClientMessage.xclient.send_event = True;
	xClientMessage.xclient.display = s_XDisplay;
	xClientMessage.xclient.window = root;
	xClientMessage.xclient.message_type = XInternAtom (s_XDisplay, "_NET_SHOWING_DESKTOP", False);
	xClientMessage.xclient.format = 32;
	xClientMessage.xclient.data.l[0] = bShow;
	xClientMessage.xclient.data.l[1] = 0;
	xClientMessage.xclient.data.l[2] = 0;
	xClientMessage.xclient.data.l[3] = 2;
	xClientMessage.xclient.data.l[4] = 0;
	
	cd_debug ("%s (%d)\n", __func__, bShow);
	XSendEvent (s_XDisplay,
		root,
		False,
		SubstructureRedirectMask | SubstructureNotifyMask,
		&xClientMessage);
}

static void cairo_dock_move_current_viewport_to (int iDesktopViewportX, int iDesktopViewportY)
{
	XEvent xClientMessage;
	Window root = DefaultRootWindow (s_XDisplay);
	
	xClientMessage.xclient.type = ClientMessage;
	xClientMessage.xclient.serial = 0;
	xClientMessage.xclient.send_event = True;
	xClientMessage.xclient.display = s_XDisplay;
	xClientMessage.xclient.window = root;
	xClientMessage.xclient.message_type = s_aNetDesktopViewport;
	xClientMessage.xclient.format = 32;
	xClientMessage.xclient.data.l[0] = iDesktopViewportX;
	xClientMessage.xclient.data.l[1] = iDesktopViewportY;
	xClientMessage.xclient.data.l[2] = 0;
	xClientMessage.xclient.data.l[3] = 0;
	xClientMessage.xclient.data.l[4] = 0;
	
	XSendEvent (s_XDisplay,
		root,
		False,
		SubstructureRedirectMask | SubstructureNotifyMask,
		&xClientMessage);
}
void cairo_dock_set_current_viewport (int iViewportNumberX, int iViewportNumberY)
{
	cairo_dock_move_current_viewport_to (iViewportNumberX * g_desktopGeometry.iXScreenWidth[CAIRO_DOCK_HORIZONTAL], iViewportNumberY * g_desktopGeometry.iXScreenHeight[CAIRO_DOCK_HORIZONTAL]);
}
void cairo_dock_set_current_desktop (int iDesktopNumber)
{
	Window root = DefaultRootWindow (s_XDisplay);
	int iTimeStamp = cairo_dock_get_xwindow_timestamp (root);
	XEvent xClientMessage;
	
	xClientMessage.xclient.type = ClientMessage;
	xClientMessage.xclient.serial = 0;
	xClientMessage.xclient.send_event = True;
	xClientMessage.xclient.display = s_XDisplay;
	xClientMessage.xclient.window = root;
	xClientMessage.xclient.message_type = s_aNetCurrentDesktop;
	xClientMessage.xclient.format = 32;
	xClientMessage.xclient.data.l[0] = iDesktopNumber;
	xClientMessage.xclient.data.l[1] = iTimeStamp;
	xClientMessage.xclient.data.l[2] = 0;
	xClientMessage.xclient.data.l[3] = 0;
	xClientMessage.xclient.data.l[4] = 0;

	XSendEvent (s_XDisplay,
		root,
		False,
		SubstructureRedirectMask | SubstructureNotifyMask,
		&xClientMessage);
}

Pixmap cairo_dock_get_window_background_pixmap (Window Xid)
{
	g_return_val_if_fail (Xid > 0, None);
	//cd_debug ("%s (%d)", __func__, Xid);
	
	Pixmap iPixmapID;
	Atom aReturnedType = 0;
	int aReturnedFormat = 0;
	unsigned long iLeftBytes, iBufferNbElements;
	Pixmap *pPixmapIdBuffer = NULL;
	Pixmap iBgPixmapID = 0;
	XGetWindowProperty (s_XDisplay, Xid, s_aRootMapID, 0, G_MAXULONG, False, XA_PIXMAP, &aReturnedType, &aReturnedFormat, &iBufferNbElements, &iLeftBytes, (guchar **)&pPixmapIdBuffer);
	if (iBufferNbElements != 0)
	{
		iBgPixmapID = *pPixmapIdBuffer;
		XFree (pPixmapIdBuffer);
	}
	else
		iBgPixmapID = None;
	cd_debug (" => rootmapid : %d", iBgPixmapID);
	return iBgPixmapID;
}

GdkPixbuf *cairo_dock_get_pixbuf_from_pixmap (int XPixmapID, gboolean bAddAlpha)  // cette fonction est inspiree par celle de libwnck.
{
	//\__________________ On recupere la taille telle qu'elle est actuellement sur le serveur X.
	Window root;  // inutile.
	int x, y;  // inutile.
	guint border_width;  // inutile.
	guint iWidth, iHeight, iDepth;
	if (! XGetGeometry (s_XDisplay,
		XPixmapID, &root, &x, &y,
		&iWidth, &iHeight, &border_width, &iDepth))
		return NULL;
	//g_print ("%s (%d) : %ux%ux%u (%d;%d)\n", __func__, XPixmapID, iWidth, iHeight, iDepth, x, y);

	//\__________________ On recupere le drawable associe.
	#if (GTK_MAJOR_VERSION < 3)
	GdkDrawable *pGdkDrawable = gdk_xid_table_lookup (XPixmapID);
	if (pGdkDrawable)
	{
		g_object_ref (G_OBJECT (pGdkDrawable));
	}
	else
	{
		//g_print ("pas d'objet GDK present, on en alloue un nouveau\n");
		GdkScreen* pScreen = gdk_screen_get_default ();
		pGdkDrawable = gdk_pixmap_foreign_new_for_screen (pScreen, XPixmapID, iWidth, iHeight, iDepth);
	}

	//\__________________ On recupere la colormap.
	GdkColormap* pColormap = gdk_drawable_get_colormap (pGdkDrawable);
	if (pColormap == NULL && gdk_drawable_get_depth (pGdkDrawable) > 1)  // pour les bitmaps, on laisse la colormap a NULL, ils n'en ont pas besoin.
	{
		GdkScreen *pScreen = gdk_drawable_get_screen (GDK_DRAWABLE (pGdkDrawable));
		if (gdk_drawable_get_depth (pGdkDrawable) == 32)
			pColormap = gdk_screen_get_rgba_colormap (pScreen);
		else
			pColormap = gdk_screen_get_rgb_colormap (pScreen);  // au pire on a un colormap nul.
		//cd_debug ("  pColormap : %x  (pScreen:%x)", pColormap, pScreen);
	}

	//\__________________ On recupere le buffer dans un GdkPixbuf.
	GdkPixbuf *pIconPixbuf = gdk_pixbuf_get_from_drawable (NULL,
		pGdkDrawable,
		pColormap,
		0,
		0,
		0,
		0,
		iWidth,
		iHeight);
	g_object_unref (G_OBJECT (pGdkDrawable));
	#else
	cairo_surface_t *surface = cairo_xlib_surface_create (s_XDisplay,
		XPixmapID,
		DefaultVisual(s_XDisplay, 0),
		iWidth,
		iHeight);
	GdkPixbuf *pIconPixbuf = gdk_pixbuf_get_from_surface(surface,
		0,
		0,
		iWidth,
		iHeight);
	cairo_surface_destroy(surface);
	#endif
	g_return_val_if_fail (pIconPixbuf != NULL, NULL);

	//\__________________ On lui ajoute un canal alpha si necessaire.
	if (! gdk_pixbuf_get_has_alpha (pIconPixbuf) && bAddAlpha)
	{
		cd_debug ("  on lui ajoute de la transparence");
		GdkPixbuf *tmp_pixbuf = gdk_pixbuf_add_alpha (pIconPixbuf, FALSE, 255, 255, 255);
		g_object_unref (pIconPixbuf);
		pIconPixbuf = tmp_pixbuf;
	}
	return pIconPixbuf;
}


void cairo_dock_set_nb_viewports (int iNbViewportX, int iNbViewportY)
{
	XEvent xClientMessage;
	Window root = DefaultRootWindow (s_XDisplay);
	
	xClientMessage.xclient.type = ClientMessage;
	xClientMessage.xclient.serial = 0;
	xClientMessage.xclient.send_event = True;
	xClientMessage.xclient.display = s_XDisplay;
	xClientMessage.xclient.window = root;
	xClientMessage.xclient.message_type = s_aNetDesktopGeometry;
	xClientMessage.xclient.format = 32;
	xClientMessage.xclient.data.l[0] = iNbViewportX * g_desktopGeometry.iXScreenWidth[CAIRO_DOCK_HORIZONTAL];
	xClientMessage.xclient.data.l[1] = iNbViewportY * g_desktopGeometry.iXScreenHeight[CAIRO_DOCK_HORIZONTAL];
	xClientMessage.xclient.data.l[2] = 0;
	xClientMessage.xclient.data.l[3] = 2;
	xClientMessage.xclient.data.l[4] = 0;

	XSendEvent (s_XDisplay,
		root,
		False,
		SubstructureRedirectMask | SubstructureNotifyMask,
		&xClientMessage);
}

void cairo_dock_set_nb_desktops (gulong iNbDesktops)
{
	XEvent xClientMessage;
	Window root = DefaultRootWindow (s_XDisplay);
	
	xClientMessage.xclient.type = ClientMessage;
	xClientMessage.xclient.serial = 0;
	xClientMessage.xclient.send_event = True;
	xClientMessage.xclient.display = s_XDisplay;
	xClientMessage.xclient.window = root;
	xClientMessage.xclient.message_type = s_aNetNbDesktops;
	xClientMessage.xclient.format = 32;
	xClientMessage.xclient.data.l[0] = iNbDesktops;
	xClientMessage.xclient.data.l[1] = 0;
	xClientMessage.xclient.data.l[2] = 0;
	xClientMessage.xclient.data.l[3] = 2;
	xClientMessage.xclient.data.l[4] = 0;

	XSendEvent (s_XDisplay,
		root,
		False,
		SubstructureRedirectMask | SubstructureNotifyMask,
		&xClientMessage);
}


gboolean cairo_dock_support_X_extension (void)
{
#ifdef HAVE_XEXTEND
	int event_base, error_base, major, minor;
	if (! XCompositeQueryExtension (s_XDisplay, &event_base, &error_base))  // on regarde si le serveur X supporte l'extension.
	{
		cd_warning ("XComposite extension not available.");
		s_bUseXComposite = FALSE;
	}
	else
	{
		major = 0, minor = 2;  // La version minimale requise pour avoir XCompositeNameWindowPixmap().
		XCompositeQueryVersion (s_XDisplay, &major, &minor);  // on regarde si on est au moins dans cette version.
		if (! (major > 0 || minor >= 2))
		{
			cd_warning ("XComposite extension too old.");
			s_bUseXComposite = FALSE;
		}
	}
	/*int iDamageError=0;
	if (! XDamageQueryExtension (s_XDisplay, &g_iDamageEvent, &iDamageError))
	{
		cd_warning ("XDamage extension not supported");
		return FALSE;
	}*/
	
	major = 0, minor = 0;
	if (! XTestQueryExtension (s_XDisplay, &event_base, &error_base, &major, &minor))
	{
		cd_warning ("XTest extension not available.");
		s_bUseXTest = FALSE;
	}
	
	if (! XineramaQueryExtension (s_XDisplay, &event_base, &error_base))
	{
		cd_warning ("Xinerama extension not supported");
		s_bUseXinerama = FALSE;
	}
	else
	{
		major = 0, minor = 0;
		if (XineramaQueryVersion (s_XDisplay, &major, &minor) == 0)
		{
			cd_warning ("Xinerama extension too old");
			s_bUseXinerama = FALSE;
		}
	}
	
	return TRUE;
#else
	cd_warning ("The dock was not compiled with the X extensions (XComposite, xinerama, xtst, XDamage, etc).");
	s_bUseXComposite = FALSE;
	s_bUseXTest = FALSE;
	s_bUseXinerama = FALSE;
	return FALSE;
#endif
}

gboolean cairo_dock_xcomposite_is_available (void)
{
	return s_bUseXComposite;
}

gboolean cairo_dock_xtest_is_available (void)
{
	return s_bUseXTest;
}

gboolean cairo_dock_xinerama_is_available (void)
{
	return s_bUseXinerama;
}


void cairo_dock_get_screen_offsets (int iNumScreen, int *iScreenOffsetX, int *iScreenOffsetY)
{
#ifdef HAVE_XEXTEND
	g_return_if_fail (s_bUseXinerama);
	int iNbScreens = 0;
	XineramaScreenInfo *pScreens = XineramaQueryScreens (s_XDisplay, &iNbScreens);
	if (pScreens != NULL)
	{
		if (iNumScreen >= iNbScreens)
		{
			cd_warning ("the number of screen where to place the dock is too big, we'll choose the last one.");
			iNumScreen = iNbScreens - 1;
		}
		*iScreenOffsetX = pScreens[iNumScreen].x_org;
		*iScreenOffsetY = pScreens[iNumScreen].y_org;
		g_desktopGeometry.iScreenWidth[CAIRO_DOCK_HORIZONTAL] = pScreens[iNumScreen].width;
		g_desktopGeometry.iScreenHeight[CAIRO_DOCK_HORIZONTAL] = pScreens[iNumScreen].height;
		
		g_desktopGeometry.iScreenWidth[CAIRO_DOCK_VERTICAL] = g_desktopGeometry.iScreenHeight[CAIRO_DOCK_HORIZONTAL];
		g_desktopGeometry.iScreenHeight[CAIRO_DOCK_VERTICAL] = g_desktopGeometry.iScreenWidth[CAIRO_DOCK_HORIZONTAL];
		cd_message (" * screen %d => (%d;%d) %dx%d\n", iNumScreen, *iScreenOffsetX, *iScreenOffsetY, g_desktopGeometry.iScreenWidth[CAIRO_DOCK_HORIZONTAL], g_desktopGeometry.iScreenHeight[CAIRO_DOCK_HORIZONTAL]);
		
		XFree (pScreens);
	}
	else
	{
		cd_warning ("No screen found from Xinerama, is it really active ?");
		*iScreenOffsetX = *iScreenOffsetY = 0;
		g_desktopGeometry.iScreenWidth[CAIRO_DOCK_HORIZONTAL] = g_desktopGeometry.iXScreenWidth[CAIRO_DOCK_HORIZONTAL];
		g_desktopGeometry.iScreenHeight[CAIRO_DOCK_HORIZONTAL] = g_desktopGeometry.iXScreenHeight[CAIRO_DOCK_HORIZONTAL];
		g_desktopGeometry.iScreenWidth[CAIRO_DOCK_VERTICAL] = g_desktopGeometry.iScreenHeight[CAIRO_DOCK_HORIZONTAL];
		g_desktopGeometry.iScreenHeight[CAIRO_DOCK_VERTICAL] = g_desktopGeometry.iScreenWidth[CAIRO_DOCK_HORIZONTAL];
	}
#else
	cd_warning ("The dock was not compiled with the support of Xinerama.");
#endif
}






void cairo_dock_set_xwindow_timestamp (Window Xid, gulong iTimeStamp)
{
	g_return_if_fail (Xid > 0);
	Atom aNetWmUserTime = XInternAtom (s_XDisplay, "_NET_WM_USER_TIME", False);
	XChangeProperty (s_XDisplay,
		Xid,
		aNetWmUserTime,
		XA_CARDINAL, 32, PropModeReplace,
		(guchar *)&iTimeStamp, 1);
}

void cairo_dock_set_strut_partial (int Xid, int left, int right, int top, int bottom, int left_start_y, int left_end_y, int right_start_y, int right_end_y, int top_start_x, int top_end_x, int bottom_start_x, int bottom_end_x)
{
	g_return_if_fail (Xid > 0);

	cd_debug ("%s (%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d", __func__, left, right, top, bottom, left_start_y, left_end_y, right_start_y, right_end_y, top_start_x, top_end_x, bottom_start_x, bottom_end_x);
	gulong iGeometryStrut[12] = {left, right, top, bottom, left_start_y, left_end_y, right_start_y, right_end_y, top_start_x, top_end_x, bottom_start_x, bottom_end_x};

	XChangeProperty (s_XDisplay,
		Xid,
		XInternAtom (s_XDisplay, "_NET_WM_STRUT_PARTIAL", False),
		XA_CARDINAL, 32, PropModeReplace,
		(guchar *) iGeometryStrut, 12);
	
	Window root = DefaultRootWindow (s_XDisplay);
	cairo_dock_set_xwindow_timestamp (Xid, cairo_dock_get_xwindow_timestamp (root));
}

void cairo_dock_set_xwindow_mask (Window Xid, long iMask)
{
	//StructureNotifyMask | /*ResizeRedirectMask*/
	//SubstructureRedirectMask |
	//SubstructureNotifyMask |  // place sur le root, donne les evenements Map, Unmap, Destroy, Create de chaque fenetre.
	//PropertyChangeMask
	XSelectInput (s_XDisplay, Xid, iMask);  // c'est le 'event_mask' d'un XSetWindowAttributes.
}

void cairo_dock_set_xwindow_type_hint (int Xid, const gchar *cWindowTypeName)
{
	g_return_if_fail (Xid > 0);
	
	gulong iWindowType = XInternAtom (s_XDisplay, cWindowTypeName, False);
	cd_debug ("%s (%d, %s=%d)", __func__, Xid, cWindowTypeName, iWindowType);
	
	XChangeProperty (s_XDisplay,
		Xid,
		s_aNetWmWindowType,
		XA_ATOM, 32, PropModeReplace,
		(guchar *) &iWindowType, 1);
}


void cairo_dock_set_xicon_geometry (int Xid, int iX, int iY, int iWidth, int iHeight)
{
	g_return_if_fail (Xid > 0);

	gulong iIconGeometry[4] = {iX, iY, iWidth, iHeight};
	
	if (iWidth == 0 || iHeight == 0)
		XDeleteProperty (s_XDisplay,
			Xid,
			s_aNetWmIconGeometry);
	else
		XChangeProperty (s_XDisplay,
			Xid,
			s_aNetWmIconGeometry,
			XA_CARDINAL, 32, PropModeReplace,
			(guchar *) iIconGeometry, 4);
}



void cairo_dock_close_xwindow (Window Xid)
{
	//g_print ("%s (%d)\n", __func__, Xid);
	g_return_if_fail (Xid > 0);
	
	XEvent xClientMessage;
	
	xClientMessage.xclient.type = ClientMessage;
	xClientMessage.xclient.serial = 0;
	xClientMessage.xclient.send_event = True;
	xClientMessage.xclient.display = s_XDisplay;
	xClientMessage.xclient.window = Xid;
	xClientMessage.xclient.message_type = XInternAtom (s_XDisplay, "_NET_CLOSE_WINDOW", False);
	xClientMessage.xclient.format = 32;
	xClientMessage.xclient.data.l[0] = cairo_dock_get_xwindow_timestamp (Xid);  // timestamp
	xClientMessage.xclient.data.l[1] = 2;  // 2 <=> pagers and other Clients that represent direct user actions.
	xClientMessage.xclient.data.l[2] = 0;
	xClientMessage.xclient.data.l[3] = 0;
	xClientMessage.xclient.data.l[4] = 0;

	Window root = DefaultRootWindow (s_XDisplay);
	XSendEvent (s_XDisplay,
		root,
		False,
		SubstructureRedirectMask | SubstructureNotifyMask,
		&xClientMessage);
	//cairo_dock_set_xwindow_timestamp (Xid, cairo_dock_get_xwindow_timestamp (root));
}

void cairo_dock_kill_xwindow (Window Xid)
{
	g_return_if_fail (Xid > 0);
	XKillClient (s_XDisplay, Xid);
}

void cairo_dock_show_xwindow (Window Xid)
{
	g_return_if_fail (Xid > 0);
	XEvent xClientMessage;
	Window root = DefaultRootWindow (s_XDisplay);

	//\______________ On se deplace sur le bureau de la fenetre a afficher (autrement Metacity deplacera la fenetre sur le bureau actuel).
	int iDesktopNumber = cairo_dock_get_xwindow_desktop (Xid);
	cairo_dock_set_current_desktop (iDesktopNumber);

	//\______________ On active la fenetre.
	//XMapRaised (s_XDisplay, Xid);  // on la mappe, pour les cas ou elle etait en zone de notification. Malheuresement, la zone de notif de gnome est bugguee, et reduit la fenetre aussitot qu'on l'a mappee :-(
	xClientMessage.xclient.type = ClientMessage;
	xClientMessage.xclient.serial = 0;
	xClientMessage.xclient.send_event = True;
	xClientMessage.xclient.display = s_XDisplay;
	xClientMessage.xclient.window = Xid;
	xClientMessage.xclient.message_type = s_aNetActiveWindow;
	xClientMessage.xclient.format = 32;
	xClientMessage.xclient.data.l[0] = 2;  // source indication
	xClientMessage.xclient.data.l[1] = 0;  // timestamp
	xClientMessage.xclient.data.l[2] = 0;  // requestor's currently active window, 0 if none
	xClientMessage.xclient.data.l[3] = 0;
	xClientMessage.xclient.data.l[4] = 0;
	
	XSendEvent (s_XDisplay,
		root,
		False,
		SubstructureRedirectMask | SubstructureNotifyMask,
		&xClientMessage);

	//cairo_dock_set_xwindow_timestamp (Xid, cairo_dock_get_xwindow_timestamp (root));
}

void cairo_dock_minimize_xwindow (Window Xid)
{
	g_return_if_fail (Xid > 0);
	XIconifyWindow (s_XDisplay, Xid, DefaultScreen (s_XDisplay));
	//Window root = DefaultRootWindow (s_XDisplay);
	//cairo_dock_set_xwindow_timestamp (Xid, cairo_dock_get_xwindow_timestamp (root));
}

void cairo_dock_lower_xwindow (Window Xid)
{
	g_return_if_fail (Xid > 0);
	XLowerWindow (s_XDisplay, Xid);
}



static void _cairo_dock_change_window_state (Window Xid, gulong iNewValue, Atom iProperty1, Atom iProperty2)
{
	g_return_if_fail (Xid > 0);
	XEvent xClientMessage;

	xClientMessage.xclient.type = ClientMessage;
	xClientMessage.xclient.serial = 0;
	xClientMessage.xclient.send_event = True;
	xClientMessage.xclient.display = s_XDisplay;
	xClientMessage.xclient.window = Xid;
	xClientMessage.xclient.message_type = s_aNetWmState;
	xClientMessage.xclient.format = 32;
	xClientMessage.xclient.data.l[0] = iNewValue;
	xClientMessage.xclient.data.l[1] = iProperty1;
	xClientMessage.xclient.data.l[2] = iProperty2;
	xClientMessage.xclient.data.l[3] = 2;
	xClientMessage.xclient.data.l[4] = 0;

	Window root = DefaultRootWindow (s_XDisplay);
	XSendEvent (s_XDisplay,
		root,
		False,
		SubstructureRedirectMask | SubstructureNotifyMask,
		&xClientMessage);

	cairo_dock_set_xwindow_timestamp (Xid, cairo_dock_get_xwindow_timestamp (root));
}
void cairo_dock_maximize_xwindow (Window Xid, gboolean bMaximize)
{
	_cairo_dock_change_window_state (Xid, bMaximize, s_aNetWmMaximizedVert, s_aNetWmMaximizedHoriz);
}

void cairo_dock_set_xwindow_fullscreen (Window Xid, gboolean bFullScreen)
{
	_cairo_dock_change_window_state (Xid, bFullScreen, s_aNetWmFullScreen, 0);
}

void cairo_dock_set_xwindow_above (Window Xid, gboolean bAbove)
{
	_cairo_dock_change_window_state (Xid, bAbove, s_aNetWmAbove, 0);
}


void cairo_dock_move_xwindow_to_absolute_position (Window Xid, int iDesktopNumber, int iPositionX, int iPositionY)  // dans le referentiel du viewport courant.
{
	g_return_if_fail (Xid > 0);
	XEvent xClientMessage;

	xClientMessage.xclient.type = ClientMessage;
	xClientMessage.xclient.serial = 0;
	xClientMessage.xclient.send_event = True;
	xClientMessage.xclient.display = s_XDisplay;
	xClientMessage.xclient.window = Xid;
	xClientMessage.xclient.message_type = XInternAtom (s_XDisplay, "_NET_WM_DESKTOP", False);
	xClientMessage.xclient.format = 32;
	xClientMessage.xclient.data.l[0] = iDesktopNumber;
	xClientMessage.xclient.data.l[1] = 2;
	xClientMessage.xclient.data.l[2] = 0;
	xClientMessage.xclient.data.l[3] = 0;
	xClientMessage.xclient.data.l[4] = 0;

	Window root = DefaultRootWindow (s_XDisplay);
	XSendEvent (s_XDisplay,
		root,
		False,
		SubstructureRedirectMask | SubstructureNotifyMask,
		&xClientMessage);
	
	xClientMessage.xclient.type = ClientMessage;
	xClientMessage.xclient.serial = 0;
	xClientMessage.xclient.send_event = True;
	xClientMessage.xclient.display = s_XDisplay;
	xClientMessage.xclient.window = Xid;
	xClientMessage.xclient.message_type = XInternAtom (s_XDisplay, "_NET_MOVERESIZE_WINDOW", False);
	xClientMessage.xclient.format = 32;
	xClientMessage.xclient.data.l[0] = StaticGravity | (1 << 8) | (1 << 9) | (0 << 10) | (0 << 11);
	xClientMessage.xclient.data.l[1] =  iPositionX;  // coordonnees dans le referentiel du viewport desire.
	xClientMessage.xclient.data.l[2] =  iPositionY;
	xClientMessage.xclient.data.l[3] = 0;
	xClientMessage.xclient.data.l[4] = 0;
	XSendEvent (s_XDisplay,
		root,
		False,
		SubstructureRedirectMask | SubstructureNotifyMask,
		&xClientMessage);

	//cairo_dock_set_xwindow_timestamp (Xid, cairo_dock_get_xwindow_timestamp (root));
}

void cairo_dock_move_xwindow_to_nth_desktop (Window Xid, int iDesktopNumber, int iDesktopViewportX, int iDesktopViewportY)
{
	g_return_if_fail (Xid > 0);
	int iRelativePositionX, iRelativePositionY;
	cairo_dock_get_xwindow_position_on_its_viewport (Xid, &iRelativePositionX, &iRelativePositionY);
	
	cairo_dock_move_xwindow_to_absolute_position (Xid, iDesktopNumber, iDesktopViewportX + iRelativePositionX, iDesktopViewportY + iRelativePositionY);
}



gulong cairo_dock_get_xwindow_timestamp (Window Xid)
{
	g_return_val_if_fail (Xid > 0, 0);
	Atom aNetWmUserTime = XInternAtom (s_XDisplay, "_NET_WM_USER_TIME", False);
	gulong iLeftBytes, iBufferNbElements = 0;
	Atom aReturnedType = 0;
	int aReturnedFormat = 0;
	gulong *pTimeBuffer = NULL;
	XGetWindowProperty (s_XDisplay, Xid, aNetWmUserTime, 0, G_MAXULONG, False, XA_CARDINAL, &aReturnedType, &aReturnedFormat, &iBufferNbElements, &iLeftBytes, (guchar **)&pTimeBuffer);
	gulong iTimeStamp = 0;
	if (iBufferNbElements > 0)
		iTimeStamp = *pTimeBuffer;
	XFree (pTimeBuffer);
	return iTimeStamp;
}

gchar *cairo_dock_get_xwindow_name (Window Xid, gboolean bSearchWmName)
{
	Atom aReturnedType = 0;
	int aReturnedFormat = 0;
	unsigned long iLeftBytes, iBufferNbElements=0;
	guchar *pNameBuffer = NULL;
	XGetWindowProperty (s_XDisplay, Xid, s_aNetWmName, 0, G_MAXULONG, False, s_aUtf8String, &aReturnedType, &aReturnedFormat, &iBufferNbElements, &iLeftBytes, &pNameBuffer);  // on cherche en priorite le nom en UTF8, car on est notifie des 2, mais il vaut mieux eviter le WM_NAME qui, ne l'etant pas, contient des caracteres bizarres qu'on ne peut pas convertir avec g_locale_to_utf8, puisque notre locale _est_ UTF8.
	if (iBufferNbElements == 0 && bSearchWmName)
		XGetWindowProperty (s_XDisplay, Xid, s_aWmName, 0, G_MAXULONG, False, s_aString, &aReturnedType, &aReturnedFormat, &iBufferNbElements, &iLeftBytes, &pNameBuffer);
	
	gchar *cName = NULL;
	if (iBufferNbElements > 0)
	{
		cName = g_strdup (pNameBuffer);
		XFree (pNameBuffer);
	}
	return cName;
}

gboolean cairo_dock_remove_version_from_string (gchar *cString)
{
	if (cString == NULL)
		return FALSE;
	int n = strlen (cString);
	gchar *str = cString + n - 1;
	do
	{
		if (g_ascii_isdigit(*str) || *str == '.')
		{
			str --;
			continue;
		}
		if (*str == '-' || *str == ' ')  // 'Glade-2', 'OpenOffice 3.1'
		{
			*str = '\0';
			return TRUE;
		}
		else
			return FALSE;
	}
	while (str != cString);
	return FALSE;
}

gchar *cairo_dock_get_xwindow_class (Window Xid, gchar **cWMClass)
{
	XClassHint *pClassHint = XAllocClassHint ();
	gchar *cClass = NULL, *cWmClass = NULL;
	if (XGetClassHint (s_XDisplay, Xid, pClassHint) != 0 && pClassHint->res_class)
	{
		cWmClass = g_strdup (pClassHint->res_class);
		
		cd_debug ("  res_name : %s(%x); res_class : %s(%x)", pClassHint->res_name, pClassHint->res_name, pClassHint->res_class, pClassHint->res_class);
		if (strcmp (pClassHint->res_class, "Wine") == 0 && pClassHint->res_name && g_str_has_suffix (pClassHint->res_name, ".exe"))
		{
			cd_debug ("  wine application detected, changing the class '%s' to '%s'", pClassHint->res_class, pClassHint->res_name);
			cClass = g_ascii_strdown (pClassHint->res_name, -1);
		}
		else if (*pClassHint->res_class == '/' && g_str_has_suffix (pClassHint->res_class, ".exe"))  // cas des applications Mono telles que tomboy ...
		{
			gchar *str = strrchr (pClassHint->res_class, '/');
			if (str)
				str ++;
			else
				str = pClassHint->res_class;
			cClass = g_ascii_strdown (str, -1);
			cClass[strlen (cClass) - 4] = '\0';
		}
		else
		{
			cClass = g_ascii_strdown (pClassHint->res_class, -1);  // on la passe en minuscule, car certaines applis ont la bonne idee de donner des classes avec une majuscule ou non suivant les fenetres.
		}
		
		cairo_dock_remove_version_from_string (cClass);  // on enleve les numeros de version (Openoffice.org-3.1)
		
		gchar *str = strchr (cClass, '.');  // on vire les .xxx, sinon on ne sait pas detecter l'absence d'extension quand on cherche l'icone (openoffice.org), ou tout simplement ca empeche de trouver l'icone (jbrout.py).
		if (str != NULL)
			*str = '\0';
		cd_debug ("got an application with class '%s'", cClass);
		
		XFree (pClassHint->res_name);
		XFree (pClassHint->res_class);
		XFree (pClassHint);
	}
	*cWMClass = cWmClass;
	return cClass;
}

gboolean cairo_dock_xwindow_is_maximized (Window Xid)
{
	g_return_val_if_fail (Xid > 0, FALSE);

	Atom aReturnedType = 0;
	int aReturnedFormat = 0;
	unsigned long iLeftBytes, iBufferNbElements = 0;
	gulong *pXStateBuffer = NULL;
	XGetWindowProperty (s_XDisplay, Xid, s_aNetWmState, 0, G_MAXULONG, False, XA_ATOM, &aReturnedType, &aReturnedFormat, &iBufferNbElements, &iLeftBytes, (guchar **)&pXStateBuffer);
	int iIsMaximized = 0;
	if (iBufferNbElements > 0)
	{
		guint i;
		for (i = 0; i < iBufferNbElements && iIsMaximized < 2; i ++)
		{
			if (pXStateBuffer[i] == s_aNetWmMaximizedVert)
				iIsMaximized ++;
			if (pXStateBuffer[i] == s_aNetWmMaximizedHoriz)
				iIsMaximized ++;
		}
	}
	XFree (pXStateBuffer);

	return (iIsMaximized == 2);
}

static gboolean _cairo_dock_window_is_in_state (Window Xid, Atom iState)
{
	g_return_val_if_fail (Xid > 0, FALSE);
	Atom aReturnedType = 0;
	int aReturnedFormat = 0;
	unsigned long iLeftBytes, iBufferNbElements = 0;
	gulong *pXStateBuffer = NULL;
	XGetWindowProperty (s_XDisplay, Xid, s_aNetWmState, 0, G_MAXULONG, False, XA_ATOM, &aReturnedType, &aReturnedFormat, &iBufferNbElements, &iLeftBytes, (guchar **)&pXStateBuffer);

	gboolean bIsInState = FALSE;
	if (iBufferNbElements > 0)
	{
		guint i;
		for (i = 0; i < iBufferNbElements; i ++)
		{
			if (pXStateBuffer[i] == iState)
			{
				bIsInState = TRUE;
				break;
			}
		}
	}

	XFree (pXStateBuffer);
	return bIsInState;
}

gboolean cairo_dock_xwindow_is_fullscreen (Window Xid)
{
	return _cairo_dock_window_is_in_state (Xid, s_aNetWmFullScreen);
}
gboolean cairo_dock_xwindow_skip_taskbar (Window Xid)
{
	return _cairo_dock_window_is_in_state (Xid, s_aNetWmSkipTaskbar);
}
void cairo_dock_xwindow_is_above_or_below (Window Xid, gboolean *bIsAbove, gboolean *bIsBelow)
{
	g_return_if_fail (Xid > 0);
	Atom aReturnedType = 0;
	int aReturnedFormat = 0;
	unsigned long iLeftBytes, iBufferNbElements = 0;
	gulong *pXStateBuffer = NULL;
	XGetWindowProperty (s_XDisplay, Xid, s_aNetWmState, 0, G_MAXULONG, False, XA_ATOM, &aReturnedType, &aReturnedFormat, &iBufferNbElements, &iLeftBytes, (guchar **)&pXStateBuffer);

	if (iBufferNbElements > 0)
	{
		guint i;
		//g_print ("iBufferNbElements : %d (%d;%d)\n", iBufferNbElements, s_aNetWmAbove, s_aNetWmBelow);
		for (i = 0; i < iBufferNbElements; i ++)
		{
			//g_print (" - %d\n", pXStateBuffer[i]);
			if (pXStateBuffer[i] == s_aNetWmAbove)
			{
				*bIsAbove = TRUE;
				*bIsBelow = FALSE;
				break;
			}
			else if (pXStateBuffer[i] == s_aNetWmBelow)
			{
				*bIsAbove = FALSE;
				*bIsBelow = TRUE;
				break;
			}
		}
	}

	XFree (pXStateBuffer);
}

gboolean cairo_dock_xwindow_is_fullscreen_or_hidden_or_maximized (Window Xid, gboolean *bIsFullScreen, gboolean *bIsHidden, gboolean *bIsMaximized, gboolean *bDemandsAttention)
{
	g_return_val_if_fail (Xid > 0, FALSE);
	//cd_debug ("%s (%d)", __func__, Xid);
	Atom aReturnedType = 0;
	int aReturnedFormat = 0;
	unsigned long iLeftBytes, iBufferNbElements = 0;
	gulong *pXStateBuffer = NULL;
	XGetWindowProperty (s_XDisplay, Xid, s_aNetWmState, 0, G_MAXULONG, False, XA_ATOM, &aReturnedType, &aReturnedFormat, &iBufferNbElements, &iLeftBytes, (guchar **)&pXStateBuffer);
	
	gboolean bValid = TRUE;
	*bIsFullScreen = FALSE;
	*bIsHidden = FALSE;
	*bIsMaximized = FALSE;
	if (bDemandsAttention != NULL)
		*bDemandsAttention = FALSE;
	if (iBufferNbElements > 0)
	{
		guint i, iNbMaximizedDimensions = 0;
		for (i = 0; i < iBufferNbElements; i ++)
		{
			if (pXStateBuffer[i] == s_aNetWmFullScreen)
			{
				*bIsFullScreen = TRUE;
			}
			else if (pXStateBuffer[i] == s_aNetWmHidden)
			{
				*bIsHidden = TRUE;
			}
			else if (pXStateBuffer[i] == s_aNetWmMaximizedVert)
			{
				iNbMaximizedDimensions ++;
				if (iNbMaximizedDimensions == 2)
					*bIsMaximized = TRUE;
			}
			else if (pXStateBuffer[i] == s_aNetWmMaximizedHoriz)
			{
				iNbMaximizedDimensions ++;
				if (iNbMaximizedDimensions == 2)
					*bIsMaximized = TRUE;
			}
			else if (pXStateBuffer[i] == s_aNetWmDemandsAttention && bDemandsAttention != NULL)
			{
				*bDemandsAttention = TRUE;
			}
			
			else if (pXStateBuffer[i] == s_aNetWmSkipTaskbar)
			{
				cd_debug ("this appli should not be in taskbar anymore");
				bValid = FALSE;
			}
		}
	}
	
	XFree (pXStateBuffer);
	return bValid;
}  // Note: for stickyness, dont use _NET_WM_STATE_STICKY; prefer "cairo_dock_get_xwindow_desktop (Xid) == -1"


static inline gboolean _cairo_dock_window_has_type (int Xid, Atom iType)
{
	g_return_val_if_fail (Xid > 0, FALSE);
	
	gboolean bIsType;
	Atom aReturnedType = 0;
	int aReturnedFormat = 0;
	unsigned long iLeftBytes, iBufferNbElements;
	gulong *pTypeBuffer = NULL;
	XGetWindowProperty (s_XDisplay, Xid, s_aNetWmWindowType, 0, G_MAXULONG, False, XA_ATOM, &aReturnedType, &aReturnedFormat, &iBufferNbElements, &iLeftBytes, (guchar **)&pTypeBuffer);
	if (iBufferNbElements != 0)
	{
		bIsType = (*pTypeBuffer == iType);
		XFree (pTypeBuffer);
	}
	else
		bIsType = FALSE;
	return bIsType;
}
gboolean cairo_dock_window_is_utility (int Xid)
{
	return _cairo_dock_window_has_type (Xid, s_aNetWmWindowTypeUtility);
}

gboolean cairo_dock_window_is_dock (int Xid)
{
	return _cairo_dock_window_has_type (Xid, s_aNetWmWindowTypeDock);
}

int cairo_dock_get_xwindow_desktop (Window Xid)
{
	int iDesktopNumber;
	gulong iLeftBytes, iBufferNbElements = 0;
	Atom aReturnedType = 0;
	int aReturnedFormat = 0;
	gulong *pBuffer = NULL;
	XGetWindowProperty (s_XDisplay, Xid, s_aNetWmDesktop, 0, G_MAXULONG, False, XA_CARDINAL, &aReturnedType, &aReturnedFormat, &iBufferNbElements, &iLeftBytes, (guchar **)&pBuffer);
	if (iBufferNbElements > 0)
		iDesktopNumber = *pBuffer;
	else
		iDesktopNumber = 0;
	
	XFree (pBuffer);
	return iDesktopNumber;
}

void cairo_dock_get_xwindow_geometry (Window Xid, int *iLocalPositionX, int *iLocalPositionY, int *iWidthExtent, int *iHeightExtent)  // renvoie les coordonnees du coin haut gauche dans le referentiel du viewport actuel. // sous KDE, x et y sont toujours nuls ! (meme avec XGetWindowAttributes).
{
	// get the geometry from X.
	Window root_return;
	int x_return=1, y_return=1;
	unsigned int width_return, height_return, border_width_return, depth_return;
	XGetGeometry (s_XDisplay, Xid,
		&root_return,
		&x_return, &y_return,
		&width_return, &height_return,
		&border_width_return, &depth_return);  // renvoie les coordonnees du coin haut gauche dans le referentiel du viewport actuel.
	
	// make another round trip to the server to query the coordinates of the window relatively to the root window (which basically gives us the (x,y) of the window); we need to do this to workaround a strange X bug: x_return and y_return are wrong (0,0, modulo the borders) (on Ubuntu 11.10 + Compiz 0.9/Metacity, not on Debian 6 + Compiz 0.8).
	int dest_x_return, dest_y_return;
	Window child_return;
	XTranslateCoordinates (s_XDisplay, Xid, root_return, 0, 0, &dest_x_return, &dest_y_return, &child_return);  // translate into the coordinate space of the root window. we need to do this, because (x_return,;y_return) is always (0;0)
	//g_print (" %d;%d %dx%d\n", x_return, y_return, width_return, height_return);
	//g_print (" -> %d;%d\n", dest_x_return, dest_y_return);
	
	// take into account the window borders
	int left=0, right=0, top=0, bottom=0;
	gulong iLeftBytes, iBufferNbElements = 0;
	Atom aReturnedType = 0;
	int aReturnedFormat = 0;
	gulong *pBuffer = NULL;
	XGetWindowProperty (s_XDisplay, Xid, XInternAtom (s_XDisplay, "_NET_FRAME_EXTENTS", False), 0, G_MAXULONG, False, XA_CARDINAL, &aReturnedType, &aReturnedFormat, &iBufferNbElements, &iLeftBytes, (guchar **)&pBuffer);
	if (iBufferNbElements > 3)
	{
		left=pBuffer[0], right=pBuffer[1], top=pBuffer[2], bottom=pBuffer[3];
	}
	if (pBuffer)
		XFree (pBuffer);
	
	*iLocalPositionX = dest_x_return - left;
	*iLocalPositionY = dest_y_return - top;
	*iWidthExtent = width_return + left + right;  // unfortunately border_width_return is always 0, so we can't use it here :-/
	*iHeightExtent = height_return + top + bottom;
}

void cairo_dock_get_xwindow_position_on_its_viewport (Window Xid, int *iRelativePositionX, int *iRelativePositionY)
{
	int iLocalPositionX, iLocalPositionY, iWidthExtent, iHeightExtent;
	cairo_dock_get_xwindow_geometry (Xid, &iLocalPositionX, &iLocalPositionY, &iWidthExtent, &iHeightExtent);
	
	while (iLocalPositionX < 0)  // on passe au referentiel du viewport de la fenetre; inutile de connaitre sa position, puisqu'ils ont tous la meme taille.
		iLocalPositionX += g_desktopGeometry.iXScreenWidth[CAIRO_DOCK_HORIZONTAL];
	while (iLocalPositionX >= g_desktopGeometry.iXScreenWidth[CAIRO_DOCK_HORIZONTAL])
		iLocalPositionX -= g_desktopGeometry.iXScreenWidth[CAIRO_DOCK_HORIZONTAL];
	while (iLocalPositionY < 0)
		iLocalPositionY += g_desktopGeometry.iXScreenHeight[CAIRO_DOCK_HORIZONTAL];
	while (iLocalPositionY >= g_desktopGeometry.iXScreenHeight[CAIRO_DOCK_HORIZONTAL])
		iLocalPositionY -= g_desktopGeometry.iXScreenHeight[CAIRO_DOCK_HORIZONTAL];
	
	*iRelativePositionX = iLocalPositionX;
	*iRelativePositionY = iLocalPositionY;
	//cd_debug ("position relative : (%d;%d) taille : %dx%d", *iRelativePositionX, *iRelativePositionY, iWidthExtent, iHeightExtent);
}

/**gboolean cairo_dock_xwindow_is_on_current_desktop (Window Xid)
{
	int iWindowDesktopNumber, iLocalPositionX, iLocalPositionY, iWidthExtent, iHeightExtent;  // coordonnees du coin haut gauche dans le referentiel du viewport actuel.
	iWindowDesktopNumber = cairo_dock_get_xwindow_desktop (Xid);
	cairo_dock_get_xwindow_geometry (Xid, &iLocalPositionX, &iLocalPositionY, &iWidthExtent, &iHeightExtent);

	//cd_debug (" -> %d/%d ; (%d ; %d)", iWindowDesktopNumber, iDesktopNumber, iGlobalPositionX, iGlobalPositionY);
	return ( (iWindowDesktopNumber == g_desktopGeometry.iCurrentDesktop || iWindowDesktopNumber == -1) &&
		iLocalPositionX + iWidthExtent > 0 &&
		iLocalPositionX < g_desktopGeometry.iXScreenWidth[CAIRO_DOCK_HORIZONTAL] &&
		iLocalPositionY + iHeightExtent > 0 &&
		iLocalPositionY < g_desktopGeometry.iXScreenHeight[CAIRO_DOCK_HORIZONTAL] );  // -1 <=> 0xFFFFFFFF en unsigned.
}*/


Window *cairo_dock_get_windows_list (gulong *iNbWindows, gboolean bStackOrder)
{
	Atom aReturnedType = 0;
	int aReturnedFormat = 0;
	Window *XidList = NULL;

	Window root = DefaultRootWindow (s_XDisplay);
	gulong iLeftBytes;
	XGetWindowProperty (s_XDisplay, root, (bStackOrder ? s_aNetClientListStacking : s_aNetClientList), 0, G_MAXLONG, False, XA_WINDOW, &aReturnedType, &aReturnedFormat, iNbWindows, &iLeftBytes, (guchar **)&XidList);
	return XidList;
}

Window cairo_dock_get_active_xwindow (void)
{
	Atom aReturnedType = 0;
	int aReturnedFormat = 0;
	unsigned long iLeftBytes, iBufferNbElements = 0;
	Window *pXBuffer = NULL;
	Window root = DefaultRootWindow (s_XDisplay);
	XGetWindowProperty (s_XDisplay, root, s_aNetActiveWindow, 0, G_MAXULONG, False, XA_WINDOW, &aReturnedType, &aReturnedFormat, &iBufferNbElements, &iLeftBytes, (guchar **)&pXBuffer);

	Window xActiveWindow = (iBufferNbElements > 0 && pXBuffer != NULL ? pXBuffer[0] : 0);
	XFree (pXBuffer);
	return xActiveWindow;
}
