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

#ifndef YY_PERF_PMU_STORAGE_WORKSAPCE_BANANA_DEVEL_BANANA_PI_R4_OPENWRT_GITHUB_DEVEL_BPI_R4_OPENWRT_V21_02_MASTER_DEVEL_BUILT_BUILD_DIR_TARGET_AARCH64_CORTEX_A53_MUSL_LINUX_MEDIATEK_MT7988_LINUX_5_4_260_TOOLS_PERF_TARGET_AARCH64_CORTEX_A53_MUSL_UTIL_PMU_BISON_H_INCLUDED
# define YY_PERF_PMU_STORAGE_WORKSAPCE_BANANA_DEVEL_BANANA_PI_R4_OPENWRT_GITHUB_DEVEL_BPI_R4_OPENWRT_V21_02_MASTER_DEVEL_BUILT_BUILD_DIR_TARGET_AARCH64_CORTEX_A53_MUSL_LINUX_MEDIATEK_MT7988_LINUX_5_4_260_TOOLS_PERF_TARGET_AARCH64_CORTEX_A53_MUSL_UTIL_PMU_BISON_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int perf_pmu_debug;
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
    PP_CONFIG = 258,               /* PP_CONFIG  */
    PP_CONFIG1 = 259,              /* PP_CONFIG1  */
    PP_CONFIG2 = 260,              /* PP_CONFIG2  */
    PP_VALUE = 261,                /* PP_VALUE  */
    PP_ERROR = 262                 /* PP_ERROR  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 30 "util/pmu.y"

	unsigned long num;
	DECLARE_BITMAP(bits, PERF_PMU_FORMAT_BITS);

#line 76 "/storage/worksapce/Banana-Devel/Banana-Pi-R4/openwrt-github-devel/BPI-R4-OPENWRT-V21.02-Master-Devel_built/build_dir/target-aarch64_cortex-a53_musl/linux-mediatek_mt7988/linux-5.4.260/tools/perf-target-aarch64_cortex-a53_musl/util/pmu-bison.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE perf_pmu_lval;

int perf_pmu_parse (struct list_head *format, char *name);

#endif /* !YY_PERF_PMU_STORAGE_WORKSAPCE_BANANA_DEVEL_BANANA_PI_R4_OPENWRT_GITHUB_DEVEL_BPI_R4_OPENWRT_V21_02_MASTER_DEVEL_BUILT_BUILD_DIR_TARGET_AARCH64_CORTEX_A53_MUSL_LINUX_MEDIATEK_MT7988_LINUX_5_4_260_TOOLS_PERF_TARGET_AARCH64_CORTEX_A53_MUSL_UTIL_PMU_BISON_H_INCLUDED  */
