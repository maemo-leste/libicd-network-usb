/*
 * This file is part of libicd-network-usb
 *
 * Copyright (C) 2010 Nokia Corporation.
 *
 * Contact: Patrik.Flykt@nokia.com <patrik.flykt@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <string.h>
#include <glib.h>
#include <gconf/gconf-client.h>

#include <osso-ic-gconf.h>
#include <network_api.h>
#include <icd_log.h>

#include "libicd_network_usb.h"


gboolean icd_nw_init (struct icd_nw_api *network_api,
		      icd_nw_watch_pid_fn watch_cb,
		      gpointer watch_cb_token,
		      icd_nw_close_fn close_cb);


/** Check whether the given interface exists
 * @param ifname the interface name
 * @return TRUE if #USB_NW_INTERFACE is up, FALSE otherwise
 */
static gboolean usb_if_exists (const gchar *ifname)
{
  int fd;
  struct ifreq iface;
  gboolean found = FALSE;

  if ((fd = socket (AF_UNIX, SOCK_DGRAM, 0)) < 0)
    return found;

  strcpy (iface.ifr_name, ifname);
  if (ioctl (fd, SIOCGIFINDEX, &iface) == 0)
    found = TRUE;

  close (fd);
  return found;
}


/** Function to bring up the link layer
 * @param network_type network type
 * @param network_attrs attributes, such as type of network_id, security, etc.
 * @param network_id IAP name or local id, e.g. SSID
 * @param link_up_cb the callback function to call when the link is up
 * @param link_up_cb_token token to pass to the callback function
 * @param private a reference to the icd_nw_api private memeber
 */
static void usb_link_up (const gchar *network_type,
			   const guint network_attrs,
			   const gchar *network_id,
			   icd_nw_link_up_cb_fn link_up_cb,
			   const gpointer link_up_cb_token,
			   gpointer *private)
{
  if (usb_if_exists (USB_NW_INTERFACE)) {
    link_up_cb (ICD_NW_SUCCESS,
		NULL,
		USB_NW_INTERFACE,
		link_up_cb_token, NULL);
  } else {
    link_up_cb (ICD_NW_ERROR,
		NULL,
		NULL,
		link_up_cb_token, NULL);
  }
}

/** Function for listing the available networks provided by the module
 * @param network_type network type or NULL
 * @param search_scope search scope; ignored
 * @param search_cb the search callback
 * @param search_cb_token token from the ICd to pass to the callback
 * @private a reference to the icd_nw_api private member
 */
static void usb_start_search (const gchar *network_type,
				guint search_scope,
				icd_nw_search_cb_fn search_cb,
				const gpointer search_cb_token,
				gpointer *private)
{
  GConfClient *gconf_client;
  GSList *list;
  gchar *gconf_path, *gconf_name, *key, *settings_type, *settings_name;
  gboolean settings_autoconnect;


  if (usb_if_exists(USB_NW_INTERFACE)) {

    gconf_client = gconf_client_get_default();
    list = gconf_client_all_dirs (gconf_client,
				  ICD_GCONF_PATH,
				  NULL);

    while (list) {
      gconf_path = list->data;

      /* IAP id */
      gconf_name = g_strrstr (gconf_path, "/");
      if (gconf_name) {
	gconf_name += 1;
	gconf_name = gconf_unescape_key (gconf_name, -1);
      }

      /* type */
      key = g_strconcat (gconf_path, "/type", NULL);
      settings_type = gconf_client_get_string (gconf_client, key, NULL);
      g_free (key);

      if (gconf_name &&
	  settings_type &&
	  strcmp (settings_type, USB_NW_TYPE) == 0) {

	/* autoconnect attribute */
	key = g_strconcat (gconf_path, "/autoconnect", NULL);
	settings_autoconnect = gconf_client_get_bool (gconf_client, key, NULL);
	g_free (key);

	/* IAP user readable name */
	key = g_strconcat (gconf_path, "/name", NULL);
	settings_name = gconf_client_get_string (gconf_client, key, NULL);
	g_free (key);
	if (settings_name == NULL)
	  settings_name = g_strdup (gconf_name);
					       
	search_cb (ICD_NW_SEARCH_CONTINUE,
		   settings_name,
		   USB_NW_TYPE,
		   ICD_NW_ATTR_IAPNAME |
		   (settings_autoconnect? ICD_NW_ATTR_AUTOCONNECT: 0),
		   gconf_name,
		   ICD_NW_LEVEL_10,
		   /** TODO: add MAC address here */
		   "",
		   0,
		   search_cb_token);
	g_free (settings_name);
      }
      g_free (gconf_name);
      g_free (settings_type);
      g_free (list->data);
      list = g_slist_delete_link (list, list);
    }
    g_object_unref (gconf_client);
  }

  search_cb (ICD_NW_SEARCH_COMPLETE,
	     NULL,
	     NULL,
	     0,
	     NULL,
	     0,
	     NULL,
	     0,
	     search_cb_token);
}

/** USB network module initialization function.
 * @param network_api icd_nw_api structure filled in by the module
 * @param watch_cb function to inform ICd that a child process is to be
 *        monitored for exit status
 * @param watch_cb_token token to pass to the watch pid function
 * @param close_cb function to inform ICd that the network connection is to be
 *        closed
 * @return TRUE on succes; FALSE on failure whereby the module is unloaded
 */
gboolean icd_nw_init (struct icd_nw_api *network_api,
		      icd_nw_watch_pid_fn watch_cb,
		      gpointer watch_cb_token,
		      icd_nw_close_fn close_cb)
{
  network_api->version = ICD_NW_MODULE_VERSION;
  network_api->link_up = usb_link_up;
  network_api->search_lifetime = 31;
  network_api->search_interval = 30;
  network_api->start_search = usb_start_search;

  return TRUE;
}
