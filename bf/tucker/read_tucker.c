/*
 * Copyright (C) 1996-1999,2001-2004,2006-2008,2010 Bernd Feige
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
/*{{{}}}*/
/*{{{  Description*/
/*
 * read_tucker.c module to read data from the Tucker format that is
 * used with the `Geodesic Net' 128-electrode system.
 *	-- Bernd Feige 16.12.1996
 */
/*}}}  */

/*{{{  Includes*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h> 
#ifdef __GNUC__
#include <unistd.h>
#endif
#include <ctype.h>
#include <read_struct.h>
#include <Intel_compat.h>
#include "transform.h"
#include "bf.h"
#include "tucker.h"
/*}}}  */

#define MAX_COMMENTLEN 1024
#define MAX_BUFFERED_TRIGGERS 3

enum ARGS_ENUM {
 ARGS_CONTINUOUS=0, 
 ARGS_TRIGFILE,
 ARGS_TRIGLIST, 
 ARGS_FROMEPOCH,
 ARGS_EPOCHS,
 ARGS_OFFSET,
 ARGS_IFILE,
 ARGS_BEFORETRIG,
 ARGS_AFTERTRIG,
 NR_OF_ARGUMENTS
};
LOCAL transform_argument_descriptor argument_descriptors[NR_OF_ARGUMENTS]={
 {T_ARGS_TAKES_NOTHING, "Continuous mode. Read the file in chunks of the given size without triggers", "c", FALSE, NULL},
 {T_ARGS_TAKES_FILENAME, "trigger_file: Read trigger points and codes from this file", "R", ARGDESC_UNUSED, (const char *const *)"*.trg"},
 {T_ARGS_TAKES_STRING_WORD, "trigger_list: Restrict epochs to these condition codes. Eg: 1,2", "t", ARGDESC_UNUSED, NULL},
 {T_ARGS_TAKES_LONG, "fromepoch: Specify start epoch beginning with 1", "f", 1, NULL},
 {T_ARGS_TAKES_LONG, "epochs: Specify maximum number of epochs to get", "e", 1, NULL},
 {T_ARGS_TAKES_STRING_WORD, "offset: The zero point 'beforetrig' is shifted by offset", "o", ARGDESC_UNUSED, NULL},
 {T_ARGS_TAKES_FILENAME, "Input file", "", ARGDESC_UNUSED, NULL},
 {T_ARGS_TAKES_STRING_WORD, "beforetrig", "", ARGDESC_UNUSED, (const char *const *)"1s"},
 {T_ARGS_TAKES_STRING_WORD, "aftertrig", "", ARGDESC_UNUSED, (const char *const *)"1s"},
};

/*{{{  Definition of read_tucker_storage*/
struct read_tucker_storage {
 struct tucker_header header;
 FILE *infile;
 FILE *triggerfile;
 int *trigcodes;
 unsigned short *last_trigvalues;
 int current_trigger;
 int next_buffered_trigger;	/* Contains the index of the next buffered trigger */
 int after_last_buffered_trigger;	/* Contains the number of buffered triggers */
 struct trigger buffered_triggers[MAX_BUFFERED_TRIGGERS];
 long current_point;
 long current_triggerpoint;
 char *EventCodes;
 long SizeofHeader;
 long NumSamples;
 long bytes_per_point;
 long beforetrig;
 long aftertrig;
 long offset;
 long fromepoch;
 long epochs;
 unsigned short *buffer;

 DATATYPE Factor;
};
/*}}}  */

/*{{{  Single point interface*/

/*{{{  read_tucker_get_filestrings: Allocate and set strings and probepos array*/
LOCAL void
read_tucker_get_filestrings(transform_info_ptr tinfo) {
 struct read_tucker_storage *local_arg=(struct read_tucker_storage *)tinfo->methods->local_storage;
 transform_argument *args=tinfo->methods->arguments;
 char const *last_sep=strrchr(args[ARGS_IFILE].arg.s, PATHSEP);
 char const *last_component=(last_sep==NULL ? args[ARGS_IFILE].arg.s : last_sep+1);

 if ((tinfo->comment=(char *)malloc(MAX_COMMENTLEN))==NULL) {
  ERREXIT(tinfo->emethods, "read_tucker: Error allocating comment\n");
 }
 snprintf(tinfo->comment, MAX_COMMENTLEN, "read_tucker %s %02d/%02d/%04d,%02d:%02d:%02d", last_component, local_arg->header.Month, local_arg->header.Day, local_arg->header.Year, local_arg->header.Hour, local_arg->header.Minute, local_arg->header.Sec);

 /* Force create_channelgrid to really allocate the channel info anew.
  * Otherwise, free_tinfo will free someone else's data ! */
 tinfo->channelnames=NULL; tinfo->probepos=NULL;
 create_channelgrid(tinfo); /* Create defaults for any missing channel info */
}
/*}}}  */

