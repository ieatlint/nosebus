/*
 * This file is part of nosebus.
 * nosebus is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * nosebus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with nosebus.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef WEB_H
#define WEB_H

#include <curl/curl.h>

struct curl_data {
	gint size;
	gchar *body;
	gint *len;
};


#define LATE_REFERER "http://www.nextbus.com/service/googleMapXMLFeed"

CURL *web_init();
CURL *web_close( CURL *curl );
gint web_get( CURL *curl, const gchar *url, gchar *body, gint *len );
gchar *web_enc( CURL *curl, gchar *str );
gchar *web_free( gchar *str );
size_t curl_cb_get( void *ptr, size_t size, size_t nmemb, void *stream );

#endif
