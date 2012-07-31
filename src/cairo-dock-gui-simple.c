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

#include <string.h>
#include <unistd.h>
#define __USE_XOPEN_EXTENDED
#include <stdlib.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>

#include "config.h"
#include "gldi-config.h"
#include "cairo-dock-config.h"
#include "cairo-dock-module-factory.h"
#include "cairo-dock-log.h"
#include "cairo-dock-gui-factory.h"
#include "cairo-dock-icon-facility.h"
#include "cairo-dock-keyfile-utilities.h"
#include "cairo-dock-animations.h"
#include "cairo-dock-dialog-manager.h"
#include "cairo-dock-dock-manager.h"
#include "cairo-dock-dock-factory.h"
#include "cairo-dock-launcher-factory.h"
#include "cairo-dock-desktop-file-factory.h"
#include "cairo-dock-X-manager.h"
#include "cairo-dock-gui-manager.h"
#include "cairo-dock-gui-factory.h"
#include "cairo-dock-gui-backend.h"
#include "cairo-dock-widget-items.h"
#include "cairo-dock-widget-themes.h"
#include "cairo-dock-widget-config.h"
#include "cairo-dock-widget-plugins.h"
#include "cairo-dock-widget.h"
#include "cairo-dock-gui-simple.h"

#define CAIRO_DOCK_PREVIEW_WIDTH 200
#define CAIRO_DOCK_PREVIEW_HEIGHT 250
#define CAIRO_DOCK_SIMPLE_PANEL_WIDTH 1200
#define CAIRO_DOCK_SIMPLE_PANEL_HEIGHT 700

#define CAIRO_DOCK_CATEGORY_ICON_SIZE 32  // a little bit larger than the tab icons (28px)

typedef struct {
	const gchar *cName;
	const gchar *cIcon;
	const gchar *cTooltip;
	CDWidget* (*build_widget) (void);
	GtkToolItem *pCategoryButton;
	GtkWidget *pMainWindow;
	CDWidget *pCdWidget;
} CDCategory;

typedef enum {
	CD_CATEGORY_ITEMS=0,
	CD_CATEGORY_PLUGINS,
	CD_CATEGORY_CONFIG,
	CD_CATEGORY_THEMES,
	CD_NB_CATEGORIES
} CDCategoryEnum;

static GtkWidget *s_pSimpleConfigWindow = NULL;
static CDCategoryEnum s_iCurrentCategory = 0;

extern gchar *g_cCairoDockDataDir;
extern CairoDock *g_pMainDock;
extern CairoDockDesktopGeometry g_desktopGeometry;

static CDWidget *_build_items_widget (void);
static CDWidget *_build_config_widget (void);
static CDWidget *_build_themes_widget (void);
static CDWidget *_build_plugins_widget (void);

static void cairo_dock_enable_apply_button (GtkWidget *pMainWindow, gboolean bEnable);
static void cairo_dock_select_category (GtkWidget *pMainWindow, CDCategoryEnum iCategory);

static CDCategory s_pCategories[CD_NB_CATEGORIES] = {
	{
		N_("Current items"),
		"icon-all.svg",
		N_("Current items"),
		_build_items_widget,
		NULL,
		NULL,
		NULL
	},{
		N_("Add-ons"),
		"icon-plug-ins.svg",
		N_("Add-ons"),
		_build_plugins_widget,
		NULL,
		NULL,
		NULL
	},{
		N_("Configuration"),
		"gtk-preferences",
		N_("Configuration"),
		_build_config_widget,
		NULL,
		NULL,
		NULL
	},{
		N_("Themes"),
		"icon-appearance.svg",
		N_("Themes"),
		_build_themes_widget,
		NULL,
		NULL,
		NULL
	}
};


static inline CDCategory *_get_category (CDCategoryEnum iCategory)
{
	return &s_pCategories[iCategory];
}

static inline void _set_current_category (CDCategoryEnum iCategory)
{
	s_iCurrentCategory = iCategory;
}