/*{{{  read_tucker_seek_point(transform_info_ptr tinfo, long point) {*/
/* This function not only seeks to the specified point, but also reads
 * the point into the buffer if necessary. The get_singlepoint function
 * then only transfers the data. */
LOCAL void
read_tucker_seek_point(transform_info_ptr tinfo, long point) {
 struct read_tucker_storage *local_arg=(struct read_tucker_storage *)tinfo->methods->local_storage;

 fseek(local_arg->infile,point*local_arg->bytes_per_point+local_arg->SizeofHeader,SEEK_SET);
 if ((int)fread(local_arg->buffer,1,local_arg->bytes_per_point,local_arg->infile)!=local_arg->bytes_per_point) {
  ERREXIT(tinfo->emethods, "read_tucker_seek_point: Error reading data\n");
 }
 /*{{{  Swap byte order if necessary*/
# ifdef LITTLE_ENDIAN
 {unsigned short *pdata=local_arg->buffer, *p_end=(unsigned short *)((char *)local_arg->buffer+local_arg->bytes_per_point);
  while (pdata<p_end) Intel_short((short unsigned int *)pdata++);
 }
# endif
 /*}}}  */
 local_arg->current_point=point;
}
/*}}}  */

/*{{{  read_tucker_get_singlepoint(transform_info_ptr tinfo, array *toarray) {*/
LOCAL int
read_tucker_get_singlepoint(transform_info_ptr tinfo, array *toarray) {
 struct read_tucker_storage *local_arg=(struct read_tucker_storage *)tinfo->methods->local_storage;
 unsigned short *pdata=local_arg->buffer;

 if (local_arg->current_point>=local_arg->NumSamples) return -1;
 read_tucker_seek_point(tinfo, local_arg->current_point);

 do {
  array_write(toarray, local_arg->Factor*(*pdata++));
 } while (toarray->message==ARRAY_CONTINUE);

 local_arg->current_point++;
 return 0;
}
/*}}}  */

/*{{{  read_tucker_nexttrigger(transform_info_ptr tinfo, long *trigpoint) {*/
/*{{{  Maintaining the buffered triggers list*/
LOCAL void 
read_tucker_reset_triggerbuffer(transform_info_ptr tinfo) {
 struct read_tucker_storage *local_arg=(struct read_tucker_storage *)tinfo->methods->local_storage;
 local_arg->next_buffered_trigger=local_arg->after_last_buffered_trigger=0;
}

LOCAL void
read_tucker_push_trigger(transform_info_ptr tinfo, long position, int code) {
 struct read_tucker_storage *local_arg=(struct read_tucker_storage *)tinfo->methods->local_storage;
 if (code==0) return;	/* Triggers with code 0 are not pushed */
 if (local_arg->after_last_buffered_trigger>MAX_BUFFERED_TRIGGERS) {
  ERREXIT(tinfo->emethods, "read_tucker_push_trigger: after_last_buffered_trigger>MAX_BUFFERED_TRIGGERS\n");
 }
 local_arg->buffered_triggers[local_arg->after_last_buffered_trigger].position= position;
 local_arg->buffered_triggers[local_arg->after_last_buffered_trigger].code    = code;
 local_arg->after_last_buffered_trigger++;
}

LOCAL int
read_tucker_pop_trigger(transform_info_ptr tinfo, long *position) {
 struct read_tucker_storage *local_arg=(struct read_tucker_storage *)tinfo->methods->local_storage;
 int code;

 if (local_arg->next_buffered_trigger>=local_arg->after_last_buffered_trigger) {
  read_tucker_reset_triggerbuffer(tinfo);
  return 0;
 }
 if (local_arg->next_buffered_trigger>=MAX_BUFFERED_TRIGGERS) {
  ERREXIT(tinfo->emethods, "read_tucker_pop_trigger: next_buffered_trigger>=MAX_BUFFERED_TRIGGERS\n");
 }
 *position=local_arg->buffered_triggers[local_arg->next_buffered_trigger].position;
  code    =local_arg->buffered_triggers[local_arg->next_buffered_trigger].code;
 local_arg->next_buffered_trigger++;
 return code;
}
/*}}}  */

