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

#include <sysdep.h>

#include <stdio.h>
#include <string.h>

#include <urjtag/error.h>
#include <urjtag/chain.h>
#include <urjtag/bus.h>

#include <urjtag/cmd.h>

#include "cmd.h"

static int
cmd_bus_run (urj_chain_t *chain, char *params[])
{
    long unsigned n;

    if (urj_cmd_params (params) != 2)
    {
        urj_error_set (URJ_ERROR_SYNTAX,
                       "%s: #parameters should be %d, not %d",
                       params[0], 2, urj_cmd_params (params));
        return URJ_STATUS_FAIL;
    }

    if (urj_cmd_test_cable (chain) != URJ_STATUS_OK)
        return URJ_STATUS_FAIL;

    if (!chain->parts)
    {
        urj_error_set (URJ_ERROR_ILLEGAL_STATE, "no parts. Run '%s' first",
                       "detect");
        return URJ_STATUS_FAIL;
    }

    if (urj_cmd_get_number (params[1], &n) != URJ_STATUS_OK)
        return URJ_STATUS_FAIL;

    return urj_bus_buses_set (n);
}

static void
cmd_bus_help (void)
{
    urj_log (URJ_LOG_LEVEL_NORMAL,
             _("Usage: %s BUS\n"
               "Change active bus.\n"
               "\n" "BUS           bus number\n"),
             "bus");
}

const urj_cmd_t urj_cmd_bus = {
    "bus",
    N_("change active bus"),
    cmd_bus_help,
    cmd_bus_run
};
