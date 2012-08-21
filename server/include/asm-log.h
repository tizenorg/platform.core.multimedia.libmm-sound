/*
 * audio-session-manager
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Jonghyuk Choi <jhchoi.choi@samsung.com>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#ifndef __ASM_LOG_H__
#define __ASM_LOG_H__

#ifdef __DEBUG_MODE__
#include <stdio.h>
#ifdef __USE_LOGMANAGER__
#include <mm_log.h>
#endif
#endif

#ifdef __DEBUG_MODE__

#ifdef __USE_LOGMANAGER__
#define asm_info_r(msg, args...) log_print_rel((MMF_LOG_OWNER), (LOG_CLASS_INFO), (msg), ##args)
#define asm_warning_r(msg, args...) log_print_rel((MMF_LOG_OWNER), (LOG_CLASS_WARNING), (msg), ##args)
#define asm_error_r(msg, args...) log_print_rel((MMF_LOG_OWNER), (LOG_CLASS_ERR), (msg), ##args)
#define asm_critical_r(msg, args...) log_print_rel((MMF_LOG_OWNER), (LOG_CLASS_CRITICAL), (msg), ##args)
#define asm_assert_r(condition)  log_assert_rel((condition))

#define asm_info(msg, args...) log_print_dbg((MMF_LOG_OWNER), LOG_CLASS_INFO, (msg), ##args)
#define asm_warning(msg, args...) log_print_dbg((MMF_LOG_OWNER), LOG_CLASS_WARNING, (msg), ##args)
#define asm_error(msg, args...) log_print_dbg((MMF_LOG_OWNER), LOG_CLASS_ERR, (msg), ##args)
#define asm_critical(msg, args...) log_print_dbg((MMF_LOG_OWNER), LOG_CLASS_CRITICAL, (msg), ##args)
#define asm_assert(condition)  log_assert_dbg((condition))

#else	//__USE_LOGMANAGER__

#define asm_info_r(msg, args...) fprintf(stderr, msg, ##args)
#define asm_warning_r(msg, args...) fprintf(stderr, msg, ##args)
#define asm_error_r(msg, args...) fprintf(stderr, msg, ##args)
#define asm_critical_r(msg, args...) fprintf(stderr, msg, ##args)
#define asm_assert_r(condition)		(condition)

#define asm_info(msg, args...) fprintf(stderr, msg, ##args)
#define asm_warning(msg, args...) fprintf(stderr, msg, ##args)
#define asm_error(msg, args...) fprintf(stderr, msg, ##args)
#define asm_critical(msg, args...) fprintf(stderr, msg, ##args)
#define asm_assert(condition)			(condition)

#endif	//__USE_LOGMANAGER__

#else	//__DEBUG_MODE__

#define asm_info_r(msg, args...)
#define asm_warning_r(msg, args...)
#define asm_error_r(msg, args...)
#define asm_critical_r(msg, args...)
#define asm_assert_r(condition)	(condition)

#define asm_info(msg, args...)
#define asm_warning(msg, args...)
#define asm_error(msg, args...)
#define asm_critical(msg, args...)
#define asm_assert(condition)		(condition)

#endif  // __DEBUG_MODE__

#endif	/* __ASM_LOG_H__ */
