#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define _GNU_SOURCE
#include <getopt.h>

#include <murphy/common/log.h>
#include <murphy/core/context.h>
#include <murphy/core/plugin.h>
#include <murphy/daemon/config.h>


/*
 * command line processing
 */

static void print_usage(const char *argv0, int exit_code, const char *fmt, ...)
{
    va_list ap;
    
    if (fmt && *fmt) {
        va_start(ap, fmt);
        vprintf(fmt, ap);
        va_end(ap);
    }
    
    printf("usage: %s [options]\n\n"
           "The possible options are:\n"
           "  -C, --config-file=PATH         main configuration file to use\n"
	   "      The default configuration file is '%s'.\n"
           "  -D, --config-dir=PATH          configuration directory to use\n"
	   "      If omitted, defaults to '%s'.\n"
	   "  -P, --plugin-dir=PATH          load plugins from DIR\n"
	   "      The default plugin directory is '%s'.\n"
           "  -t, --log-target=TARGET        log target to use\n"
           "      TARGET is one of stderr,stdout,syslog, or a logfile path\n"
           "  -l, --log-level=LEVELS         logging level to use\n"
           "      LEVELS is a comma separated list of info, error and warning\n"
           "  -v, --verbose                  increase logging verbosity\n"
           "  -f, --foreground               don't daemonize\n"
#if 0
	   "  -a, --plugin=name:key=value    set plugin config variable\n"
	   "      E.g -a foo:bar=xyzzy sets the value of configuration key\n"
	   "      'bar' to the value 'xyzzy' for plugin 'foo'.\n"
#endif
           "  -h, --help                     show help on usage\n",
           argv0, MRP_DEFAULT_CONFIG_FILE, MRP_DEFAULT_CONFIG_DIR,
	   MRP_DEFAULT_PLUGIN_DIR);

    if (exit_code < 0)
	return;
    else
	exit(exit_code);
}


static void print_plugin_help(mrp_context_t *ctx)
{
    MRP_UNUSED(ctx);

#if 0
    mrp_plugin_t    *plugin;
    mrp_list_hook_t *p, *n;

    mrp_load_all_plugins(ctx);
    
    printf("\n");
    printf("Available plugins:\n\n");
    mrp_list_foreach(&ctx->plugins, p, n) {
	plugin = mrp_list_entry(p, typeof(*plugin), hook);
	printf("- %s plugin '%s' (%s)\n",
	       plugin->dynamic ? "external" : "internal",
	       plugin->descriptor->name, plugin->path);
	
	if (plugin->descriptor->help != NULL && plugin->descriptor->help[0])
	    printf("%s\n", plugin->descriptor->help);
    }
    
    printf("\n");
    printf("Note that you can disable any plugin from the command line by\n");
    printf("using the '-a name:%s' option.\n", MURPHY_PLUGIN_ARG_DISABLED);
#endif
}


static void config_set_defaults(mrp_context_t *ctx)
{
    ctx->config_file = MRP_DEFAULT_CONFIG_FILE;
    ctx->config_dir  = MRP_DEFAULT_CONFIG_DIR;
    ctx->plugin_dir  = MRP_DEFAULT_PLUGIN_DIR;
    ctx->log_mask    = MRP_LOG_MASK_ERROR;
    ctx->log_target  = MRP_LOG_TO_STDERR;
}


