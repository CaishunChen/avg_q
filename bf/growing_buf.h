/*
 * Copyright (C) 1996-2000,2003,2007 Bernd Feige
 * 
 * This file is part of avg_q.
 * 
 * avg_q is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * avg_q is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with avg_q.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _GROWING_BUF_H
#define _GROWING_BUF_H

#ifndef Bool
#define Bool int
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

typedef struct {
 char *buffer_start;
 char *buffer_end;	/* Points one character past the end of the buffer */
 long current_length;
 Bool can_be_freed;

 char *delimiters;
 char delim_protector;
 char *current_token;
 Bool have_token;
 int nr_of_tokens;
} growing_buf;

extern void growing_buf_clear(growing_buf *buf);
extern void growing_buf_init(growing_buf *buf);
extern Bool growing_buf_allocate(growing_buf *buf, long length);
extern Bool growing_buf_takewithlength(growing_buf *buf, char const *str, long length);
extern Bool growing_buf_takethis(growing_buf *buf, char const *str);
extern Bool growing_buf_append(growing_buf *buf, char const *from, int length);
extern Bool growing_buf_appendstring(growing_buf *buf, char const *str);
extern Bool growing_buf_appendchar(growing_buf *buf, char const c);
extern void growing_buf_free(growing_buf *buf);

extern void growing_buf_settothis(growing_buf *buf, char *str);

extern Bool growing_buf_firsttoken(growing_buf *buf);
extern Bool growing_buf_firstsingletoken(growing_buf *buf);
extern Bool growing_buf_nexttoken(growing_buf *buf);
extern Bool growing_buf_nextsingletoken(growing_buf *buf);

extern void growing_buf_read_line(FILE *infile, growing_buf *buf);
#endif