/* Buffering of triggers takes place because multiple condition codes 
 * may occur at one time
 * (from Stimulation, Keypad and Keyboard or, in the case of event tables,
 * just multiple events registered with the same time stamp). This way,
 * the events always occur separately from each other from read_tucker' point
 * of view, which of course makes it impossible to look for specific 
 * combinations of events (which would be unlikely anyway, however). */
LOCAL int
read_tucker_nexttrigger(transform_info_ptr tinfo, long *trigpoint) {
 struct read_tucker_storage *local_arg=(struct read_tucker_storage *)tinfo->methods->local_storage;
 int code;
 int eventno;
 unsigned short *pdata;

 /* Return buffered trigger if available */
 code=read_tucker_pop_trigger(tinfo, trigpoint);
 if (code!=0) {
  return code;
 }

 if (local_arg->triggerfile!=NULL) {
  code=read_trigger_from_trigfile(local_arg->triggerfile, tinfo->sfreq, trigpoint, NULL);
  return code;
 }

 while (local_arg->current_triggerpoint<local_arg->NumSamples && code==0) {
  read_tucker_seek_point(tinfo, local_arg->current_triggerpoint);
  pdata=local_arg->buffer+local_arg->header.NChan;

  for (eventno=0; eventno<local_arg->header.NEvents; eventno++) {
   /* Trigger on the first non-zero point */
   if (local_arg->last_trigvalues[eventno]==0 && pdata[eventno]!=0) {
    code=eventno+1;
    read_tucker_push_trigger(tinfo, local_arg->current_triggerpoint, code);
   }
   local_arg->last_trigvalues[eventno]=pdata[eventno];
  }
  local_arg->current_triggerpoint++;
 }

 /* If all of the pushed codes were 0, then this will yield 0 (buffer empty): */
 return read_tucker_pop_trigger(tinfo, trigpoint);
}
/*}}}  */
/*}}}  */

