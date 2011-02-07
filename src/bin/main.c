/* Copyright (C) 2009, 2010  Olga Yakovleva <yakovleva.o.v@gmail.com> */

/* This program is free software: you can redistribute it and/or modify */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation, either version 3 of the License, or */
/* (at your option) any later version. */

/* This program is distributed in the hope that it will be useful, */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the */
/* GNU General Public License for more details. */

/* You should have received a copy of the GNU General Public License */
/* along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include "RHVoice.h"

static int sample_rate;

static void write_wave_header()
{
  int byte_rate=2*sample_rate;
  unsigned char header[]={
    /* RIFF header */
    'R','I','F','F',            /* ChunkID */
    /* We cannot determine this number in advance */
    /* This is what espeak puts in this field when writing to stdout */
    0x24,0xf0,0xff,0x7f,        /* ChunkSize */
    'W','A','V','E',            /* Format */
    /* fmt */
    'f','m','t',' ',            /* Subchunk1ID */
    16,0,0,0,                 /* Subchunk1Size */
    1,0,                        /* AudioFormat (1=PCM) */
    1,0,                        /* NumChannels */
    0,0,0,0,                    /* SampleRate */
    0,0,0,0,                    /* ByteRate */
    2,0,                        /* BlockAlign */
    16,0,                       /* BitsPerSample */
    /* data */
    'd','a','t','a',          /* Subchunk2ID */
    /* Again using espeak as an example */
    0x00,0xf0,0xff,0x7f};     /* Subchunk2Size */
  /* Write actual sample rate */
  header[24]=sample_rate&0xff;
  header[25]=(sample_rate>>8)&0xff;
  header[26]=(sample_rate>>16)&0xff;
  header[27]=(sample_rate>>24)&0xff;
  /* Write actual byte rate */
  header[28]=byte_rate&0xff;
  header[29]=(byte_rate>>8)&0xff;
  header[30]=(byte_rate>>16)&0xff;
  header[31]=(byte_rate>>24)&0xff;
  fwrite(header,sizeof(header),1,stdout);
}

static int wave_header_written=0;

int static callback(const short *samples,int num_samples,const RHVoice_event *events,int num_events)
{
  if(!samples)
    return 1;
  if(!wave_header_written)
    {
      write_wave_header();
      wave_header_written=1;
    }
  fwrite(samples,sizeof(short)*num_samples,1,stdout);
  return 1;
}

static struct option program_options[]=
  {
    {"help",no_argument,0,'h'},
    {"version",no_argument,0,'V'},
    {"rate",required_argument,0,'r'},
    {"pitch",required_argument,0,'p'},
    {"volume",required_argument,0,'v'},
    {"voice-directory",required_argument,0,'d'},
    {"user-dict",required_argument,0,'u'},
    {"no-pseudo-english",no_argument,0,'R'},
    {"ssml",no_argument,0,'s'},
    {0,0,0,0}
  };

static void show_version()
{
  printf("%s %s\n",PACKAGE,RHVoice_get_version());
}

static void show_help()
{
  show_version();
  printf("a speech synthesizer for Russian language\n\n");
  printf("usage: RHVoice [options]\n"
         "reads text from stdin (expects UTF-8 encoding)\n"
         "writes speech output to stdout\n"
         "-h, --help                   print this help message and exit\n"
         "-V, --version                print the program version and exit\n"
         "-r, --rate=<number>          speech rate, default is 1.0\n"
         "-p, --pitch=<number>         speech pitch, default is 1.0\n"
         "-v, --volume=<number>        speech volume, default is 1.0\n"
         "-d, --voice-directory=<path> path to voice files\n"
         "-u, --user-dict=<path>       path to the user dictionary\n"
         "-R, --no-pseudo-english      do not use pseudo-English pronunciation\n"
         "-s, --ssml                   ssml input\n");
}

int main(int argc,char **argv)
{
  const char *voxdir=VOXDIR;
  RHVoice_message msg;
  const char *dictpath=NULL;
  float rate=1.0;
  float pitch=1.0;
  float volume=1.0;
  int pseudo_english=1;
  int is_ssml=0;
  char *text;
  int size=1000;
  int c;
  int i;
  while((c=getopt_long(argc,argv,"d:u:hVr:p:v:Rs",program_options,&i))!=-1)
    {
      switch(c)
        {
        case 'V':
          show_version();
          return 0;
        case 'h':
          show_help();
          return 0;
        case 'r':
          rate=strtof(optarg,NULL);
          if(rate<=0.0)
            rate=1.0;
          break;
        case 'p':
          pitch=strtof(optarg,NULL);
          if(pitch<=0.0)
            pitch=1.0;
          break;
        case 'v':
          volume=strtof(optarg,NULL);
          if(volume<=0.0)
            volume=1.0;
          break;
        case 'R':
          pseudo_english=0;
          break;
        case 's':
          is_ssml=1;
          break;
        case 'd':
          voxdir=optarg;
          break;
        case 'u':
          dictpath=optarg;
          break;
        case 0:
          break;
        default:
          return 1;
        }
    }
  text=(char*)malloc(size*sizeof(char));
  if(text==NULL)
    return 1;
  for(i=0,c=getc(stdin);c!=EOF;c=getc(stdin),i++)
    {
      if((i+1)==size)
        {
          size+=1000;
          text=(char*)realloc(text,size*sizeof(char));
          if(text==NULL)
            return 1;
        }
      text[i]=c;
    }
  if(i==0)
    {
      free(text);
      return 0;
    }
  text[i]='\0';
  sample_rate=RHVoice_initialize(voxdir,callback);
  if(sample_rate==0)
    {
      free(text);
      return 1;
    }
  msg=RHVoice_new_message_utf8((const uint8_t*)text,i,is_ssml);
  free(text);
  if(msg==NULL)
    {
      RHVoice_terminate();
      return 1;
    }
  RHVoice_speak(msg);
  RHVoice_delete_message(msg);
  RHVoice_terminate();
  return 0;
}