int mrp_parse_cmdline(mrp_context_t *ctx, int argc, char **argv)
{
    #define OPTIONS "C:D:l:t:fP:a:vdh"
    struct option options[] = {
	{ "config-file", required_argument, NULL, 'C' },
	{ "config-dir" , required_argument, NULL, 'D' },
	{ "plugin-dir" , required_argument, NULL, 'P' },
	{ "log-level"  , required_argument, NULL, 'l' },
	{ "log-target" , required_argument, NULL, 't' },
	{ "verbose"    , optional_argument, NULL, 'v' },
	{ "debug"      , no_argument      , NULL, 'd' },
	{ "foreground" , no_argument      , NULL, 'f' },
#if 0
	{ "plugin"    , required_argument, NULL, 'a' },
#endif
	{ "help"      , no_argument      , NULL, 'h' },
	{ NULL, 0, NULL, 0 }
    };

    int  opt, debug;
#if 0
    char arg[256], *plugin, *key, *value;
#endif

    debug = FALSE;
    config_set_defaults(ctx);
    
    while ((opt = getopt_long(argc, argv, OPTIONS, options, NULL)) != -1) {
        switch (opt) {
        case 'C':
	    ctx->config_file = optarg;
	    break;

	case 'D':
	    ctx->config_dir = optarg;
	    break;
	    
	case 'P':
	    ctx->plugin_dir = optarg;
	    break;

	case 'v':
	    ctx->log_mask <<= 1;
	    ctx->log_mask  |= 1;
	    break;

	case 'l':
	    ctx->log_mask = mrp_log_parse_levels(optarg);
	    if (ctx->log_mask < 0)
		print_usage(argv[0], EINVAL, "invalid log level '%s'", optarg);
	    break;

	case 't':
	    ctx->log_target = mrp_log_parse_target(optarg);
	    if (!ctx->log_target)
		print_usage(argv[0], EINVAL, "invalid log target '%s'", optarg);
	    break;

	case 'd':
	    debug = TRUE;
	    break;
	    
	case 'f':
	    ctx->foreground = TRUE;
	    break;

#if 0
	case 'a':
	    strncpy(arg, optarg, sizeof(arg) - 1);
	    arg[sizeof(arg) - 1] = '\0';
	    
	    plugin = arg;

	    key = strchr(plugin, ':');
	    if (key == NULL)
		print_usage(argv[0], EINVAL, "invalid plugin arg '%s'", optarg);
	    else
		*key++ = '\0';
	    
	    value = strchr(key, '=');
	    if (value != NULL)
		*value++ = '\0';
	    else
		value = "TRUE";
	    
	    if (!mrp_plugin_set(ctx, plugin, key, value))
		si_log_error("failed to set '%s' = '%s' for plugin '%s'",
			     key, value, plugin);
	    break;
#endif
	    
	case 'h':
	    print_usage(argv[0], -1, "");
	    print_plugin_help(ctx);
	    exit(0);
	    break;
	    
        default:
	    print_usage(argv[0], EINVAL, "invalid option '%c'", opt);
	}
    }

    if (debug)
	ctx->log_mask |= MRP_LOG_MASK_DEBUG;
    
    return TRUE;
}


/*
 * configuration file processing
 */

typedef struct {
    char  buf[MRP_CFG_MAXLINE];          /* input buffer */
    char *token;                         /* current token */
    char *in;                            /* filling pointer */
    char *out;                           /* consuming pointer */
    char *next;                          /* next token buffer position */
    int   fd;                            /* input file */
    int   error;                         /* whether has encounted and error */
    char *file;                          /* file being processed */
    int   line;                          /* line number */
    int   next_newline;
    int   was_newline;
} input_t;


#define COMMON_ACTION_FIELDS						\
    action_type_t   type;                /* action to execute */	\
    mrp_list_hook_t hook                 /* to command sequence */

typedef enum {                           /* action types */
    ACTION_UNKNOWN = 0,
    ACTION_LOAD,                         /* load a plugin */
    ACTION_TRYLOAD,                      /* load a plugin, ignore errors */
    ACTION_IF                            /* if-else branch */
} action_type_t;

typedef enum {                           /* branch operators */
    BR_UNKNOWN = 0,
    BR_PLUGIN_EXISTS,                    /* test if a plugin exists */
} branch_op_t;

typedef struct {                         /* a generic action */
    COMMON_ACTION_FIELDS;                /* type, hook */
} any_action_t;

typedef struct {                         /* a command-type of action */
    COMMON_ACTION_FIELDS;                /* type, hook */
    char **args;                         /* arguments for the action */
    int    narg;                         /* number of arguments */
} cmd_action_t;

typedef struct {                         /* a command-type of action */
    COMMON_ACTION_FIELDS;                /* type, hook */
    char             *name;              /* plugin to load */
    char             *instance;          /* load as this instance */
    mrp_plugin_arg_t *args;              /* plugin arguments */
    int               narg;              /* number of arguments */
} load_action_t;

