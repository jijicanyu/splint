/*
** LCLint - annotation-assisted static program checker
** Copyright (C) 1994-2000 University of Virginia,
**         Massachusetts Institute of Technology
**
** This program is free software; you can redistribute it and/or modify it
** under the terms of the GNU General Public License as published by the
** Free Software Foundation; either version 2 of the License, or (at your
** option) any later version.
** 
** This program is distributed in the hope that it will be useful, but
** WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** General Public License for more details.
** 
** The GNU General Public License is available from http://www.gnu.org/ or
** the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
** MA 02111-1307, USA.
**
** For information on lclint: lclint-request@cs.virginia.edu
** To report a bug: lclint-bug@cs.virginia.edu
** For more information: http://lclint.cs.virginia.edu
*/
/*
** llmain.c
**
** Main module for LCLint checker
*/

# include <signal.h>

/*
** Ensure that WIN32 and _WIN32 are both defined or both undefined.
*/

# ifdef WIN32
# ifndef _WIN32
# error "Inconsistent definitions."
# endif
# else
# ifdef _WIN32
# error "Inconsistent definitions."
# endif
# endif

# ifdef WIN32
# include <windows.h>
# include <process.h>
# endif

# include "lclintMacros.nf"
# include "llbasic.h"
# include "osd.h"

# ifndef NOLCL
# include "gram.h"
# include "lclscan.h"
# include "scanline.h"
# include "lclscanline.h"
# include "lclsyntable.h"
# include "lcltokentable.h"
# include "lslparse.h"
# include "scan.h"
# include "syntable.h"
# include "tokentable.h"
# include "lslinit.h"
# include "lclinit.h"
# include "lh.h"
# include "imports.h"
# endif

# include "version.h"
# include "herald.h"
# include "fileIdList.h"
# include "lcllib.h"
# include "cgrammar.h"
# include "llmain.h"
# include "portab.h"
# include "cpp.h"
# include <time.h>

extern /*@external@*/ int yydebug;

static void printMail (void);
static void printMaintainer (void);
static void printReferences (void);
static void printFlags (void);
static void printAnnotations (void);
static void printParseErrors (void);
static void printComments (void);
static void describePrefixCodes (void);
static void cleanupFiles (void);
static void showHelp (void);
static void interrupt (int p_i);
static void loadrc (FILE *p_rcfile, cstringSList *p_passThroughArgs);
static void describeVars (void);
static bool specialFlagsHelp (char *p_next);
static bool hasShownHerald = FALSE;

static bool anylcl = FALSE;
static clock_t inittime;

static /*@only@*/ /*@null@*/ tsource *initFile = (tsource *) 0;

static fileIdList preprocessFiles (fileIdList)
  /*@modifies fileSystem@*/ ;

# ifndef NOLCL

static
void lslCleanup (void)
   /*@globals killed g_symtab@*/
   /*@modifies internalState, g_symtab@*/
{
  /*
  ** Cleanup all the LCL/LSL.
  */

  static bool didCleanup = FALSE;

  llassert (!didCleanup);
  llassert (anylcl);

  didCleanup = TRUE;

  lsymbol_destroyMod ();
  LCLSynTableCleanup ();
  LCLTokenTableCleanup ();
  LCLScanLineCleanup ();
  LCLScanCleanup ();

  /* clean up LSL parsing */

  lsynTableCleanup ();
  ltokenTableCleanup ();
  lscanLineCleanup ();
  LSLScanCleanup ();

  symtable_free (g_symtab);
  sort_destroyMod (); 
}

static
  void lslInit (void)
  /*@globals undef g_symtab; @*/
  /*@modifies g_symtab, internalState, fileSystem; @*/
{
  /*
  ** Open init file provided by user, or use the default LCL init file 
  */
  
  cstring larchpath = context_getLarchPath ();
  tsource *LSLinitFile = (tsource *) 0;

  setCodePoint ();

  if (initFile == (tsource *) 0)
    {
      initFile = tsource_create (INITFILENAME, LCLINIT_SUFFIX, FALSE);
      
      if (!tsource_getPath (cstring_toCharsSafe (larchpath), initFile))
	{
	  lldiagmsg (message ("Continuing without LCL init file: %s",
			      cstring_fromChars (tsource_fileName (initFile))));
	}
      else 
	{
	  if (!tsource_open (initFile))
	    {
	      lldiagmsg (message ("Continuing without LCL init file: %s",
				  cstring_fromChars (tsource_fileName (initFile))));
	    }
	}
    }
  else 
    {
      if (!tsource_open (initFile))
	{
	  lldiagmsg (message ("Continuing without LCL init file: %s",
			      cstring_fromChars (tsource_fileName (initFile))));
	}
    }

  /* Initialize checker */

  lsymbol_initMod ();
  LCLSynTableInit ();

  setCodePoint ();

  LCLSynTableReset ();
  LCLTokenTableInit ();

  setCodePoint ();

  LCLScanLineInit ();
  setCodePoint ();
  LCLScanLineReset ();
  setCodePoint ();
  LCLScanInit ();

  setCodePoint ();

  /* need this to initialize LCL checker */
  llassert (initFile != NULL);
      
  if (tsource_isOpen (initFile))
    {
      setCodePoint ();

      LCLScanReset (initFile);
      LCLProcessInitFileInit ();
      LCLProcessInitFileReset ();

      setCodePoint ();
      LCLProcessInitFile ();
      LCLProcessInitFileCleanup ();

      setCodePoint ();
      check (tsource_close (initFile));
    }
  
  /* Initialize LSL init files, for parsing LSL signatures from LSL */
  
  LSLinitFile = tsource_create ("lslinit.lsi", ".lsi", FALSE);
  
  if (!tsource_getPath (cstring_toCharsSafe (larchpath), LSLinitFile))
    {
      lldiagmsg (message ("Continuing without LSL init file: %s",
			  cstring_fromChars (tsource_fileName (LSLinitFile))));
    }
  else 
    {
      if (!tsource_open (LSLinitFile))
	{
	  lldiagmsg (message ("Continuing without LSL init file: %s",
			      cstring_fromChars (tsource_fileName (LSLinitFile))));
	}
    }
      
  setCodePoint ();
  lsynTableInit ();
  lsynTableReset ();

  setCodePoint ();
  ltokenTableInit ();

  setCodePoint ();
  lscanLineInit ();
  lscanLineReset ();
  LSLScanInit ();

  if (tsource_isOpen (LSLinitFile))
    {
      setCodePoint ();
      LSLScanReset (LSLinitFile);
      LSLProcessInitFileInit ();
      setCodePoint ();
      LSLProcessInitFile ();
      setCodePoint ();
      check (tsource_close (LSLinitFile));
    }
      
  tsource_free (LSLinitFile);
  
  if (lclHadError ())
    {
      lclplainerror 
	(cstring_makeLiteral ("LSL init file error.  Attempting to continue."));
    }
  
  setCodePoint ();
  g_symtab = symtable_new ();
  
  /* 
  ** sort_init must come after symtab has been initialized 
  */
  sort_init ();
  abstract_init ();
  setCodePoint ();
  
  inittime = clock ();
  
  /* 
  ** Equivalent to importing old spec_csupport.lcl
  ** define immutable LCL type "bool" and bool constants TRUE and FALSE
  ** and initialized them to be equal to LSL's "true" and "false".
  **
  ** Reads in CTrait.syms (derived from CTrait.lsl) on LARCH_PATH.
  */
      
  LCLBuiltins (); 
  LCLReportEolTokens (FALSE);
}

