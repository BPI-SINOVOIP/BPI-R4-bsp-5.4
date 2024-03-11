/* A Bison parser, made by GNU Bison 3.7.4.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2020 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_PARSE_EVENTS_STORAGE_WORKSAPCE_BANANA_DEVEL_BANANA_PI_R4_OPENWRT_GITHUB_DEVEL_BPI_R4_OPENWRT_V21_02_MASTER_DEVEL_BUILT_BUILD_DIR_TARGET_AARCH64_CORTEX_A53_MUSL_LINUX_MEDIATEK_MT7988_LINUX_5_4_260_TOOLS_PERF_TARGET_AARCH64_CORTEX_A53_MUSL_UTIL_PARSE_EVENTS_BISON_H_INCLUDED
# define YY_PARSE_EVENTS_STORAGE_WORKSAPCE_BANANA_DEVEL_BANANA_PI_R4_OPENWRT_GITHUB_DEVEL_BPI_R4_OPENWRT_V21_02_MASTER_DEVEL_BUILT_BUILD_DIR_TARGET_AARCH64_CORTEX_A53_MUSL_LINUX_MEDIATEK_MT7988_LINUX_5_4_260_TOOLS_PERF_TARGET_AARCH64_CORTEX_A53_MUSL_UTIL_PARSE_EVENTS_BISON_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int parse_events_debug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    PE_START_EVENTS = 258,         /* PE_START_EVENTS  */
    PE_START_TERMS = 259,          /* PE_START_TERMS  */
    PE_VALUE = 260,                /* PE_VALUE  */
    PE_VALUE_SYM_HW = 261,         /* PE_VALUE_SYM_HW  */
    PE_VALUE_SYM_SW = 262,         /* PE_VALUE_SYM_SW  */
    PE_RAW = 263,                  /* PE_RAW  */
    PE_TERM = 264,                 /* PE_TERM  */
    PE_VALUE_SYM_TOOL = 265,       /* PE_VALUE_SYM_TOOL  */
    PE_EVENT_NAME = 266,           /* PE_EVENT_NAME  */
    PE_NAME = 267,                 /* PE_NAME  */
    PE_BPF_OBJECT = 268,           /* PE_BPF_OBJECT  */
    PE_BPF_SOURCE = 269,           /* PE_BPF_SOURCE  */
    PE_MODIFIER_EVENT = 270,       /* PE_MODIFIER_EVENT  */
    PE_MODIFIER_BP = 271,          /* PE_MODIFIER_BP  */
    PE_NAME_CACHE_TYPE = 272,      /* PE_NAME_CACHE_TYPE  */
    PE_NAME_CACHE_OP_RESULT = 273, /* PE_NAME_CACHE_OP_RESULT  */
    PE_PREFIX_MEM = 274,           /* PE_PREFIX_MEM  */
    PE_PREFIX_RAW = 275,           /* PE_PREFIX_RAW  */
    PE_PREFIX_GROUP = 276,         /* PE_PREFIX_GROUP  */
    PE_ERROR = 277,                /* PE_ERROR  */
    PE_PMU_EVENT_PRE = 278,        /* PE_PMU_EVENT_PRE  */
    PE_PMU_EVENT_SUF = 279,        /* PE_PMU_EVENT_SUF  */
    PE_KERNEL_PMU_EVENT = 280,     /* PE_KERNEL_PMU_EVENT  */
    PE_ARRAY_ALL = 281,            /* PE_ARRAY_ALL  */
    PE_ARRAY_RANGE = 282,          /* PE_ARRAY_RANGE  */
    PE_DRV_CFG_TERM = 283          /* PE_DRV_CFG_TERM  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 101 "util/parse-events.y"

	char *str;
	u64 num;
	struct list_head *head;
	struct parse_events_term *term;
	struct tracepoint_name {
		char *sys;
		char *event;
	} tracepoint_name;
	struct parse_events_array array;

#line 104 "/storage/worksapce/Banana-Devel/Banana-Pi-R4/openwrt-github-devel/BPI-R4-OPENWRT-V21.02-Master-Devel_built/build_dir/target-aarch64_cortex-a53_musl/linux-mediatek_mt7988/linux-5.4.260/tools/perf-target-aarch64_cortex-a53_musl/util/parse-events-bison.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif

/* Location type.  */
#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE YYLTYPE;
struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif



int parse_events_parse (void *_parse_state, void *scanner);

#endif /* !YY_PARSE_EVENTS_STORAGE_WORKSAPCE_BANANA_DEVEL_BANANA_PI_R4_OPENWRT_GITHUB_DEVEL_BPI_R4_OPENWRT_V21_02_MASTER_DEVEL_BUILT_BUILD_DIR_TARGET_AARCH64_CORTEX_A53_MUSL_LINUX_MEDIATEK_MT7988_LINUX_5_4_260_TOOLS_PERF_TARGET_AARCH64_CORTEX_A53_MUSL_UTIL_PARSE_EVENTS_BISON_H_INCLUDED  */
