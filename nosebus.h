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

#ifndef NOSEBUS_H
#define NOSEBUS_H

typedef struct {
	gchar *destTitle;
	gint minutes;
} nbArrival;

typedef struct {
	gchar *line;
	gchar *dest;
	gchar *stop;
} nbQuery;

typedef struct {
	CURL *web;
	nbQuery *query;
	GSList *arrivals;
	gchar *errmsg;
	gchar *key;
} nbData;

void nb_printArrivals( nbData *nb );
void nb_freeArrivals( GSList *arrivals );
void nb_saveKey( nbData *nb );
gboolean nb_getKey( nbData *nb, gboolean usecache );

void nb_help();
nbQuery *nb_cmdParse( int argc, char **argv );
nbQuery *nb_freeQuery( nbQuery *nbq );


#endif