typedef struct {                         /* a branch test action */
    COMMON_ACTION_FIELDS;                /* type, hook */
    branch_op_t      op;                 /* branch operator */
    char            *arg1;               /* argument for the operator */
    char            *arg2;               /* argument for the operator */
    mrp_list_hook_t  pos;                /* postitive branch */
    mrp_list_hook_t  neg;                /* negative branch */
} branch_action_t;


static char *get_next_token(input_t *in);
static int get_next_line(input_t *in, char **args, size_t size);
static any_action_t *parse_action(input_t *in, char *a, char **args, int narg);
static void free_action(any_action_t *action);
static void free_if_else(branch_action_t *branch);
static int exec_action(mrp_context_t *ctx, any_action_t *action);

mrp_cfgfile_t *mrp_parse_cfgfile(const char *path)
{
    mrp_cfgfile_t *cfg = NULL;
    input_t        input;
    char          *args[MRP_CFG_MAXARGS];
    int            narg;
    any_action_t  *a;

    memset(&input, 0, sizeof(input));
    input.token  = input.buf;
    input.in     = input.buf;
    input.out    = input.buf;
    input.next   = input.buf;
    input.fd     = open(path, O_RDONLY);
    input.file   = (char *)path;
    input.line   = 1;
    
    if (input.fd < 0) {
	mrp_log_error("Failed to open configuration file '%s' (%d: %s).",
		      path, errno, strerror(errno));
	goto fail;
    }

    cfg = mrp_allocz(sizeof(*cfg));

    if (cfg == NULL) {
	mrp_log_error("Failed to allocate configuration file buffer.");
	goto fail;
    }

    mrp_list_init(&cfg->actions);

    while ((narg = get_next_line(&input, args, sizeof(args))) > 0) {
	a = parse_action(&input, args[0], args + 1, narg - 1);
		
	if (a != NULL)
	    mrp_list_append(&cfg->actions, &a->hook);
	else
	    goto fail;
    }

    if (narg == 0)
	return cfg;

 fail:
    if (input.fd >= 0)
	close(input.fd);
    if (cfg)
	mrp_free_cfgfile(cfg);
    
    return NULL;
}


void mrp_free_cfgfile(mrp_cfgfile_t *cfg)
{
    mrp_list_hook_t *p, *n;
    any_action_t    *a;

    mrp_list_foreach(&cfg->actions, p, n) {
	a = mrp_list_entry(p, typeof(*a), hook);
	free_action(a);
    }

    mrp_free(cfg);
}


int mrp_exec_cfgfile(mrp_context_t *ctx, mrp_cfgfile_t *cfg)
{
    mrp_list_hook_t *p, *n;
    any_action_t    *a;

    mrp_list_foreach(&cfg->actions, p, n) {
	a = mrp_list_entry(p, typeof(*a), hook);
	if (!exec_action(ctx, a))
	    return FALSE;
    }

    return TRUE;
}