/*{{{  read_tucker_init(transform_info_ptr tinfo) {*/
METHODDEF void
read_tucker_init(transform_info_ptr tinfo) {
 struct read_tucker_storage *local_arg=(struct read_tucker_storage *)tinfo->methods->local_storage;
 transform_argument *args=tinfo->methods->arguments;
 struct stat statbuff;

 /*{{{  Process options*/
 local_arg->fromepoch=(args[ARGS_FROMEPOCH].is_set ? args[ARGS_FROMEPOCH].arg.i : 1);
 local_arg->epochs=(args[ARGS_EPOCHS].is_set ? args[ARGS_EPOCHS].arg.i : -1);
 /*}}}  */

 if((local_arg->infile=fopen(args[ARGS_IFILE].arg.s,"rb"))==NULL) {
  ERREXIT1(tinfo->emethods, "read_tucker_init: Can't open file %s\n", MSGPARM(args[ARGS_IFILE].arg.s));
 }
 read_struct((char *)&local_arg->header, sm_tucker, local_arg->infile);
#ifdef LITTLE_ENDIAN
 change_byteorder((char *)&local_arg->header, sm_tucker);
#endif
 if (local_arg->header.NEvents>0) {
  if ((local_arg->EventCodes=(char *)malloc(local_arg->header.NEvents*4))==NULL) {
   ERREXIT(tinfo->emethods, "read_tucker_init: Error allocating event code table\n");
  }
  if ((int)fread((void *)local_arg->EventCodes, 4, local_arg->header.NEvents, local_arg->infile)!=local_arg->header.NEvents) {
   ERREXIT(tinfo->emethods, "read_tucker_init: Error reading event code table\n");
  }
  if ((local_arg->last_trigvalues=(unsigned short *)calloc(local_arg->header.NEvents, sizeof(unsigned short)))==NULL) {
   ERREXIT(tinfo->emethods, "read_tucker_init: Error allocating event memory\n");
  }
 } else {
  local_arg->EventCodes=NULL;
  local_arg->last_trigvalues=NULL;
  if (!args[ARGS_TRIGFILE].is_set && !args[ARGS_CONTINUOUS].is_set) {
   ERREXIT(tinfo->emethods, "read_tucker_init: No event trace present!\n");
  }
 }
 local_arg->SizeofHeader = ftell(local_arg->infile);

 local_arg->bytes_per_point=(local_arg->header.NChan+local_arg->header.NEvents)*sizeof(short);
 fstat(fileno(local_arg->infile),&statbuff);
 local_arg->NumSamples = (statbuff.st_size-local_arg->SizeofHeader)/local_arg->bytes_per_point;
 tinfo->points_in_file=local_arg->NumSamples;
 local_arg->Factor=((DATATYPE)local_arg->header.Range)/(1<<local_arg->header.Bits);
 if ((local_arg->buffer=(unsigned short *)malloc(local_arg->bytes_per_point))==NULL) {
  ERREXIT(tinfo->emethods, "read_tucker_init: Error allocating buffer memory\n");
 }
 
 tinfo->sfreq=local_arg->header.SampRate;
 /*{{{  Parse arguments that can be in seconds*/
 local_arg->beforetrig=tinfo->beforetrig=gettimeslice(tinfo, args[ARGS_BEFORETRIG].arg.s);
 local_arg->aftertrig=tinfo->aftertrig=gettimeslice(tinfo, args[ARGS_AFTERTRIG].arg.s);
 local_arg->offset=(args[ARGS_OFFSET].is_set ? gettimeslice(tinfo, args[ARGS_OFFSET].arg.s) : 0);
 /*}}}  */

 if (local_arg->beforetrig==0 && local_arg->aftertrig==0) {
  if (args[ARGS_CONTINUOUS].is_set) {
   /* Read the whole file as one epoch: */
   local_arg->aftertrig=local_arg->NumSamples;
   TRACEMS1(tinfo->emethods, 1, "read_tucker_init: Reading ALL data (%d points)\n", MSGPARM(local_arg->aftertrig));
  } else {
   ERREXIT(tinfo->emethods, "read_tucker_init: Zero epoch length.\n");
  }
 }
 if (args[ARGS_TRIGFILE].is_set) {
  if (strcmp(args[ARGS_TRIGFILE].arg.s,"stdin")==0) {
   local_arg->triggerfile=stdin;
  } else
  if ((local_arg->triggerfile=fopen(args[ARGS_TRIGFILE].arg.s, "r"))==NULL) {
   ERREXIT1(tinfo->emethods, "read_tucker_init: Can't open trigger file >%s<\n", MSGPARM(args[ARGS_TRIGFILE].arg.s));
  }
 } else {
  local_arg->triggerfile=NULL;
 }
 if (args[ARGS_TRIGLIST].is_set) {
  growing_buf buf;
  Bool havearg;
  int trigno=0;

  growing_buf_init(&buf);
  growing_buf_takethis(&buf, args[ARGS_TRIGLIST].arg.s);
  buf.delimiters=",";

  havearg=growing_buf_firsttoken(&buf);
  if ((local_arg->trigcodes=(int *)malloc((buf.nr_of_tokens+1)*sizeof(int)))==NULL) {
   ERREXIT(tinfo->emethods, "read_tucker_init: Error allocating triglist memory\n");
  }
  while (havearg) {
   local_arg->trigcodes[trigno]=atoi(buf.current_token);
   havearg=growing_buf_nexttoken(&buf);
   trigno++;
  }
  local_arg->trigcodes[trigno]=0;   /* End mark */
  growing_buf_free(&buf);
 } else {
  local_arg->trigcodes=NULL;
 }

 read_tucker_reset_triggerbuffer(tinfo);
 local_arg->current_trigger=0;
 local_arg->current_point=0;
 local_arg->current_triggerpoint=0;

 tinfo->methods->init_done=TRUE;
}
/*}}}  */

