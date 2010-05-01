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
#include <stdio.h>
#include <curl/curl.h>
#include <string.h>

#include "nosebus.h"
#include "web.h"
#include "predict.h"

#define VERSION "1.0.0"

#define MAX_RETRIES 3
#define TMP_KEYFILE "/tmp/nosebusKey"
#define KEY_BUF_SIZE 8192
#define NB_KEY_URL "http://www.nextbus.com/googleMap/googleMap.jsp?a=sf-muni&r=F&d=F__IBCTRO&s=5650"

int main( int argc, char **argv ) {
	nbData *nb = NULL;
	gint retval,i;

	if( argc == 1 ) {
		nb_help();
		return 0;
	}

	nb = g_new0( nbData, 1 );

	nb->query = nb_cmdParse( argc, argv );
	if( nb->query == NULL ) {
		return 1;
	}



	/* init curl */
	nb->web = web_init();

	/* get the key */
	if( nb_getKey( nb, TRUE ) ) {
		fprintf( stderr,"Failed to get a key from NextBus\n");
		return TRUE;
	}

	/* make the query
	 * This is done in a loop to allow for retries.
	 * In the event the key is invalid, it will attempt to get a new key and retry */
	for( retval = 2, i = 0; retval != 0 && i < MAX_RETRIES ; i++ ) {
		retval = predict_get( nb );
		if( retval == 1 ) {
			if( nb_getKey( nb, FALSE ) ) {
				fprintf( stderr,"Failed to get a key from NextBus\n");
				return TRUE;
			}
		} else if( retval == -1 ) {
			fprintf( stderr,"Failed to get prediction information!\n");
			if( nb->errmsg ) {
				fputs( nb->errmsg, stderr );
			}
			return TRUE;
		}
	}

	/* cleanup curl */
	nb->web = web_close( nb->web );

	/* print the data */
	if( nb->arrivals != NULL ) {
		nb_printArrivals( nb );
	} else if( i != MAX_RETRIES ) { /* This tells us we didn't hit the MAX_RETRIES limit, but still received no predictions */
		printf("No prediction available\n");
	} else {
		/* This should hopefully not be reached.  This means that the format of the 
		 * page has changed such that we are no longer able to parse it, or some other
		 * measure has prevented us from successfully loading valid XML data. */
		fprintf(stderr,"NextBus site is not currently producing recognisable output.  It may no longer be compatible with this application\n");
	}

	/* cleanup data */
	nb_freeQuery( nb->query );
	nb_freeArrivals( nb->arrivals );
	if( nb->key )
		g_free( nb->key );
	g_free( nb );


	return 0;
}

/* This function prints the arrival estimates.
 * It assumes that nb->arrivals has valid data (but will safely handle it when empty). */
void nb_printArrivals( nbData *nb ) {
	GSList *trav;
	nbArrival *arr;

	for( trav = nb->arrivals; trav != NULL; trav = trav->next ) {
		arr = trav->data;

		/* There are two values you can reference -- arr->minutes and arr->destTitle */
		printf("%d minutes (%s)\n", arr->minutes, arr->destTitle );
	}
}

/* This function frees the dynamically allocated memory from nb->arrivals.
 * It will safely handle an empty list of arrivals */
void nb_freeArrivals( GSList *arrivals ) {
	nbArrival *arr;

	for( ; arrivals != NULL; arrivals = arrivals->next ) {
		arr = arrivals->data;
		g_free( arr->destTitle );
		g_free( arr );
	}
	g_slist_free( arrivals );
}

/* This function attempts to save the NextBus key to TMP_KEYFILE (Defined at the top of this file).
 * If for any reason it is unable to, it will print a warning to stderr.  All errors are non-fatal. */
void nb_saveKey( nbData *nb ) {
	if( !g_file_set_contents( TMP_KEYFILE, nb->key, strlen( nb->key ), NULL ) ) {
		fprintf(stderr,"Warning: Unable to save NextBus key to " TMP_KEYFILE ".  Continuing without saving...\n");
	}
}

/* This function attempts to get a NextBus key.
 * If "usecache" is TRUE, it will first attempt to obtain a key from TMP_KEYFILE.
 * Otherwise, it will download one from the NextBus website.
 *
 * Function returns FALSE when a key is successfully obtained, and TRUE when it fails. 
 * An error message will be printed to stderr in the event of failure. */