static inline CDCategory *_get_current_category (void)
{
	return _get_category (s_iCurrentCategory);
}

static CDWidget *_build_items_widget (void)
{
	CDWidget *pCdWidget = CD_WIDGET (cairo_dock_items_widget_new ());
	
	return pCdWidget;
}

static CDWidget *_build_config_widget (void)
{
	CDWidget *pCdWidget = CD_WIDGET (cairo_dock_config_widget_new ());
	
	return pCdWidget;
}

static CDWidget *_build_themes_widget (void)
{
	CDWidget *pCdWidget = CD_WIDGET (cairo_dock_themes_widget_new (GTK_WINDOW (s_pSimpleConfigWindow)));
	
	return pCdWidget;
}

static CDWidget *_build_plugins_widget (void)
{
	CDWidget *pCdWidget = CD_WIDGET (cairo_dock_plugins_widget_new ());
	
	return pCdWidget;
}

static void on_click_quit (GtkButton *button, GtkWidget *pMainWindow)
{
	gtk_widget_destroy (pMainWindow);
}
static void on_click_apply (GtkButton *button, GtkWidget *pMainWindow)
{
	CDCategory *pCategory = _get_current_category ();
	g_print ("%s (%s, %p -> %p)\n", __func__, pCategory->cName, pCategory, pCategory->pCdWidget);
	CDWidget *pCdWidget = pCategory->pCdWidget;
	if (pCdWidget)
		cairo_dock_widget_apply (pCdWidget);
}
GtkWidget *cairo_dock_build_generic_gui_window2 (const gchar *cTitle, int iWidth, int iHeight)
{
	//\_____________ make a new window.
	GtkWidget *pMainWindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_icon_from_file (GTK_WINDOW (pMainWindow), GLDI_SHARE_DATA_DIR"/"CAIRO_DOCK_ICON, NULL);
	if (cTitle != NULL)
		gtk_window_set_title (GTK_WINDOW (pMainWindow), cTitle);
	
	GtkWidget *pMainVBox = _gtk_vbox_new (0*CAIRO_DOCK_FRAME_MARGIN);  // all elements will be packed in a VBox
	gtk_container_add (GTK_CONTAINER (pMainWindow), pMainVBox);
	
	//\_____________ add apply/quit buttons.
	GtkWidget *pButtonsHBox = _gtk_hbox_new (CAIRO_DOCK_FRAME_MARGIN*2);
	gtk_box_pack_end (GTK_BOX (pMainVBox),
		pButtonsHBox,
		FALSE,
		FALSE,
		0);
	
	GtkWidget *pQuitButton = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
	g_signal_connect (G_OBJECT (pQuitButton), "clicked", G_CALLBACK(on_click_quit), pMainWindow);
	gtk_box_pack_end (GTK_BOX (pButtonsHBox),
		pQuitButton,
		FALSE,
		FALSE,
		0);
	
	GtkWidget *pApplyButton = gtk_button_new_from_stock (GTK_STOCK_APPLY);
	g_signal_connect (G_OBJECT (pApplyButton), "clicked", G_CALLBACK(on_click_apply), pMainWindow);
	gtk_box_pack_end (GTK_BOX (pButtonsHBox),
		pApplyButton,
		FALSE,
		FALSE,
		0);
	g_object_set_data (G_OBJECT (pMainWindow), "apply-button", pApplyButton);
	
	GtkWidget *pSwitchButton = cairo_dock_make_switch_gui_button ();
	gtk_box_pack_start (GTK_BOX (pButtonsHBox),
		pSwitchButton,
		FALSE,
		FALSE,
		0);
	
	//\_____________ add a status-bar.
	GtkWidget *pStatusBar = gtk_statusbar_new ();
	#if (GTK_MAJOR_VERSION < 3)
	gtk_statusbar_set_has_resize_grip (GTK_STATUSBAR (pStatusBar), FALSE);  // removed in GTK3 (gtk_window_set_has_resize_grip)
	#endif
	gtk_box_pack_start (GTK_BOX (pButtonsHBox),  // pMainVBox
		pStatusBar,
		FALSE,
		FALSE,
		0);
	g_object_set_data (G_OBJECT (pMainWindow), "status-bar", pStatusBar);
	
	//\_____________ resize the window (placement is done later).
	gtk_window_resize (GTK_WINDOW (pMainWindow),
		MIN (iWidth, g_desktopGeometry.iXScreenWidth[CAIRO_DOCK_HORIZONTAL]),
		MIN (iHeight, g_desktopGeometry.iXScreenHeight[CAIRO_DOCK_HORIZONTAL] - (g_pMainDock && g_pMainDock->container.bIsHorizontal ? g_pMainDock->iMaxDockHeight : 0)));
	
	gtk_widget_show_all (pMainWindow);
	
	return pMainWindow;
}