static void
lslProcess (fileIdList lclfiles)
   /*@globals undef g_currentSpec, undef g_currentSpecName, g_currentloc,
              undef killed g_symtab; @*/
   /*@modifies g_currentSpec, g_currentSpecName, g_currentloc, internalState, fileSystem; @*/
{
  char *path = NULL;
  bool parser_status = FALSE;
  bool overallStatus = FALSE;
  
  lslInit ();
  
  context_resetSpecLines ();

  fileIdList_elements (lclfiles, fid)
    {
      char *actualName = (char *) dmalloc (sizeof (*actualName));
      char *oactualName = actualName;
      char *fname = cstring_toCharsSafe (fileName (fid));
      
      if (osd_getPath (g_localSpecPath, fname, &actualName) == OSD_FILENOTFOUND)
	{
	  if (mstring_equal (g_localSpecPath, "."))
	    {
	      lldiagmsg (message ("Spec file not found: %s",
				  cstring_fromChars (fname)));
	    }
	  else
	    {
	      lldiagmsg (message ("Spec file not found: %s (on %s)", 
				  cstring_fromChars (fname), 
				  cstring_fromChars (g_localSpecPath)));
	    }
	}
      else
	{
	  tsource *specFile;
	  
	  while (*actualName == '.' && *(actualName + 1) == CONNECTCHAR) 
	    {
	      actualName += 2;
	    }
	  
	  specFile = tsource_create (actualName, LCL_SUFFIX, TRUE);
	  llassert (specFile != (tsource *) 0);
	  
	  g_currentSpec = cstring_fromChars (mstring_copy (actualName));

	  g_currentSpecName = specFullName 
	    (cstring_toCharsSafe (g_currentSpec),
	     &path);

	  setSpecFileId (fid);
	  	  
	  if (context_getFlag (FLG_SHOWSCAN))
	    {
	      lldiagmsg (message ("< reading spec %s >", g_currentSpec));
	    }
	  
	  /* Open source file */
	  
	  if (!tsource_open (specFile))
	    {
	      lldiagmsg (message ("Cannot open file: %s",
				  cstring_fromChars (tsource_fileName (specFile))));
	      tsource_free (specFile);
	    }
	  else
	    {
	      scopeInfo dummy_scope = (scopeInfo) dmalloc (sizeof (*dummy_scope));
	      dummy_scope->kind = SPE_INVALID;
	      
	      lhInit (specFile);
	      LCLScanReset (specFile);
	      
	      /* 
              ** Minor hacks to allow more than one LCL file to
	      ** be scanned, while keeping initializations
	      */
	      
	      symtable_enterScope (g_symtab, dummy_scope);
	      resetImports (cstring_fromChars (g_currentSpecName));
	      context_enterLCLfile ();
	      (void) lclHadNewError ();
	      
	      parser_status = (ylparse () != 0);
	      context_exitLCLfile ();
	      lhCleanup ();
	      overallStatus = parser_status || lclHadNewError (); 

	      if (context_getFlag (FLG_DOLCS))
		{
		  if (overallStatus)
		    {
 		      outputLCSFile (path, "%%FAILED Output from ",
				     g_currentSpecName);
		    }
		  else
		    {
		      outputLCSFile (path, "%%PASSED Output from ", 
				     g_currentSpecName);
		    }
		}

	      (void) tsource_close (specFile);
	      tsource_free (specFile);

	      symtable_exitScope (g_symtab);
	    }
	}
      
      sfree (oactualName);
    } end_fileIdList_elements; 
  
    /* Can cleanup lsl stuff right away */

      lslCleanup ();
  
  g_currentSpec = cstring_undefined;
  g_currentSpecName = NULL;
}
# endif

static void handlePassThroughFlag (char *arg)
{
  char *curarg = arg;
  char *quotechar = strchr (curarg, '\"');
  int offset = 0;
  bool open = FALSE;

  while (quotechar != NULL)
    {
      if (*(quotechar - 1) == '\\')
	{
	  char *tp = quotechar - 2;
	  bool escape = TRUE;

	  while (*tp == '\\')
	    {
	      escape = !escape;
	      tp--;
	    }
	  
	  if (escape)
	    {
	      curarg = quotechar + 1;
	      quotechar = strchr (curarg, '\"');
	      continue;
	    }
	}
      
      *quotechar = '\0';
      offset = (quotechar - arg) + 2;
      
      if (open)
	{
	  arg = cstring_toCharsSafe
	    (message ("%s\"\'%s", 
		      cstring_fromChars (arg), 
		      cstring_fromChars (quotechar + 1))); 
	  open = FALSE;
	}
      else
	{
	  arg = cstring_toCharsSafe
	    (message ("%s\'\"%s", 
		      cstring_fromChars (arg), 
		      cstring_fromChars (quotechar + 1)));
	  open = TRUE;
	}
      
      curarg = arg + offset;
      quotechar = strchr (curarg, '\"');
    }

  if (open)
    {
      showHerald ();
      llerror (FLG_BADFLAG,
	       message ("Unclosed quote in flag: %s",
			cstring_fromChars (arg)));
    }
  else
    {
      if (arg[0] == 'D') {
	cstring def;
	
	/* 
	** If the value is surrounded by single quotes ('), remove
	** them.  This is an artifact of UNIX command line?
	*/

	def = osd_fixDefine (arg + 1);
	DPRINTF (("Do define: %s", def));
	cppDoDefine (def);
	DPRINTF (("After define"));
	cstring_free (def);
      } else if (arg[0] == 'U') {
	cppDoUndefine (cstring_fromChars (arg + 1));
      } else {
	BADBRANCH;
      }
    }
}

void showHerald (void)
{
  if (hasShownHerald || context_getFlag (FLG_QUIET)) return;

  else
    {
      fprintf (g_msgstream, "%s\n\n", LCL_VERSION);
      hasShownHerald = TRUE;
      llflush ();
    }
}

static void addFile (fileIdList files, /*@only@*/ cstring s)
{
  if (fileTable_exists (context_fileTable (), s))
    {
      showHerald ();
      lldiagmsg (message ("File listed multiple times: %s", s));
      cstring_free (s);
    }
  else
    {
      fileIdList_add (files, fileTable_addFileOnly (context_fileTable (), s));
    }
}

/*
** Disable MSVC++ warning about return value.  Methinks humbly lclint control
** comments are a mite more legible.
*/

# ifdef WIN32
# pragma warning (disable:4035) 
# endif

int main (int argc, char *argv[])
# ifdef NOLCL
  /*@globals killed undef g_currentloc,
	     killed undef yyin,
                    undef g_msgstream;
   @*/
  /*@modifies g_currentloc, fileSystem,
	      yyin; 
  @*/
# else
  /*@globals killed undef g_currentloc,
	     killed undef initFile,
	     killed       g_localSpecPath,  
	     killed undef g_currentSpec,
	     killed undef g_currentSpecName,
	     killed undef yyin,
                    undef g_msgstream;
   @*/
  /*@modifies g_currentloc, initFile, 
              g_localSpecPath, g_currentSpec, g_currentSpecName, fileSystem,
	      yyin; 
  @*/
