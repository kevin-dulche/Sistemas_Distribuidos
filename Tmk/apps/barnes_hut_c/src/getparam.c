#include "Tmk.h"
#include "stdinc.h"

local string *defaults = NULL;        /* vector of "name=value" strings */

local int scanbind(string bvec[], string name);
local bool matchname(string bind, string name);
local string extrvalue(string arg);

/*
 * GETPARAM: export version prompts user for value.
 */

string getparam(name)
  string name;                        /* name of parameter */
{
    int i, leng;
    string def;
    char buf[128];

    if (defaults == NULL)
        barnes_error("getparam: called before initparam\n");
    i = scanbind(defaults, name);              /* find name in defaults    */
    if (i < 0)
        barnes_error("getparam: %s unknown\n", name);
    def = extrvalue(defaults[i]);             /* extract default value    */
    if (*def != '\0')
        fprintf(stderr, "enter %s [%s]: ", name, def);
    else
        fprintf(stderr, "enter %s: ", name);
    gets(buf);                                /* read users response      */
    leng = strlen(buf) + 1;
    if (leng > 1)
        return (strcpy(malloc(leng), buf));   /* return users value, or   */
    else
        return (def);                         /* return default value     */
}

/*
 * GETIPARAM, ..., GETDPARAM: get int, long, bool, or double parameters.
 */

int getiparam(name)
  string name;                        /* name of parameter */
{
    string val;
    
    for (val = ""; *val == '\0'; )            /* while nothing input      */
        val = getparam(name);                 /*   obtain value from user */
    return (atoi(val));                       /* convert to an integer    */
}

long getlparam(name)
  string name;                        /* name of parameter */
{
    string val;
    
    for (val = ""; *val == '\0'; )
        val = getparam(name);
    return (atol(val));                         /* convert to a long int */
}

bool getbparam(name)
  string name;                        /* name of parameter */
{
    string val;
    
    for (val = ""; *val == '\0'; )
        val = getparam(name);
    if (strchr("tTyY1", *val) != NULL)                /* is value true?  */
        return (TRUE);
    if (strchr("fFnN0", *val) != NULL)                /* is value false? */
        return (FALSE);
    barnes_error("getbparam: %s=%s not bool\n", name, val);
}

double getdparam(name)
  string name;                        /* name of parameter */
{
    string val;
    
    for (val = ""; *val == '\0'; )
        val = getparam(name);
    return (atof(val));                           /* convert to a double */
}



/*
 * SCANBIND: scan binding vector for name, return index.
 */

local int scanbind(bvec, name)
  string bvec[];
  string name;
{
    int i;
    
    for (i = 0; bvec[i] != '\0'; i++)
        if (matchname(bvec[i], name))
            return (i);
    return (-1);
}

/*
 * MATCHNAME: determine if "name=value" matches "name".
 */

local bool matchname(bind, name)
  string bind, name;
{
    char *bp, *np;
    
    bp = bind;
    np = name;
    while (*bp == *np) {
        bp++;
        np++;
    }
    return (*bp == '=' && *np == '\0');
}

/*
 * EXTRVALUE: extract value from name=value string.
 */

local string extrvalue(arg)
  string arg;                        /* string of the form "name=value" */
{
    char *ap;
    
    ap = (char *) arg;
    while (*ap != '\0')
        if (*ap++ == '=')
            return ((string) ap);
    return (NULL);
}