static any_action_t *parse_load(action_type_t t, char **argv, int argc)
{
    load_action_t    *action;
    mrp_plugin_arg_t *args, *a;
    int               i, start;
    char             *k, *v;


    if (argc < 1 || (action = mrp_allocz(sizeof(*action))) == NULL) {
	mrp_log_error("Failed to allocate load config action.");
	return NULL;
    }
    
    mrp_list_init(&action->hook);
    action->type = t;
    action->name = mrp_strdup(argv[0]);
    
    if (action->name == NULL) {
	mrp_log_error("Failed to allocate load config action.");
	mrp_free(action);
	return NULL;
    }

    if (argc > 2 && !strcmp(argv[1], MRP_KEYWORD_AS)) {
	action->instance = mrp_strdup(argv[2]);
	start = 3;

	if (action->instance == NULL) {
	    mrp_log_error("Failed to allocate load config action.");
	    mrp_free(action->name);
	    mrp_free(action);
	    goto fail;
	}
    }
    else
	start = 1;

    if (start < argc) {
	if ((args = mrp_allocz_array(typeof(*args), argc - 1)) != NULL) {
	    for (i = start, a = args; i < argc; i++, a++) {
		if (*argv[i] == MRP_START_COMMENT)
		    break;

		k = argv[i];
		v = strchr(k, '=');

		if (v != NULL)
		    *v++ = '\0';
		
		a->type = MRP_PLUGIN_ARG_TYPE_STRING;
		a->key  = mrp_strdup(k);
		a->str  = v ? mrp_strdup(v) : NULL;

		if (a->key == NULL || (a->str == NULL && v != NULL)) {
		    mrp_log_error("Failed to allocate plugin arg %s%s%s.",
				  k, v ? "=" : "", v ? v : "");
		    goto fail;
		}
	    }
	}
    }
    else
	args = NULL;
    
    action->args = args;
    action->narg = argc - 1;
    
    return (any_action_t *)action;
    
    
 fail:
    if (args != NULL) {
	for (i = 1; i < argc && args[i].key != NULL; i++) {
	    mrp_free(args[i].key);
	    mrp_free(args[i].str);
	}
	mrp_free(args);
    }
    
    return NULL;
}


static void free_load(load_action_t *action)
{
    int i;
    
    if (action != NULL) {
	mrp_free(action->name);
	
	for (i = 0; i < action->narg; i++) {
	    mrp_free(action->args[i].key);
	    mrp_free(action->args[i].str);
	}

	mrp_free(action->args);
    }
}


static int exec_load(mrp_context_t *ctx, load_action_t *a)
{
    if (!mrp_load_plugin(ctx, a->name, a->instance, NULL, 0))
	return (a->type == ACTION_TRYLOAD);
    else
	return TRUE;
}


static any_action_t *parse_if_else(input_t *in, char **argv, int argc)
{
    branch_action_t *branch;
    mrp_list_hook_t *actions;
    any_action_t    *a;
    char            *args[MRP_CFG_MAXARGS], *op, *name;
    int              start, narg, pos;
    
    if (argc < 2) {
	mrp_log_error("%s:%d: invalid use of if-conditional.",
		      in->file, in->line - 1);
	return NULL;
    }
    
    start = in->line - 1;
    op    = argv[0];
    name  = argv[1];

    if (strcmp(op, MRP_KEYWORD_EXISTS)) {
	mrp_log_error("%s:%d: unknown operator '%s' in if-conditional.",
		      in->file, in->line - 1, op);
    }

    branch = mrp_allocz(sizeof(*branch));
    
    if (branch != NULL) {
	mrp_list_init(&branch->pos);
	mrp_list_init(&branch->neg);
	
	branch->type = ACTION_IF;
	branch->op   = BR_PLUGIN_EXISTS;
	branch->arg1 = mrp_strdup(name);
	
	if (branch->arg1 == NULL) {
	    mrp_log_error("Failed to allocate configuration if-conditional.");
	    goto fail;
	}
	
	pos     = TRUE;
	actions = &branch->pos;
	while ((narg = get_next_line(in, args, sizeof(args))) > 0) {
	    if (narg == 1) {
		if (!strcmp(args[0], MRP_KEYWORD_END))
		    return (any_action_t *)branch;

		if (!strcmp(args[0], MRP_KEYWORD_ELSE)) {
		    if (pos) {
			actions = &branch->neg;
			pos = FALSE;
		    }
		    else {
			mrp_log_error("%s:%d: extra else without if.",
				      in->file, in->line - 1);
			goto fail;
		    }
		}
	    }
	    else {
		a = parse_action(in, args[0], args + 1, narg - 1);
		
		if (a != NULL)
		    mrp_list_append(actions, &a->hook);
		else
		    goto fail;
	    }
	}
    }
    else {
	mrp_log_error("Failed to allocate configuration if-conditional.");
	return NULL;
    }

    mrp_log_error("%s:%d: unterminated if-conditional (missing 'end')",
		  in->file, start);
    
 fail:
    free_if_else(branch);
    return NULL;
}


