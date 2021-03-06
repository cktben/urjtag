/*
 * $Id$
 *
 * Copyright (C) 2009, Rutger Hofman, VU Amsterdam
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
 */

#ifndef URJ_LOG_H
#define URJ_LOG_H

#include <stdarg.h>

#include "types.h"

/**
 * Log state.
 */
typedef struct URJ_LOG_STATE
{
    urj_log_level_t     level;                  /**< logging level */
    int               (*out_vprintf) (const char *fmt, va_list ap);
    int               (*err_vprintf) (const char *fmt, va_list ap);
}
urj_log_state_t;

extern urj_log_state_t urj_log_state;

int urj_do_log (urj_log_level_t level, const char *fmt, ...)
#ifdef __GNUC__
                        __attribute__ ((format (printf, 2, 3)))
#endif
    ;

#define urj_log(lvl, ...) \
        do { \
            if ((lvl) >= urj_log_state.level) \
                urj_do_log (lvl, __VA_ARGS__); \
        } while (0)

/**
 * Print warning unless logging level is > URJ_LOG_LEVEL_WARNING
 *
 * @param e urj_error_t value
 * @param ... consists of a printf argument set. It needs to start with a
 *      const char *fmt, followed by arguments used by fmt.
 */
#define urj_warning(...) \
    do { \
        urj_log (URJ_LOG_LEVEL_WARNING, "%s:%d %s() Warning: ", \
                 __FILE__, __LINE__, __func__); \
        urj_log (URJ_LOG_LEVEL_WARNING, __VA_ARGS__); \
    } while (0)

#endif /* URJ_LOG_H */