static inline GtkWidget *_make_image (const gchar *cImage, int iSize)
{
	g_return_val_if_fail (cImage != NULL, NULL);
	GtkWidget *pImage = NULL;
	if (strncmp (cImage, "gtk-", 4) == 0)
	{
		if (iSize >= 48)
			iSize = GTK_ICON_SIZE_DIALOG;
		else if (iSize >= 32)
			iSize = GTK_ICON_SIZE_LARGE_TOOLBAR;
		else
			iSize = GTK_ICON_SIZE_BUTTON;
		pImage = gtk_image_new_from_stock (cImage, iSize);
	}
	else
	{
		gchar *cIconPath = NULL;
		if (*cImage != '/')
		{
			cIconPath = g_strconcat (g_cCairoDockDataDir, "/config-panel/", cImage, NULL);
			if (!g_file_test (cIconPath, G_FILE_TEST_EXISTS))
			{
				g_free (cIconPath);
				cIconPath = g_strconcat (CAIRO_DOCK_SHARE_DATA_DIR"/icons/", cImage, NULL);
			}
		}
		GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_size (cIconPath ? cIconPath : cImage, iSize, iSize, NULL);
		g_free (cIconPath);
		if (pixbuf != NULL)
		{
			pImage = gtk_image_new_from_pixbuf (pixbuf);
			g_object_unref (pixbuf);
		}
	}
	return pImage;
}
static GtkToolItem *_make_toolbutton (const gchar *cLabel, const gchar *cImage, int iSize, GtkRadioToolButton *group)
{
	GtkToolItem *pWidget = (group ? gtk_radio_tool_button_new_from_widget (group) : gtk_radio_tool_button_new (NULL));
	gtk_tool_item_set_homogeneous (pWidget, TRUE);
	gtk_tool_item_set_expand (pWidget, TRUE);
	
	GtkWidget *hbox = _gtk_hbox_new (CAIRO_DOCK_FRAME_MARGIN);
	
	GtkWidget *pImage = _make_image (cImage, iSize);
	///gtk_tool_button_set_icon_widget (GTK_TOOL_BUTTON (pWidget), pImage);
	GtkWidget *pAlign = gtk_alignment_new (0.5, 0.5, 0., 1.);
	gtk_alignment_set_padding (GTK_ALIGNMENT (pAlign), 0, 0, CAIRO_DOCK_FRAME_MARGIN, 0);
	gtk_container_add (GTK_CONTAINER (pAlign), pImage);
	gtk_box_pack_start (GTK_BOX (hbox),
		pAlign,
		FALSE,
		FALSE,
		0);
	
	GtkWidget *pLabel = gtk_label_new (cLabel);
	pAlign = gtk_alignment_new (0.5, 0.5, 0., 1.);
	gtk_alignment_set_padding (GTK_ALIGNMENT (pAlign), 0, 0, CAIRO_DOCK_FRAME_MARGIN, 0);
	gtk_container_add (GTK_CONTAINER (pAlign), pLabel);
	gtk_box_pack_start (GTK_BOX (hbox),
		pAlign,
		FALSE,
		FALSE,
		0);
	
	gtk_tool_button_set_icon_widget (GTK_TOOL_BUTTON (pWidget), hbox);
	
	return pWidget;
}

