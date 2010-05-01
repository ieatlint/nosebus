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


#ifndef PREDICT_H
#define PREDICT_H

#define BASE_URL "http://www.nextbus.com/service/googleMapXMLFeed?command=predictionsForMultiStops&a=sf-muni&"
#define MPRED_URL "http://www.nextbus.com/service/googleMapXMLFeed?command=predictionsForMultiStops&a=sf-muni&stops=%s|%s|%s&key=%s"

#define KEY "120780473219"
#define BUF_SIZE 4096

struct xml_data {
	GSList *arrivals;

	gchar *curDir;
	nbArrival *curArr;//pointer to the current <prediction> tag being worked on

	/* for <keyForNextTime> tags */
	gchar *key;

	/* For <Error> tags */
	gchar *errmsg;
	gboolean error;//this cannot be tested to see if an error occured!
};



/* xml funcs */
gint predict_get( nbData *nb );
void xml_cb_predict_start( GMarkupParseContext *context, const gchar *element, const gchar **attr_names,
		const gchar **attr_values, gpointer data, GError **error );
void xml_cb_predict_text( GMarkupParseContext *context, const gchar *text, gsize len, gpointer data, GError **error );
gint xml_arrivals_sort( gconstpointer a, gconstpointer b );

#endif