# endif
{
  bool first_time = TRUE;
  bool showhelp = FALSE;
  bool allhelp = TRUE;
  bool expsuccess;
  tsource *sourceFile = (tsource *) 0;
 
  fileIdList dercfiles;
  cstringSList fl = cstringSList_undefined;
  cstringSList passThroughArgs = cstringSList_undefined;
  fileIdList cfiles;
  fileIdList lclfiles;
  clock_t before, lcltime, libtime, pptime, cptime, rstime;
  int i = 0;

  g_msgstream = stdout;

  (void) signal (SIGINT, interrupt);
  (void) signal (SIGSEGV, interrupt); 

  cfiles = fileIdList_create ();
  lclfiles = fileIdList_create ();

  flags_initMod ();
  typeIdSet_initMod ();
  cppReader_initMod ();

  setCodePoint ();

  g_currentloc = fileloc_createBuiltin ();

  before = clock ();
  context_initMod ();
  context_setInCommandLine ();

  if (argc <= 1)
    {
      showHelp ();
      llexit (LLGIVEUP);
    }

  setCodePoint ();
  yydebug = 0;

  /*
  ** Add include directories from environment.
  */

  {
    char *incval = mstring_copy (osd_getEnvironmentVariable (INCLUDE_VAR));

    if (incval != NULL)
      {
	/*
	** Each directory on the include path is a system include directory.
	*/

	DPRINTF (("include: %s", incval));
	context_setString (FLG_SYSTEMDIRS, cstring_fromCharsNew (incval));

	while (incval != NULL)
	  {
	    char *nextsep = strchr (incval, SEPCHAR);

	    if (nextsep != NULL)
	      {
		cstring dir;
		*nextsep = '\0';
		dir = cstring_fromCharsNew (incval);

		if (cstring_length (dir) == 0
		    || !isalpha ((int) cstring_firstChar (dir)))
		  {
		    /* 
		    ** win32 environment values can have special values,
		    ** ignore them
		    */
		  }
		else
		  {
		    DPRINTF (("Add include: %s", dir));
		    cppAddIncludeDir (dir);
		  }

		*nextsep = SEPCHAR;
		incval = nextsep + 1;
		cstring_free (dir);
	      }
	    else
	      {
		break;
	      }
	  }
      }
  }

  /*
  ** check RCFILE for default flags
  */

  {
    cstring home = cstring_fromChars (osd_getHomeDir ());
    char *fname  = NULL;
    FILE *rcfile;
    bool defaultf = TRUE;
    bool nof = FALSE;

    for (i = 1; i < argc; i++)
      {
	char *thisarg;
	thisarg = argv[i];
	
	if (*thisarg == '-' || *thisarg == '+')
	  {
	    thisarg++;

	    if (mstring_equal (thisarg, "nof"))
	      {
		nof = TRUE;
	      }
	    else if (mstring_equal (thisarg, "f"))
	      {
		if (++i < argc)
		  {
		    defaultf = FALSE;
		    fname = argv[i];
		    rcfile = fopen (fname, "r");

		    if (rcfile != NULL)
		      {
			fileloc oloc = g_currentloc;
			
			g_currentloc = fileloc_createRc (cstring_fromChars (fname));
			loadrc (rcfile, &passThroughArgs);
			fileloc_reallyFree (g_currentloc); 
			g_currentloc = oloc;
		      }
		    else 
		      {
			showHerald ();
			lldiagmsg (message ("Options file not found: %s", 
					    cstring_fromChars (fname)));
		      }
		  }
		else
		  llfatalerror
		    (cstring_makeLiteral ("Flag f to select options file "
					  "requires an argument"));
	      }
	    else
	      {
		; /* wait to process later */
	      }
	  }
      }
    
    if (fname == NULL)
      {
	if (!cstring_isEmpty (home)) {
	  fname = cstring_toCharsSafe (message ("%s%h%s", home, CONNECTCHAR,
						cstring_fromChars (RCFILE)));
	  mstring_markFree (fname);
	}
      }

    setCodePoint ();

    if (!nof && defaultf)
      {
	if (!mstring_isEmpty (fname)) {
	  rcfile = fopen (fname, "r");
	  
	  if (rcfile != NULL)
	    {
	      fileloc oloc = g_currentloc;
	      
	      g_currentloc = fileloc_createRc (cstring_fromChars (fname));
	      loadrc (rcfile, &passThroughArgs);
	      fileloc_reallyFree (g_currentloc);
	      g_currentloc = oloc;
	    }
	}

# if defined(MSDOS) || defined(OS2)
	fname = cstring_toCharsSafe (message ("%s",
					      cstring_fromChars (RCFILE)));
# else
	fname = cstring_toCharsSafe (message ("./%s", 
					      cstring_fromChars (RCFILE)));
# endif

	rcfile = fopen (fname, "r");

	if (rcfile != NULL)
	  {
	    fileloc oloc = g_currentloc;

	    g_currentloc = fileloc_createRc (cstring_fromChars (fname));
	    loadrc (rcfile, &passThroughArgs);
	    fileloc_reallyFree (g_currentloc);
	    g_currentloc = oloc;
	  }

	sfree (fname); 
      }
  }
  
  setCodePoint ();
  
  for (i = 1; i < argc; i++)
    {
      char *thisarg;
      flagcode opt;
      
      thisarg = argv[i];
      
      if (showhelp)
	{
	  if (allhelp)
	    {
	      showHerald ();
	    }
	  
	  allhelp = FALSE;
	  
	  if (*thisarg == '-' || *thisarg == '+')
	    {
	      thisarg++;	/* skip '-' */
	    }
	  if (mstring_equal (thisarg, "modes"))
	    {
	      llmsg (describeModes ());
	    }
	  else if (mstring_equal (thisarg, "vars")
		   || mstring_equal (thisarg, "env"))
	    {
	      describeVars ();
	    }
	  else if (mstring_equal (thisarg, "annotations"))
	    {
	      printAnnotations ();
	    }
	  else if (mstring_equal (thisarg, "parseerrors"))
	    {
	      printParseErrors ();
	    }
	  else if (mstring_equal (thisarg, "comments"))
	    {
	      printComments ();
	    }
	  else if (mstring_equal (thisarg, "prefixcodes"))
	    {
	      describePrefixCodes ();
	    }
	  else if (mstring_equal (thisarg, "references") 
		   || mstring_equal (thisarg, "refs"))
	    {
	      printReferences ();
	    }
	  else if (mstring_equal (thisarg, "mail"))
	    {
	      printMail ();
	    }
	  else if (mstring_equal (thisarg, "maintainer")
		   || mstring_equal (thisarg, "version"))
	    {
	      printMaintainer ();
	    }
	  else if (mstring_equal (thisarg, "flags"))
	    {
	      if (i + 1 < argc)
		{
		  char *next = argv[i + 1];
		  
		  if (specialFlagsHelp (next))
		    {
		      i++;
		    }
		  else
		    {
		      flagkind k = identifyCategory (cstring_fromChars (next));
		      
		      if (k != FK_NONE)
			{
			  printCategory (k);
			  i++;
			}
		    }
		}
	      else
		{
		  printFlags ();
		}
	    }
	  else
	    {
	      cstring s = describeFlag (cstring_fromChars (thisarg));
	      
	      if (cstring_isDefined (s))
		{
		  llmsg (s);
		}
	    }
	}
      else
	{
	  if (*thisarg == '-' || *thisarg == '+')
	    {
	      bool set = (*thisarg == '+');
	      cstring flagname;
	      
	      thisarg++;	/* skip '-' */
	      flagname = cstring_fromChars (thisarg);
	      
	      opt = identifyFlag (flagname);
	      
	      if (flagcode_isSkip (opt))
		{
		  ;
		}
	      else if (flagcode_isInvalid (opt))
		{
		  if (isMode (flagname))
		    {
		      context_setMode (flagname);
		    }
		  else
		    {
		      llgloberror (message ("Unrecognized option: %s", 
					    cstring_fromChars (thisarg)));
		    }
		}
	      else
		{
		  context_userSetFlag (opt, set);
		  
		  if (flagcode_hasArgument (opt))
		    {
		      if (opt == FLG_HELP)
			{
			  showhelp = TRUE;
			}
		      else if (flagcode_isPassThrough (opt)) /* -D or -U */
			{ 
			  passThroughArgs = cstringSList_add 
			    (passThroughArgs, cstring_fromChars (thisarg));
			}
		      else if (flagcode_hasValue (opt))
			{
			  if (++i < argc)
			    {
			      setValueFlag (opt, cstring_fromChars (argv[i]));
			    }
			  else
			    {
			      llfatalerror 
				(message
				 ("Flag %s must be followed by a number",
				  flagcode_unparse (opt)));
			    }
			} 
		      else if (opt == FLG_INCLUDEPATH || opt == FLG_SPECPATH)
			{
			  cstring dir = cstring_suffix (cstring_fromChars (thisarg), 1); /* skip over I */
			  
			  switch (opt)
			    {
			    case FLG_INCLUDEPATH:
			      cppAddIncludeDir (dir);
			      /*@switchbreak@*/ break;
			    case FLG_SPECPATH:
			      /*@-mustfree@*/
			      g_localSpecPath = cstring_toCharsSafe
				(message ("%s%h%s", 
					  cstring_fromChars (g_localSpecPath), 
					  SEPCHAR,
					  dir));
			      /*@=mustfree@*/
			      /*@switchbreak@*/ break;
			      BADDEFAULT;
			    }
			}
		      else if (flagcode_hasString (opt)
			       || opt == FLG_INIT || opt == FLG_OPTF)
			{
			  if (++i < argc)
			    {
			      cstring arg = cstring_fromChars (argv[i]);
			      
			      if (opt == FLG_OPTF)
				{
				  ; /* -f already processed */
				}
			      else if (opt == FLG_INIT)
				{
# ifndef NOLCL
				  initFile = tsource_create 
				    (cstring_toCharsSafe (arg), 
				     LCLINIT_SUFFIX, FALSE);
# endif
				  break;
				}
			      else
				{
				  setStringFlag (opt, arg);
				}
			    }
			  else
			    {
			      llfatalerror 
				(message
				 ("Flag %s must be followed by a string",
				  flagcode_unparse (opt)));
			    }
			}
		      else
			{
			  /* no argument */
			}
		    }
		}
	    }
	  else /* its a filename */
	    {
	      fl = cstringSList_add (fl, cstring_fromChars (thisarg));
	    }
	}
    }

  setCodePoint ();  

  /*
  ** create lists of C and LCL files
  */

  cstringSList_elements (fl, current)
    {
      char *fname = cstring_toCharsSafe (current);
      char *ext = strrchr (fname, '.');

      if (ext == NULL)
	{
	  /* no extension --- both C and LCL with default extensions */
	  
	  addFile (cfiles, message ("%s.c", cstring_fromChars (fname)));
	  addFile (lclfiles, message ("%s.lcl", cstring_fromChars (fname)));
	}
      else if (isCext (ext))
	{
	  addFile (cfiles, cstring_fromCharsNew (fname));
	}
      else 
	{
	  if (!mstring_equal (ext, ".lcl"))
	    {
	      lldiagmsg (message ("Unrecognized file extension: %s (assuming lcl)", 
				  cstring_fromChars (ext)));
	    }

	  addFile (lclfiles, cstring_fromCharsNew (fname));
	}
    } end_cstringSList_elements;
  
  
  showHerald ();

  
  if (showhelp)
    {
      if (allhelp)
	{
	  showHelp ();
	}
      fprintf (g_msgstream, "\n");

      fileIdList_free (cfiles);
      fileIdList_free (lclfiles);
      
      llexit (LLSUCCESS);
    }

# ifdef DOANNOTS
  initAnnots ();
# endif

  inittime = clock ();

  context_resetErrors ();
  context_clearInCommandLine ();

  anylcl = !fileIdList_isEmpty (lclfiles);

  if (context_doMerge ())
    {
      cstring m = context_getMerge ();

      if (context_getFlag (FLG_SHOWSCAN))
	{
	  fprintf (g_msgstream, "< loading %s ", cstring_toCharsSafe (m));
	}

      loadState (m);

      if (context_getFlag (FLG_SHOWSCAN))
	{
	  fprintf (g_msgstream, " >\n");
	}

      if (!usymtab_existsType (context_getBoolName ()))
	{
	  usymtab_initBool (); 
	}
    }
  else
    {
      if (!context_getFlag (FLG_NOLIB) && loadStandardState ())
	{
	  ;
	}
      else
	{
	  ctype_initTable ();
	}

      /* setup bool type and constants */
      usymtab_initBool (); 
    }

  fileloc_free (g_currentloc);
  g_currentloc = fileloc_createBuiltin ();

  libtime = clock ();
  
  if (anylcl)
    {
# ifdef NOLCL
      llfatalerror (cstring_makeLiteral ("This version of LCLint does not handle LCL files."));
# else
      lslProcess (lclfiles);
# endif
    }

  /*
  ** pre-processing
  **
  ** call the pre-preprocessor and /lib/cpp to generate appropriate
  ** files
  **
  */

  context_setInCommandLine ();

  cppReader_initialize ();

  DPRINTF (("Pass through: %s", cstringSList_unparse (passThroughArgs)));
  
  cstringSList_elements (passThroughArgs, thisarg) {
    handlePassThroughFlag (cstring_toCharsSafe (thisarg));
  } end_cstringSList_elements;

  cstringSList_free (passThroughArgs);

  cleanupMessages ();

  cppReader_saveDefinitions ();
  
  context_clearInCommandLine ();

  if (!context_getFlag (FLG_NOPP))
    {
      llflush ();

      if (context_getFlag (FLG_SHOWSCAN))
	{
	  fprintf (stderr, "< preprocessing"); 
	}
      
      lcltime = clock ();

      context_setPreprocessing ();
      dercfiles = preprocessFiles (cfiles);
      context_clearPreprocessing ();

      fileIdList_free (cfiles);

      if (context_getFlag (FLG_SHOWSCAN))
	{
	  fprintf (stderr, " >\n");
	}
      
      pptime = clock ();
    }
  else
    {
      lcltime = clock ();
      dercfiles = cfiles;
      pptime = clock ();
    }
  
  /*
  ** now, check all the corresponding C files
  **
  ** (for now these are just <file>.c, but after pre-processing
  **  will be <tmpprefix>.<file>.c)
  */

  {
# ifdef WIN32
    int nfiles = /*@-unrecog@*/ _fcloseall (); /*@=unrecog@*/

    if (nfiles != 0) 
      {
	llbug (message ("Files unclosed: %d", nfiles));
      }
# endif
  }

  exprNode_initMod ();

  fileIdList_elements (dercfiles, fid)
    {
      sourceFile = tsource_create (cstring_toCharsSafe (fileName (fid)),
				   C_SUFFIX, TRUE);
      context_setFileId (fid);
      
      /* Open source file  */
      
      if (sourceFile == (tsource *) 0 || (!tsource_open (sourceFile)))
	{
	  /* previously, this was ignored  ?! */
	  llbug (message ("Could not open temp file: %s", fileName (fid)));
	}
      else
	{
	  yyin = sourceFile->file; /*< shared <- only */
	
	  llassert (yyin != NULL);

	  if (context_getFlag (FLG_SHOWSCAN))
	    {
	      	      lldiagmsg (message ("< checking %s >", rootFileName (fid)));
	    }
	  
	  /*
	  ** Every time, except the first time, through the loop,
	  ** need to call yyrestart to clean up the parse buffer.
	  */

	  if (!first_time)
	    {
	      (void) yyrestart (yyin);	
	    }
	  else
	    {
	      first_time = FALSE;
	    }
	  
	  context_enterFile ();
	  (void) yyparse ();
	  context_exitFile ();
		    
	  (void) tsource_close (sourceFile);
	}
      
    } end_fileIdList_elements;

  cptime = clock ();
  
  /* process any leftover macros */

  context_processAllMacros ();
  
  /* check everything that was specified was defined */
  
  /* don't check if no c files were processed ?
  **   is this correct behaviour?
  */
  
  if (context_getFlag (FLG_SHOWSCAN))
    {
      lldiagmsg (cstring_makeLiteral ("< global checks >"));
    }

  cleanupMessages ();
  
  if (context_getLinesProcessed () > 0)
    {
      usymtab_allDefined ();
    }

  if (context_maybeSet (FLG_TOPUNUSED))
    {
      uentry ue = usymtab_lookupSafe (cstring_makeLiteralTemp ("main"));

      if (uentry_isValid (ue))
	{
	  uentry_setUsed (ue, fileloc_observeBuiltin ());
	}

      usymtab_allUsed ();
    }

  if (context_maybeSet (FLG_EXPORTLOCAL))
    {
      usymtab_exportLocal ();
    }

  
  if (context_maybeSet (FLG_EXPORTHEADER))
    {
      usymtab_exportHeader ();
    }

  if (context_getFlag (FLG_SHOWUSES))
    {
      usymtab_displayAllUses ();
    }

  context_checkSuppressCounts ();

  if (context_doDump ())
    {
      cstring dump = context_getDump ();

      dumpState (dump);
    }

# ifdef DOANNOTS
  printAnnots ();
# endif

  cleanupFiles ();

  if (context_getFlag (FLG_SHOWSUMMARY))
    {
      summarizeErrors (); 
    }

  
  {
    bool isQuiet = context_getFlag (FLG_QUIET);
    cstring specErrors = cstring_undefined;
# ifndef NOLCL
    int nspecErrors = lclNumberErrors ();
# endif
    
    expsuccess = TRUE;

    if (context_neednl ())
      fprintf (g_msgstream, "\n");
    
# ifndef NOLCL
    if (nspecErrors > 0)
      {
	if (nspecErrors == context_getLCLExpect ())
	  {
	    specErrors = 
	      message ("%d spec error%p found, as expected\n       ", 
		       nspecErrors);
	  }
	else
	  {
	    if (context_getLCLExpect () > 0)
	      {
		specErrors = 
		  message ("%d spec error%p found, expected %d\n       ", 
			   nspecErrors,
			   (int) context_getLCLExpect ());
	      }
	    else
	      {
		specErrors = message ("%d spec error%p found\n       ",
				      nspecErrors);
		expsuccess = FALSE;
	      }
	  }
      }
    else
	{
	  if (context_getLCLExpect () > 0)
	    {
	      specErrors = message ("No spec errors found, expected %d\n       ", 
				    (int) context_getLCLExpect ());
	      expsuccess = FALSE;
	    }
	}
# endif

      if (context_anyErrors ())
	{
	  if (context_numErrors () == context_getExpect ())
	    {
	      if (!isQuiet) {
		llmsg (message ("Finished LCLint checking --- "
				"%s%d code error%p found, as expected",
				specErrors, context_numErrors ()));
	      }
	    }
	  else
	    {
	      if (context_getExpect () > 0)
		{
		  if (!isQuiet) {
		    llmsg (message 
			   ("Finished LCLint checking --- "
			    "%s%d code error%p found, expected %d",
			    specErrors, context_numErrors (), 
			    (int) context_getExpect ()));
		  }

		  expsuccess = FALSE;
		}
	      else
		{
		  
		  if (!isQuiet) {
		    llmsg (message ("Finished LCLint checking --- "
				    "%s%d code error%p found", 
				    specErrors, context_numErrors ()));
		  }

		  expsuccess = FALSE;
		}
	    }
	}
      else
	{
	  if (context_getExpect () > 0)
	    {
	      if (!isQuiet) {
		llmsg (message
		       ("Finished LCLint checking --- "
			"%sno code errors found, expected %d", 
			specErrors,
			(int) context_getExpect ()));
	      }

	      expsuccess = FALSE;
	    }
	  else
	    {
	      if (context_getLinesProcessed () > 0)
		{
		  if (!isQuiet) {
		    llmsg (message ("Finished LCLint checking --- %sno code errors found", 
				    specErrors));
		  }
		}
	      else
		{
		  if (!isQuiet) {
		    llmsg (message ("Finished LCLint checking --- %sno code processed", 
				    specErrors));
		  }
		}
	    }
	}

      cstring_free (specErrors);
  }
  
  if (context_getFlag (FLG_STATS))
    {
      clock_t ttime = clock () - before;
      int specLines = context_getSpecLinesProcessed ();
      
      rstime = clock ();
      
      if (specLines > 0)
	{
	  fprintf (g_msgstream, "%d spec, ", specLines);
	}
      
# ifndef CLOCKS_PER_SEC
      fprintf (g_msgstream, "%d source lines in %ld time steps (steps/sec unknown)\n", 
	       context_getLinesProcessed (), 
	       (long) ttime);
# else
      fprintf (g_msgstream, "%d source lines in %.2f s.\n", 
	       context_getLinesProcessed (), 
	       (double) ttime / CLOCKS_PER_SEC);
# endif
    }
  else
    {
      rstime = clock ();
    }
  
  if (context_getFlag (FLG_TIMEDIST))
    {
      clock_t ttime = clock () - before;
      
      if (ttime > 0)
	{
	  char *msg = (char *) dmalloc (256 * sizeof (*msg));
	  
	  if (anylcl)
	    {
	      sprintf (msg, 
		       "Time distribution (percent): initialize %.2f / lcl %.2f / "
		       "pre-process %.2f / c check %.2f / finalize %.2f \n", 
		       (100.0 * (double) (libtime - before) / ttime),
		       (100.0 * (double) (lcltime - libtime) / ttime),
		       (100.0 * (double) (pptime - lcltime) / ttime),
		       (100.0 * (double) (cptime - pptime) / ttime),
		       (100.0 * (double) (rstime - cptime) / ttime));
	    }
	  else
	    {
	      sprintf (msg, 
		       "Time distribution (percent): initialize %.2f / "
		       "pre-process %.2f / c check %.2f / finalize %.2f \n", 
		       (100.0 * (double) (libtime - before) / ttime),
		       (100.0 * (double) (pptime - libtime) / ttime),
		       (100.0 * (double) (cptime - pptime) / ttime),
		       (100.0 * (double) (rstime - cptime) / ttime));
	    }
	  
	  llgenindentmsgnoloc (cstring_fromCharsO (msg));
	}
    }

  llexit (expsuccess ? LLSUCCESS : LLFAILURE);
}