static void _build_category_widget (CDCategory *pCategory)
{
	g_print ("%s (%s)\n", __func__, pCategory->cName);
	pCategory->pCdWidget = pCategory->build_widget ();
	gtk_widget_show_all (pCategory->pCdWidget->pWidget);
	g_print (" %p -> %p\n", pCategory, pCategory->pCdWidget);
	
	GtkWidget *pMainVBox = gtk_bin_get_child (GTK_BIN (pCategory->pMainWindow));
	gtk_box_pack_start (GTK_BOX (pMainVBox), pCategory->pCdWidget->pWidget, TRUE, TRUE, 0);
	gtk_box_reorder_child (GTK_BOX (pMainVBox), pCategory->pCdWidget->pWidget, 2);
}

static void _on_select_category (GtkButton *button, CDCategory *pCategory)
{
	if (gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (button)))  // build/show the widget
	{
		g_print ("activate %s\n", pCategory->cName);
		if (pCategory->pCdWidget == NULL)
		{
			_build_category_widget (pCategory);
		}
		gtk_widget_show (pCategory->pCdWidget->pWidget);
		cairo_dock_enable_apply_button (pCategory->pMainWindow, cairo_dock_widget_can_apply (pCategory->pCdWidget));
		_set_current_category ((GPOINTER_TO_INT (pCategory) - GPOINTER_TO_INT (s_pCategories))/sizeof(CDCategory));
	}
	else  // hide the widget
	{
		g_print ("deactivate %s\n", pCategory->cName);
		if (pCategory->pCdWidget)  // already built -> hide it
			gtk_widget_hide (pCategory->pCdWidget->pWidget);
		cairo_dock_enable_apply_button (pCategory->pMainWindow, FALSE);
	}
}

static void _on_window_destroyed (GtkWidget *pMainWindow, gpointer data)
{
	CDCategory *pCategory;
	int i;
	for (i = 0; i < CD_NB_CATEGORIES; i ++)
	{
		pCategory = _get_category (i);
		pCategory->pCategoryButton = NULL;
		pCategory->pMainWindow = NULL;
		cairo_dock_widget_free (pCategory->pCdWidget);
		pCategory->pCdWidget = NULL;
	}
	s_pSimpleConfigWindow = NULL;
}

GtkWidget *cairo_dock_build_simple_gui_window (void)
{
	if (s_pSimpleConfigWindow != NULL)
	{
		gtk_window_present (GTK_WINDOW (s_pSimpleConfigWindow));
		return s_pSimpleConfigWindow;
	}
	
	//\_____________ build a new config window
	GtkWidget *pMainWindow = cairo_dock_build_generic_gui_window2 (_("Cairo-Dock configuration"),
		CAIRO_DOCK_SIMPLE_PANEL_WIDTH, CAIRO_DOCK_SIMPLE_PANEL_HEIGHT);
	s_pSimpleConfigWindow = pMainWindow;
	g_signal_connect (G_OBJECT (pMainWindow), "destroy", G_CALLBACK(_on_window_destroyed), NULL);
	
	//\_____________ add categories
	GtkWidget *pToolBar = gtk_toolbar_new ();
	//g_object_set (G_OBJECT (pToolBar), "fill", TRUE, NULL);
	//g_object_set (G_OBJECT (pToolBar), "homogeneous", TRUE, NULL);
	gtk_toolbar_set_style (GTK_TOOLBAR (pToolBar), GTK_TOOLBAR_BOTH_HORIZ);
	gtk_toolbar_set_show_arrow (GTK_TOOLBAR (pToolBar), FALSE);
	
	GtkWidget *pMainVBox = gtk_bin_get_child (GTK_BIN (pMainWindow));
	gtk_box_pack_start (GTK_BOX (pMainVBox),
		pToolBar,
		FALSE,
		FALSE,
		0);
	g_object_set_data (G_OBJECT (pMainWindow), "tool-bar", pToolBar);
	
	GtkRadioToolButton *group = NULL;
	CDCategory *pCategory;
	int i;
	for (i = 0; i < CD_NB_CATEGORIES; i ++)
	{
		pCategory = _get_category (i);
		pCategory->pCategoryButton = _make_toolbutton (pCategory->cName,
			pCategory->cIcon,
			CAIRO_DOCK_CATEGORY_ICON_SIZE,
			group);
		if (!group)
			group = GTK_RADIO_TOOL_BUTTON (pCategory->pCategoryButton);
		g_signal_connect (G_OBJECT (pCategory->pCategoryButton), "toggled", G_CALLBACK(_on_select_category), pCategory);
		pCategory->pMainWindow = pMainWindow;
		gtk_toolbar_insert (GTK_TOOLBAR (pToolBar), pCategory->pCategoryButton, -1);
	}
	
	gtk_widget_show_all (pToolBar);
	return pMainWindow;
}


