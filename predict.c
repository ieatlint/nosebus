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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <curl/curl.h>

#include "nosebus.h"
#include "web.h"
#include "predict.h"

gint predict_get( nbData *nb ) {
	GMarkupParseContext *xmldata = NULL;
	GMarkupParser xmlparser = {	xml_cb_predict_start,//start
					NULL, //end
					xml_cb_predict_text,//text
					NULL,//non-xml data
					NULL }; //error handler
	
	gchar body[BUF_SIZE];
	gint len = BUF_SIZE;
	struct xml_data parsed = {0};
	gchar url[256];

	sprintf(url, MPRED_URL, nb->query->agency, nb->query->line, nb->query->dest, nb->query->stop, nb->key );

	if (web_get( nb->web, url, body, &len ) ) {
		fprintf(stderr,"Failed to download XML data\n");
		return -1;
	}

	#ifdef DEBUG2
	fprintf(stderr,"Reply body:\n%s\n***** EOF *****\n", body );
	#endif

	xmldata = g_markup_parse_context_new( &xmlparser, 0, &parsed, NULL );
	g_markup_parse_context_parse( xmldata, body, len, NULL );

	g_markup_parse_context_free( xmldata );


	if( parsed.errmsg == NULL ) {
		nb->arrivals = g_slist_sort( parsed.arrivals, xml_arrivals_sort );
		g_free( nb->key );
		nb->key = parsed.key;
	} else if( strstr( parsed.errmsg, "map page" ) ) {
		g_free( parsed.errmsg );
		return 1;
	} else if( strlen( parsed.errmsg ) > 5 ) {
		nb->errmsg = parsed.errmsg;
		return -1;
	} else if( parsed.errmsg ) {
		fprintf(stderr,"The string length of errmsg is %d.  content:\n%s\n", (int)strlen( parsed.errmsg ), parsed.errmsg );
	}

	if( parsed.errmsg )
		g_free( parsed.errmsg );
	if( parsed.curDir )
		g_free( parsed.curDir );

	return 0;
}

void xml_cb_predict_start( GMarkupParseContext *context, const gchar *element, const gchar **attr_names,
		const gchar **attr_values, gpointer data, GError **error ) {
	struct xml_data *info = data;
	info->error = FALSE;

	for(; *attr_names != NULL; attr_names++, attr_values++ ) {
		if( !g_strcmp0( element, "direction" ) ) {
			/* <direction title=""> */
			if( info->curDir != NULL )
				g_free( info->curDir );
			info->curDir = g_strdup( *attr_values );
		} else if( !g_strcmp0( element, "prediction" ) ) {
			/* <prediction seconds="" minutes="" epochTime="" isDeparture="" dirTag="" vehicle="" block="" /> */

			if( !g_strcmp0( *attr_names, "minutes" ) ) {
				info->curArr = g_malloc( sizeof( nbArrival ) );

				info->arrivals = g_slist_append( info->arrivals, info->curArr );

				info->curArr->minutes = atoi( *attr_values );
				info->curArr->destTitle = g_strdup( info->curDir );
			}
		} else if( !g_strcmp0( element, "keyForNextTime" ) ) {
			/* <keyForNextTime value=""> */
			info->key = g_strdup( *attr_values  );
		} else if( !g_strcmp0( element, "Error" ) ) {
			info->error = TRUE;
		}
	}
}

void xml_cb_predict_text( GMarkupParseContext *context, const gchar *text, gsize len, gpointer data, GError **error ) {
	struct xml_data *info = data;
	if( info->error ) {
		if( len > 5 ) {
			if( info->errmsg ) {
				fprintf( stderr,"errmsg overflow.  old value: %s\n", info->errmsg );
				g_free( info->errmsg );
			}
			info->errmsg = g_strndup( text, len );
		}
	}
}

gint xml_arrivals_sort( gconstpointer a, gconstpointer b ) {
	const nbArrival *first = a, *second = b;
	return first->minutes - second->minutes;
}