static void free_if_else(branch_action_t *branch)
{
    mrp_list_hook_t *p, *n;
    any_action_t    *a;

    if (branch != NULL) {
	mrp_free(branch->arg1);
	mrp_free(branch->arg2);
	
	mrp_list_foreach(&branch->pos, p, n) {
	    a = mrp_list_entry(p, typeof(*a), hook);
	    free_action(a);
	}

	mrp_list_foreach(&branch->neg, p, n) {
	    a = mrp_list_entry(p, typeof(*a), hook);
	    free_action(a);
	}
	
	mrp_free(branch);
    }
}


static int exec_if_else(mrp_context_t *ctx, branch_action_t *branch)
{
    mrp_list_hook_t *p, *n, *actions;
    any_action_t    *a;

    if (branch->op != BR_PLUGIN_EXISTS || branch->arg1 == NULL)
	return FALSE;

    if (mrp_plugin_exists(ctx, branch->arg1))
	actions = &branch->pos;
    else
	actions = &branch->neg;

    mrp_list_foreach(actions, p, n) {
	a = mrp_list_entry(p, typeof(*a), hook);
	
	if (!exec_action(ctx, a))
	    return FALSE;
    }
    
    return TRUE;
}


static any_action_t *parse_action(input_t *in, char *act, char **args, int narg)
{
    any_action_t *a;

    if (!strcmp(act, MRP_KEYWORD_LOAD))
	a = parse_load(ACTION_LOAD, args, narg);
    else if (!strcmp(act, MRP_KEYWORD_TRYLOAD))
	a = parse_load(ACTION_TRYLOAD, args, narg);
    else if (!strcmp(act, MRP_KEYWORD_IF))
	a = parse_if_else(in, args, narg);
    else {
	mrp_log_error("Unknown command '%s' in file '%s'.", args[0], in->file);
	a = NULL;
    }

    return a;
}


static void free_action(any_action_t *action)
{
    mrp_list_delete(&action->hook);
    
    switch (action->type) {
    case ACTION_LOAD:
    case ACTION_TRYLOAD:
	free_load((load_action_t *)action);
	break;
	
    case ACTION_IF:
	free_if_else((branch_action_t *)action);
	break;
	
    default:
	mrp_log_error("Unknown configuration action of type 0x%x.",
		      action->type);
	mrp_list_delete(&action->hook);
	mrp_free(action);
    }
}


static int exec_action(mrp_context_t *ctx, any_action_t *action)
{
    switch (action->type) {
    case ACTION_LOAD:
    case ACTION_TRYLOAD:
	return exec_load(ctx, (load_action_t *)action);
	
    case ACTION_IF:
	return exec_if_else(ctx, (branch_action_t *)action);
	
    default:
	mrp_log_error("Unknown configuration action of type 0x%x.",
		      action->type);
	mrp_list_delete(&action->hook);
	mrp_free(action);
	
	return FALSE;
    }
}


static int get_next_line(input_t *in, char **args, size_t size)
{
    char *token;
    int   narg;

    narg = 0;
    while ((token = get_next_token(in)) != NULL && narg < (int)size) {
	if (in->error)
	    return -1;
	
	if (token[0] != '\n')
	    args[narg++] = token;
	else {
	    if (*args[0] != MRP_START_COMMENT && narg && *args[0] != '\n')
		return narg;
	    else
		narg = 0;
	}
    }

    if (in->error)
	return -1;

    if (narg >= (int)size) {
	mrp_log_error("Too many tokens on line %d of %s.",
		      in->line - 1, in->file);
	return -1;
    }
    else {
	if (*args[0] != MRP_START_COMMENT && *args[0] != '\n')
	    return narg;
	else
	    return 0;
    }
}


static inline void skip_whitespace(input_t *in)
{
    while ((*in->out == ' ' || *in->out == '\t') && in->out < in->in)
	in->out++;
}