static void cairo_dock_enable_apply_button (GtkWidget *pMainWindow, gboolean bEnable)
{
	GtkWidget *pApplyButton = g_object_get_data (G_OBJECT (pMainWindow), "apply-button");
	if (bEnable)
		gtk_widget_show (pApplyButton);
	else
		gtk_widget_hide (pApplyButton);
}


static void cairo_dock_select_category (GtkWidget *pMainWindow, CDCategoryEnum iCategory)
{
	CDCategory *pCategory = _get_category (iCategory);
	gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (pCategory->pCategoryButton), TRUE);  // will first emit a signal to the currently selected button, which will hide the current widget, and then to the new button, which will show the widget.
}


  ///////////////
 /// BACKEND ///
///////////////

static GtkWidget *show_main_gui (void)
{
	if (s_pSimpleConfigWindow == NULL)
		cairo_dock_build_simple_gui_window ();
	
	cairo_dock_select_category (s_pSimpleConfigWindow, CD_CATEGORY_CONFIG);
	return s_pSimpleConfigWindow;
}

static void show_module_gui (const gchar *cModuleName)
{
	if (s_pSimpleConfigWindow == NULL)
		cairo_dock_build_simple_gui_window ();
	
	cairo_dock_select_category (s_pSimpleConfigWindow, CD_CATEGORY_ITEMS);
	/// TODO: find a way to present a module that is not activated...
	
}

static void close_gui (void)
{
	if (s_pSimpleConfigWindow != NULL)
		gtk_widget_destroy (s_pSimpleConfigWindow);
}

static void update_module_state (const gchar *cModuleName, gboolean bActive)
{
	if (s_pSimpleConfigWindow == NULL)
		return;
	CDCategory *pCategory = _get_category (CD_CATEGORY_PLUGINS);
	if (pCategory->pCdWidget != NULL)  // category is built
	{
		cairo_dock_widget_plugins_update_module_state (PLUGINS_WIDGET (pCategory->pCdWidget), cModuleName, bActive);
	}
}

static void update_modules_list (void)
{
	if (s_pSimpleConfigWindow == NULL)
		return;
	CDCategory *pCategory = _get_category (CD_CATEGORY_PLUGINS);
	if (pCategory->pCdWidget != NULL)  // category is built
	{
		cairo_dock_widget_plugins_reload (PLUGINS_WIDGET (pCategory->pCdWidget));
	}
}

static void update_shortkeys (void)
{
	if (s_pSimpleConfigWindow == NULL)
		return;
	CDCategory *pCategory = _get_category (CD_CATEGORY_CONFIG);
	if (pCategory->pCdWidget != NULL)  // category is built
	{
		cairo_dock_widget_config_update_shortkeys (CONFIG_WIDGET (pCategory->pCdWidget));
	}
}