/*{{{  read_tucker(transform_info_ptr tinfo) {*/
METHODDEF DATATYPE *
read_tucker(transform_info_ptr tinfo) {
 struct read_tucker_storage *local_arg=(struct read_tucker_storage *)tinfo->methods->local_storage;
 transform_argument *args=tinfo->methods->arguments;
 array myarray;
 FILE *infile=local_arg->infile;
 Bool not_correct_trigger=FALSE;
 long trigger_point, file_start_point, file_end_point;

 if (local_arg->epochs--==0) return NULL;
 tinfo->beforetrig=local_arg->beforetrig;
 tinfo->aftertrig=local_arg->aftertrig;
 tinfo->nr_of_points=local_arg->beforetrig+local_arg->aftertrig;
 tinfo->nr_of_channels=local_arg->header.NChan;
 tinfo->nrofaverages=1;
 if (tinfo->nr_of_points<=0) {
  ERREXIT1(tinfo->emethods, "read_tucker: Invalid nr_of_points %d\n", MSGPARM(tinfo->nr_of_points));
 }

 /*{{{  Find the next window that fits into the actual data*/
 /* This is just for the Continuous option (no trigger file): */
 file_end_point=local_arg->current_point-1;
 do {
  if (args[ARGS_CONTINUOUS].is_set) {
   /* Simulate a trigger at current_point+beforetrig */
   file_start_point=file_end_point+1;
   trigger_point=file_start_point+tinfo->beforetrig;
   file_end_point=trigger_point+tinfo->aftertrig-1;
   if (file_end_point>=local_arg->NumSamples) return NULL;
   local_arg->current_trigger++;
   local_arg->current_point+=tinfo->nr_of_points;
   tinfo->condition=0;
  } else 
  do {
   tinfo->condition=read_tucker_nexttrigger(tinfo, &trigger_point);
   if (tinfo->condition==0) return NULL;	/* No more triggers in file */
   file_start_point=trigger_point-tinfo->beforetrig+local_arg->offset;
   file_end_point=trigger_point+tinfo->aftertrig-1-local_arg->offset;
   
   if (local_arg->trigcodes==NULL) {
    not_correct_trigger=FALSE;
   } else {
    int trigno=0;
    not_correct_trigger=TRUE;
    while (local_arg->trigcodes[trigno]!=0) {
     if (local_arg->trigcodes[trigno]==tinfo->condition) {
      not_correct_trigger=FALSE;
      break;
     }
     trigno++;
    }
   }
   local_arg->current_trigger++;
  } while (not_correct_trigger || file_start_point<0 || file_end_point>=local_arg->NumSamples);
 } while (--local_arg->fromepoch>0);
 TRACEMS3(tinfo->emethods, 1, "read_tucker: Reading around tag %d at %d, condition=%d\n", MSGPARM(local_arg->current_trigger), MSGPARM(trigger_point), MSGPARM(tinfo->condition));
 fseek(infile, file_start_point*local_arg->bytes_per_point, SEEK_SET);

 /*{{{  Configure myarray*/
 myarray.element_skip=tinfo->itemsize=1;
 myarray.nr_of_elements=tinfo->nr_of_channels;
 myarray.nr_of_vectors=tinfo->nr_of_points;
 if (array_allocate(&myarray)==NULL) {
  ERREXIT(tinfo->emethods, "read_tucker: Error allocating data\n");
 }
 /* Byte order is multiplexed, ie channels fastest */
 tinfo->multiplexed=TRUE;
 /*}}}  */

 read_tucker_get_filestrings(tinfo);
 read_tucker_seek_point(tinfo, file_start_point);
 do {
  read_tucker_get_singlepoint(tinfo, &myarray);
 } while (myarray.message!=ARRAY_ENDOFSCAN);

 tinfo->z_label=NULL;
 tinfo->sfreq=local_arg->header.SampRate;
 tinfo->tsdata=myarray.start;
 tinfo->length_of_output_region=tinfo->nr_of_channels*tinfo->nr_of_points;
 tinfo->leaveright=0;
 tinfo->data_type=TIME_DATA;

 return tinfo->tsdata;
}
/*}}}  */

/*{{{  read_tucker_exit(transform_info_ptr tinfo) {*/
METHODDEF void
read_tucker_exit(transform_info_ptr tinfo) {
 struct read_tucker_storage *local_arg=(struct read_tucker_storage *)tinfo->methods->local_storage;

 if (local_arg->triggerfile!=NULL) {
  if (local_arg->triggerfile!=stdin) fclose(local_arg->triggerfile);
  local_arg->triggerfile=NULL;
 }
 fclose(local_arg->infile);
 free_pointer((void **)&local_arg->trigcodes);
 free_pointer((void **)&local_arg->EventCodes);
 free_pointer((void **)&local_arg->last_trigvalues);

 tinfo->methods->init_done=FALSE;
}
/*}}}  */

/*{{{  select_read_tucker(transform_info_ptr tinfo) {*/
GLOBAL void
select_read_tucker(transform_info_ptr tinfo) {
 tinfo->methods->transform_init= &read_tucker_init;
 tinfo->methods->transform= &read_tucker;
 tinfo->methods->transform_exit= &read_tucker_exit;
 tinfo->methods->get_singlepoint= &read_tucker_get_singlepoint;
 tinfo->methods->seek_point= &read_tucker_seek_point;
 tinfo->methods->get_filestrings= &read_tucker_get_filestrings;
 tinfo->methods->method_type=GET_EPOCH_METHOD;
 tinfo->methods->method_name="read_tucker";
 tinfo->methods->method_description=
  "Get-epoch method to read binary files in the `Tucker' format used with the"
  " `Geodesic Net' EEG system.\n";
 tinfo->methods->local_storage_size=sizeof(struct read_tucker_storage);
 tinfo->methods->nr_of_arguments=NR_OF_ARGUMENTS;
 tinfo->methods->argument_descriptors=argument_descriptors;
}
/*}}}  */