/*
** Reenable return value warnings.
*/

#pragma warning (default:4035)

void
showHelp (void)
{
  showHerald ();
  
  llmsglit ("Source files are .c, .h and .lcl files.  If there is no suffix,");
  llmsglit ("   LCLint will look for <file>.c and <file>.lcl.");
  llmsglit ("");
  llmsglit ("Use lclint -help <topic or flag name> for more information");
  llmsglit ("");
  llmsglit ("Topics:");
  llmsglit ("");
  llmsglit ("   annotations (describes source-code annotations)");
  llmsglit ("   comments (describes control comments)");
  llmsglit ("   flags (describes flag categories)");
  llmsglit ("   flags <category> (describes flags in category)");
  llmsglit ("   flags all (short description of all flags)");
  llmsglit ("   flags alpha (list all flags alphabetically)");
  llmsglit ("   flags full (full description of all flags)");
  llmsglit ("   mail (information on mailing lists)");
  llmsglit ("   modes (show mode settings)");
  llmsglit ("   parseerrors (help on handling parser errors)");
  llmsglit ("   prefixcodes (character codes in namespace prefixes)");
  llmsglit ("   references (sources for more information)");
  llmsglit ("   vars (environment variables)"); 
  llmsglit ("   version (information on compilation, maintainer)");
  llmsglit ("");
}

