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

#include <glib.h>
#include <curl/curl.h>
#include <string.h>

#include "web.h"



/* opens a curl handle and sets default values.
 * Returns NULL on error and a valid handle on success. */
CURL *web_init() {
	CURL *curl;

	if( curl_global_init( CURL_GLOBAL_NOTHING ) )
		return NULL;
	
	curl = curl_easy_init();
	if( curl == NULL )
		return NULL;
	
	if(	curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, curl_cb_get ) ||
		curl_easy_setopt( curl, CURLOPT_USERAGENT, "FIXME" ) ||
		curl_easy_setopt( curl, CURLOPT_REFERER, LATE_REFERER ) ) {
		return web_close( curl );//error setting option, cleanup and close
	}

//	curl_easy_setopt( curl, CURLOPT_VERBOSE, 1 );

	return curl;
}

/* closes an open curl handle and always returns NULL */
CURL *web_close( CURL *curl ) {
	curl_easy_cleanup( curl );
	curl_global_cleanup();
	return NULL;
}

/* Gets the page and stores it in body.
 *
 * vars:
 * curl - valid curl handler
 * url - a valid url
 * body - a gchar array
 * len - the size of body (max length)
 * 
 * Return value:
 * On success, the page is stored in body, and the length of the data is
 * stored in len.
 *
 * On failure, body and len are undefined (but may contain an amount of
 * data equiv to the size of body).
 */
gint web_get( CURL *curl, const gchar *url, gchar *body, gint *len ) {
	struct curl_data get;

	if( curl_easy_setopt( curl, CURLOPT_URL, url ) ) {
		fprintf(stderr,"Failed to set URL to %s\n", url );
		return -1;
	}

	if( curl_easy_setopt( curl, CURLOPT_WRITEDATA, &get ) ) {
		fprintf(stderr,"Failed to set writedata\n" );
		return -1;
	}

	get.size = *len;
	get.body = body;
	get.len = len;
	*get.len = 0;

	if( curl_easy_perform( curl ) ) {
		fprintf(stderr,"Failed to perform\n" );
		return -1;
	}

	return 0;
}

gchar *web_enc( CURL *curl, gchar *str ) {
	return curl_easy_escape( curl ,str, 0 );
}

gchar *web_free( gchar *str ) {
	curl_free( str );
	return NULL;
}

/* this is a callback function for curl, which stores the downloaded data
 * into an array */
size_t curl_cb_get( void *ptr, size_t size, size_t nmemb, void *stream ) {
	struct curl_data *get = stream;
	gint total = size * nmemb;

	if( total == 0 )
		return 0;
	
	if( *get->len + total > get->size ) {//too big for buffer!
		fprintf(stderr,"Failed to clear buffer -- %d/%d exceeds buffer size of %d.\n", total, *get->len, get->size );
		return 0;
	}

	memcpy( get->body + *get->len, ptr, total );
	*get->len += total;

	return total;
}
