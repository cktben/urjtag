/*
 * $Id$
 *
 * Copyright (C) 2003 ETC s.r.o.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * Written by Marcel Telka <marcel@telka.sk>, 2003.
 *
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "cable.h"
#include "tap.h"
#include "chain.h"

#include "jtag.h"

#define	DETECT_PATTERN_SIZE	8
#define	MAX_REGISTER_LENGTH	1024
#define	TEST_COUNT		5
#define	TEST_THRESHOLD		100		/* in % */

static int
detect_register_size( chain_t *chain, FILE *f )
{
	int len;
	tap_register *rz;
	tap_register *rout;
	tap_register *rpat;

	for (len = 1; len <= MAX_REGISTER_LENGTH; len++) {
		int p;
		int ok = 0;

		fprintf( f, _("\tTesting register length: %d\n"), len );

		rz = register_fill( register_alloc( len ), 0 );
		rout = register_alloc( DETECT_PATTERN_SIZE + len );
		rpat = register_inc( register_fill( register_alloc( DETECT_PATTERN_SIZE + len ), 0 ) );

		for (p = 1; p < (1 << DETECT_PATTERN_SIZE); p++) {
			int i;
			const char *s;
			ok = 0;

			s = register_get_string( rpat );
			while (*s)
				s++;
			fprintf( f, _("\t\tPattern: %s, "), s - DETECT_PATTERN_SIZE );

			for (i = 0; i < TEST_COUNT; i++) {
				tap_shift_register( chain, rz, NULL, 0 );
				tap_shift_register( chain, rpat, rout, 0 );

				register_shift_right( rout, len );

				if (register_compare( rpat, rout ) == 0)
					ok++;
			}
			fprintf( f, _("%d %%\n"), 100 * ok / TEST_COUNT );
			if (100 * ok / TEST_COUNT < TEST_THRESHOLD) {
				ok = 0;
				break;
			}

			register_inc( rpat );
		}

		register_free( rz );
		register_free( rout );
		register_free( rpat );

		if (ok)
			return len;
	}

	return -1;
}

static void
jtag_reset( chain_t *chain )
{
	chain_set_trst( chain, 0 );
	sleep( 1 );
	chain_set_trst( chain, 1 );
	sleep( 1 );

	tap_reset( chain );
}

#define	MAX_CHAIN_LENGTH	128

void
discovery( chain_t *chain, const char *filename )
{
	int i;
	int irlen;
	tap_register *ir;
	tap_register *irz;
	FILE *f = NULL;

	tap_register *id = register_alloc( 32 );
	tap_register *zeros = register_fill( register_alloc( 32 ), 0 );
	tap_register *ones = register_fill( register_alloc( 32 ), 1 );

	if (id && zeros && ones) {
		f = fopen( filename, "w" );
		if (!f)
			printf( _("Error: Unable to create file '%s'.\n"), filename );
	} else
		printf( _("Error: Out of memory!\n") );

	if (!id || !zeros || !ones || !f) {
		register_free( id );
		register_free( zeros );
		register_free( ones );
		return;
	}

	printf( _("Detecting JTAG chain length:\n") );
	fprintf( f, _("Detecting JTAG chain length:\n") );

	jtag_reset( chain );

	tap_capture_dr( chain );
	for (i = 0; i < MAX_CHAIN_LENGTH; i++) {
		tap_shift_register( chain, zeros, id, 0 );
		if (!register_compare( id, zeros ))
			break;				/* end of chain */

		if (!register_compare( ones, id )) {
			printf( _("bad JTAG connection (TDO is 1)\n") );
			fprintf( f, _("bad JTAG connection (TDO is 1)\n") );
			break;
		}

		printf( _("ID[%d]: %s\n"), i, register_get_string( id ) );
		fprintf( f, _("ID[%d]: %s\n"), i, register_get_string( id ) );
	}

	register_free( id );
	register_free( zeros );
	register_free( ones );

	if (i == MAX_CHAIN_LENGTH) {
		printf( _("Warning: Maximum internal JTAG chain length exceeded!\n") );
		fprintf( f, _("Warning: Maximum internal JTAG chain length exceeded!\n") );
	} else {
		printf( _("JTAG chain length is %d\n"), i );
		fprintf( f, _("JTAG chain length is %d\n"), i );
	}

	/* detecting IR size */
	jtag_reset( chain );

	printf( _("Detecting IR size...\n") );
	fprintf( f, _("Detecting IR size:\n") );

	tap_capture_ir( chain );
	irlen = detect_register_size( chain, f );

	printf( _("IR length is %d\n\n"), irlen );
	fprintf( f, _("IR length is %d\n\n"), irlen );

	if (irlen < 1) {
		printf( _("Error: Invalid IR length!\n") );
		fclose( f );
		return;
	}

	ir = register_fill( register_alloc( irlen ), 0 );
	irz = register_duplicate( ir );

	if (!ir || !irz) {
		register_free( ir );
		register_free( irz );
		fclose( f );
		printf( _("Error: Out of memory!\n") );
		return;
	}

	for (;;) {
		int rs;

		jtag_reset( chain );

		tap_capture_ir( chain );
		tap_shift_register( chain, ir, NULL, 1 );

		printf( _("Detecting DR size for IR %s ...\n"), register_get_string( ir ) );
		fprintf( f, _("Detecting DR size for IR %s:\n"), register_get_string( ir ) );

		tap_capture_dr( chain );
		rs = detect_register_size( chain, f );

		printf( _("DR length for IR %s is %d\n\n"), register_get_string( ir ), rs );
		fprintf( f, _("DR length for IR %s is %d\n\n"), register_get_string( ir ), rs );

		register_inc( ir );
		if (register_compare( ir, irz ) == 0)
			break;
	}
	register_free( ir );
	register_free( irz );

	fclose( f );
}