gboolean nb_getKey( nbData *nb, gboolean usecache ) {
	gchar body[KEY_BUF_SIZE];
	gint len = KEY_BUF_SIZE;
	gint i,x;
	gchar *tmp;

	/* If usecase is TRUE, attempt to get the key from TMP_KEYFILE */
	if( usecache && g_file_test( TMP_KEYFILE, G_FILE_TEST_IS_REGULAR ) && g_file_get_contents( TMP_KEYFILE, &nb->key, NULL, NULL ) ) {
		tmp = strchr( nb->key, '\n' );
		if( tmp )
			tmp = '\0';
		#ifdef DEBUG
		fprintf(stderr,"Obtained key from cache (%s)\n", nb->key );
		#endif
		return FALSE;
	}

	/* Attempt to download the NextBus page that contains a key */
	if( web_get( nb->web, NB_KEY_URL, body, &len ) ) {
		fprintf(stderr,"Failed to get key\n" );
		return TRUE;
	}

	/* Parse the body of the page for the string "keyF" (from "keyForNextTime").
	 * Record the value of this tag, if present */
	for( i = 0; i < (len - 4) ; i++ ) {
		if( body[i] == 'k' && body[i+1] == 'e' && body[i+2] == 'y' && body[i+3] == 'F' ) {
			for( ; i < len && body[i-1] != '"'; i++ );
			if( i == len ) break;//if eof, exit
			x = i;//start of key
			for( ; i < len && body[i] != '"'; i++ );
			if( i == len ) break;//if eof, exit
			body[i] = '\0';//i will be the end of the key
			nb->key = g_strdup( body+x );

			break;
		}
	}

	/* detect failure */
	if( nb->key == NULL ) {
		fprintf(stderr,"Failed to find key in output!\n");
		return TRUE;
	}
	#ifdef DEBUG
	fprintf(stderr,"Downloaded key from nextbus (%s)\n", nb->key );
	#endif

	/* Attempt to save the key */
	nb_saveKey( nb );

	return FALSE;
}

/* This simply prints the help message that appears when the program fails
 * to receive valid input at execution */
void nb_help() {
	printf( "Nosebus version " VERSION "\n"
		"nosebus -l [line] -d [dest] -s [stop]\n"
		"-l <line>\tSpecify the transit line.\n"
		"-d <dest>\tSpecify the destination of the line\n"
		"-s <stop>\tSpecify the stop for the line\n" );
}

/* This function will parse the data given to the program from the command line.
 *
 * It will dynmically create a nbQuery object and return it on success.
 * On failure, it will return NULL.
 */
nbQuery *nb_cmdParse( int argc, char **argv ) {
	gint i;
	nbQuery *nbq;

	nbq = g_new0( nbQuery, 1 );

	for( i = 1; i < argc; i++ ) {
		if( argv[i][0] != '-' ) {
			fprintf( stderr, "Unknown data at argument %d: %s\n", i-1, argv[i] );
			nb_freeQuery( nbq );
			return NULL;
		}

		switch( argv[i][1] ) {
			case 'l':
				if( nbq->line ) {
					g_free( nbq->line );
					nbq->line = NULL;
				}
				if( argv[i][2] != '\0' )
					nbq->line = g_strdup( argv[i]+2 );
				else if( i+1 < argc )
					nbq->line = g_strdup( argv[++i] );
				else {
					fprintf( stderr, "No data provided for -l switch as required.\n" );
					nb_freeQuery( nbq );
					return NULL;
				}
				break;
			case 'd':
				if( nbq->dest ) {
					g_free( nbq->dest );
					nbq->dest = NULL;
				}
				if( argv[i][2] != '\0' )
					nbq->dest = g_strdup( argv[i]+2 );
				else if( i+1 < argc )
					nbq->dest = g_strdup( argv[++i] );
				else {
					fprintf( stderr, "No data provided for -d switch as required.\n" );
					nb_freeQuery( nbq );
					return NULL;
				}
				break;
			case 's':
				if( nbq->stop ) {
					g_free( nbq->stop );
					nbq->stop = NULL;
				}
				if( argv[i][2] != '\0' )
					nbq->stop = g_strdup( argv[i]+2 );
				else if( i+1 < argc )
					nbq->stop = g_strdup( argv[++i] );
				else {
					fprintf( stderr, "No data provided for -s switch as required.\n" );
					nb_freeQuery( nbq );
					return NULL;
				}
				break;
			default:
				fprintf( stderr, "Unknown switch at argument %d: %s\n", i+1, argv[i] );
				nb_freeQuery( nbq );
				return NULL;
		}
	}

	/* Sanity check -- verify all three values have been set */
	if( !nbq->line || !nbq->dest || !nbq->stop ) {
		fprintf( stderr, "Failed to specify all required values.\nYou must specify a line, destination and stop!\n" );
		nb_freeQuery( nbq );
		return NULL;
	}

	return nbq;
}

/* This function frees all dynamically allocated data from a nbQuery type. */
nbQuery *nb_freeQuery( nbQuery *nbq ) {
	if( nbq != NULL ) {
		if( nbq->line != NULL )
			g_free( nbq->line );
		if( nbq->dest != NULL )
			g_free( nbq->dest );
		if( nbq->stop != NULL )
			g_free( nbq->stop );
		g_free( nbq );
	}

	return NULL;
}