static void update_desklet_params (CairoDesklet *pDesklet)
{
	if (s_pSimpleConfigWindow == NULL || pDesklet == NULL || pDesklet->pIcon == NULL)
		return;
	
	CDCategory *pCategory = _get_category (CD_CATEGORY_ITEMS);
	if (pCategory->pCdWidget != NULL)  // category is built
	{
		cairo_dock_items_widget_update_desklet_params (ITEMS_WIDGET (pCategory->pCdWidget), pDesklet);
	}
}

static void update_desklet_visibility_params (CairoDesklet *pDesklet)
{
	if (s_pSimpleConfigWindow == NULL || pDesklet == NULL || pDesklet->pIcon == NULL)
		return;
	
	CDCategory *pCategory = _get_category (CD_CATEGORY_ITEMS);
	if (pCategory->pCdWidget != NULL)  // category is built
	{
		cairo_dock_items_widget_update_desklet_visibility_params (ITEMS_WIDGET (pCategory->pCdWidget), pDesklet);
	}
}

static void update_module_instance_container (CairoDockModuleInstance *pInstance, gboolean bDetached)
{
	if (s_pSimpleConfigWindow == NULL || pInstance == NULL)
		return;
	
	CDCategory *pCategory = _get_category (CD_CATEGORY_ITEMS);
	if (pCategory->pCdWidget != NULL)  // category is built
	{
		cairo_dock_items_widget_update_module_instance_container (ITEMS_WIDGET (pCategory->pCdWidget), pInstance, bDetached);
	}
}

static GtkWidget *show_gui (Icon *pIcon, CairoContainer *pContainer, CairoDockModuleInstance *pModuleInstance, int iShowPage)
{
	if (s_pSimpleConfigWindow == NULL)
		cairo_dock_build_simple_gui_window ();
	
	cairo_dock_select_category (s_pSimpleConfigWindow, CD_CATEGORY_ITEMS);
	
	CDCategory *pCategory = _get_category (CD_CATEGORY_ITEMS);
	if (pCategory->pCdWidget == NULL)  // build the widget immediately, because we want to select the given item
	{
		_build_category_widget (pCategory);
	}
	
	cairo_dock_items_widget_select_item (ITEMS_WIDGET (pCategory->pCdWidget), pIcon, pContainer, pModuleInstance, iShowPage);
}

static void reload_items (void)
{
	if (s_pSimpleConfigWindow == NULL)
		return;
	
	CDCategory *pCategory = _get_category (CD_CATEGORY_ITEMS);
	g_print ("%s (%p)\n", __func__, pCategory->pCdWidget);
	if (pCategory->pCdWidget != NULL)  // category is built
	{
		cairo_dock_items_widget_reload (ITEMS_WIDGET (pCategory->pCdWidget));
		if (s_iCurrentCategory != CD_CATEGORY_ITEMS)
			gtk_widget_hide (pCategory->pCdWidget->pWidget);
	}
}

static void reload (void)
{
	if (s_pSimpleConfigWindow == NULL)
		return;
	CDCategory *pCategory;
	
	pCategory = _get_category (CD_CATEGORY_ITEMS);
	if (pCategory->pCdWidget != NULL)  // category is built
	{
		cairo_dock_items_widget_reload (ITEMS_WIDGET (pCategory->pCdWidget));
		if (s_iCurrentCategory != CD_CATEGORY_ITEMS)
			gtk_widget_hide (pCategory->pCdWidget->pWidget);
	}
	
	pCategory = _get_category (CD_CATEGORY_CONFIG);
	if (pCategory->pCdWidget != NULL)  // category is built
	{
		cairo_dock_config_widget_reload (CONFIG_WIDGET (pCategory->pCdWidget));
		if (s_iCurrentCategory != CD_CATEGORY_CONFIG)
			gtk_widget_hide (pCategory->pCdWidget->pWidget);
	}
	
	pCategory = _get_category (CD_CATEGORY_PLUGINS);
	if (pCategory->pCdWidget != NULL)  // category is built
	{
		cairo_dock_widget_plugins_reload (PLUGINS_WIDGET (pCategory->pCdWidget));
		if (s_iCurrentCategory != CD_CATEGORY_PLUGINS)
			gtk_widget_hide (pCategory->pCdWidget->pWidget);
	}
}