static bool
specialFlagsHelp (char *next)
{
  if ((next != NULL) && (*next != '-') && (*next != '+'))
    {
      if (mstring_equal (next, "alpha"))
	{
	  printAlphaFlags ();
	  return TRUE;
	}
      else if (mstring_equal (next, "all"))
	{
	  printAllFlags (TRUE, FALSE);
	  return TRUE;
	}
      else if (mstring_equal (next, "categories")
	       || mstring_equal (next, "cats"))
	{
	  listAllCategories ();
	  return TRUE;
	}
      else if (mstring_equal (next, "full"))
	{
	  printAllFlags (FALSE, TRUE);
	  return TRUE;
	}
      else
	{
	  return FALSE;
	}
    }
  else
    {
      return FALSE;
    }
}

void
printParseErrors (void)
{
  llmsglit ("Parse Errors");
  llmsglit ("------------");
  llmsglit ("");
  llmsglit ("LCLint will sometimes encounter a parse error for code that "
	    "can be parsed with a local compiler. There are a few likely "
	    "causes for this and a number of techniques that can be used "
	    "to work around the problem.");
  llmsglit ("");
  llmsglit ("Compiler extensions --- compilers sometimes extend the C "
	    "language with compiler-specific keywords and syntax. While "
	    "it is not advisible to use these, oftentimes one has no choice "
	    "when the system header files use compiler extensions. ");
  llmsglit ("");
  llmsglit ("LCLint supports some of the GNU (gcc) compiler extensions, "
	    "if the +gnuextensions flag is set. You may be able to workaround "
	    "other compiler extensions by using a pre-processor define. "
	    "Alternately, you can surround the unparseable code with");
  llmsglit ("");
  llmsglit ("   # ifndef __LCLINT__");
  llmsglit ("   ...");
  llmsglit ("   # endif");
  llmsglit ("");
  llmsglit ("Missing type definitions --- an undefined type name will usually "
	    "lead to a parse error. This ofter occurs when a standard header "
	    "file defines some type that is not part of the standard library. ");
  llmsglit ("By default, LCLint does not process the local files corresponding "
	    "to standard library headers, but uses a library specification "
	    "instead so dependencies on local system headers can be detected. "
	    "If another system header file that does not correspond to a "
	    "standard library header uses one of these superfluous types, "
	    "a parse error will result.");
  llmsglit ("");
  llmsglit ("If the parse error is inside a posix standard header file, the "
	    "first thing to try is +posixlib. This make LCLint use "
	    "the posix library specification instead of reading the posix "
	    "header files.");
  llmsglit ("");
  llmsglit ("Otherwise, you may need to either manually define the problematic "
	    "type (e.g., add -Dmlink_t=int to your .lclintrc file) or force "
	    "lclint to process the header file that defines it. This is done "
	    "by setting -skipansiheaders or -skipposixheaders before "
	    "the file that defines the type is #include'd.");
  llmsglit ("(See lclint -help "
	    "skipansiheaders and lclint -help skipposixheaders for a list of "
	    "standard headers.)  For example, if <sys/local.h> uses a type "
	    "defined by posix header <sys/types.h> but not defined by the "
	    "posix library, we might do: ");
  llmsglit ("");
  llmsglit ("   /*@-skipposixheaders@*/");
  llmsglit ("   # include <sys/types.h>");
  llmsglit ("   /*@=skipposixheaders@*/");
  llmsglit ("   # include <sys/local.h>");
  llmsglit ("");
  llmsglit ("to force LCLint to process <sys/types.h>.");
  llmsglit ("");
  llmsglit ("At last resort, +trytorecover can be used to make LCLint attempt "
	    "to continue after a parse error.  This is usually not successful "
	    "and the author does not consider assertion failures when +trytorecover "
	    "is used to be bugs.");
}