static char *get_next_token(input_t *in)
{
    ssize_t len;
    int     diff, size;
    int     quote, quote_line;
    char   *p, *q;

    /*
     * Newline:
     *
     *     If the previous token was terminated by a newline,
     *     take care of properly returning and administering
     *     the newline token here.
     */

    if (in->next_newline) {
	in->next_newline = FALSE;
	in->was_newline  = TRUE;
	in->line++;

	return "\n";
    }
    
    
    /*
     * if we just finished a line, discard all old data/tokens
     */

    if (*in->token == '\n' || in->was_newline) {
	diff = in->out - in->buf;
	size = in->in - in->out;
	memmove(in->buf, in->out, size);
	in->out  -= diff;
	in->in   -= diff;
	in->next  = in->buf;
	*in->in   = '\0';
    }

    /*
     * refill the buffer if we're empty or just flushed all tokens
     */

    if (in->token == in->buf && in->fd != -1) {
	size = sizeof(in->buf) - 1 - (in->in - in->buf);
	len  = read(in->fd, in->in, size);
	
	if (len < size) {
	    close(in->fd);
	    in->fd = -1;
	}
	
	if (len < 0) {
	    mrp_log_error("Failed to read from config file (%d: %s).",
			  errno, strerror(errno));
	    in->error = TRUE;
	    close(in->fd);
	    in->fd = -1;

	    return NULL;
	}
	
	in->in += len;
	*in->in = '\0';
    }

    if (in->out >= in->in)
	return NULL;

    skip_whitespace(in);
    
    quote = FALSE;

    p = in->out;
    q = in->next;
    in->token = q;

    while (p < in->in) {
	/*printf("[%c]\n", *p == '\n' ? '.' : *p);*/
	switch (*p) {
	    /*
	     * Quoting:
	     *
	     *     If we're not within a quote, mark a quote started.
	     *     Otherwise if quote matches, close quoting. Otherwise
	     *     copy the quoted quote verbatim.
	     */
	case '\'':
	case '\"':
	    if (!quote) {
		quote      = *p++;
		quote_line = in->line;
	    }
	    else {
		if (*p == quote) {
		    quote      = FALSE;
		    quote_line = 0;
		    p++;
		}
		else {
		    *q++ = *p++;
		}
	    }
	    in->was_newline = FALSE;
	    break;
	    
	    /*
	     * Whitespace:
	     *
	     *     If we're quoting, copy verbatim. Otherwise mark the end
	     *     of the token.
	     */
	case ' ':
	case '\t':
	    if (quote)
		*q++ = *p++;
	    else {
		p++;
		*q++ = '\0';
		
		in->out  = p;
		in->next = q;

		return in->token;
	    }
	    in->was_newline = FALSE;
	    break;

	    /*
	     * Escaping:
	     *
	     *     If the last character in the input, copy verbatim.
	     *     Otherwise if it escapes a '\n', skip both. Otherwise
	     *     copy the escaped character verbatim.
	     */
	case '\\':
	    if (p < in->in - 1) {
		p++;
		if (*p != '\n')
		    *q++ = *p++;
		else {
		    p++;
		    in->line++;
		    in->out = p;
		    skip_whitespace(in);
		    p = in->out;
		}
	    }
	    else
		*q++ = *p++;
	    in->was_newline = FALSE;
	    break;
	    
	    /*
	     * Newline:
	     *
	     *     We don't allow newlines to be quoted. Otherwise
	     *     if the token is not the newline itself, we mark
	     *     the next token to be newline and return the token
	     *     it terminated.
	     */
	case '\n':
	    if (quote) {
		mrp_log_error("%s:%d: Unterminated quote (%c) started "
			      "on line %d.", in->file, in->line, quote,
			      quote_line);
		in->error = TRUE;

		return NULL;
	    }
	    else {
		*q = '\0';
		p++;
		
		in->out  = p;
		in->next = q;
		
		if (in->token == q) {
		    in->line++;
		    in->was_newline = TRUE;
		    return "\n";
		}
		else {
		    in->next_newline = TRUE;
		    return in->token;
		}
	    }
	    break;

	default:
	    *q++ = *p++;
	    in->was_newline = FALSE;
	}
    }
    
    if (in->fd == -1) {
	*q = '\0';
	in->out = p;
	in->in = q;
	
	return in->token;
    }
    else {
	mrp_log_error("Input line %d of file %s exceeds allowed length.",
		      in->line, in->file);
	return NULL;
    }
}