////////////////////
/// CORE BACKEND ///
////////////////////

static void set_status_message_on_gui (const gchar *cMessage)
{
	if (s_pSimpleConfigWindow == NULL)
		return;
	GtkWidget *pStatusBar = g_object_get_data (G_OBJECT (s_pSimpleConfigWindow), "status-bar");
	gtk_statusbar_pop (GTK_STATUSBAR (pStatusBar), 0);  // clear any previous message, underflow is allowed.
	gtk_statusbar_push (GTK_STATUSBAR (pStatusBar), 0, cMessage);
}

static void reload_current_widget (CairoDockModuleInstance *pInstance, int iShowPage)
{
	g_return_if_fail (s_pSimpleConfigWindow != NULL);
	
	CDCategory *pCategory = _get_category (CD_CATEGORY_ITEMS);
	if (pCategory->pCdWidget != NULL)  // category is built
	{
		cairo_dock_items_widget_reload_current_widget (ITEMS_WIDGET (pCategory->pCdWidget), pInstance, iShowPage);
		if (s_iCurrentCategory != CD_CATEGORY_ITEMS)
			gtk_widget_hide (pCategory->pCdWidget->pWidget);
	}
}

static void show_module_instance_gui (CairoDockModuleInstance *pModuleInstance, int iShowPage)
{
	show_gui (pModuleInstance->pIcon, NULL, pModuleInstance, iShowPage);
}

static CairoDockGroupKeyWidget *get_widget_from_name (CairoDockModuleInstance *pInstance, const gchar *cGroupName, const gchar *cKeyName)
{
	g_return_val_if_fail (s_pSimpleConfigWindow != NULL, NULL);
	cd_debug ("%s (%s, %s)", __func__, cGroupName, cKeyName);
	CDCategory *pCategory = _get_current_category ();
	CDWidget *pCdWidget = pCategory->pCdWidget;
	if (pCdWidget)  /// check that the widget represents the given instance...
		return cairo_dock_gui_find_group_key_widget_in_list (pCdWidget->pWidgetList, cGroupName, cKeyName);
	else
		return NULL;
}


void cairo_dock_register_simple_gui_backend (void)
{
	CairoDockMainGuiBackend *pBackend = g_new0 (CairoDockMainGuiBackend, 1);
	
	pBackend->show_main_gui 					= show_main_gui;
	pBackend->show_module_gui 					= show_module_gui;
	pBackend->close_gui 						= close_gui;
	pBackend->update_module_state 				= update_module_state;
	pBackend->update_module_instance_container 	= update_module_instance_container;
	pBackend->update_desklet_params 			= update_desklet_params;
	pBackend->update_desklet_visibility_params 	= update_desklet_visibility_params;
	pBackend->update_modules_list 				= update_modules_list;
	pBackend->update_shortkeys 					= update_shortkeys;
	pBackend->show_gui 							= show_gui;
	pBackend->reload_items 						= reload_items;
	pBackend->reload 							= reload;
	pBackend->cDisplayedName 					= _("Advanced Mode");  // name of the other backend.
	pBackend->cTooltip 							= _("The advanced mode lets you tweak every single parameter of the dock. It is a powerful tool to customise your current theme.");
	
	cairo_dock_register_config_gui_backend (pBackend);
	
	CairoDockGuiBackend *pConfigBackend = g_new0 (CairoDockGuiBackend, 1);
	
	pConfigBackend->set_status_message_on_gui 	= set_status_message_on_gui;
	pConfigBackend->reload_current_widget 		= reload_current_widget;
	pConfigBackend->show_module_instance_gui 		= show_module_instance_gui;
	pConfigBackend->get_widget_from_name 		= get_widget_from_name;
	
	cairo_dock_register_gui_backend (pConfigBackend);
}