void
printAnnotations (void)
{
  llmsglit ("Annotations");
  llmsglit ("-----------");
  llmsglit ("");
  llmsglit ("Annotations are stylized comments that document certain "
	    "assumptions about functions, variables, parameters, and types. ");
  llmsglit ("");
  llmsglit ("They may be used to indicate where the representation of a "
	    "user-defined type is hidden, to limit where a global variable may "
	    "be used or modified, to constrain what a function implementation "
            "may do to its parameters, and to express checked assumptions about "
	    "variables, types, structure fields, function parameters, and "
	    "function results.");
  llmsglit ("");
  llmsglit ("Annotations are introduced by \"/*@\". The role of the @ may be "
	    "played by any printable character, selected using -commentchar <char>.");
  llmsglit ("");
  llmsglit ("Consult the User's Guide for descriptions of checking associated with each annotation.");
  llmsglit ("");
  llmsglit ("Globals: (in function declarations)");
  llmsglit ("   /*@globals <globitem>,+ @*/");
  llmsglit ("      globitem is an identifier, internalState or fileSystem");
  llmsglit ("");
  llmsglit ("Modifies: (in function declarations)");
  llmsglit ("   /*@modifies <moditem>,+ @*/");
  llmsglit ("      moditem is an lvalue");
  llmsglit ("   /*@modifies nothing @*/");
  llmsglit ("   /*@*/   (Abbreviation for no globals and modifies nothing.)");
  llmsglit ("");
  llmsglit ("Iterators:");
  llmsglit ("   /*@iter <identifier> (<parameter-type-list>) @*/ - declare an iterator");
  llmsglit ("");
  llmsglit ("Constants:");
  llmsglit ("   /*@constant <declaration> @*/ - declares a constant");
  llmsglit ("");
  llmsglit ("Alternate Types:");
  llmsglit ("   /*@alt <basic-type>,+ @*/");
  llmsglit ("   (e.g., int /*@alt char@*/ is a type matching either int or char)");
  llmsglit ("");
  llmsglit ("Declarator Annotations");
  llmsglit ("");
  llmsglit ("Type Definitions:");
  llmsglit ("   /*@abstract@*/ - representation is hidden from clients");
  llmsglit ("   /*@concrete@*/ - representation is visible to clients");
  llmsglit ("   /*@immutable@*/ - instances of the type cannot change value");
  llmsglit ("   /*@mutable@*/ - instances of the type can change value");
  llmsglit ("   /*@refcounted@*/ - reference counted type");
  llmsglit ("");
  llmsglit ("Global Variables:");
  llmsglit ("   /*@unchecked@*/ - weakest checking for global use");
  llmsglit ("   /*@checkmod@*/ - check modification by not use of global");
  llmsglit ("   /*@checked@*/ - check use and modification of global");
  llmsglit ("   /*@checkedstrict@*/ - check use of global strictly");
  llmsglit ("");
  llmsglit ("Memory Management:");
  llmsglit ("   /*@dependent@*/ - a reference to externally-owned storage");
  llmsglit ("   /*@keep@*/ - a parameter that is kept by the called function");
  llmsglit ("   /*@killref@*/ - a refcounted parameter, killed by the call");
  llmsglit ("   /*@only@*/ - an unshared reference");
  llmsglit ("   /*@owned@*/ - owner of storage that may be shared by /*@dependent@*/ references");
  llmsglit ("   /*@shared@*/ - shared reference that is never deallocated");
  llmsglit ("   /*@temp@*/ - temporary parameter");
  llmsglit ("");
  llmsglit ("Aliasing:");
  llmsglit ("   /*@unique@*/ - may not be aliased by any other visible reference");
  llmsglit ("   /*@returned@*/ - may be aliased by the return value");
  llmsglit ("");
  llmsglit ("Exposure:");
  llmsglit ("   /*@observer@*/ - reference that cannot be modified");
  llmsglit ("   /*@exposed@*/ - exposed reference to storage in another object");
  llmsglit ("");
  llmsglit ("Definition State:");
  llmsglit ("   /*@out@*/ - storage reachable from reference need not be defined");
  llmsglit ("   /*@in@*/ - all storage reachable from reference must be defined");
  llmsglit ("   /*@partial@*/ - partially defined, may have undefined fields");
  llmsglit ("   /*@reldef@*/ - relax definition checking");
  llmsglit ("");
  llmsglit ("Global State: (for globals lists, no /*@, since list is already in /*@\'s)");
  llmsglit ("   undef - variable is undefined before the call");
  llmsglit ("   killed - variable is undefined after the call");
  llmsglit ("");
  llmsglit ("Null State:");
  llmsglit ("   /*@null@*/ - possibly null pointer");
  llmsglit ("   /*@notnull@*/ - non-null pointer");
  llmsglit ("   /*@relnull@*/ - relax null checking");
  llmsglit ("");
  llmsglit ("Null Predicates:");
  llmsglit ("   /*@truenull@*/ - if result is TRUE, first parameter is NULL");
  llmsglit ("   /*@falsenull@*/ - if result is TRUE, first parameter is not NULL");
  llmsglit ("");
  llmsglit ("Execution:");
  llmsglit ("   /*@exits@*/ - function never returns");
  llmsglit ("   /*@mayexit@*/ - function may or may not return");
  llmsglit ("   /*@trueexit@*/ - function does not return if first parameter is TRUE");
  llmsglit ("   /*@falseexit@*/ - function does not return if first parameter if FALSE");
  llmsglit ("   /*@neverexit@*/ - function always returns");
  llmsglit ("");
  llmsglit ("Side-Effects:");
  llmsglit ("   /*@sef@*/ - corresponding actual parameter has no side effects");
  llmsglit ("");
  llmsglit ("Declaration:");
  llmsglit ("   /*@unused@*/ - need not be used (no unused errors reported)");
  llmsglit ("   /*@external@*/ - defined externally (no undefined error reported)");
  llmsglit ("");
  llmsglit ("Case:");
  llmsglit ("   /*@fallthrough@*/ - fall-through case");
  llmsglit ("");
  llmsglit ("Break:");
  llmsglit ("   /*@innerbreak@*/ - break is breaking an inner loop or switch");
  llmsglit ("   /*@loopbreak@*/ - break is breaking a loop");
  llmsglit ("   /*@switchbreak@*/ - break is breaking a switch");
  llmsglit ("   /*@innercontinue@*/ - continue is continuing an inner loop");
  llmsglit ("");
  llmsglit ("Unreachable Code:");
  llmsglit ("   /*@notreached@*/ - statement may be unreachable.");
  llmsglit ("");
  llmsglit ("Special Functions:");
  llmsglit ("   /*@printflike@*/ - check variable arguments like printf");
  llmsglit ("   /*@scanflike@*/ - check variable arguments like scanf");
}

void
printComments (void)
{
  llmsglit ("Control Comments");
  llmsglit ("----------------");
  llmsglit ("");
  llmsglit ("Setting Flags");
  llmsglit ("");
  llmsglit ("Most flags (all except those characterized as \"globally-settable only\") can be set locally using control comments. A control comment can set flags locally to override the command line settings. The original flag settings are restored before processing the next file.");
  llmsglit ("");
  llmsglit ("The syntax for setting flags in control comments is the same as that of the command line, except that flags may also be preceded by = to restore their setting to the original command-line value. For instance,");
  llmsglit ("   /*@+boolint -modifies =showfunc@*/");
  llmsglit ("sets boolint on (this makes bool and int indistinguishable types), sets modifies off (this prevents reporting of modification errors), and sets showfunc to its original setting (this controls  whether or not the name of a function is displayed before a message).");
  llmsglit ("");
  llmsglit ("Error Suppression");
  llmsglit ("");
  llmsglit ("Several comments are provided for suppressing messages. In general, it is usually better to use specific flags to suppress a particular error permanently, but the general error suppression flags may be more convenient for quickly suppressing messages for code that will be corrected or documented later.");
  llmsglit ("");
  llmsglit ("/*@ignore@*/ ... /*@end@*/");
  llgenindentmsgnoloc
    (cstring_makeLiteral 
     ("No errors will be reported in code regions between /*@ignore@*/ and /*@end@*/. These comments can be used to easily suppress an unlimited number of messages."));
  llmsglit ("/*@i@*/");
    llgenindentmsgnoloc
    (cstring_makeLiteral 
     ("No errors will be reported from an /*@i@*/ comment to the end of the line."));
  llmsglit ("/*@i<n>@*/");
  llgenindentmsgnoloc
    (cstring_makeLiteral 
     ("No errors will be reported from an /*@i<n>@*/ (e.g., /*@i3@*/) comment to the end of the line. If there are not exactly n errors suppressed from the comment point to the end of the line, LCLint will report an error."));
  llmsglit ("/*@t@*/, /*@t<n>@*/");
  llgenindentmsgnoloc
    (cstring_makeLiteral 
     ("Like i and i<n>, except controlled by +tmpcomments flag. These can be used to temporarily suppress certain errors. Then, -tmpcomments can be set to find them again."));
  llmsglit ("");
  llmsglit ("Type Access");
  llmsglit ("");
  llmsglit ("/*@access <type>@*/"); 
  llmsglit ("   Allows the following code to access the representation of <type>");
  llmsglit ("/*@noaccess <type>@*/");
  llmsglit ("   Hides the representation of <type>");
  llmsglit ("");
  llmsglit ("Macro Expansion");
  llmsglit ("");
  llmsglit ("/*@notfunction@*/");
  llgenindentmsgnoloc 
    (cstring_makeLiteral
     ("Indicates that the next macro definition is not intended to be a "
      "function, and should be expanded in line instead of checked as a "
      "macro function definition."));
}

  
void
printFlags (void)
{
  llmsglit ("Flag Categories");
  llmsglit ("---------------");
  listAllCategories ();
  llmsglit ("\nTo see the flags in a flag category, do\n   lclint -help flags <category>");
  llmsglit ("To see a list of all flags in alphabetical order, do\n   lclint -help flags alpha");
  llmsglit ("To see a full description of all flags, do\n   lclint -help flags full");
}

void
printMaintainer (void)
{
  llmsg (message ("Maintainer: %s", cstring_makeLiteralTemp (LCLINT_MAINTAINER)));
  llmsglit (LCL_COMPILE);
}

void
printMail (void)
{
  llmsglit ("Mailing Lists");
  llmsglit ("-------------");
  llmsglit ("");
  llmsglit ("There are two mailing lists associated with LCLint: ");
  llmsglit ("");
  llmsglit ("   lclint-announce@virginia.edu");
  llmsglit ("");
  llmsglit ("      Reserved for announcements of new releases and bug fixes.");
  llmsglit ("      To subscribe, send a message to majordomo@virginia.edu with body: ");
  llmsglit ("           subscribe lclint-announce");
  llmsglit ("");
  llmsglit ("   lclint-interest@virginia.edu");
  llmsglit ("");
  llmsglit ("      Informal discussions on the use and development of lclint.");
  llmsglit ("      To subscribe, send a message to majordomo@virginia.edu with body: ");
  llmsglit ("           subscribe lclint-interest");
}

void
printReferences (void)
{
  llmsglit ("References");
  llmsglit ("----------");
  llmsglit ("");
  llmsglit ("The LCLint web site is http://lclint.cs.virginia.edu");
  llmsglit ("");
  llmsglit ("Technical papers relating to LCLint include:");
  llmsglit ("");
  llmsglit ("   David Evans. \"Static Detection of Dynamic Memory Errors\".");  
  llmsglit ("   SIGPLAN Conference on Programming Language Design and ");
  llmsglit ("   Implementation (PLDI '96), Philadelphia, PA, May 1996.");
  llmsglit ("");
  llmsglit ("   David Evans, John Guttag, Jim Horning and Yang Meng Tan. ");
  llmsglit ("   \"LCLint: A Tool for Using Specifications to Check Code\".");
  llmsglit ("   SIGSOFT Symposium on the Foundations of Software Engineering,");
  llmsglit ("   December 1994.");
  llmsglit ("");
  llmsglit ("A general book on Larch is:");
  llmsglit ("");
  llmsglit ("   Guttag, John V., Horning, James J., (with Garland, S. J., Jones, ");
  llmsglit ("   K. D., Modet, A., and Wing, J. M.), \"Larch: Languages and Tools ");
  llmsglit ("   for Formal Specification\", Springer-Verlag, 1993.");
}

void
describePrefixCodes (void)
{
  llmsglit ("Prefix Codes");
  llmsglit ("------------");
  llmsglit ("");
  llmsglit ("These characters have special meaning in name prefixes:");
  llmsglit ("");
  llmsg (message ("   %h  Any uppercase letter [A-Z]", PFX_UPPERCASE));
  llmsg (message ("   %h  Any lowercase letter [a-z]", PFX_LOWERCASE));
  llmsg (message ("   %h  Any character (valid in a C identifier)", PFX_ANY));
  llmsg (message ("   %h  Any digit [0-9]", PFX_DIGIT));
  llmsg (message ("   %h  Any non-uppercase letter [a-z0-9_]", PFX_NOTUPPER));
  llmsg (message ("   %h  Any non-lowercase letter [A-Z0-9_]", PFX_NOTLOWER));
  llmsg (message ("   %h  Any letter [A-Za-z]", PFX_ANYLETTER));
  llmsg (message ("   %h  Any letter or digit [A-Za-z0-9]", PFX_ANYLETTERDIGIT));
  llmsglit ("   *  Zero or more repetitions of the previous character class until the end of the name");
}

void
describeVars (void)
{
  cstring eval;
  char *def;

  eval = context_getLarchPath ();
  def = osd_getEnvironmentVariable (LARCH_PATH);

  if (def != NULL || 
      !cstring_equal (eval, cstring_fromChars (DEFAULT_LARCHPATH)))
    {
      llmsg (message ("LARCH_PATH = %s", eval));
    }
  else
    {
      llmsg (message ("LARCH_PATH = <not set> (default = %s)",
		      cstring_fromChars (DEFAULT_LARCHPATH)));
    }
  
  llmsglit ("   --- path used to find larch initialization files and LSL traits");

  eval = context_getLCLImportDir ();
  def = osd_getEnvironmentVariable (LCLIMPORTDIR);

  if (def != NULL ||
      !cstring_equal (eval, cstring_fromChars (DEFAULT_LCLIMPORTDIR)))
    {
      llmsg (message ("%q = %s", cstring_makeLiteral (LCLIMPORTDIR), eval));
    }
  else
    {
      llmsg (message ("%s = <not set, default: %s>", cstring_makeLiteralTemp (LCLIMPORTDIR), 
		      cstring_makeLiteralTemp (DEFAULT_LCLIMPORTDIR))); 
    }
  
  llmsglit ("   --- directory containing lcl standard library files "
	    "(import with < ... >)");;

  {
    cstring dirs = context_getString (FLG_SYSTEMDIRS);
    llmsg (message 
	   ("systemdirs = %s (set by include envirnoment variable or -systemdirs)",
	    dirs));

  }
}

void
interrupt (int i)
{
  switch (i)
    {
    case SIGINT:
      fprintf (stderr, "*** Interrupt\n");
      llexit (LLINTERRUPT);
    case SIGSEGV:
      {
	cstring loc;
	
	/* Cheat when there are parse errors */
	checkParseError (); 
	
	fprintf (stderr, "*** Segmentation Violation\n");
	
	/* Don't catch it if fileloc_unparse causes a signal */
	(void) signal (SIGSEGV, NULL);

	loc = fileloc_unparse (g_currentloc);
	
	fprintf (stderr, "*** Location (not trusted): %s\n", 
		 cstring_toCharsSafe (loc));
	cstring_free (loc);
	printCodePoint ();
	fprintf (stderr, "*** Please report bug to %s\n", LCLINT_MAINTAINER);
	exit (LLGIVEUP);
      }
    default:
      fprintf (stderr, "*** Signal: %d\n", i);
      /*@-mustfree@*/
      fprintf (stderr, "*** Location (not trusted): %s\n", 
	       cstring_toCharsSafe (fileloc_unparse (g_currentloc)));
      /*@=mustfree@*/
      printCodePoint ();
      fprintf (stderr, "*** Please report bug to %s ***\n", LCLINT_MAINTAINER);
      exit (LLGIVEUP);
    }
}

void
cleanupFiles (void)
{
  static bool doneCleanup = FALSE;

  /* make sure this is only called once! */

  if (doneCleanup) return;

  setCodePoint ();

  if (context_getFlag (FLG_KEEP))
    {
      check (fputs ("Temporary files kept:\n", stderr) != EOF);
      fileTable_printTemps (context_fileTable ());
    }
  else
    {
# ifdef WIN32
      int nfiles = /*@-unrecog@*/ _fcloseall (); /*@=unrecog@*/
      
      if (nfiles != 0) 
	{
	  llbug (message ("Files unclosed: %d", nfiles));
	}
# endif
      fileTable_cleanup (context_fileTable ());
    }

  doneCleanup = TRUE;
}

/*
** cleans up temp files (if necessary)
** exits lclint
*/

/*@exits@*/ void
llexit (int status)
{
  DPRINTF (("llexit: %d", status));

# ifdef WIN32
  if (status == LLFAILURE) 
    {
      _fcloseall ();
    }
# endif

  cleanupFiles ();

  if (status != LLFAILURE)
    {
      context_destroyMod ();
      exprNode_destroyMod ();
      
      sRef_destroyMod ();
      uentry_destroyMod ();
      typeIdSet_destroyMod ();
      
# ifdef USEDMALLOC
      dmalloc_shutdown ();
# endif
    }

  exit ((status == LLSUCCESS) ? EXIT_SUCCESS : EXIT_FAILURE);
}

void
loadrc (FILE *rcfile, cstringSList *passThroughArgs)
{
  char *s = mstring_create (MAX_LINE_LENGTH);
  char *os = s;

  DPRINTF (("Pass through: %s", cstringSList_unparse (*passThroughArgs)));

  s = os;

  while (fgets (s, MAX_LINE_LENGTH, rcfile) != NULL)
    {
      char c;
      bool set = FALSE;	    
      char *thisflag;
      flagcode opt;

      DPRINTF (("Line: %s", s));
      DPRINTF (("Pass through: %s", cstringSList_unparse (*passThroughArgs)));
      incLine ();
            
      while (*s == ' ' || *s == '\t' || *s == '\n') 
	{
	  s++;
	  incColumn ();
	}
      
      while (*s != '\0')
	{
	  bool escaped = FALSE;
	  bool quoted = FALSE;
	  c = *s;

	  DPRINTF (("Process: %s", s));
	  DPRINTF (("Pass through: %s", cstringSList_unparse (*passThroughArgs)));
	  /* comment characters */
	  if (c == '#' || c == ';' || c == '\n') 
	    {
	      /*@innerbreak@*/
	      break;
	    }
	  
	  if (c == '-' || c == '+')
	    {
	      set = (c == '+');
	    }
	  else
	    {
	      showHerald ();
	      llerror (FLG_SYNTAX, 
		       message ("Bad flag syntax (+ or - expected, "
				"+ is assumed): %s", 
				cstring_fromChars (s)));
	      s--;
	      set = TRUE;
	    }
	  
	  s++;
	  incColumn ();
	  
	  thisflag = s;
	  
	  while ((c = *s) != '\0')
	    { /* remember to handle spaces and quotes in -D and -U ... */
	      if (escaped)
		{
		  escaped = FALSE;
		}
	      else if (quoted)
		{
		  if (c == '\\')
		    {
		      escaped = TRUE;
		    }
		  else if (c == '\"')
		    {
		      quoted = FALSE;
		    }
		  else
		    {
		      ;
		    }
		}
	      else if (c == '\"')
		{
		  quoted = TRUE;
		}
	      else
		{
		 if (c == ' ' || c == '\t' || c == '\n')
		   {
		     /*@innerbreak@*/ break;
		   }
	       }
		  
	      s++; 
	      incColumn ();
	    }

	  DPRINTF (("Nulling: %c", *s));
	  *s = '\0';

	  if (mstring_isEmpty (thisflag))
	    {
	      llfatalerror (message ("Missing flag: %s",
				     cstring_fromChars (os)));
	    }

	  DPRINTF (("Flag: %s", thisflag));

	  opt = identifyFlag (cstring_fromChars (thisflag));
	  
	  if (flagcode_isSkip (opt))
	    {
	      ;
	    }
	  else if (flagcode_isInvalid (opt))
	    {
	      if (isMode (cstring_fromChars (thisflag)))
		{
		  context_setMode (cstring_fromChars (thisflag));
		}
	      else
		{
		  llerror (FLG_BADFLAG,
			   message ("Unrecognized option: %s", 
				    cstring_fromChars (thisflag)));
		}
	    }
	  else
	    {
	      context_userSetFlag (opt, set);

	      if (flagcode_hasArgument (opt))
		{
		  if (opt == FLG_HELP)
		    {
		      showHerald ();
		      llerror (FLG_BADFLAG,
			       message ("Cannot use help in rc files"));
		    }
		  else if (flagcode_isPassThrough (opt)) /* -D or -U */
		    {
		      cstring arg = cstring_fromCharsNew (thisflag);
		      cstring_markOwned (arg);
		      *passThroughArgs = cstringSList_add (*passThroughArgs, arg);
		      DPRINTF (("Pass through: %s",
				cstringSList_unparse (*passThroughArgs)));
		    }
		  else if (opt == FLG_INCLUDEPATH 
			   || opt == FLG_SPECPATH)
		    {
		      cstring dir = cstring_suffix (cstring_fromChars (thisflag), 1); /* skip over I/S */
		      		      
		      switch (opt)
			{
			case FLG_INCLUDEPATH:
			  cppAddIncludeDir (dir);
			  /*@switchbreak@*/ break;
			case FLG_SPECPATH:
			  /*@-mustfree@*/
			  g_localSpecPath = cstring_toCharsSafe
			    (message ("%s:%s", cstring_fromChars (g_localSpecPath), dir));
			  /*@=mustfree@*/
			  /*@switchbreak@*/ break;
			  BADDEFAULT;
			}
		    }
		  else if (flagcode_hasString (opt)
			   || flagcode_hasValue (opt)
			   || opt == FLG_INIT || opt == FLG_OPTF)
		    {
		      cstring extra = cstring_undefined;
		      char *rest, *orest;
		      char rchar;
		      
		      *s = c;
		      rest = mstring_copy (s);
		      DPRINTF (("Here: rest = %s", rest));
		      orest = rest;
		      *s = '\0';
		      
		      while ((rchar = *rest) != '\0'
			     && (isspace ((int) rchar)))
			{
			  rest++;
			  s++;
			}
		      
		      DPRINTF (("Yo: %s", rest));

		      while ((rchar = *rest) != '\0' 
			     && !isspace ((int) rchar))
			{
			  extra = cstring_appendChar (extra, rchar);
			  rest++; 
			  s++;
			}
		      
		      DPRINTF (("Yo: %s", extra));
		      sfree (orest);

		      if (cstring_isUndefined (extra))
			{
			  showHerald ();
			  llerror 
			    (FLG_BADFLAG,
			     message
			     ("Flag %s must be followed by an argument",
			      flagcode_unparse (opt)));
			}
		      else
			{
			  s--;
			  
			  DPRINTF (("Here we are: %s", extra));

			  if (flagcode_hasValue (opt))
			    {
			      DPRINTF (("Set value flag: %s", extra));
			      setValueFlag (opt, extra);
			      cstring_free (extra);
			    }
			  else if (opt == FLG_OPTF)
			    {
			      FILE *innerf = fopen (cstring_toCharsSafe (extra), "r");
			      cstring_markOwned (extra);
			      
			      if (innerf != NULL)
				{
				  fileloc fc = g_currentloc;
				  g_currentloc = fileloc_createRc (extra);
				  loadrc (innerf, passThroughArgs);
				  fileloc_reallyFree (g_currentloc);
				  g_currentloc = fc;
				}
			      else 
				{
				  showHerald ();
				  llerror
				    (FLG_SYNTAX, 
				     message ("Options file not found: %s", 
					      extra));
				}
			    }
			  else if (opt == FLG_INIT)
			    {
# ifndef NOLCL
			      llassert (initFile == NULL);
			      
			      initFile = tsource_create 
				(cstring_toCharsSafe (extra), 
				 LCLINIT_SUFFIX, FALSE);
			      cstring_markOwned (extra);
# else
			      cstring_free (extra);
# endif
			    }
			  else if (flagcode_hasString (opt))
			    {
			      if (cstring_firstChar (extra) == '"')
				{
				  if (cstring_lastChar (extra) == '"')
				    {
				      char *extras = cstring_toCharsSafe (extra);
				      
				      llassert (extras[strlen(extras) - 1] == '"');
				      extras[strlen(extras) - 1] = '\0';
				      extra = cstring_fromChars (extras + 1); 
				      DPRINTF (("Remove quites: %s", extra));
				    }
				  else
				    {
				      llerror
					(FLG_SYNTAX, 
					 message ("Unmatched \" in option string: %s", 
						  extra));
				    }
				}
			      
			      setStringFlag (opt, extra);
			    }
			  else
			    {
			      cstring_free (extra);
			      BADEXIT;
			    }
			}
		    }
		  else
		    {
		      BADEXIT;
		    }
		}
	    }
	  
	  *s = c;
	  DPRINTF (("Pass through: %s", cstringSList_unparse (*passThroughArgs)));
	  while ((c == ' ') || (c == '\t'))
	    {
	      c = *(++s);
	      incColumn ();
	    } 
	}
      DPRINTF (("Pass through: %s", cstringSList_unparse (*passThroughArgs)));
      s = os;
    }

  DPRINTF (("Pass through: %s", cstringSList_unparse (*passThroughArgs)));
  sfree (os); 
  check (fclose (rcfile) == 0);
}

static fileIdList preprocessFiles (fileIdList fl)
  /*@modifies fileSystem@*/
{
  bool msg = (context_getFlag (FLG_SHOWSCAN) && fileIdList_size (fl) > 10);
  int skip = (fileIdList_size (fl) / 5);
  int filesprocessed = 0;
  fileIdList dfiles = fileIdList_create ();

  fileloc_free (g_currentloc);
  g_currentloc = fileloc_createBuiltin ();

  fileIdList_elements (fl, fid)
    {
      char *ppfname = cstring_toCharsSafe (fileName (fid));

      if (!(osd_fileIsReadable (ppfname)))
	{
	  lldiagmsg (message ("Cannot open file: %s",
			      cstring_fromChars (ppfname)));
	}
      else
	{
	  fileId  dfile = fileTable_addCTempFile (context_fileTable (), fid);
	  
	  llassert (!mstring_isEmpty (ppfname));
	  
	  if (msg)
	    {
	      if ((filesprocessed % skip) == 0) 
		{
		  if (filesprocessed == 0) {
		    fprintf (stderr, " ");
		  }
		  else {
		    fprintf (stderr, ".");
		  }
		  
		  (void) fflush (stderr);
		}
	      filesprocessed++;
	    }

	  if (cppProcess (cstring_fromChars (ppfname), 
			  fileName (dfile)) != 0) 
	    {
	      llfatalerror (message ("Preprocessing error for file: %s", 
				     rootFileName (fid)));
	    }
	  
	  fileIdList_add (dfiles, dfile);
	}
    } end_fileIdList_elements; 
    
    return dfiles;
}