/* $Id: mod_httpbl.c,v 1.5 2007/08/28 01:09:09 oxyrubber Exp $ */

/*
 *   mod_httpbl for Apache 2.0
 *   Copyright (c) by Unspam Technologies, Inc.
 *   Written by David Wortham (httpbl+djwortham@gmail.com)
 *
 *   This code will create an Apache Module called mod_HTTPBL.  mod_HTTPBL is designed
 *   to allow web-administrators to filter their visitor traffic based on "threat ratings"
 *   and other feedback given by http://httpbl.org/ (run by Unspam Technologies, Inc.).
 *
 *   mod_HTTPBL is designed to allow the administrator to configure some or all of a particular
 *   website, more specifcally a "Virtual Host", to use a DNS query on the IP address of a HTTP
 *   visitor to determine if httpbl.org considers this visitor a threat (i.e. known website
 *   email "havester" or has leaked email data from a website to a "spammer").  Specifications
 *   for descriptions of the HTTPBL will be available at http://www.httpbl.org/ once they are
 *   completed.  Instructions for using and configuring mod_HTTPBL will also be available at
 *   this web server.
 *
 *   Terms of Service:
 *      Use of this module in connection with the http:BL service is subject to Project Honey Pot's
 *      Terms of Service. You can view these terms of service by visiting:
 *      http://www.projecthoneypot.org/terms_of_service_use.php
 *      While you are free to modify and redistribute this source code subject to the terms of the
 *      GPL (v. 2) software license, continuing to query the http:BL service after you modify the code
 *      may violate the Terms of Service (e.g., if you removed the throttles that limit the number of
 *      queries sent to http:BL's DNS). In order to ensure you do not violate the Terms of Service,
 *      please either disconnect the module from querying from and posting to the http:BL service, or
 *      seek the permission from Project Honey Pot, before you deploy modified code.
 *
 *   License information:
 *       Parts of this module are licensed under the GPL (GNU General Public License)
 *       and other parts are licensed under the Apache License.  Since GPL is considered
 *       to be more strict, this module will adhere to the GPL requirements, but will retain
 *       the licensing requirements for both the GNU General Public License and the Apache
 *       License.
 *
 *       Code has been reused/borrowed from the following sources:
 *       - mod_access_rbl    (Copyright (c) 1995-1999 The Apache Group.  All rights reserved.)
 *           * RBL (DNS) request call code
 *           * Code to set access restrictions based on directory structure and settings in httpd.conf
 *       - mod_evasive       (Copyright (c) Jonathan A. Zdziarski)
 *           * Code for HashTable struct and associated functions
 *       - mod_speling       (by Alexei Kosut <akosut@organic.com> June, 1996)
 *           * code for intercepting 404 errors before they are finished processing
 *       - libyahoo2         (Copyright (C) 2002-2004, Philip S Tellis <philip.tellis AT gmx.net>)
 *           * code for url-encoding a string
 *
 *   GPL LICENSE:
 *       This program is free software; you can redistribute it and/or
 *       modify it under the terms of the GNU General Public License
 *       as published by the Free Software Foundation; version 2
 *       of the License.
 *       
 *       This program is distributed in the hope that it will be useful,
 *       but WITHOUT ANY WARRANTY; without even the implied warranty of
 *       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *       GNU General Public License for more details.
 *       
 *       You should have received a copy of the GNU General Public License
 *       along with this program; if not, write to the Free Software
 *       Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *   APACHE LICENSE:
 *       Redistribution and use in source and binary forms, with or without
 *       modification, are permitted provided that the following conditions
 *       are met:
 *       
 *       1. Redistributions of source code must retain the above copyright
 *          notice, this list of conditions and the following disclaimer. 
 *       
 *       2. Redistributions in binary form must reproduce the above copyright
 *          notice, this list of conditions and the following disclaimer in
 *          the documentation and/or other materials provided with the
 *          distribution.
 *       
 *       3. All advertising materials mentioning features or use of this
 *          software must display the following acknowledgment:
 *          "This product includes software developed by the Apache Group
 *          for use in the Apache HTTP server project (http://www.apache.org/)."
 *       
 *       4. The names "Apache Server" and "Apache Group" must not be used to
 *          endorse or promote products derived from this software without
 *          prior written permission. For written permission, please contact
 *          apache@apache.org.
 *       
 *       5. Products derived from this software may not be called "Apache"
 *          nor may "Apache" appear in their names without prior written
 *          permission of the Apache Group.
 *       
 *       6. Redistributions of any form whatsoever must retain the following
 *          acknowledgment:
 *          "This product includes software developed by the Apache Group
 *          for use in the Apache HTTP server project (http://www.apache.org/)."
 *       
 *       THIS SOFTWARE IS PROVIDED BY THE APACHE GROUP ``AS IS'' AND ANY
 *       EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *       IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *       PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE APACHE GROUP OR
 *       ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *       SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *       NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *       LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *       HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 *       STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *       ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 *       OF THE POSSIBILITY OF SUCH DAMAGE.
 *       ====================================================================
 *       
 *       This software consists of voluntary contributions made by many
 *       individuals on behalf of the Apache Group and was originally based
 *       on public domain software written at the National Center for
 *       Supercomputing Applications, University of Illinois, Urbana-Champaign.
 *       For more information on the Apache Group and the Apache HTTP server
 *       project, please see <http://www.apache.org/>.
 *
 *
 *
 *   INSTALLATION:
 *       To install this module first compile it into a
 *       DSO file and install it into Apache's modules directory 
 *       by running:
 *       
 *         $ apxs -c -i -a mod_httpbl.c
 *       
 *       Then activate it in Apache's httpd.conf file for instance
 *       for the URL /foo in as follows:
 *       
 *         ------------ in your httpd.conf, add this: -------------
 *         <IfModule mod_httpbl.c>
 *             HTTPBLRBLEnabled     On
 *             HTTPBLLogDir         /path/to/httpbl/logs/dir
 *             HTTPBLAccessKey      *** get_this_from http://www.projecthoneypot.org ***
 *             # any additional server-wide directives
 *         </IfModule>
 *
 *         <Location /httpbl>
 *             # directory-specific directives
 *         </Location>
 *         ------------                               -------------
 *         Some directive notes:
 *         - the HTTPBLLogDir is necessary if compiled with DEBUG mode on (VERBOSITY < APLOG_DEBUG)
 *
 *       Verify that your httpd.conf parses correctly with this command:
 *       /path/to/apache/bin/apachectl configtest
 *
 *       Then to restart Apache, use: 
 *       /path/to/apache/bin/apachectl restart
 *
 *       The module is installed and running.
 *      
 *       For more information on how to , visit the module documentation page:
 *       {URL TO DOCS}
 */

/* Begin #defines Section */
#define AP_MOD_HTTPBL_BASEREVISION "v. 1.0.0a1 build16"

#define CBUFFSIZE                               (2048)
#define ap_min(a,b)                             ((a)<(b))?(a):(b)
#define ap_max(a,b)                             ((a)>(b))?(a):(b)

#define DEFAULT_CACHEFILE_FILENAME_SUFFIX       ".cache"
#define DEFAULT_HTTPBL_FILENAME_PREFIX          "httpbl_"
#define DEFAULT_404_CACHE_METAFILE_SUFFIX       ".meta"

#ifndef DEFAULT_SERVER_ROOT_DIRECTORY           // the "#ifndef" is to detect if the module was compiled with an argument to override this #define
    #define DEFAULT_SERVER_ROOT_DIRECTORY       "/path/to/apache/dir/"      // hardcoded for development... will fail (probably a SegFault) on any FileSys without this path
#endif

#define DEFAULT_LOG_DIR		                    DEFAULT_SERVER_ROOT_DIRECTORY"logs/" // Default temp directory
#define DEFAULT_REPOS_DIR		                DEFAULT_SERVER_ROOT_DIRECTORY"repos/" // Default temp directory

#define DEFAULT_HITLIST_CACHEFILE_BASENAME      DEFAULT_HTTPBL_FILENAME_PREFIX"hashlist.cache"
#define DEFAULT_WHITELIST_CACHEFILE_BASENAME    DEFAULT_HTTPBL_FILENAME_PREFIX"whitelist.cache"
#define DEFAULT_404_CACHE_DATAFILE_BASENAME     DEFAULT_HTTPBL_FILENAME_PREFIX"404.cache"
#define DEFAULT_404_CACHE_METAFILE_BASENAME     DEFAULT_HTTPBL_FILENAME_PREFIX"404.meta"
#define DEFAULT_HTTPBL_LOG_FILE                 DEFAULT_HTTPBL_FILENAME_PREFIX"module.log"
#define DEFAULT_WHITELIST_TESTING_LOG_FILE      DEFAULT_HTTPBL_FILENAME_PREFIX"whitelist_token_testing.log"
#define DEFAULT_HONEYPOT_TESTING_LOG_FILE       DEFAULT_HTTPBL_FILENAME_PREFIX"honeypot_token_testing.log"
#define DEFAULT_RBL_TOGGLE_LOG_FILE             DEFAULT_HTTPBL_FILENAME_PREFIX"rbl_toggle_directive.log"
#define DEFAULT_DIR_CFG_LOG_FILE                DEFAULT_HTTPBL_FILENAME_PREFIX"dir_cfg.log"
#define DEFAULT_HTTPBL_MERGE_TRACE_LOG_FILE     DEFAULT_HTTPBL_FILENAME_PREFIX"merge_trace.log"
#define DEFAULT_REPLACE_EMAILS_LOG_FILE          DEFAULT_HTTPBL_FILENAME_PREFIX"replace_emails.log"
#define DEFAULT_HTTPBL_TESTING_LOG_FILE         DEFAULT_HTTPBL_FILENAME_PREFIX"testing.log"

//#define DEFAULT_HONEYPOT_REQUEST_URL            "http://192.168.3.104:11000/test/cgi/filmverdict.php"
//#define DEFAULT_HONEYPOT_REQUEST_URL            "http://192.168.3.104:11000/cgi/serve.php"
//#define DEFAULT_HONEYPOT_REQUEST_URL            "http://hbr1.projecthoneypot.org/cgi/serve.php"
#define DEFAULT_HONEYPOT_REQUEST_URL            "http://hpr1.projecthoneypot.org/cgi/serve.php"
//#define DEFAULT_404_POST_URL                    "http://localhost:18000/raw.html"           // the URL to submit (404 and POST) data to
//#define DEFAULT_404_URL                         "http://404report.projecthoneypot.org/"     // the URL to submit (404) data to
#define DEFAULT_404_URL                         "http://192.168.3.100:11000/record_404.php"     // the URL to submit (404) data to
//#define DEFAULT_POST_URL                        "http://postreport.projecthoneypot.org/"    // the URL to submit (POST) data to
#define DEFAULT_POST_URL                        "http://192.168.3.100:11000/record_post.php"    // the URL to submit (POST) data to
#define DEFAULT_CHALLENGE_URL                   "/challenge/captcha.html"                   // the beginning of the URL-path for the diagnostics page for the challenge page
//#define DEFAULT_CHALLENGE_URL                   "/challenge/js_test.html"                   // the beginning of the URL-path for the diagnostics page for the JS challenge page
#define DEFAULT_HTTPBL_TESTING_URL              "/httpbl_diagnostics"                       // the beginning of the URL-path for the diagnostics page
#define DEFAULT_HASH_TBL_SIZE                   3097ul                                      // Default hash table size
#define DEFAULT_BLOCKING_PERIOD                 1440ul                                      // Default for RBL Lookup-ed IPs; cache active for 4 hours
#define WHITELIST_IP_ADDRESS                    "0.0.0.20"                                  // IP to be used for storing clean addresses (instead of NULL)
#define CLEANLIST_IP_ADDRESS                    "0.0.0.10"                                  // IP to be used for storing clean addresses (instead of NULL)

#define DEFAULT_EMAIL_REWRITE_TEXT              "(PROTECTED EMAIL)"
#define DEFAULT_EMAIL_REWRITE_LINK              "."                                         // link to the same page (similar tp <a href="#">...</a>

#define DIRECTIVE_TEXT_RBL_TOGGLE               "HTTPBLRBLEnabled"
#define DIRECTIVE_TEXT_404_TOGGLE               "HTTPBLCapture404s"
#define DIRECTIVE_TEXT_POST_TOGGLE              "HTTPBLCapturePOSTs"
#define DIRECTIVE_TEXT_CHALLENGE_TOKEN          "HTTPBLChallengeToken"
#define DIRECTIVE_TEXT_CHALLENGE_URL            "HTTPBLChallengeURL"
#define DIRECTIVE_TEXT_DIAGNOSTICS_URL          "HTTPBLTestingURL"
#define DIRECTIVE_TEXT_HONEYPOT_URL             "HTTPBLHoneypotURL"
#define DIRECTIVE_TEXT_ACCESS_KEY               "HTTPBLAccessKey"
#define DIRECTIVE_TEXT_EXEMPT_TOGGLE            "HTTPBLExempt"
#define DIRECTIVE_TEXT_HASH_TABLE_SIZE          "HTTPBLHashTableSize"
#define DIRECTIVE_TEXT_BLOCKING_PERIOD          "HTTPBLBlockingPeriod"
#define DIRECTIVE_TEXT_LOG_DIR                  "HTTPBLLogDir"
#define DIRECTIVE_TEXT_REPOS_DIR                "HTTPBLReposDir"
#define DIRECTIVE_TEXT_WHITELIST_AN_IPADDR      "HTTPBLWhitelist"
#define DIRECTIVE_TEXT_404_SUBMISSION_URL       "HTTPBL404POSTURL"
#define DIRECTIVE_TEXT_404_SUBMISSION_INTERVAL  "HTTPBL404POSTInterval"
#define DIRECTIVE_TEXT_404_SUBMISSION_MAX_RETRIES "HTTPBL404POSTMaxRetries"
#define DIRECTIVE_TEXT_404_SUBMISSION_MIN_RECORDS "HTTPBL404POSTWhenRecordsReaches"
#define DIRECTIVE_TEXT_EXTERNAL_POST_TIMEOUT    "HTTPBLExtPOSTTimeout"
#define DIRECTIVE_TEXT_EXTERNAL_PROXY_INFO      "HTTPBLExtHTTPProxy"
#define DIRECTIVE_TEXT_EXTERNAL_HTTPAUTH_INFO   "HTTPBLExtHTTPAuth"
#define DIRECTIVE_TEXT_DEFAULT_ACTION           "HTTPBLDefaultAction"
#define DIRECTIVE_TEXT_RBL_DOMAIN               "HTTPBLRBLDomain"
#define DIRECTIVE_TEXT_REWRITE_EMAIL_TEXT       "HTTPBLRewriteEmailLinksTo"
#define DIRECTIVE_TEXT_REWRITE_EMAIL_LINKS      "HTTPBLRewriteEmailTextTo"
#define DIRECTIVE_TEXT_RBL_REQ_HANDLER          "HTTPBLRBLReqHandler"
#define DIRECTIVE_TEXT_FORBIDDEN_TEMPLATE_PATH  "HTTPBLCustomForbiddenTemplate"

#define UNSET_INT                               -999                                    // sentinel value; this value should be easy to find in debug logs
#define UNSET_STR                               NULL
#define UNSET_OBJ                               NULL

#ifdef UINT64_MAX
#define MAX_APR_UINT64_T                        UINT64_MAX
#else
#define MAX_APR_UINT64_T                        0xffffffffffffffff
#endif

#define DEFAULT_ACTION_UNSET                    0
#define HTTPBL_ACTION_DENY                      1
#define HTTPBL_ACTION_CHALLENGE                 2
#define HTTPBL_ACTION_ALLOW                     3
#define HTTPBL_ACTION_ALLOW_XLATE_EMAILS        4
#define HTTPBL_ACTION_HONEYPOT                  5

#define ERS_DEFAULT                             1
#define ERS_LINK                                1
#define ERS_TEXT                                2
#define ERS_SIMPLE                              2
#define ERS_OFF                                 -1
#define ERS_NONE                                -1

#define VALID_URL_CHARS                         "0123456789abcdefghijklmnopqrstuvwxyz/%+?=&#:@_.,"
#define VALID_TOKEN_CHARS                       "0123456789abcdefghijklmnopqrstuvwxyz"  // adding additional valid characters to a token could require additional coding of directive/handler functions
#define VALID_DOMAIN_CHARS                      "0123456789abcdefghijklmnopqrstuvwxyz."
#define VALID_HEXA_CHARS                        "0123456789abcdef"
#define VALID_ALPHA_CHARS                       "abcdefghijklmnopqrstuvwxyz"
#define WHITESPACE_CHARS                        "\t\r\n "
#define CRLF_STR		                        "\r\n"

#ifndef MAX_RBL_DIRECTIVES                      // the "#ifndef" is to detect if the module was compiled with an argument to override this #define
    #define MAX_RBL_DIRECTIVES                  200
#endif
#define MIN_404_INTERVAL                        60
#define MIN_404_RECORD_COUNT                    10
#define MAX_404_RECORD_COUNT                    1000
#define MAX_404_RETRIES                         100

#define TEST_INTERNET_DOMAIN                    "www.projecthoneypot.org"
#define TEST_RBL_DOMAIN                         "1.1.1.127.dnsbl.httpbl.org"
#define TEST_RBL_RESPONSE                       "127.2.3.4"
#define TEST_DOMAIN3                            "dnsbl.httpbl.org"

#define DEF_SOCK_TIMEOUT	                    (APR_USEC_PER_SEC * 2)  // socket timeout interval for external connection
#define BUFSIZE			                        32768
#define MAX_POST_LENGTH                         500                     // the max size of GET/POST parameters (in kiloBytes) to forward to a honeypot

#define DEFAULT_ACTION                          HTTPBL_ACTION_ALLOW

/* BEGIN DEVELOPMENT #DEFINEs */
// SHOULD_CACHE, SHOULD_404, and SHOULD_... are triggers to quickly disable/enable features for debugging purposes
// comment out any one of these #defines to toggle that mode/feature off
#define SHOULD_NTT_INSERT
//#define SHOULD_CACHE
//#define SHOULD_SUBMIT_404s
//#define SHOULD_SUBMIT_POSTs                                             // for after successful splitting of 404s and POSTs
//#define SHOULD_ALLOW_CHALLENGES
#define SHOULD_REQUEST_HONEYPOTS
//#define CACHE_WITH_SHM

/*
 * possible values for VERBOSITY are as follows:
 * APLOG_EMERG    0
 * APLOG_ALERT    1
 * APLOG_CRIT     2
 * APLOG_ERR      3
 * APLOG_WARNING  4
 * APLOG_NOTICE   5
 * APLOG_INFO     6
 * APLOG_DEBUG    7
 */
#define VERBOSITY                               APLOG_ERR                               // under debug mode, a HUGE amount of logging is done
                                                                                        // the LogDir directive MUST be set to a valid writeable directory (SegFaults otherwise)
//#define IS_TEST_MODE                                                                    // under test mode, POST params will be human easily-readable, rather than GET-URL formatted KVPs

#define DEFAULT_RBL_SERVER_DOMAIN               "dnsbl.httpbl.org"                       // HTTPBL domain with global zone (returns all possible data on a given IP)

#ifdef IS_TEST_MODE
    #define LINE_SEPARATOR                          CRLF_STR
    #define FIELD_SEPARATOR                         "\t"
#else                                           
    #define LINE_SEPARATOR                          ""
    #define FIELD_SEPARATOR                         "&"
#endif
/* END DEVELOPMENT #DEFINEs */

// from IPC example module
#if !defined(OS2) && !defined(WIN32) && !defined(BEOS) && !defined(NETWARE)
    #include "unixd.h"
    #define MOD_EXIPC_SET_MUTEX_PERMS // XXX Apache should define something
#endif
/*  -------------------------------------------------------------------- */
/* End #defines Section */



/* Begin #includes Section */
/*  -------------------------------------------------------------------- */
#include "apr.h"
#include "apr_version.h"
#include "apr_signal.h"
#include "apr_buckets.h"
#include "apr_strings.h"
#include "apr_network_io.h"
#include "apr_file_io.h"
#include "apr_time.h"
#include "apr_getopt.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_portable.h"
#include "ap_config.h"
#include "ap_release.h"
#include "apr_poll.h"
#define APR_WANT_STRFUNC
#include "apr_want.h"
#include "apr_base64.h"
#if APR_HAVE_STDIO_H
    #include <stdio.h>
#endif
#if APR_HAVE_STDLIB_H
    #include <stdlib.h>
    #include <math.h>
#endif
#if APR_HAVE_CTYPE_H
    #include <ctype.h>
#endif
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <syslog.h>
#include <errno.h>
#include <netdb.h>
#include <time.h>
#include <math.h>
#include <stdarg.h>
#if APR_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if APR_HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "httpd.h"
#include "http_main.h"
#include "http_core.h"  
#include "http_config.h"
#include "http_log.h"
#include "http_request.h"
#include "http_protocol.h"
#include "ap_config.h"
#include "apr_buckets.h"
#include "util_script.h"
#include "util_filter.h"
#include "apr_tables.h"
#include "http_vhost.h"
#include "http_config.h"
#include "pcre.h"
//#include <pcre/pcre.h>
/*  -------------------------------------------------------------------- */
/* End #includes Section */


// declare the module "httpbl_module"
module AP_MODULE_DECLARE_DATA httpbl_module;

/* Begin Struct Definition Section */
struct ab_connection
{
    apr_pool_t*         ctx;
    apr_socket_t*       aprsock;
    int                 state;
    apr_size_t          read;           // amount of bytes read
    apr_size_t          bread;          // amount of body read
    apr_size_t          rwrite;
    apr_size_t          rwrote;         // keep pointers in what we write - across EAGAINs
    apr_size_t          length;         // Content-Length value used for keep-alive
    char                cbuff[CBUFFSIZE];// a buffer to store server response header
    int                 cbx;            // offset in cbuffer
    int                 keepalive;      // non-zero if a keep-alive request
    int                 gotheader;      // non-zero if we have the entire header in cbuff
    apr_time_t          start;          // Start of connection
    apr_time_t          connect;        // Connected, start writing
    apr_time_t          endwrite;       // Request written
    apr_time_t          beginread;      // First byte of input
    apr_time_t          done;           // Connection closed
    int                 socknum;
};

struct ab_data
{
    int                 read;           // number of bytes read
    apr_time_t          starttime;      // start time of connection in seconds since Jan. 1, 1970
    apr_interval_time_t waittime;       // Between writing request and reading response
    apr_interval_time_t ctime;          // time in ms to connect
    apr_interval_time_t time;           // time in ms for connection
};

struct ext_connection_params
{
    char*               full_url;               // external connection URL-path (everything in a full URL after the TLD of the domain name) to POST to
    char*               post_url;               // external connection URL-path (everything in a full URL after the TLD of the domain name) to POST to
    apr_file_t*         debug_logfile;          // external connection logfile (if debugging is enabled)
    char*               hostname;               // external connection host IP (or domain name)
    apr_port_t          hostport;               // external connection host PORT
    char*               proxy_ip;               // external connection proxy IP (or domain name), if proxy needed
    apr_port_t          proxy_port;             // external connection proxy PORT, if proxy needed
    char*               http_auth_user;         // external connection HTTP auth username, if HTTP authorization is needed
    char*               http_auth_pass;         // external connection HTTP auth password, if HTTP authorization is needed
    char*               colonhost;
    char*               request_data;           // external connection request data (including header)
    char*               response_data;          // external connection response data (including header)
    char*               response_content;       // external connection response data
    apr_uri_t*          parsed_uri;
    char*               method;
    char*               content_type;
};

/* NTT Root Tree */
struct ntt
{
    long                size;
    long                items;
    struct ntt_node**   tbl;
};

/* NTT Node (entry in the ntt root tree) */
struct ntt_node
{
    char*               key;
    char*               rbl_value;
    apr_time_t          timestamp;
    long                count;
    struct ntt_node*    next;
};

/* NTT Cursor */
struct ntt_c
{
    long                iter_index;
    struct ntt_node*    iter_next;
};

/* er (email-replacement) structure */
typedef struct er_struct
{
    apr_bucket_brigade* bb;
    char*               word;
} er_struct;

/* RBL Handler - store directives for RBL buckets */
typedef struct
{
    /* all integers are valid only between 1-255; 0 and below are sentinel values */
    int                 score_lb;       // lower bound for the score
    int                 score_ub;       // upper bound ...
    int                 days_lb;        // lower bound for the number of days since last activity
    int                 days_ub;        // upper bound ...

    /* a bitset representing any/all categories of offenses */
    unsigned long       category_bs;    // the bitset 

    /* a bitset representing any/all HTTP "verbs" (GET, POST, HEAD, etc.) */
    apr_uint64_t        verb_bs;

    /* an additional member to designate what action is to be taken for all matching requests */
    char*               action_string;
} rbl_handler;

/* httpbl_dir_cfg - a configuration structure for directory-based RBL configuration */
typedef struct
{
    int                 is_httpbl_enabled;
    int                 is_404_recording_enabled;
    int                 is_POST_recording_enabled;
    apr_table_t*        bucket_table;
    rbl_handler**       the_rbl_handlers;
    int                 num_of_rbl_handlers;
    int                 is_exempt;
    char*               dirpath;        // used exclusively for IDing the serve/dir with which this **r_cfg is associated
    char*               token_str;
    char*               challenge_url;
    char*               honeypot_url;
    char*               access_key;
    char*               dns_lookup_domain;
    char*               email_rewrite_text;
    char*               email_rewrite_link;
    int                 ers;
    int                 default_action;
} httpbl_dir_cfg;



/*
 *   ;
 */
enum
{
    BUCKETACTION_ALLOW       = 0,
    BUCKETACTION_CHALLENGE   = 1,
    BUCKETACTION_DENY        = 2
};

/*
 *   ;
 */
enum
{
    httpbl_test_not_tried   = 0,
    httpbl_test_passed      = 1,
    httpbl_test_failed      = 2
};
/* End Struct-Definition Section */








static char* get_log_dir();
static char* get_repos_dir();










// 
typedef struct exipc_data
{
    apr_uint64_t counter; 
    // More fields if necessary
} exipc_data;





apr_shm_t*          hl_shm          = NULL; // hitlist sharedmemory
apr_shm_t*          wl_shm          = NULL; // whitelist sharedmemory
apr_shm_t*          cl_shm          = NULL; // cleanlist sharedmemory
apr_shm_t*          FOFcd_shm       = NULL; // cachedata sharedmemory
apr_shm_t*          FOFcm_shm       = NULL; // cachemeta sharedmemory

char*               hl_shm_fn       = NULL; // hitlist sharedmemory filename
char*               wl_shm_fn       = NULL; // whitelist sharedmemory filename
char*               cl_shm_fn       = NULL; // cleanlist sharedmemory filename
char*               FOFcd_shm_fn    = NULL; // cachedata sharedmemory filename
char*               FOFcm_shm_fn    = NULL; // cachemeta sharedmemory filename

apr_global_mutex_t* hl_mutex        = NULL; // hitlist mutex
apr_global_mutex_t* wl_mutex        = NULL; // whitelist mutex
apr_global_mutex_t* cl_mutex        = NULL; // cleanlist mutex
apr_global_mutex_t* FOFcd_mutex     = NULL; // cachedata mutex
apr_global_mutex_t* FOFcm_mutex     = NULL; // cachemeta mutex

char*               hl_mutex_fn     = NULL; // hitlist mutex filename
char*               wl_mutex_fn     = NULL; // whitelist mutex filename
char*               cl_mutex_fn     = NULL; // cleanlist mutex filename
char*               FOFcd_mutex_fn  = NULL; // cachedata mutex filename
char*               FOFcm_mutex_fn  = NULL; // cachemeta mutex filename

int                 g_ipc_counter   = 0;






























/* Begin Function Prototype Section */
// Yahoo URL-safe string functions
static int              isurlchar                           (unsigned char);
char*                   yahoo_urlencode                     (apr_pool_t*,       const char*);
char*                   yahoo_urldecode                     (apr_pool_t*,       const char*);
static int              charcnt                             (const char*,       char);
// General utility functions (string manipulation and testing)
static int              httpbl_string_matches               (const char*,       const char*);
char*                   apr_ptrim                           (apr_pool_t*,       const char*);
char*                   apr_ptrim_head                      (apr_pool_t*,       const char*);
char*                   apr_ptrim_tail                      (apr_pool_t*,       const char*);

int                     is_empty_string                     (char*);

static long             get_response_code_from_repsonse_data(apr_pool_t*,       char*);
static int              parse_url_into_conx                 (apr_pool_t*,       struct ext_connection_params*,          apr_file_t*);
/* IP parsing and variable translation (hostent, string, int)  */
static int              getOctetIntFromIPString             (apr_pool_t*,       const char*, int);
static char*            getIPStringFromHostent              (struct hostent*,   request_rec*);
// initialize and reclaim the global NTT structures
static void             create_white_list                   (apr_pool_t *);
static void             create_hit_list                     (apr_pool_t *);
static void*            init_httpbl_structures              (apr_pool_t *,      server_rec*);
static apr_status_t     destroy_hit_list                    (void*);
// Access and/or Mutate 404 cache and config globals
//static void             init_default_httpbl_404_config      (apr_pool_t*);
static int              clear_httpbl_404_cache              (apr_pool_t*);
static int              serialize_404_cache_meta            (apr_pool_t*);
static int              unserialize_404_cache_meta          (apr_pool_t*);
static int              areEnough404sToPost                 ();
static int              enoughTimeHasPassedToPost           (apr_pool_t* p);
static int              cacheFileIsSmallEnoughToPost        (apr_pool_t*);
static int              enoughPostTriesHaveElapsed          ();
// NTT-specific functions
long                    ntt_hashcode                        (struct ntt*,       const char*);
struct ntt_node*        ntt_node_create                     (const char*);
struct ntt*             ntt_create                          (long);
struct ntt_node*        ntt_find                            (/*apr_pool_t*, */  struct ntt*,        const char* key);
struct ntt_node*        ntt_float                           (/*apr_pool_t*, */  struct ntt*,        long, struct ntt_node*, struct ntt_node*);
struct ntt_node*        ntt_delete_below                    (apr_pool_t*,       struct ntt*,        long, struct ntt_node*);
struct ntt_node*        ntt_insert                          (/*apr_pool_t*, */  struct ntt*,        const char*,            const char*,        apr_time_t);
int                     ntt_delete                          (/*apr_pool_t*, */  struct ntt*,        const char*);
int                     ntt_destroy                         (/*apr_pool_t*, */  struct ntt*);
struct ntt_node*        c_ntt_first                         (/*apr_pool_t*, */  struct ntt*,        struct ntt_c*);
struct ntt_node*        c_ntt_next                          (/*apr_pool_t*, */  struct ntt*,        struct ntt_c*);
static int              ntt_is_expired                      (struct ntt_node*);
// Whitelist-specific functions
static int              whitelist_insert                    (apr_pool_t*,       const char*);
int                     is_whitelist_outofdate              ();
int                     unserialize_hashlist                (apr_pool_t*);
int                     unserialize_whitelist               (apr_pool_t*);
int                     unserialize_hashtable_from_file     (apr_pool_t*,       apr_file_t*,        apr_file_t*,            struct ntt*);
int                     serialize_hitlist_to_file           (apr_pool_t*);
int                     serialize_whitelist_to_file         (apr_pool_t*);
int                     serialize_hashtable_to_file         (apr_pool_t*,       apr_file_t*,        apr_file_t*,            struct ntt*);
// Testing Functions for outputting specific text to the browser
static void             outputIPOkayJSAlertString           (request_rec*);
static void             outputIPAddressJSAlertString        (request_rec*);
// Server and Directory Configuration Functions
static void*            httpbl_create_svr_conf              (apr_pool_t*,       server_rec*);
static void*            httpbl_create_dir_conf              (apr_pool_t*,       char*);
static void*            httpbl_merge_dir_conf               (apr_pool_t*,       void*,              void*);
// Apache Directive-handlers
static const char*      directive_add_to_whitelist          (cmd_parms*,        void*,              const char*);
static const char*      directive_set_forbidden_template_uri(cmd_parms*,        void*,              const char*);
static const char*      directive_set_hash_tbl_size         (cmd_parms*,        void*,              const char*);
static const char*      directive_set_blocking_period       (cmd_parms*,        void*,              const char*);
static const char*      directive_set_log_dir               (cmd_parms*,        void*,              const char*);
// Page-handlers
static int              is_access_allowed                   (apr_pool_t*,       httpbl_dir_cfg*,    httpbl_dir_cfg*,        char*, request_rec*);
static int              handle_honeypot_request             (request_rec*);
static int              handle_404_recording                (request_rec*);
//static int              handle_this_request_as_a_honeypot   (request_rec* r);
static char*            check_via                           (request_rec*,      const char*);
static int              access_checker                      (request_rec*);
static void             register_hooks                      (apr_pool_t*);


static char*            get_post_kvp_string                 (request_rec*,      long);
/* End Function Prototype Section */


/* Begin HTTPBL Globals definitions section */
static struct ntt*                  hit_list                            = NULL; // Our dynamic hash table
static struct ntt*                  white_list                          = NULL; // Our dynamic hash table for whitelisted addresses
static unsigned long                reccomended_hash_table_size         = DEFAULT_HASH_TBL_SIZE;
    static unsigned long                actual_hash_table_size              = DEFAULT_HASH_TBL_SIZE;
static int                          blocking_period                     = DEFAULT_BLOCKING_PERIOD;
static char*                        repos_dir                           = NULL;
static char*                        log_dir                             = NULL;
static char*                        forbidden_template                  = NULL;
static char*                        whitelist_filepath                  = NULL;
static char*                        hitlist_filepath                    = NULL;

// Four-Oh-Four globals for in-memory metadata
static apr_time_t                   g_FOF_last_post_time                = 0;
static int                          g_FOF_cur_count                     = 0;
static int                          g_FOF_cur_tries                     = 0;
// Four-Oh-Four globals for config data
static int                          g_FOF_enable_404_capture            = UNSET_INT;// default to 404 capture ON
static int                          g_FOF_enable_POST_capture           = UNSET_INT;// default to POST capture ON
static char*                        g_FOF_post_url                      = NULL;
static int                          g_FOF_capture_enabled               = UNSET_INT;
static char*                        g_FOF_cache_datafile_path           = NULL;
static char*                        g_FOF_cache_metafile_path           = NULL;
static int                          g_FOF_min_interval                  = 180;
static int                          g_FOF_max_timeout                   = 2;
static int                          g_FOF_min_count                     = 10;
static int                          g_FOF_max_retries                   = 4;
static char*                        g_FOF_post_proxy_ip                 = NULL;
static char*                        g_FOF_post_proxy_port               = NULL;
static char*                        g_FOF_post_httpauth_un              = NULL;
static char*                        g_FOF_post_httpauth_pw              = NULL;

static char*                        g_FOF_cache_data_filepath           = NULL;
static char*                        g_FOF_cache_meta_filepath           = NULL;
static char*                        g_httpbl_log_filepath               = NULL;
static char*                        g_whitelist_testing_filepath        = NULL;
static char*                        g_honeypot_testing_filepath         = NULL;
static char*                        g_httpbl_testing_filepath           = NULL;
static char*                        g_rbl_toggle_log_filepath           = NULL;
static char*                        g_dir_cfg_log_filepath              = NULL;
static char*                        g_httbl_merge_trace_log_filepath    = NULL;
static char*                        g_httpbl_replace_emails_log_filepath= NULL;
static char*                        g_httpbl_testing_url                = DEFAULT_HTTPBL_TESTING_URL;
static apr_time_t                   g_whitelist_last_modtime            = 0;
static apr_time_t                   g_hitlist_last_modtime              = 0;
static int                          g_enable_rbl_lookup                 = UNSET_INT;// default to RBL lookup ON

static int                          g_an_access_key_was_set             = 0;
static char*                        g_FOF_cache_string                  = NULL;
static int                          g_FOF_cache_string_l                = 0;
/* End HTTPBL Globals Section */







/*
 *   Begin function definitions
 */


/* 
 *   isurlchar - return 1/TRUE if the arguement (char) does not need ot be escaped to keep a string URL-safe
 *   From Yahoo!'s C library
 *   @return 1 iff character c is a character that DOES NOT need to be URL-encoded
 */
static int isurlchar(unsigned char c)
{
    return(apr_isalnum(c) || '/' == c || '%' == c || '?' == c || '&' == c || '#' == c || '.' == c || '-' == c || '_' == c);
}

/*
 *   make and return a URL-safe string from the parameter string (instr), modified for the APR library to use apr_pool_t for pool resource allocation
 *   requires 'isurlchar (char)'
 *   From Yahoo!'s C library
 *   future to-do: should be modified to work with APR memory pools
 */
char* yahoo_urlencode(apr_pool_t* p, const char* instr)
{

    int     ipos    = 0;
    int     bpos    = 0;
    char*   str     = NULL;
    int     len     = 0;

    if(!instr)
        return apr_pstrdup(p, "");

    len     = strlen(instr);

    if(!(str=apr_palloc(p, sizeof(char)*(3*len+1))))
        return "";

    while(instr[ipos])
    {
        while(isurlchar(instr[ipos]))
            str[bpos++] = instr[ipos++];
        if(!instr[ipos])
            break;

        snprintf(&str[bpos], 4, "%%%.2x", instr[ipos]);
        bpos    +=3;
        ipos++;
    }
    str[bpos]   = '\0';

    return(str);
}

/*
 *   make and return a URL-decoded string from the parameter string (instr), modified for the APR library to use apr_pool_t for pool resource allocation
 *   requires 'isurlchar (char)'
 *   From Yahoo!'s C library
 */
char* yahoo_urldecode(apr_pool_t* p, const char* instr)
{
	int         ipos        = 0,
                bpos        = 0;
	char*       str         = NULL;
	char        entity[3]   = {0,0,0};
	unsigned    dec         = 0;
	int         len         = 0;

    if(!instr)
        return apr_pstrdup(p, "");

    len = strlen(instr);

    if(!(str = apr_palloc(p, sizeof(char)*(3*len+1))))
        return "";

	while(instr[ipos])
    {
		while(instr[ipos] && instr[ipos] != '%')
			if(instr[ipos] == '+')
            {
				str[bpos++] = ' ';
				ipos++;
			}
            else
            {
				str[bpos++] = instr[ipos++];
            }
		if(!instr[ipos])
			break;

		if(instr[ipos+1] && instr[ipos+2])
        {
			ipos++;
			entity[0]   = instr[ipos++];
			entity[1]   = instr[ipos++];
			sscanf(entity, "%2x", &dec);
			str[bpos++] = (char)dec;
		}
        else
        {
			str[bpos++] = instr[ipos++];
		}
	}
	str[bpos]   = '\0';

	/* free extra alloc'ed mem. */
	len     = strlen(str);

	return (str);
}

/*
 *   a wrapper for "strcmp(char*, char*)" (which doesn't die on NULL strings)
 *   returns 1 if the strings have identical string content (before the first '\0') or are both NULL
 *           0 if only one string is NULL or the string contents of the two strings differ before the first '\0'
 */
static int httpbl_string_matches(const char* str1, const char* str2)
{
    if(str1==str2) // if the strings are identical (memory locations)
        return 1;

    if(str1==NULL || str2==NULL) // if only one of the strings is NULL
        return 0;

    return (strcmp(str1, str2)==0); // otherwise depend on strcmp to do the comparison
}

/*
 *   a wrapper for "strncmp(char*, char*)" (which doesn't die on NULL strings)
 *   returns 1 if the strings have identical string content (before the first '\0') or are both NULL
 *           0 if only one string is NULL or the string contents of the two strings differ before the first '\0'
 */
static int httpbl_string_nmatches(const char* str1, const char* str2, int match_length)
{
    if(str1==str2) // if the strings are identical (memory locations)
        return 1;

    if(str1==NULL || str2==NULL) // if only one of the strings is NULL
        return 0;

    return (strncmp(str1, str2, match_length)==0); // otherwise depend on strcmp to do the comparison
}


/*
 *   Determine if an action is not equal to the DEFAULT_ACTION_UNSET value (therefore the action has been set)
 *   @return TRUE (non-zero) IFF the_int is not equal to DEFAULT_ACTION_UNSET
 */
static int is_set_action(int the_action)
{
    return (the_action != DEFAULT_ACTION_UNSET);
}


/*
 *   Determine if an integer is not equal to the UNSET_INT value (therefore the int has been set)
 *   @return TRUE (non-zero) IFF the_int is not equal to UNSET_INT
 */
static int is_set_int(int the_int)
{
    return (the_int != UNSET_INT);
}


/*
 *   Determine if an integer is not equal to the UNSET_INT value (therefore the int has been set)
 *   This function calls is_set_int(...) to check for UNSET_INT
 *   @return TRUE (non-zero) IFF the_int is not equal to UNSET_INT and is non-zero.
 *   @return FALSE (non-zero) IFF the_int is equal to UNSET_INT or is zero.
 *
 */
static int is_enabled_int(int the_int)
{
    return (is_set_int(the_int) && the_int);
}


/*
 *   Given an action (int/enum), return a human-readable string representing that action.
 */
static char* get_action_printable_string(int the_action)
{
    char*   the_return_value    = NULL;

    switch(the_action)
    {
    case HTTPBL_ACTION_ALLOW:
        the_return_value    = "ALLOW";
        break;
    case HTTPBL_ACTION_DENY:
        the_return_value    = "DENY";
        break;
#ifdef SHOULD_ALLOW_CHALLENGES
    case HTTPBL_ACTION_CHALLENGE:
        the_return_value    = "CHALLENGE";
        break;
#endif
    case HTTPBL_ACTION_ALLOW_XLATE_EMAILS:
        the_return_value    = "ALLOW_XLATE_EMAILS";
        break;
#ifdef SHOULD_REQUEST_HONEYPOTS
    case HTTPBL_ACTION_HONEYPOT:
        the_return_value    = "HONEYPOT";
        break;
#endif
    case DEFAULT_ACTION_UNSET:
        the_return_value    = "UNSET";
        break;
    default:
        the_return_value    = "NULL";
        break;
    }

    return the_return_value;
}





/*
 *   Get the value of the log_dir global variable (the server-wide path to the httpbl debug logs directory)
 */
static char* get_log_dir()
{
    return (log_dir);
}



/*
 *   Get the value of the repos_dir global variable (the server-wide path to the httpbl cachefile directory)
 */
static char* get_repos_dir()
{
    return (repos_dir);
}



/*
 *   Get a string containing the sentinel IPv4 address used to identify whitelisted IPs
 *   @return a newly allocated (from the p parameter) C-string containing the sentinel IP string (used to detect a clean RBL response)
 */
static char* get_whitelist_rbl_value(apr_pool_t* p)
{
    return apr_pstrdup(p, WHITELIST_IP_ADDRESS);
}



/*
 *   Get a string containing the sentinel IPv4 address used to identify whitelisted IPs
 *   @return a newly allocated (from the p parameter) C-string containing the sentinel IP string (used to detect a clean RBL response)
 */
static char* get_cleanlist_rbl_value(apr_pool_t* p)
{
    return apr_pstrdup(p, CLEANLIST_IP_ADDRESS);
}



/*
 *   @return 1 iff the_string contains the same data before the first '\0'
 */
static int string_matches_whitelist_rbl_value(const char* the_string)
{
    return httpbl_string_matches(the_string, WHITELIST_IP_ADDRESS);
}



/*
 *   @return 1 iff the_string contains the same data before the first '\0'
 */
static int string_matches_cleanlist_rbl_value(const char* the_string)
{
    return httpbl_string_matches(the_string, CLEANLIST_IP_ADDRESS);
}


/*
 *   get the string representing the beginning of the URL-path for honeypot requests
 */
static char* get_this_requests_honeypot_url(request_rec* r)
{
    char*   the_url = NULL;

    httpbl_dir_cfg* svr_cfg = (httpbl_dir_cfg*)ap_get_module_config(r->server->module_config, &httpbl_module);
    the_url = svr_cfg->honeypot_url;

    return the_url;
}


/*
 *   get the string representing the beginning of the URL-path for diagnostics requests
 */
static char* get_this_requests_g_httpbl_testing_url(request_rec* r)
{
    return g_httpbl_testing_url;
}

/*
 *   get the string representing the link to replace email address links ("mailto:"s) for this context
 */
static char* get_email_rewrite_link(request_rec* r)
{
        httpbl_dir_cfg* svr             = (httpbl_dir_cfg*)ap_get_module_config(r->server->module_config, &httpbl_module);
        httpbl_dir_cfg* dir             = (httpbl_dir_cfg*)ap_get_module_config(r->per_dir_config, &httpbl_module);

        if(dir->email_rewrite_link)
            return dir->email_rewrite_link;
        else if(svr->email_rewrite_link)
            return svr->email_rewrite_link;
        else
            return DEFAULT_EMAIL_REWRITE_LINK;
}

/*
 *   get the string representing the text to replace email addresses for this context
 */
static char* get_email_rewrite_text(request_rec* r)
{
        httpbl_dir_cfg* svr             = (httpbl_dir_cfg*)ap_get_module_config(r->server->module_config, &httpbl_module);
        httpbl_dir_cfg* dir             = (httpbl_dir_cfg*)ap_get_module_config(r->per_dir_config, &httpbl_module);

        if(dir->email_rewrite_text)
            return dir->email_rewrite_text;
        else if(svr->email_rewrite_text)
            return svr->email_rewrite_text;
        else
            return DEFAULT_EMAIL_REWRITE_TEXT;
}

/*
    Return the "default action" set by a configuration (dir has priority over svr) from a given request_rec*
    @param      r   the request_rec* of the request to be handled
    @return         the DEFAULT_ACTION value (int)
*/
static int get_default_action_from_request(request_rec *r)
{
    server_rec*     svr         = r->server;
    httpbl_dir_cfg* dir_cfg     = (httpbl_dir_cfg*)ap_get_module_config(r->per_dir_config, &httpbl_module);
    httpbl_dir_cfg* svr_cfg     = (httpbl_dir_cfg*)ap_get_module_config(svr->module_config, &httpbl_module);

    if(dir_cfg->default_action == HTTPBL_ACTION_ALLOW           ||
       dir_cfg->default_action == HTTPBL_ACTION_DENY            ||
#ifdef SHOULD_ALLOW_CHALLENGES
       dir_cfg->default_action == HTTPBL_ACTION_CHALLENGE       ||
#endif
#ifdef SHOULD_REQUEST_HONEYPOTS
       dir_cfg->default_action == HTTPBL_ACTION_HONEYPOT        ||
#endif
       0)
        return dir_cfg->default_action;
    else if(svr_cfg->default_action == HTTPBL_ACTION_ALLOW      ||
            svr_cfg->default_action == HTTPBL_ACTION_DENY       ||
#ifdef SHOULD_ALLOW_CHALLENGES
            svr_cfg->default_action == HTTPBL_ACTION_CHALLENGE  ||
#endif
#ifdef SHOULD_REQUEST_HONEYPOTS
            svr_cfg->default_action == HTTPBL_ACTION_HONEYPOT   ||
#endif
            0)
        return svr_cfg->default_action;
    else // if all else fails (no configuration 'default_action's set), fall back to the globally defined constant DEFAULT_ACTION
        return DEFAULT_ACTION;
}


/*
 *   If the parameter integer is not equal to a defined DEFAULT_ACTION, set it to the preprocessor constant DEFAULT_ACTION
 *   @return coalesce(the_return_value, DEFAULT_ACTION)
 *   @param the_value_to_coalesce an integer to check against all valid defined DEFAULT_ACTIONS
 */
static int coalesce_default_action(int the_value_to_coalesce)
{
    int the_return_value    = DEFAULT_ACTION;
    if(the_value_to_coalesce == HTTPBL_ACTION_ALLOW             ||
       the_value_to_coalesce == HTTPBL_ACTION_CHALLENGE         ||
#ifdef SHOULD_ALLOW_CHALLENGES
       the_value_to_coalesce == HTTPBL_ACTION_DENY              ||
#endif
#ifdef SHOULD_REQUEST_HONEYPOTS
       the_value_to_coalesce == HTTPBL_ACTION_HONEYPOT          ||
#endif
       0)
        the_return_value    = the_value_to_coalesce;

    return the_return_value;
}


/*
    @return OK if rbl is exempted for this request
    @return DECLINED if the rbl is exempted for this request
*/
static char* get_access_key_for_this_request(request_rec* r, httpbl_dir_cfg* this_dir_cfg, httpbl_dir_cfg* this_svr_cfg)
{
    char*   the_return_value        = NULL;

    if(!this_dir_cfg) // get the directory config from the request_rec if is wasn't passed
        this_dir_cfg    = (httpbl_dir_cfg*)ap_get_module_config(r->per_dir_config, &httpbl_module);

    if(!this_svr_cfg) // get the server module configs from the request_rec if is wasn't passed
        this_svr_cfg    = (httpbl_dir_cfg*)ap_get_module_config(r->server->module_config, &httpbl_module);

    if (this_dir_cfg && this_svr_cfg)
    {
        if(this_dir_cfg->access_key)
        {
            the_return_value    = this_dir_cfg->access_key;
        }
        else if(this_svr_cfg->access_key)
        {
            the_return_value    = this_svr_cfg->access_key;
        }
    }
    else if(this_dir_cfg)
    {
        if(this_dir_cfg->access_key)
        {
            the_return_value    = this_dir_cfg->access_key;
        }
    }
    else if(this_svr_cfg)
    {
        if(this_svr_cfg->access_key)
        {
            the_return_value    = this_svr_cfg->access_key;
        }
    }
    return the_return_value;
}


/*
    ;
*/
static char* get_rbl_domain(request_rec* r, void* dir_cfg, void* svr_cfg, char* default_rbl_domain)
{
    httpbl_dir_cfg* this_dir_cfg    = (httpbl_dir_cfg*)dir_cfg;
    httpbl_dir_cfg* this_svr_cfg    = (httpbl_dir_cfg*)svr_cfg;
    char*   the_return_value        = NULL;

    // preference order: dir_cfg, svr_cfg, default_rbl_domain (as passed)
    if(this_dir_cfg && this_svr_cfg)
    {
        if(this_dir_cfg->dns_lookup_domain)
        {
            the_return_value = apr_pstrdup(r->pool, this_dir_cfg->dns_lookup_domain);
        }
        else
        {
            if(this_svr_cfg->dns_lookup_domain)
            {
                the_return_value = apr_pstrdup(r->pool, this_svr_cfg->dns_lookup_domain);
            }
            else
            {
                the_return_value = apr_pstrdup(r->pool, default_rbl_domain);
            }
        }
    }
    else if(this_dir_cfg)
    {
        if(this_dir_cfg->dns_lookup_domain)
            the_return_value = apr_pstrdup(r->pool, this_dir_cfg->dns_lookup_domain);
        else
            the_return_value = apr_pstrdup(r->pool, default_rbl_domain);
    }
    else if(this_svr_cfg)
    {
        if(this_svr_cfg->dns_lookup_domain)
            the_return_value = apr_pstrdup(r->pool, this_svr_cfg->dns_lookup_domain);
        else
            the_return_value = apr_pstrdup(r->pool, default_rbl_domain);
    }

    if(!the_return_value)
    {
        the_return_value  = apr_pstrdup(r->pool, default_rbl_domain);
    }

    return the_return_value;
}


/*
    @return OK          if rbl is exempted for this request
    @return DECLINED    if the rbl is exempted for this request
*/
static int is_httpbl_rbl_exempted_for_this_request(request_rec* r, httpbl_dir_cfg* this_dir_cfg, httpbl_dir_cfg* this_svr_cfg)
{
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "HTTPBL: - entered is_httpbl_rbl_exempted_for_this_request(r, dir: %p, svr: %p)", this_dir_cfg, this_svr_cfg);

    if (this_dir_cfg && this_svr_cfg)
    {
        if(is_enabled_int(this_dir_cfg->is_exempt))
        {
            ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: - Access granted via exemption @ dir_cfg level (1)");
            return DECLINED;
        }
        else if(!is_set_int(this_dir_cfg->is_exempt) && is_enabled_int(this_svr_cfg->is_exempt))
        {
            ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: - Access granted via exemption @ svr_cfg level (1)");
            return DECLINED;
        }
    }
    else if(this_dir_cfg)
    {
        if(is_enabled_int(this_dir_cfg->is_exempt))
        {
            ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: - Access granted via exemption @ dir_cfg level (2)");
            return DECLINED;
        }
    }
    else if(this_svr_cfg)
    {
        if(is_enabled_int(this_svr_cfg->is_exempt))
        {
            ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: - Access granted via exemption @ svr_cfg level (2)");
            return DECLINED;
        }
    }
    else
    {
        return DECLINED;
    }

    ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: - is_httpbl_rbl_exempted_for_this_request(...) returning Not Exempt.");
    return OK;
}


/*
    @return OK          if rbl is enabled for this request
    @return DECLINED    if the rbl is disabled for this request
*/
static int is_httpbl_rbl_enabled_for_this_request(request_rec* r, httpbl_dir_cfg* this_dir_cfg, httpbl_dir_cfg* this_svr_cfg)
{
    if (this_dir_cfg && this_svr_cfg)
    {
        // if HTTPBL is disabled at the directory level (which takes priority)
        if(is_set_int(this_dir_cfg->is_httpbl_enabled) &&
           !is_enabled_int(this_dir_cfg->is_httpbl_enabled))
            return DECLINED;

        // if HTTPBL is not set at the directory level, but is disabled at the server level
        if(!is_set_int(this_dir_cfg->is_httpbl_enabled) &&
           is_set_int(this_svr_cfg->is_httpbl_enabled) &&
           !is_enabled_int(this_svr_cfg->is_httpbl_enabled))
            return DECLINED;
    }
    else if(this_dir_cfg)
    {
        // if HTTPBL is disabled at the directory level
        if(is_set_int(this_dir_cfg->is_httpbl_enabled) &&
           !is_enabled_int(this_dir_cfg->is_httpbl_enabled))
            return DECLINED;
    }
    else if(this_svr_cfg)
    {
        // if HTTPBL is disabled at the server level (and no directory config struct is available)
        if(is_set_int(this_svr_cfg->is_httpbl_enabled) &&
           !is_enabled_int(this_svr_cfg->is_httpbl_enabled))
            return DECLINED;
    }
    else
    {
        // we are disabled because we don't have any directory or server config structs to reference
        return DECLINED;
    }

    // we are not disabled; return OK (which will only tell the calling function we are going to continue with this request... not serve the requested page... yet)
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "HTTPBL: - is_httpbl_rbl_enabled_for_this_request(...) returning Enabled.");
    return OK;
}




/*
 *   Init the white list structure
 */
static void create_white_list(apr_pool_t* p)
{
    // Create a new hit list for this child process
    white_list = ntt_create(reccomended_hash_table_size);

#ifdef SHOULD_CACHE
    if(whitelist_filepath)
         // populate the existing whitelist from serialized-whitelist file, if available
        unserialize_whitelist_from_file(p);
#endif
}

/*
 *   Init the hit list structure
 */
static void create_hit_list(apr_pool_t* p)
{
    // Create a new hit list for this child process
    hit_list = ntt_create(reccomended_hash_table_size);

#ifdef SHOULD_CACHE
    if(hitlist_filepath)
        // populate the existing hitlist from serialized-hitlist file
        unserialize_hitlist_from_file(p);
#endif
}

/*
 *   restructure_repos_filepaths
 *   Reset filepaths of all files stored in the repos directory.
 */
static void restructure_repos_filepaths(apr_pool_t* p)
{
    whitelist_filepath          = apr_psprintf(p, "%s%s", (repos_dir)?repos_dir:"", DEFAULT_WHITELIST_CACHEFILE_BASENAME);
    hitlist_filepath            = apr_psprintf(p, "%s%s", (repos_dir)?repos_dir:"", DEFAULT_HITLIST_CACHEFILE_BASENAME);
    g_FOF_cache_data_filepath   = apr_psprintf(p, "%s%s", (repos_dir)?repos_dir:"", DEFAULT_404_CACHE_DATAFILE_BASENAME);
    g_FOF_cache_meta_filepath   = apr_psprintf(p, "%s%s", (repos_dir)?repos_dir:"", DEFAULT_404_CACHE_METAFILE_BASENAME);
}

/*
 *   Initialize the httpbl_404_config attribute values
 */
static void init_default_httpbl_404_config(apr_pool_t* p)
{
    restructure_repos_filepaths(p);
    g_FOF_post_url                  = apr_psprintf(p, DEFAULT_404_URL);
    g_FOF_capture_enabled           = 1;
    g_FOF_min_interval              = 60;
    g_FOF_max_timeout               = 2;
    g_FOF_min_count                 = 10;
    g_FOF_max_retries               = 2;
    g_FOF_post_proxy_ip             = apr_psprintf(p, "");
    g_FOF_post_proxy_port           = apr_psprintf(p, "");
    g_FOF_post_httpauth_un          = apr_psprintf(p, "");
    g_FOF_post_httpauth_pw          = apr_psprintf(p, "");
}

/*
 *   Reset the httpbl_404_cache attribute values
 */
static int clear_httpbl_404_cache(apr_pool_t* p)
{
    apr_status_t    rv              = 1;
    apr_file_t*     fp              = NULL;

    g_FOF_last_post_time                  = apr_time_now(); // set last_post_time to the current timestamp
    g_FOF_cur_count                       = 0;
    g_FOF_cur_tries                       = 0;

#ifndef SHOULD_CACHE
    if(!g_FOF_cache_string)
        free(g_FOF_cache_string);
    g_FOF_cache_string                  = NULL;
    g_FOF_cache_string_l                = 0;
#else
    rv = apr_file_remove(g_FOF_cache_data_filepath, p);

    if(rv != APR_SUCCESS)
    {
        ap_log_error(APLOG_MARK, APLOG_INFO, 0, NULL, "HTTPBL: clear_httpbl_404_cache: Could not delete the httpbl 404 cache datafile (%s).", g_FOF_cache_meta_filepath);
        return 0;
    }

    rv = apr_file_remove(g_FOF_cache_meta_filepath, p);

    if(rv != APR_SUCCESS)
    {
        ap_log_error(APLOG_MARK, APLOG_INFO, 0, NULL, "HTTPBL: clear_httpbl_404_cache: Could not delete the httpbl 404 cache metafile (%s).\n", g_FOF_cache_meta_filepath);
        return 0;
    }
#endif

    return 1;
}

/*
 *   Init the hit list structure
 */
static void* init_httpbl_structures(apr_pool_t* p, server_rec* s) 
{
    httpbl_dir_cfg* the_return_value    = (httpbl_dir_cfg *)apr_palloc( p, sizeof(httpbl_dir_cfg));
//    the_return_value                = httpbl_create_svr_conf(p, s);

    // Create a new hit list for this child process
    if(g_enable_rbl_lookup)
    {
        create_white_list(p);
        create_hit_list(p);
    }

    if(is_enabled_int(g_FOF_enable_404_capture))
    {
        init_default_httpbl_404_config(p);
#ifdef SHOULD_CACHE
        clear_httpbl_404_cache(p);
#endif
    }

    return the_return_value;
}

/*
 *   Destructor for the hit_list
 */
static int destroy_hit_list(void* not_used)
{
    // serialize hit list to hash_list cache file

    return ntt_destroy(hit_list);
}


/*******************************************
 *  NTT Functions
 *******************************************/


enum
{
    ntt_num_primes = 28
};

static unsigned long ntt_prime_list[ntt_num_primes] = 
{
    53ul,         97ul,         193ul,       389ul,       769ul,
    1543ul,       3079ul,       6151ul,      12289ul,     24593ul,
    49157ul,      98317ul,      196613ul,    393241ul,    786433ul,
    1572869ul,    3145739ul,    6291469ul,   12582917ul,  25165843ul,
    50331653ul,   100663319ul,  201326611ul, 402653189ul, 805306457ul,
    1610612741ul, 3221225473ul, 4294967291ul
};

/*
 *   test the timestamp of a given ntt_node and return true if the timestamp is older than 'blocking_period' seconds
 */
static int ntt_is_expired(struct ntt_node *n)
{
    apr_time_t t = apr_time_now();

    return ((t - n->timestamp) >= apr_time_make(blocking_period * 60, 0)); // blocking period is defined in Minutes
}


/*
 *   Find the numeric position in the hash table based on key and modulus
 */
long ntt_hashcode(struct ntt* ntt, const char* key)
{
    unsigned long val = 0;
    while(*key)
    {
        val = 5 * val + *key;
        ++key;
    }
    return(val % ntt->size);
}

/*
 *   Creates a single node in the tree
 */
struct ntt_node* ntt_node_create(const char* key)
{
    struct ntt_node*    node        = (struct ntt_node*)calloc(1, sizeof(struct ntt_node));

    if(!node)
        return NULL;

    if(!(node->key = strdup(key)))
    {
        free(node);
        return NULL;
    }

    node->timestamp     = apr_time_now();
    node->next          = NULL;
    return(node);
}

/*
 *   Tree initializer
 */
struct ntt* ntt_create(long size)
{
    long        i       = 0;
    struct ntt* ntt     = (struct ntt*)calloc(1, sizeof(struct ntt));

    if(!ntt)
        return NULL;

    while(ntt_prime_list[i] < size)
        i++;

    ntt->size  = ntt_prime_list[i];
    ntt->tbl   = (struct ntt_node**)calloc(ntt->size, sizeof(struct ntt_node*));
    if(!ntt->tbl)
    {
        actual_hash_table_size  = 0;
        if(ntt)
            free(ntt);
        ntt = NULL;
    }
    else
    {
        ntt->items = 0;
        actual_hash_table_size  = ntt->size;
    }

    return(ntt);
}

/*
 *   Find an object in the tree
 */
struct ntt_node* ntt_find(struct ntt* ntt, const char* key)
{
    long                hash_code   = 0;
    struct ntt_node*    node        = NULL;

    if(!ntt || !key) // we need both ntt and key to be non-NULL
        return NULL;

    hash_code   = ntt_hashcode(ntt, key);

    if(hash_code < 0 || hash_code >= actual_hash_table_size)
        return NULL;

    node        = ntt->tbl[hash_code];

    while(node && !ntt_is_expired(node))
    {
        if(httpbl_string_matches(key, node->key))
            return(node);
        else
            node        = node->next;
    }

    // expired or does not exist
    return((struct ntt_node*)NULL);
}

/*
 *   Float the node to the top of the bucket and update the timestamp
 */
struct ntt_node* ntt_float(struct ntt* ntt, long hash_code, struct ntt_node* node, struct ntt_node* parent)
{
    struct ntt_node*    tmp_node        = NULL;
    struct ntt_node*    bucket_head     = NULL;
    apr_time_t          time_now        = apr_time_now();

    node->timestamp     = time_now;
    bucket_head         = ntt->tbl[hash_code];
    tmp_node            = node;
    if(parent)
        parent->next        = node->next;
    else                // parent is NULL - this is the only node in the LL
        return node;    // simply update the timestamp

    if(bucket_head)
    {
        node->next          = bucket_head;
        ntt->tbl[hash_code] = node/*->next*/;
    }

    return node;
}

/*
 *   Delete any ntt_nodes older than the parameter 'node' in this bucket
 */
struct ntt_node* ntt_delete_below(apr_pool_t* p, struct ntt* ntt, long hash_code, struct ntt_node* node)
{
    struct ntt_node* tmp_node   = NULL;

    if (node)
    {
//        bucket_head = ntt->tbl[hash_code];
        while (node->next)
        {
            tmp_node    = node->next;
            if(tmp_node)
            {
                node->next      = tmp_node->next;
                free(tmp_node->key);
                tmp_node->key   = NULL;
                free(tmp_node);
                tmp_node        = NULL;
                ntt->items--;
            }
        }
    }

    return node;
}

/*
 * Insert a node into the tree (or update an existing node with the matching key).
 * The RBL value should be NULL or a valid c-string containing a IPv4 in decimal-dot notation
 *  @param ntt          the ntt struct pointer (to traverse then insert into or update)
 *  @param key          the IPv4 (decimal-dot notation) address to use as the key to insert into the ntt
 *  @param rbl_returned the RBL value returned by the server to be used (can be NULL)
 *  @param timestamp    the apr_time_t timestamp of the last hit recorded by the (key) IP address
 */
struct ntt_node* ntt_insert(struct ntt* ntt, const char* key, const char* rbl_returned, apr_time_t timestamp)
{
    long                hash_code               = 0;
    struct ntt_node*    parent                  = NULL;
    struct ntt_node*    node                    = NULL;
    struct ntt_node*    bucket_head             = NULL;
    struct ntt_node*    new_node                = NULL;
    apr_pool_t*         p                       = NULL;
    char*               filename                = NULL;

    apr_status_t        rv                      = 1;
    apr_status_t        logfile_open_status     = 1;
    apr_status_t        logfile_locked_status   = 1;
    apr_status_t        logfile_unlocked_status = 1;
    apr_status_t        logfile_closed_status   = 1;

    apr_time_t          t                       = apr_time_now();

    if(!ntt)
        return NULL;

    hash_code   = ntt_hashcode(ntt, key);
    parent      = NULL;
    node        = ntt->tbl[hash_code];
    bucket_head = ntt->tbl[hash_code];

    // traverse the bucket to find a node with the same key
    while(node/* && !ntt_is_expired(node)*/)
    {

        if(node && httpbl_string_matches(key, node->key))
        {
            // bad form to return from inside a loop; set new_node to the updated node instead
            new_node        = ntt_float(ntt, hash_code, node, parent);
            // after a float, we are done with this loop
            break;
        }

        parent      = node;
        node        = node->next;
    }
    
    // if a node with this key already exists in this hash table, update it and return it
    if(new_node)
        return new_node;
    
    // Create a new node
    new_node            = ntt_node_create(key);
    new_node->timestamp = timestamp;
    if(rbl_returned)
        new_node->rbl_value = strdup(rbl_returned); // if the RBL returned value is available, set it;
    else
        new_node->rbl_value = strdup(CLEANLIST_IP_ADDRESS);

    // Insert the new node into the ntt structure
    if(parent)
    {
        // Existing parent
        parent->next    = new_node->next;
        new_node->next  = ntt->tbl[hash_code];

        // Return the locked node
    }
    else
    {
        // No existing parent; add directly to hash table
    }
    ntt->tbl[hash_code] = new_node;
    ntt->items++;

    return new_node;
}

/*
 *   Tree destructor
 */
int ntt_destroy(struct ntt* ntt)
{
    struct ntt_node*    node        = NULL;
    struct ntt_node*    next        = NULL;
    struct ntt_c        c;

    if(!ntt)
        return -1;

    node        = c_ntt_first(ntt, &c);
    while(node)
    {
        next        = c_ntt_next(ntt, &c);
        ntt_delete(ntt, node->key);
        node        = next;
    }

    free(ntt->tbl);
    ntt->tbl    = NULL;
    free(ntt);
    ntt         = NULL;
    ntt = (struct ntt*)NULL;

    // serialize
    return 0;
}

/*
 *   Delete a single node in the tree, given the key of the node to delete
 */
int ntt_delete(struct ntt* ntt, const char* key)
{
    long                hash_code   = 0;
    struct ntt_node*    del_node    = NULL;
    struct ntt_node*    parent      = NULL;
    struct ntt_node*    node        = NULL;

    if (!ntt || !key) // we can't match something that doesn't exist
        return -1;

    hash_code   = ntt_hashcode(ntt, key);
    node        = ntt->tbl[hash_code];      // set the node to the head of the linked list

    // traverse the ntt struct to find the node with a matching key
    while(node && !httpbl_string_matches(key, node->key))
    {
        if (httpbl_string_matches(key, node->key))
        {
            del_node    = node;
            node        = NULL;
        }

        if (!del_node)
        {
            parent      = node;
            node        = node->next;
        }
    }

    // ;
    if(del_node)
    {
        if(parent)
            parent->next = del_node->next;
        else
            ntt->tbl[hash_code] = del_node->next;

        free(del_node->key);
        del_node->key   = NULL;
        free(del_node);
        del_node        = NULL;
        ntt->items--;

        return 0;
    }

    return -5;
}

/*
 *   Point cursor to first item in tree
 */
struct ntt_node* c_ntt_first(struct ntt* ntt, struct ntt_c* c)
{
    c->iter_index   = 0;
    c->iter_next    = (struct ntt_node *)NULL;

    return (c_ntt_next(ntt, c));
}

/*
 *   Point cursor to next iteration in tree
 */
struct ntt_node* c_ntt_next(struct ntt* ntt, struct ntt_c* c)
{
    long                index   = 0;
    struct ntt_node*    node    = c->iter_next;

    if(!ntt)
        return NULL;

    if(node)
    {
        c->iter_next = node->next;
        return(node);
    }
    else
    {
        while(c->iter_index < ntt->size)
        {
            index = c->iter_index++;

            if(ntt->tbl[index])
            {
                c->iter_next = ntt->tbl[index]->next;
                return(ntt->tbl[index]);
            }
        }
    }

    return ((struct ntt_node*)NULL);
}

/***************************
 *  END NTT Functions
 ***************************/

/*
 *   whitelist_insert
 *   handle the ntt_insert functionality for inserting a key (an IP address string) into the whitelist.
 *   Can be used by a directive or a static function (at any time during the module's lifetime before the whitelist is freed).
 *   @param  entry   a string (a char* stored in memory which was allocted from a module-lifetime pool) containing an IP address in decimal-dot notation (i.e. "127.0.0.1")
 *   @return         TRUE if entry is a correctly formatted IP address; FALSE otherwise
 */
static int whitelist_insert(apr_pool_t* pool, const char* entry)
{
#ifdef SHOULD_CACHE
    if (is_whitelist_outofdate())
        if(!unserialize_whitelist_from_file(pool)) // populate the existing whitelist from serialized-whitelist file
            ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, NULL, "HTTPBL: Error loading whitelist from cachefile (%s)", whitelist_filepath);
#endif

#ifdef SHOULD_NTT_INSERT
    ntt_insert(white_list, entry, NULL, apr_time_now());
#endif

#ifdef SHOULD_CACHE
    serialize_whitelist_to_file(pool);
#endif

    return 1;
}

/*
 *   hitlist_insert
 *   handle the ntt_insert functionality for inserting a key (an IP address string) into the hitlist.
 *   Can be used by a directive or a static function (at any time during the module's lifetime before the hitlist is freed).
 *   @param  entry   a string (a char* stored in memory which was allocted from a module-lifetime pool) containing an IP address in decimal-dot notation (i.e. "127.0.0.1")
 *   @return         TRUE if entry is a correctly formatted IP address; FALSE otherwise
 */
static int hitlist_insert(apr_pool_t* pool, const char* entry, const char* the_rbl_value, request_rec* r)
{
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "HTTPBL: Entered hitlist_insert");

    // future: check incoming entry and the_rbl_value for valid IP(v4 decimal-dot) format

#ifdef SHOULD_CACHE
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "HTTPBL: Attempting to load hitlist from cachefile");
    if(!unserialize_hitlist_from_file(pool)) // update hitlist from cache-file
    {
        ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: Error loading hitlist from cachefile (%s)", hitlist_filepath);
    }
    else
    {
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "HTTPBL: Successfully loaded hitlist from cachefile");
    }
#endif

#ifdef SHOULD_NTT_INSERT
    ntt_insert(hit_list, entry, the_rbl_value, apr_time_now());
#endif

#ifdef SHOULD_CACHE
    serialize_hitlist_to_file(pool);
#endif

    return 1;
}


/*
 *   Is this request a honeypot request (token URI)?
 *   @param  r       the request_rec* from the request handling function (contains required data)
 *   @return         TRUE iff the request is an attempt to request a honeypot page; FALSE otherwise
 */
static int is_request_a_honeypot_token(request_rec* r)
{
    char*       test_against    = get_this_requests_honeypot_url(r);
    char*       tmpstr          = NULL;

#ifdef SHOULD_REQUEST_HONEYPOTS
    if(!test_against)
        return 0;
    else if(httpbl_string_nmatches(r->uri, test_against, strlen(test_against))) // if test_against matches the substring at the beginning of r->uri
        return 1;
    else
#endif
        return 0;
}


/*
 *   Is this request a httpbl testing request (token URI)?
 *   @param  r       the request_rec* from the request handling function (contains required data)
 *   @return         TRUE iff the request is an attempt to request the HTTPBl Testing page; FALSE otherwise
 */
static int is_request_a_httpbl_testing_token(request_rec* r)
{
    char*       test_against    = get_this_requests_g_httpbl_testing_url(r);
    char*       tmpstr          = NULL;

    if(!test_against)
        return 0;
    else if(httpbl_string_nmatches(r->uri, test_against, strlen(test_against))) // if test_against matches the first substring of r->uri
        return 1;
    else
        return 0;
}

/*
 *   Is this request a whitelisting request (internally redirected from a challenge-pass page)?
 *   @param  r       the request_rec* from the request handling function (contains required data)
 *   @return         TRUE iff the request is an attempt to whitelist an IP address (after passing a captcha challenge); FALSE otherwise
 */
static int is_request_a_whitelist_token(request_rec* r)
{
    char*           test_against    = "/httpbl_token/";
    char*           tmpstr          = NULL;
    apr_size_t      outlen          = 0;
    apr_file_t*     the_file        = NULL;
    apr_status_t    rv              = 1;

    httpbl_dir_cfg* this_cfg    = (httpbl_dir_cfg*)ap_get_module_config(r->per_dir_config, &httpbl_module);

    if (!this_cfg || !(this_cfg->token_str))
        this_cfg    = (httpbl_dir_cfg*)ap_get_module_config(r->server->module_config, &httpbl_module);

    if(!strncmp(r->uri, test_against, strlen(test_against))) // if test_against is identical in content to the first substring of r->uri
        return 1;
    else
        return 0;
}

/*
 *   Check an IP agaist the whitelist.
 *   @return         true (non-zero integer) if the IP is on the whitelist.
 *
 *   Note: needs to be upgraded to take advantage of APR pools
 */
int is_whitelisted(const char* ip)
{
    struct ntt_node*    n               = ntt_find(white_list, ip); // First see if the IP itself is in the whitelist
    if(!n || (n && ntt_is_expired(n))) // if the requesting IP does not have an active record in the whitelist
        return 0;
    else
        return 1;
}

/*
 *   Is the whitelist in memory older than the serialized whitelist file?
 */
int is_whitelist_outofdate()
{
    struct stat statbuf;

    if(!(lstat(whitelist_filepath, &statbuf)))
        return -1;

    if (statbuf.st_mtime > g_whitelist_last_modtime)
        return 1; // return out of date
    else
        return 0; // return up to date
}

/*
 *   Merge file hitlist into this child process's hitlist-in-memory
 */
int unserialize_hitlist_from_file(apr_pool_t* p)
{
    char*           the_hitlist_filename    = NULL;
    apr_file_t*     the_hashlist_file       = NULL;
    apr_file_t*     the_logger_file         = NULL;
    apr_status_t    rv                      = 1;
    apr_status_t    logfile_opened_status   = 1;
    apr_status_t    hashfile_lock_obtained  = 1;
    int             the_return_value        = 0;

    apr_size_t      outlen                  = 0;
    char*           tmpstr                  = NULL;

    if(whitelist_filepath)
    {
        rv      = apr_file_open(&the_hashlist_file, hitlist_filepath, APR_READ | APR_CREATE, APR_OS_DEFAULT, p);
        ap_log_error(APLOG_MARK, APLOG_INFO, 0, NULL, "HTTPBL: HTTPBL; unserialize_hitlist_from_file: file (%s) opened [%sSUCCESSFULLY].", hitlist_filepath, (rv == APR_SUCCESS)?"":"UN");
        if(rv == APR_SUCCESS)
        {
            hashfile_lock_obtained  = apr_file_lock(the_hashlist_file, APR_FLOCK_SHARED);
            ap_log_error(APLOG_MARK, APLOG_INFO, 0, NULL, "HTTPBL: HTTPBL; unserialize_hitlist_from_file: file (%s) locked [%sSUCCESSFULLY]", hitlist_filepath, (hashfile_lock_obtained == APR_SUCCESS)?"":"UN");

#if VERBOSITY >= APLOG_DEBUG
            char*   log_filename    = apr_psprintf(p, "%s%s%s-%ld.log", get_log_dir(), DEFAULT_HTTPBL_FILENAME_PREFIX, "hitlist", getpid());
    
            logfile_opened_status   = apr_file_open(&the_logger_file, log_filename, APR_WRITE | APR_APPEND | APR_CREATE, APR_OS_DEFAULT, p);
            ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: HTTPBL; unserialize_hitlist_from_file: file (%s) opened [%sSUCCESSFULLY].", log_filename, (logfile_opened_status == APR_SUCCESS)?"":"UN");

            if(logfile_opened_status == APR_SUCCESS)
            {
                tmpstr          = apr_psprintf(p, "// File Opened (filename: %s)\n", hitlist_filepath);
                outlen          = strlen(tmpstr);
                rv              = apr_file_write(the_logger_file, tmpstr, &outlen);
            }
#endif

            the_return_value    =  unserialize_hashtable_from_file(p, the_hashlist_file, (logfile_opened_status == APR_SUCCESS)?the_logger_file:NULL, hit_list); // run unserialization process here

#if VERBOSITY >= APLOG_DEBUG
            if(logfile_opened_status == APR_SUCCESS)
            {
                apr_status_t    log_close_status    = apr_file_close(the_logger_file);
                ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: HTTPBL; unserialize_hitlist_from_file: file (%s) closed [%sSUCCESSFULLY].", log_filename, (log_close_status == APR_SUCCESS)?"":"UN");
            }
#endif

            if(hashfile_lock_obtained == APR_SUCCESS)
            {
                apr_status_t    hashlist_unlock_status  = apr_file_unlock(the_hashlist_file);
                ap_log_error(APLOG_MARK, APLOG_INFO, 0, NULL, "HTTPBL: HTTPBL; unserialize_hitlist_from_file: file (%s) unlocked [%sSUCCESSFULLY].", hitlist_filepath, (hashlist_unlock_status == APR_SUCCESS)?"":"UN");
            }
            apr_status_t    hashlist_close_status   = apr_file_close(the_hashlist_file);
            ap_log_error(APLOG_MARK, APLOG_INFO, 0, NULL, "HTTPBL: HTTPBL; unserialize_hitlist_from_file: file (%s) closed [%sSUCCESSFULLY].", hitlist_filepath, (hashlist_close_status == APR_SUCCESS)?"":"UN");
    
            return the_return_value;
        }
        else
            return 0;
    }
    else
        return 0;
}

/*
 *   Merge file whitelist into this child process's whitelist-in-memory
 */
int unserialize_whitelist_from_file(apr_pool_t* p)
{
    char*           the_whitelist_filename  = NULL;
    apr_file_t*     the_hashlist_file       = NULL;
    apr_file_t*     the_logger_file         = NULL;
    apr_status_t    rv                      = 1;
    apr_status_t    logfile_opened_status   = 1;
    apr_status_t    hashfile_lock_obtained  = 1;
    apr_status_t    logfile_lock_obtained   = 1;
    int             the_return_value        = 0;

    apr_size_t      outlen                  = 0;
    char*           tmpstr                  = NULL;

    if(whitelist_filepath)
    {
        rv      = apr_file_open(&the_hashlist_file, whitelist_filepath, APR_READ | APR_CREATE, APR_OS_DEFAULT, p);
        ap_log_error(APLOG_MARK, APLOG_INFO, 0, NULL, "HTTPBL: HTTPBL; unserialize_hitlist_from_file: file (%s) opened [%sSUCCESSFULLY].", whitelist_filepath, (rv == APR_SUCCESS)?"":"UN");

        if(rv == APR_SUCCESS)
        {
            hashfile_lock_obtained  = apr_file_lock(the_hashlist_file, APR_FLOCK_SHARED);
#if VERBOSITY >= APLOG_DEBUG
            ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: HTTPBL; unserialize_hitlist_from_file: file (%s) locked [%sSUCCESSFULLY].", whitelist_filepath, (hashfile_lock_obtained == APR_SUCCESS)?"":"UN");
            char*   log_filename    = apr_psprintf(p, "%s%s%s-%ld.log", get_log_dir(), DEFAULT_HTTPBL_FILENAME_PREFIX, "whitelist", getpid());
    
            logfile_opened_status   = apr_file_open(&the_logger_file, log_filename, APR_WRITE | APR_APPEND | APR_CREATE, APR_OS_DEFAULT, p);
    
            if(logfile_opened_status == APR_SUCCESS)
            {
                logfile_lock_obtained  = apr_file_lock(the_logger_file, APR_FLOCK_EXCLUSIVE);
                ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: HTTPBL; unserialize_hitlist_from_file: file (%s) opened [%sSUCCESSFULLY].", log_filename, (logfile_lock_obtained == APR_SUCCESS)?"":"UN");
    
                tmpstr          = apr_psprintf(p, "// File Opened (filename: %s)\n", whitelist_filepath);
                outlen          = strlen(tmpstr);
                rv              = apr_file_write(the_logger_file, tmpstr, &outlen);
            }
#endif

            // run unserialization process here
            the_return_value    =  unserialize_hashtable_from_file(p, the_hashlist_file, (logfile_opened_status == APR_SUCCESS)?the_logger_file:NULL, white_list); // run unserialization process here

#if VERBOSITY >= APLOG_DEBUG
            if(logfile_lock_obtained == APR_SUCCESS)
            {
                apr_status_t    logfile_unlock_status  = apr_file_unlock(the_logger_file);
                ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: HTTPBL; unserialize_hitlist_from_file: file (%s) unlocked [%sSUCCESSFULLY].", whitelist_filepath, (logfile_unlock_status == APR_SUCCESS)?"":"UN");
            }

            if(logfile_opened_status == APR_SUCCESS)
            {
                apr_status_t    log_close_status    = apr_file_close(the_logger_file);
                ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: HTTPBL; unserialize_hitlist_from_file: file (%s) closed [%sSUCCESSFULLY].", whitelist_filepath, (log_close_status == APR_SUCCESS)?"":"UN");
            }
            
#endif
            if(hashfile_lock_obtained == APR_SUCCESS)
            {
                apr_status_t    hashlist_unlock_status  = apr_file_unlock(the_hashlist_file);
#if VERBOSITY >= APLOG_DEBUG
                ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: HTTPBL; unserialize_hitlist_from_file: file (%s) unlocked [%sSUCCESSFULLY].", log_filename, (hashlist_unlock_status == APR_SUCCESS)?"":"UN");
#endif
            }
            
            apr_status_t    hashlist_close_status   = apr_file_close(the_hashlist_file);
#if VERBOSITY >= APLOG_DEBUG
            ap_log_error(APLOG_MARK, APLOG_INFO, 0, NULL, "HTTPBL: HTTPBL; unserialize_hitlist_from_file: file (%s) closed [%sSUCCESSFULLY].", log_filename, (hashlist_close_status == APR_SUCCESS)?"":"UN");
#endif
    
            return the_return_value;
        }
        else
            return 0;
    }
    else
        return 0;
}

/*
 *   Unserialize the file parameter and add all entries of the unserialization to the ntt parameter
 *   Possible Issues: The comment truncation mechanism may fail to remove the entire comment (or detect a comment) if
 *       the line-reader does not read the entire line in 2047 chars
 *
 *   Future:  consider rewriting for file-io speed improvement; look into function: "apr_file_read_full"
 */
int unserialize_hashtable_from_file(apr_pool_t* p, apr_file_t* the_hashlist_file, apr_file_t* the_logger_file, struct ntt* the_hash_list)
{
    apr_status_t        rv                      = 1;
    apr_status_t        logfile_opened_status   = 1;
    apr_status_t        hashfile_lock_obtained  = 1;
    apr_status_t        logfile_lock_obtained   = 1;
    apr_status_t        file_line_read_rv       = 1;
    apr_size_t          outlen                  = 0;
    apr_size_t          outlen2                 = 0;
    char*               tmpstr                  = NULL;
    apr_size_t          len                     = 0;
    pcre*               re                      = NULL;
    char*               ds_regex_pattern        = "^|([0-9]{1,3})[.]([0-9]{1,3})[.]([0-9]{1,3})[.]([0-9]{1,3})|([0-9]{1,})|([0-9]{1,})|([0-9]{1,3})[.]([0-9]{1,3})[.]([0-9]{1,3})[.]([0-9]{1,3})|$"; // dilineated_str regex to match (with backreferences)
    char*               line_buffer             = (char*)apr_palloc(p, 2048*sizeof(char));
    char*               ip_address              = NULL;
    char*               rbl_value2              = NULL;
    long                hash_code               = -1;
    const char*         error;
    int                 erroffset               = 0;
    int                 rc                      = 0;
    apr_time_t          timestamp               = apr_time_make(0,0);
    int                 num_of_possible_matches = 0;
    int                 size_of_ovec            = num_of_possible_matches*3*sizeof(int);
    int*                ovector                 = apr_palloc(p, size_of_ovec);

    ip_address                                  = get_whitelist_rbl_value(p);

    ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: HTTPBL; entered unserialize_hashtable_from_file(...); the_hashlist_file: %p, the_hashlist_file: %p, the_hashlist_file: %p.", the_hashlist_file, the_logger_file, the_hash_list);

    // read line by line; if the line is empty, stop reading
    outlen                                      = 2047;
    file_line_read_rv                           = apr_file_gets(line_buffer, outlen, the_hashlist_file);

    while(file_line_read_rv == APR_SUCCESS)
    {
        num_of_possible_matches = strlen(line_buffer);
        size_of_ovec            = num_of_possible_matches*3*sizeof(int);
        ovector                 = apr_palloc(p, size_of_ovec);

        if(!is_empty_string(line_buffer))
        {
            // trim whitespace from the head and tail ends of the string
            line_buffer = apr_ptrim(p, line_buffer);
#if VERBOSITY >= APLOG_DEBUG
            if(the_logger_file)
            {
                tmpstr          = apr_psprintf(p, "// Line Read (line: '%s'; length: %ld)\n", line_buffer, outlen);
                outlen          = strlen(tmpstr);
                rv              = apr_file_write(the_logger_file, tmpstr, &outlen);
            }
#endif

            ip_address              = get_whitelist_rbl_value(p);
            rbl_value2              = apr_pstrdup(p, "");
            hash_code               = -1;
            timestamp               = apr_time_make(0, 0);

            if(line_buffer && strlen(line_buffer)>0)
            {
                // remove comments from line_buffer string
                if(strstr(line_buffer, "//")/* || httpbl_string_matches(apr_ptrim(p, line_buffer), "") */) // if line_buffer contains a line-comment dilineator
                {
                    char* comment_pointer   = strstr(line_buffer, "//");
                    comment_pointer[0]      = '\0';
                }


                re = pcre_compile(
                                    ds_regex_pattern,     // the pattern
                                    0,                    // default options
                                    &error,               // for error message
                                    &erroffset,           // for error offset
                                    NULL);                // use default character tables
                if (re == NULL)
                {
#if VERBOSITY >= APLOG_DEBUG
                    if(the_logger_file)
                    {
                        tmpstr          = apr_psprintf(p, "PCRE compilation failed at offset %d: %s\n", erroffset, error);
                        outlen          = strlen(tmpstr);
                        rv              = apr_file_write(the_logger_file, tmpstr, &outlen);
                    }
#endif
                    continue;
                }
                rc = pcre_exec(
                                    re,                   // the compiled pattern
                                    NULL,                 // no extra data - we didn't study the pattern
                                    line_buffer,          // the subject string
                                    (int)strlen(line_buffer),// the length of the subject
                                    0,                    // start at offset 0 in the subject
                                    0,                    // default options
                                    ovector,              // output vector for substring information
                                    size_of_ovec);        // number of elements in the output vector
        
                if (rc < 0)
                {
                    if(rc == PCRE_ERROR_NOMATCH)
                    {
#if VERBOSITY >= APLOG_DEBUG
                        if(the_logger_file)
                        {
                            tmpstr          = apr_psprintf(p, "PCRE failed to match a line (\"%s\")", line_buffer);
                            outlen          = strlen(tmpstr);
                            rv              = apr_file_write(the_logger_file, tmpstr, &outlen);
                        }
#endif
                        continue;
                    }
                    else
                    {
#if VERBOSITY >= APLOG_DEBUG
                        if(the_logger_file)
                        {
                            tmpstr          = apr_psprintf(p, "PCRE matching error %d\n", rc);
                            outlen          = strlen(tmpstr);
                            rv              = apr_file_write(the_logger_file, tmpstr, &outlen);
                        }
#endif
                        continue;
                    }
                }



                // line format "|field2|field3|field4|field5|"
                if(line_buffer &&
                   !httpbl_string_matches(apr_ptrim(p, line_buffer), "") &&
                   !strstr(line_buffer, "//"))
                {
                    char*       clean_line  = apr_ptrim(p, line_buffer);
                    char*       delimeter   = ":";
                    char*       token       = (char*)apr_palloc(p, outlen*sizeof(char));
                    char*       running1    = apr_pstrdup(p, clean_line);
                    char*       running2    = NULL;
                    int         i           = 0;
                    char*       field1      = apr_pcalloc(p, outlen*sizeof(char));
                    char*       field2      = apr_pcalloc(p, outlen*sizeof(char));
                    char*       field3      = apr_pcalloc(p, outlen*sizeof(char));
                    char*       field4      = apr_pcalloc(p, outlen*sizeof(char));
                    char*       field5      = apr_pcalloc(p, outlen*sizeof(char));
                    char*       field6      = apr_pcalloc(p, outlen*sizeof(char));
                    apr_size_t  len;
                    /*
                        regex test to make sure the line_buffer is formatted correctly
                    */

                    ip_address  = (char*)apr_palloc(p, 64*sizeof(char));
                    // use str_tok to make sure each space-deliniated string is small enough to prevent a buffer overflow

                    char*       delims      = "|";
//                    char*   this_word   = (char*)apr_palloc(p, strlen(line_buffer)*sizeof(char));
                    field1              = strsep(&line_buffer, delims);
                    field2              = strsep(&line_buffer, delims);
                    field3              = strsep(&line_buffer, delims);
                    field4              = strsep(&line_buffer, delims);
                    field5              = strsep(&line_buffer, delims);
                    field6              = strsep(&line_buffer, delims);

                    ip_address          = apr_pstrdup(p, field2);
                    hash_code           = (long)atol(field3);
                    sscanf(apr_ptrim(p, field4), "%"APR_TIME_T_FMT, &timestamp);
                    rbl_value2          = apr_pstrdup(p, field5);
#if VERBOSITY >= APLOG_DEBUG
                    if(the_logger_file)
                    {
                        tmpstr          = apr_psprintf(p, "Field2:\t%s (%d); IP:\t%s\n", field2, strlen(field2), ip_address);
                        outlen2         = strlen(tmpstr);
                        rv              = apr_file_write(the_logger_file, tmpstr, &outlen2);

                        tmpstr          = apr_psprintf(p, "Field3:\t%s (%d); HASH:\t%lu\n", field3, strlen(field3), hash_code);
                        outlen2         = strlen(tmpstr);
                        rv              = apr_file_write(the_logger_file, tmpstr, &outlen2);

                        tmpstr          = apr_psprintf(p, "Field4:\t%s (%d); TIMESTAMP:\t%"APR_TIME_T_FMT"\n", field4, strlen(field4), timestamp);
                        outlen2         = strlen(tmpstr);
                        rv              = apr_file_write(the_logger_file, tmpstr, &outlen2);

                        tmpstr          = apr_psprintf(p, "Field5:\t%s (%d); RBL\t:\t%s\n", field5, strlen(field5), rbl_value2);
                        outlen2         = strlen(tmpstr);
                        rv              = apr_file_write(the_logger_file, tmpstr, &outlen2);

                        tmpstr          = apr_psprintf(p, "RBL Value:\t%s (%d)\n", rbl_value2, strlen(rbl_value2));
                        outlen2         = strlen(tmpstr);
                        rv              = apr_file_write(the_logger_file, tmpstr, &outlen2);

                        tmpstr          = apr_psprintf(p, "rbl_value != NULL:\t%s\n", (rbl_value2)?"TRUE":"FALSE");
                        outlen2         = strlen(tmpstr);
                        rv              = apr_file_write(the_logger_file, tmpstr, &outlen2);

                        tmpstr          = apr_psprintf(p, "httpbl_string_matches(rbl_value, \"\"):\t%s\n", httpbl_string_matches(rbl_value2, "")?"TRUE":"FALSE");
                        outlen2         = strlen(tmpstr);
                        rv              = apr_file_write(the_logger_file, tmpstr, &outlen2);

                        tmpstr          = apr_psprintf(p, "httpbl_string_matches(rbl_value, \"(null)\"):\t%s\n", httpbl_string_matches(rbl_value2, "(null)")?"TRUE":"FALSE");
                        outlen2         = strlen(tmpstr);
                        rv              = apr_file_write(the_logger_file, tmpstr, &outlen2);
                    }
#endif
                    int is_valid_entry  = 1;

#if VERBOSITY >= APLOG_DEBUG
                    if(the_logger_file)
                    {
                        tmpstr          = apr_psprintf(p, "Field2:\t%s (%d); IP:\t%s\n", field2, strlen(field2), ip_address);
                        outlen2         = strlen(tmpstr);
                        rv              = apr_file_write(the_logger_file, tmpstr, &outlen2);
                    }
#endif
                    // validate ip; make sure it is only (numeric + periods) and make sure it is not either of the special RBL values (either whitelisted or cleanlisted)
                    if(strspn(ip_address, "0123456789.") < strlen(ip_address) ||
                       string_matches_whitelist_rbl_value(ip_address) ||
                       string_matches_cleanlist_rbl_value(ip_address) ||
                       strlen(ip_address) < 7 ||
                       strlen(ip_address) > 15)
                        is_valid_entry  = -1;


#if VERBOSITY >= APLOG_DEBUG
                    if(the_logger_file)
                    {
                        tmpstr          = apr_psprintf(p, "hash_code:(%ld) < 0 == %s\n", hash_code, (hash_code<0)?"TRUE":"FALSE");
                        outlen2         = strlen(tmpstr);
                        rv              = apr_file_write(the_logger_file, tmpstr, &outlen2);

                        tmpstr          = apr_psprintf(p, "hash_code(%ld) >= hash_table_size (%ld) == %s\n", hash_code, actual_hash_table_size, (hash_code>actual_hash_table_size)?"TRUE":"FALSE");
                        outlen2         = strlen(tmpstr);
                        rv              = apr_file_write(the_logger_file, tmpstr, &outlen2);
                    }
#endif
                    // validate hash code (not really necessary or helpful)
                    if(hash_code < 0 ||
                       hash_code >= actual_hash_table_size)
                        is_valid_entry  = -2;


#if VERBOSITY >= APLOG_DEBUG
                    if(the_logger_file)
                    {
                        char*       rfcTimestamp            = apr_pcalloc(p, APR_RFC822_DATE_LEN * sizeof(char));
                        char*       rfcTimeNow              = apr_pcalloc(p, APR_RFC822_DATE_LEN * sizeof(char));
                        char*       rfcTimeOldestAcceptable = apr_pcalloc(p, APR_RFC822_DATE_LEN * sizeof(char));
                        apr_time_t  tmp_timestamp           = 0;

                        apr_rfc822_date(rfcTimestamp, timestamp);

                        tmpstr          = apr_psprintf(p, "Timestamp(%s; %"APR_TIME_T_FMT")\n", rfcTimestamp, timestamp);
                        outlen2         = strlen(tmpstr);
                        rv              = apr_file_write(the_logger_file, tmpstr, &outlen2);

                        tmp_timestamp   = apr_time_now(); // set to time_now
                        apr_rfc822_date(rfcTimeNow, tmp_timestamp);

                        tmpstr          = apr_psprintf(p, "Timestamp >= (%s; %"APR_TIME_T_FMT") == %s\n", rfcTimeNow, tmp_timestamp, (timestamp >= tmp_timestamp)?"TRUE":"FALSE");
                        outlen2         = strlen(tmpstr);
                        rv              = apr_file_write(the_logger_file, tmpstr, &outlen2);

                        tmp_timestamp   = apr_time_make(apr_time_sec(tmp_timestamp) - (blocking_period * 60), 0); // set to oldest acceptable timestamp
                        apr_rfc822_date(rfcTimeOldestAcceptable, tmp_timestamp);

                        tmpstr          = apr_psprintf(p, "Timestamp < (%s; %"APR_TIME_T_FMT") == %s\n", rfcTimeOldestAcceptable, tmp_timestamp, (timestamp<tmp_timestamp)?"TRUE":"FALSE" );
                        outlen2         = strlen(tmpstr);
                        rv              = apr_file_write(the_logger_file, tmpstr, &outlen2);
                    }
#endif
                    // validate timestamp
                    if(timestamp >= apr_time_now() ||
                       timestamp < (apr_time_make(apr_time_sec(apr_time_now()) - (blocking_period * 60), 0)))
                        is_valid_entry  = -3;


#if VERBOSITY >= APLOG_DEBUG
                    if(the_logger_file)
                    {
                        tmpstr          = apr_psprintf(p, "RBL Value:\t%s\n", rbl_value2);
                        outlen2         = strlen(tmpstr);
                        rv              = apr_file_write(the_logger_file, tmpstr, &outlen2);

                        tmpstr          = apr_psprintf(p, "httpbl_string_matches(rbl_value2, \"\")\n", (httpbl_string_matches(rbl_value2, ""))?"TRUE":"FALSE");
                        outlen2         = strlen(tmpstr);
                        rv              = apr_file_write(the_logger_file, tmpstr, &outlen2);

                        tmpstr          = apr_psprintf(p, "httpbl_string_matches(rbl_value2, \"(null)\")\n", (httpbl_string_matches(rbl_value2, "(null)"))?"TRUE":"FALSE");
                        outlen2         = strlen(tmpstr);
                        rv              = apr_file_write(the_logger_file, tmpstr, &outlen2);
                    }
#endif
                    // validate rbl returned; 
                    if(httpbl_string_matches(rbl_value2, "") || httpbl_string_matches(rbl_value2, "(null)"))
                        is_valid_entry  = -4;

                    if(is_valid_entry>0)
                    {
#if VERBOSITY >= APLOG_DEBUG
                        if(the_logger_file)
                        {
                            char* tmpstrZZ  = NULL;
                            tmpstrZZ        = apr_psprintf(p, "Inserting:\n");
                            outlen2         = strlen(tmpstrZZ);
                            rv              = apr_file_write(the_logger_file, tmpstrZZ, &outlen2);

                            tmpstrZZ        = apr_psprintf(p, "\tIP:\t\t%s (%d)\n", ip_address, strlen(ip_address));
                            outlen2         = strlen(tmpstrZZ);
                            rv              = apr_file_write(the_logger_file, tmpstrZZ, &outlen2);

                            tmpstrZZ        = apr_psprintf(p, "\tHASH CODE:\t%ld\n", hash_code);
                            outlen2         = strlen(tmpstrZZ);
                            rv              = apr_file_write(the_logger_file, tmpstrZZ, &outlen2);

                            tmpstrZZ        = apr_psprintf(p, "\tTIMESTAMP:\t%"APR_TIME_T_FMT"\n", timestamp);
                            outlen2         = strlen(tmpstrZZ);
                            rv              = apr_file_write(the_logger_file, tmpstrZZ, &outlen2);

                            tmpstrZZ        = apr_psprintf(p, "\tRBL VALUE:\t%s (%d)\n", rbl_value2, strlen(rbl_value2));
                            outlen2         = strlen(tmpstrZZ);
                            rv              = apr_file_write(the_logger_file, tmpstrZZ, &outlen2);
                        }
#endif
                        // insert new ntt_node into the ntt structure
#ifdef SHOULD_NTT_INSERT
                        struct ntt_node* the_new_node = ntt_insert(the_hash_list, ip_address, rbl_value2, apr_time_make(timestamp, 0));
#endif
                    }
                    else
                    {
#if VERBOSITY >= APLOG_DEBUG
                        if(the_logger_file)
                        {
                            tmpstr          = apr_psprintf(p, "// NOT A VALID ENTRY (%d)\n", is_valid_entry);
                            outlen2         = strlen(tmpstr);
                            rv              = apr_file_write(the_logger_file, tmpstr, &outlen2);
                        }
#endif
                    }
                }
                else
                {
#if VERBOSITY >= APLOG_DEBUG
                    if(the_logger_file)
                    {
                        tmpstr          = apr_psprintf(p, "// Skipping Line (1)\n");
                        outlen2         = strlen(tmpstr);
                        rv              = apr_file_write(the_logger_file, tmpstr, &outlen2);
                    }
#endif
                }
            }
            else
            {
#if VERBOSITY >= APLOG_DEBUG
                if(the_logger_file)
                {
                    tmpstr          = apr_psprintf(p, "// Empty (2)\n");
                    outlen2         = strlen(tmpstr);
                    rv              = apr_file_write(the_logger_file, tmpstr, &outlen2);
                }
#endif
            }

#if VERBOSITY >= APLOG_DEBUG
            if(the_logger_file)
            {
                tmpstr          = apr_psprintf(p, "\n\n\n");
                outlen2         = strlen(tmpstr);
                rv              = apr_file_write(the_logger_file, tmpstr, &outlen2);
            }
#endif
        }
        else
        {
#if VERBOSITY >= APLOG_DEBUG
            if(the_logger_file)
            {
                tmpstr          = apr_psprintf(p, "// Skipping Line\n");
                outlen2         = strlen(tmpstr);
                rv              = apr_file_write(the_logger_file, tmpstr, &outlen2);
            }
#endif
        }
        line_buffer         = apr_pcalloc(p, outlen*sizeof(char));
        file_line_read_rv   = apr_file_gets(line_buffer, outlen, the_hashlist_file);
    }
#if VERBOSITY >= APLOG_DEBUG
    if(the_logger_file)
    {
        tmpstr          = apr_psprintf(p, "//File Closed (pid: %ld)\n", getpid());
        outlen2         = strlen(tmpstr);
        rv              = apr_file_write(the_logger_file, tmpstr, &outlen2);
    }
#endif

    return 1;
}

/*
 *   Write this child process's hitlist-in-memory to file
 */
int serialize_hitlist_to_file(apr_pool_t* p)
{
    char*           the_hitlist_filename    = NULL;
    apr_file_t*     the_hashlist_file       = NULL;
    apr_file_t*     the_logger_file         = NULL;
    apr_status_t    rv                      = 1;
    apr_status_t    logfile_opened_status   = 1;
    apr_status_t    hashfile_lock_obtained  = 1;
    apr_status_t    logfile_lock_obtained   = 1;
    int             the_return_value        = 0;

    apr_size_t      outlen                  = 0;
    char*           tmpstr                  = NULL;

    if(whitelist_filepath)
        rv      = apr_file_open(&the_hashlist_file, hitlist_filepath, APR_WRITE | APR_TRUNCATE | APR_CREATE, APR_OS_DEFAULT, p);

    if(rv == APR_SUCCESS)
    {
        hashfile_lock_obtained  = apr_file_lock(the_hashlist_file, APR_FLOCK_EXCLUSIVE);
#if VERBOSITY >= APLOG_DEBUG
        char*   log_filename    = apr_psprintf(p, "%s%s%s-%ld.log", get_log_dir(), DEFAULT_HTTPBL_FILENAME_PREFIX, "hitlist", getpid());

        logfile_opened_status   = apr_file_open(&the_logger_file, log_filename, APR_WRITE | APR_APPEND | APR_CREATE, APR_OS_DEFAULT, p);

        if(logfile_opened_status == APR_SUCCESS)
        {
            logfile_lock_obtained  = apr_file_lock(the_hashlist_file, APR_FLOCK_EXCLUSIVE);

            tmpstr          = apr_psprintf(p, "//File Opened (filename: %s)\n", hitlist_filepath);
            outlen          = strlen(tmpstr);
            rv              = apr_file_write(the_logger_file, tmpstr, &outlen);
        }
#endif

        the_return_value    =  serialize_hashtable_to_file(p, the_hashlist_file, (logfile_opened_status == APR_SUCCESS)?the_logger_file:NULL, hit_list); // run unserialization process here

#if VERBOSITY >= APLOG_DEBUG
        if(logfile_opened_status == APR_SUCCESS)
            apr_file_unlock(the_logger_file);
        apr_file_close(the_logger_file);
#endif
        if(hashfile_lock_obtained == APR_SUCCESS)
            apr_file_unlock(the_hashlist_file);
        apr_file_close(the_hashlist_file);

        return the_return_value;
    }
    else
        return 0;
}

/*
 *   Write this child process's whitelist-in-memory to file
 */
int serialize_whitelist_to_file(apr_pool_t* p)
{
    char*           the_hitlist_filename    = NULL;
    apr_file_t*     the_hashlist_file       = NULL;
    apr_file_t*     the_logger_file         = NULL;
    apr_status_t    rv                      = 1;
    apr_status_t    logfile_opened_status   = 1;
    apr_status_t    hashfile_lock_obtained  = 1;
    apr_status_t    logfile_lock_obtained   = 1;
    apr_size_t      outlen                  = 0;
    char*           tmpstr                  = NULL;
    int             the_return_value        = 0;

    if(whitelist_filepath)
        rv      = apr_file_open(&the_hashlist_file, hitlist_filepath, APR_WRITE | APR_TRUNCATE | APR_CREATE, APR_OS_DEFAULT, p);

    if(rv == APR_SUCCESS)
    {
        hashfile_lock_obtained  = apr_file_lock(the_hashlist_file, APR_FLOCK_EXCLUSIVE);
#if VERBOSITY >= APLOG_DEBUG
        char*   log_filename    = apr_psprintf(p, "%s%s%s-%ld.log", get_log_dir(), DEFAULT_HTTPBL_FILENAME_PREFIX, "whitelist", getpid());

        logfile_opened_status   = apr_file_open(&the_logger_file, log_filename, APR_WRITE | APR_APPEND | APR_CREATE, APR_OS_DEFAULT, p);

        if(logfile_opened_status == APR_SUCCESS)
        {
            logfile_lock_obtained  = apr_file_lock(the_hashlist_file, APR_FLOCK_EXCLUSIVE);

            tmpstr          = apr_psprintf(p, "//File Opened (filename: %s)\n", hitlist_filepath);
            outlen          = strlen(tmpstr);
            rv              = apr_file_write(the_logger_file, tmpstr, &outlen);
        }
#endif

        the_return_value    =  serialize_hashtable_to_file(p, the_hashlist_file, (logfile_opened_status == APR_SUCCESS)?the_logger_file:NULL, hit_list); // run unserialization process here

#if VERBOSITY >= APLOG_DEBUG
        if(logfile_opened_status == APR_SUCCESS)
            apr_file_unlock(the_logger_file);
        apr_file_close(the_logger_file);
#endif

        if(hashfile_lock_obtained == APR_SUCCESS)
            apr_file_unlock(the_hashlist_file);
        apr_file_close(the_hashlist_file);

        return the_return_value;
    }
    else
        return 0;
}

/*
 *    Serialize the hashtable to a file (saving the hashlist in a persistent manner).
 *    This is necessary to allow child processes to update each other and to restore the
 *    hashtable after a webserver restart.
 *
 *    Future:  consider rewriting for file-io speed improvement; look into function: "apr_file_write_full"
 */
int serialize_hashtable_to_file(apr_pool_t* p, apr_file_t* the_hashtable_file, apr_file_t* the_log_file, struct ntt* hash_table)
{
    struct ntt_node**   the_hash_list               = hash_table->tbl;
    struct ntt_node*    the_hash_branch             = NULL;
    long                i                           = 0;
    apr_file_t*         the_hashtable_datafile      = NULL;
    apr_file_t*         the_hashtable_metafile      = NULL;
    char*               the_hashtable_metafilename  = NULL;
    apr_status_t        rv                          = 1;
    apr_status_t        data_file_opened            = 1;
    apr_status_t        meta_file_opened            = 1;
    apr_size_t          outlen                      = 0;
    char*               tmpstr                      = NULL;
    apr_time_t          t                           = apr_time_now();

    for(i=0; i<hit_list->size; i++)
    {
        the_hash_branch  = the_hash_list[i];

        if(!the_hash_branch)
        {
#if VERBOSITY >= APLOG_DEBUG
            if(the_log_file)
            {
                tmpstr          = apr_psprintf(p, "%d => NULL\n", i);
                outlen          = strlen(tmpstr);
                rv              = apr_file_write(the_log_file, tmpstr, &outlen);
            }
#endif
        }
        else
        {
#if VERBOSITY >= APLOG_DEBUG
            if(the_log_file)
            {
                tmpstr          = apr_psprintf(p, "%d => LINKED LIST\n", i);
                outlen          = strlen(tmpstr);
                rv              = apr_file_write(the_log_file, tmpstr, &outlen);
            }
#endif
            while(the_hash_branch)
            {
                if(!ntt_is_expired(the_hash_branch))
                {
#if VERBOSITY >= APLOG_DEBUG
                    if(the_log_file)
                    {
                            tmpstr          = apr_psprintf(p, "\t|%s|%ld|%"APR_TIME_T_FMT"|%s|// (written to cache file, ip-string-length:%u)\n", the_hash_branch->key, i, the_hash_branch->timestamp, (the_hash_branch->rbl_value)?(the_hash_branch->rbl_value):CLEANLIST_IP_ADDRESS, strlen(the_hash_branch->key));
                            outlen          = strlen(tmpstr);
                            rv              = apr_file_write(the_log_file, tmpstr, &outlen);
                    }
#endif

                    tmpstr          = apr_psprintf(p, "|%s|%ld|%"APR_TIME_T_FMT"|%s|\n", the_hash_branch->key, i, the_hash_branch->timestamp, (the_hash_branch->rbl_value)?(the_hash_branch->rbl_value):CLEANLIST_IP_ADDRESS);
                    outlen          = strlen(tmpstr);
                    rv              = apr_file_write(the_hashtable_file, tmpstr, &outlen);
                }
                else
                {
#if VERBOSITY >= APLOG_DEBUG
                    if(the_log_file)
                    {
                        tmpstr          = apr_psprintf(p, "\t|%s|%ld|%"APR_TIME_T_FMT"|%s| // (NOT written to cache file... expired)\n", the_hash_branch->key, i, the_hash_branch->timestamp, (the_hash_branch->rbl_value)?(the_hash_branch->rbl_value):CLEANLIST_IP_ADDRESS);
                        outlen          = strlen(tmpstr);
                        rv              = apr_file_write(the_log_file, tmpstr, &outlen);
                    }
#endif
                }
    
                the_hash_branch  = the_hash_branch->next;
            }
        }
    }

//        tmpstr          = apr_psprintf(p, "// Serialized file closed. PID #%ld\n", getpid());
//        outlen          = strlen(tmpstr);
//        rv              = apr_file_write(the_hashtable_datafile, tmpstr, &outlen);

    return 1;
}


/*
 *   Function for use during testing to output the IP address of the page requestor and the status of a page request (200 or 403)
 */
static void outputIPOkayJSAlertString(request_rec *r)
{
    char*           text_add            = r->connection->remote_ip;
    ap_rputs("<html><body><pre style=\"background-color: #FFFF00;\">Requesting address: ", r);
    ap_rputs(text_add, r);
    ap_rputs(" is okay. ACCESS GRANTED.</pre></body></html>\n", r);

    return;
}

/*
 *   Function for use during testing to output the IP address of the page requestor and the status of a page request (200 or 403)
 */
static void outputIPAddressJSAlertString(request_rec *r)
{
    char*           text_add            = r->connection->remote_ip;
    ap_rputs("<html><body><pre style=\"background-color: #FFFF00;\">Requesting address: ", r);
    ap_rputs(text_add, r);
    ap_rputs(".  ACCESS DENIED.</pre></body></html>\n", r);

    return;
}

/*
 *   Read the Hostent structure and return a string of the IP contained in the Hostent
 */
static char* getIPStringFromHostent(struct hostent* theHostEnt, request_rec *r)
{
    char*           theReturnValue      = NULL;
    struct in_addr  h_addr;    // internet address

    if(!theHostEnt || !(theHostEnt->h_addr_list) || !(theHostEnt->h_addr_list[0]) )
        return NULL;

    h_addr.s_addr   = *((unsigned long*)theHostEnt->h_addr_list[0]);
    theReturnValue  = apr_psprintf(r->pool, "%s", inet_ntoa(h_addr));

    return theReturnValue;
}

/*
 *   Parse the ip_string for the xth octet_position, then return the integer value.
 *   @params
 *   @params p               temporary pool to use to allocate memory
 *   @params ip_string       the string to use for parsing the IP string
 *   @params octet_position  the position of the octet to be used (1-4 for IPv4 addresses; 1 = first octet, 4 = last octet)
 */
static int getOctetIntFromIPString(apr_pool_t* p, const char* ip_string, int octet_position)
{
    int the_return_value    = -1;

    if(1<=octet_position &&
       4>=octet_position &&
       ip_string) // only check the IP string if the octet position is valid AND the IP String exists
    {
        int     i           = 0;
        char*   the_octet   = NULL;
        char*   delimeter   = ".";
        char*   token       = (char*)apr_palloc(p, 256*sizeof(char));
        char*   running     = apr_pstrdup(p, ip_string);

        for(i=1; i<=ap_min(octet_position+1, 4); i++)
        {
            token       = strsep(&running, delimeter);
            if(token)
            {
                if(i == octet_position)
                {
                    the_return_value    = atoi(token);
/*
                    the_octet   = token;
                    if(the_octet)
                    {
                        the_return_value = atoi(the_octet);
                        break;
                    }
*/
                }
            }
            else
            {
                break;
            }
        }
    }
    else
    {
        the_return_value    = -2;
    }

    return the_return_value;
}

/*
 *   Test a string to see if it contains non-whitespace characters.
 */
int is_empty_string(char* str)
{
    // If there is a character other than a whitepace character, return FALSE (0)
    return(!str || strspn(str, WHITESPACE_CHARS)>0);
}

/*
 *   Trim a string (remove leading and trailing whitespace characters) using APR memory pools.
 */
char* apr_ptrim(apr_pool_t* p, const char* str)
{
    return apr_ptrim_head(p, apr_ptrim_tail(p, str));
}

/*
 *   Trim whitespace from the head of a string using APR memory pools.
 */
char* apr_ptrim_head(apr_pool_t* p, const char* str)
{
    char*   the_headtrimmed_string  = NULL;
// for 'trim' functionality
    // get the index of the first non-whitepsace character
    size_t the_index            = strspn(str, WHITESPACE_CHARS);
    if(the_index>0)
        the_headtrimmed_string  = apr_pstrdup(p, str+the_index); // move the pointer of str to the first non-whitespace character
    else
        the_headtrimmed_string  = apr_pstrdup(p, str);

    return the_headtrimmed_string;
}

/*
 *   Trim whitespace from the tail of a string using APR memory pools.
 */
char* apr_ptrim_tail(apr_pool_t* p, const char* str)
{
    char*   the_tailtrimmed_string  = apr_pstrdup(p, str);
    char    current_char        = '\0';
    int     i                   = 0;

    // start at the end of the string and replace the last character with \0 until we reach a non-whitespace character
    for(i=strlen(the_tailtrimmed_string)-1; i>0 && strrchr(WHITESPACE_CHARS, the_tailtrimmed_string[i]); i--)
        the_tailtrimmed_string[i]   = '\0';

    return the_tailtrimmed_string;
}

/*
 *   An APR wrapper for tolower (which uses an APR pool to allocate memory)
 */
static char* apr_str_tolower(apr_pool_t* p, const char* instring)
{
    char*   outstring   = apr_pstrdup(p, instring);
    char*   i           = NULL;

    for(i=outstring; i<strchr(outstring, '\0'); i++)
        if(apr_isalpha(*i) && apr_isupper(*i))
            *i  = apr_tolower(*i);

    return outstring;
}

/*
 *   match two strings but ignore the cases of the strings)
 *   i.e. "abcde" will match "AbCDe"
 */
static int apr_string_matches_ignore_case(apr_pool_t* pool, const char* str1, const char* str2)
{
    return httpbl_string_matches(apr_str_tolower(pool, str1), apr_str_tolower(pool, str2));
}

/*
 *   write the contents of the svr_cfg structure to a file named "svr_cfg_trace[%PID%]"
 */
static int dump_svr_cfg_to_file(apr_pool_t* p, httpbl_dir_cfg* this_svr_cfg)
{
#if VERBOSITY >= APLOG_DEBUG
    apr_file_t*         the_file;
    apr_status_t        rv;
    apr_size_t          outlen;
    char*               tmpstr;
    apr_time_t          t               = apr_time_now();
    char*               the_filename    = apr_psprintf(p, "%s%ssvr_cfg_trace[%ld].log", get_log_dir(), DEFAULT_HTTPBL_FILENAME_PREFIX, getpid());
    if(the_filename)
        rv              = apr_file_open(&the_file, the_filename, APR_WRITE | APR_APPEND | APR_CREATE, APR_OS_DEFAULT, p);
    else
        return 0; // we can't do anything if we don't have the correct filename

    if(rv == APR_SUCCESS)
    {
        tmpstr          = apr_psprintf(p, "\n\n\n// File opened for writing! (%ld)\n", t);
        outlen          = strlen(tmpstr);
        rv              = apr_file_write(the_file, tmpstr, &outlen);
    
        tmpstr          = apr_psprintf(p, "httpbl_dir_cfg: {\n");
        outlen          = strlen(tmpstr);
        rv              = apr_file_write(the_file, tmpstr, &outlen);
    
        tmpstr          = apr_psprintf(p, "\tis_httpbl_enabled            = %s\n",  !is_set_int(this_svr_cfg->is_httpbl_enabled)?"UNSET":is_enabled_int(this_svr_cfg->is_httpbl_enabled)?"TRUE":"FALSE");
        outlen          = strlen(tmpstr);
        rv              = apr_file_write(the_file, tmpstr, &outlen);
    
        tmpstr          = apr_psprintf(p, "\tis_404_recording_enabled     = %s\n",  !is_set_int(this_svr_cfg->is_404_recording_enabled)?"UNSET":is_enabled_int(this_svr_cfg->is_404_recording_enabled)?"TRUE":"FALSE");
        outlen          = strlen(tmpstr);
        rv              = apr_file_write(the_file, tmpstr, &outlen);
    
        tmpstr          = apr_psprintf(p, "\tis_404_recording_enabled     = %s\n",  !is_set_int(this_svr_cfg->is_POST_recording_enabled)?"UNSET":is_enabled_int(this_svr_cfg->is_POST_recording_enabled)?"TRUE":"FALSE");
        outlen          = strlen(tmpstr);
        rv              = apr_file_write(the_file, tmpstr, &outlen);
    
        tmpstr          = apr_psprintf(p, "\tdefault_action               = %s\n",  get_action_printable_string(this_svr_cfg->default_action));
        outlen          = strlen(tmpstr);
        rv              = apr_file_write(the_file, tmpstr, &outlen);
    
        tmpstr          = apr_psprintf(p, "\ttoken_str                    = %s\n",  this_svr_cfg->token_str);
        outlen          = strlen(tmpstr);
        rv              = apr_file_write(the_file, tmpstr, &outlen);
    
        tmpstr          = apr_psprintf(p, "\tnum_of_rbl_handlers          = %u\n",  this_svr_cfg->num_of_rbl_handlers);
        outlen          = strlen(tmpstr);
        rv              = apr_file_write(the_file, tmpstr, &outlen);
    
        tmpstr          = apr_psprintf(p, "\tthe_rbl_handlers             = %s\n",  (this_svr_cfg->the_rbl_handlers)?"NOT_NULL":"NULL");
        outlen          = strlen(tmpstr);
        rv              = apr_file_write(the_file, tmpstr, &outlen);
    
        if(this_svr_cfg->the_rbl_handlers && this_svr_cfg->num_of_rbl_handlers>0)
        {
            int i = 0;
            rbl_handler* this_rbl_handler   = NULL;
            for(i=0; i<this_svr_cfg->num_of_rbl_handlers; i++)
            {
                this_rbl_handler    = (this_svr_cfg->the_rbl_handlers)[i];
    
                tmpstr          = apr_psprintf(p, "\tRBL Handler #%u", i);
                outlen          = strlen(tmpstr);
                rv              = apr_file_write(the_file, tmpstr, &outlen);
    
                if(!this_rbl_handler)
                {
    
                    tmpstr          = apr_psprintf(p, "            NULL\n");
                    outlen          = strlen(tmpstr);
                    rv              = apr_file_write(the_file, tmpstr, &outlen);
                }
                else
                {
    
                    tmpstr          = apr_psprintf(p, "            RBL Handler [action: %s] [days: %d-%d] [score: %d-%d] [verbs: %"APR_UINT64_T_FMT"] [categories: %u]\n", (this_rbl_handler->action_string != NULL)?this_rbl_handler->action_string:"", this_rbl_handler->days_lb, this_rbl_handler->days_ub, this_rbl_handler->score_lb, this_rbl_handler->score_ub, this_rbl_handler->verb_bs, this_rbl_handler->category_bs);
                    outlen          = strlen(tmpstr);
                    rv              = apr_file_write(the_file, tmpstr, &outlen);
                }
            }
        }
    
        tmpstr          = apr_psprintf(p, "}\n");
        outlen          = strlen(tmpstr);
        rv              = apr_file_write(the_file, tmpstr, &outlen);
    
        rv              = apr_file_close(the_file);
    }
#endif
    return 1;
}

/*
 *   write the contents of this dir_cfg structure to a file named "httpbl_dir_cfg_trace[%PID%]"
 */
static int dump_dir_cfg_to_file(apr_pool_t* p, httpbl_dir_cfg* this_dir_cfg)
{
#if VERBOSITY >= APLOG_DEBUG
    apr_file_t*         the_file        = NULL;
    apr_status_t        rv              = 1;
    apr_size_t          outlen          = 0;
    char*               tmpstr          = NULL;
    apr_time_t          t               = apr_time_now();
    char*               the_filename    = apr_psprintf(p, "%s%sdir_cfg_trace[%ld].log", get_log_dir(), DEFAULT_HTTPBL_FILENAME_PREFIX, getpid());

    if(the_filename)
        rv              = apr_file_open(&the_file, the_filename, APR_WRITE | APR_APPEND | APR_CREATE, APR_OS_DEFAULT, p);
    else
        return 0; // we can't do anything if we don't have the correct filename

    if(rv == APR_SUCCESS)
    {
        tmpstr          = apr_psprintf(p, "\n\n\n// File opened for writing! (%ld)\n", t);
        outlen          = strlen(tmpstr);
        rv              = apr_file_write(the_file, tmpstr, &outlen);
    
        tmpstr          = apr_psprintf(p, "httpbl_dir_cfg: {\n");
        outlen          = strlen(tmpstr);
        rv              = apr_file_write(the_file, tmpstr, &outlen);
    
        tmpstr          = apr_psprintf(p, "\tis_httpbl_enabled            = %s\n",  !is_set_int(this_dir_cfg->is_httpbl_enabled)?"UNSET":is_enabled_int(this_dir_cfg->is_httpbl_enabled)?"TRUE":"FALSE");
        outlen          = strlen(tmpstr);
        rv              = apr_file_write(the_file, tmpstr, &outlen);
    
        tmpstr          = apr_psprintf(p, "\tis_404_recording_enabled     = %s\n",  !is_set_int(this_dir_cfg->is_404_recording_enabled)?"UNSET":is_enabled_int(this_dir_cfg->is_404_recording_enabled)?"TRUE":"FALSE");
        outlen          = strlen(tmpstr);
        rv              = apr_file_write(the_file, tmpstr, &outlen);
    
        tmpstr          = apr_psprintf(p, "\tis_POST_recording_enabled    = %s\n",  !is_set_int(this_dir_cfg->is_POST_recording_enabled)?"UNSET":is_enabled_int(this_dir_cfg->is_POST_recording_enabled)?"TRUE":"FALSE");
        outlen          = strlen(tmpstr);
        rv              = apr_file_write(the_file, tmpstr, &outlen);
    
        tmpstr          = apr_psprintf(p, "\tdefault_action               = %s\n",  get_action_printable_string(this_dir_cfg->default_action));
        outlen          = strlen(tmpstr);
        rv              = apr_file_write(the_file, tmpstr, &outlen);
    
        tmpstr          = apr_psprintf(p, "\ttoken_str                    = %s\n",  this_dir_cfg->token_str);
        outlen          = strlen(tmpstr);
        rv              = apr_file_write(the_file, tmpstr, &outlen);
    
        tmpstr          = apr_psprintf(p, "\tnum_of_rbl_handlers          = %d\n",  this_dir_cfg->num_of_rbl_handlers);
        outlen          = strlen(tmpstr);
        rv              = apr_file_write(the_file, tmpstr, &outlen);
    
        tmpstr          = apr_psprintf(p, "\tthe_rbl_handlers             = %s\n",  (this_dir_cfg->the_rbl_handlers)?"NOT_NULL":"NULL");
        outlen          = strlen(tmpstr);
        rv              = apr_file_write(the_file, tmpstr, &outlen);
    
        if(this_dir_cfg->the_rbl_handlers && this_dir_cfg->num_of_rbl_handlers>0)
        {
            int i = 0;
            rbl_handler* this_rbl_handler   = NULL;
            for(i=0; i<this_dir_cfg->num_of_rbl_handlers; i++)
            {
                this_rbl_handler    = this_dir_cfg->the_rbl_handlers[i];
    
                tmpstr          = apr_psprintf(p, "\tRBL Handler #%d", i);
                outlen          = strlen(tmpstr);
                rv              = apr_file_write(the_file, tmpstr, &outlen);
    
                if(!this_rbl_handler)
                {
                    tmpstr          = apr_psprintf(p, "            NULL\n");
                    outlen          = strlen(tmpstr);
                    rv              = apr_file_write(the_file, tmpstr, &outlen);
                }
                else
                {
                    tmpstr          = apr_psprintf(p, "            RBL Handler [categories: %d] [score: %d-%d] [days: %d-%d] [verbs: %"APR_UINT64_T_FMT"] [action: %s]\n", this_rbl_handler->category_bs, this_rbl_handler->score_lb, this_rbl_handler->score_ub, this_rbl_handler->days_lb, this_rbl_handler->days_ub, this_rbl_handler->verb_bs, (this_rbl_handler->action_string != NULL)?this_rbl_handler->action_string:"");
                    outlen          = strlen(tmpstr);
                    rv              = apr_file_write(the_file, tmpstr, &outlen);
                }
            }
        }
    
        tmpstr          = apr_psprintf(p, "}\n");
        outlen          = strlen(tmpstr);
        rv              = apr_file_write(the_file, tmpstr, &outlen);
    
        rv              = apr_file_close(the_file);
    }
#endif
    return 1;
}

/*
 *   dump the values of either a dir_cfg or a svr_cfg structure to a logfile.
 */
void dump_rbl_toggle_to_log(apr_pool_t* pool, httpbl_dir_cfg* dir_cfg, httpbl_dir_cfg* svr_cfg, char* intro_str)
{
    apr_status_t    rv          = 1;
    apr_file_t*     fp          = NULL;
    char*           tmpstr      = NULL;
    apr_size_t      outlen      = 0;

    // record the fact that we posted to a logfile
    if(g_rbl_toggle_log_filepath)
        rv = apr_file_open(&fp, g_rbl_toggle_log_filepath, APR_APPEND | APR_WRITE | APR_CREATE, APR_OS_DEFAULT, pool);
    else
        return; // we can't do anything without the correct filename

    if(rv == APR_SUCCESS)
    {
        char* is_httpbl_enabled_str = NULL;
        char* is_404_enabled_str    = NULL;
        char* tmpstr2               = NULL;

        tmpstr  = apr_psprintf(pool, "-----------------------------\nRBL TOGGLE:%s\n", intro_str);
        outlen = strlen(tmpstr);
        apr_file_write(fp, tmpstr, &outlen);

        if(svr_cfg)
        {
            tmpstr  = apr_psprintf(pool, "HTTPBL ON (svr):\t%s (pid: %ld)\n", !is_set_int(svr_cfg->is_httpbl_enabled)?"UNSET":is_enabled_int(svr_cfg->is_httpbl_enabled)?"TRUE":"FALSE", (long)getpid());
            outlen = strlen(tmpstr);
            apr_file_write(fp, tmpstr, &outlen);
    
            tmpstr  = apr_psprintf(pool, "404 RECORDING ON (svr):\t%d (pid: %ld)\n", !is_set_int(svr_cfg->is_404_recording_enabled)?"UNSET":is_enabled_int(svr_cfg->is_404_recording_enabled)?"TRUE":"FALSE", (long)getpid());
            outlen = strlen(tmpstr);
            apr_file_write(fp, tmpstr, &outlen);

            tmpstr  = apr_psprintf(pool, "POST RECORDING ON (svr):\t%d (pid: %ld)\n", !is_set_int(svr_cfg->is_POST_recording_enabled)?"UNSET":is_enabled_int(svr_cfg->is_POST_recording_enabled)?"TRUE":"FALSE", (long)getpid());
            outlen = strlen(tmpstr);
            apr_file_write(fp, tmpstr, &outlen);

            tmpstr  = apr_psprintf(pool, "DEFAULT_ACTION (svr):\t%s (pid: %ld)\n", get_action_printable_string(svr_cfg->default_action), (long)getpid());
            outlen = strlen(tmpstr);
            apr_file_write(fp, tmpstr, &outlen);
    
            tmpstr  = apr_psprintf(pool, "TOKEN (svr):\t%d (pid: %ld)\n", (svr_cfg->token_str != NULL)?svr_cfg->token_str:"NULL", (long)getpid());
            outlen = strlen(tmpstr);
            apr_file_write(fp, tmpstr, &outlen);
        }

        if(dir_cfg)
        {
            tmpstr  = apr_psprintf(pool, "HTTPBL ON (dir):\t%s (pid: %ld)\n", !is_set_int(dir_cfg->is_httpbl_enabled)?"UNSET":is_enabled_int(dir_cfg->is_httpbl_enabled)?"TRUE":"FALSE", (long)getpid());
            outlen = strlen(tmpstr);
            apr_file_write(fp, tmpstr, &outlen);
    
            tmpstr  = apr_psprintf(pool, "404 RECORDING ON (dir):\t%d (pid: %ld)\n", !is_set_int(dir_cfg->is_404_recording_enabled)?"UNSET":is_enabled_int(dir_cfg->is_404_recording_enabled)?"TRUE":"FALSE", (long)getpid());
            outlen = strlen(tmpstr);
            apr_file_write(fp, tmpstr, &outlen);

            tmpstr  = apr_psprintf(pool, "POST RECORDING ON (dir):\t%d (pid: %ld)\n", !is_set_int(dir_cfg->is_POST_recording_enabled)?"UNSET":is_enabled_int(dir_cfg->is_POST_recording_enabled)?"TRUE":"FALSE", (long)getpid());
            outlen = strlen(tmpstr);
            apr_file_write(fp, tmpstr, &outlen);

            tmpstr  = apr_psprintf(pool, "DEFAULT_ACTION (dir):\t%s (pid: %ld)\n", get_action_printable_string(dir_cfg->default_action), (long)getpid());
            outlen = strlen(tmpstr);
            apr_file_write(fp, tmpstr, &outlen);
    
            tmpstr  = apr_psprintf(pool, "TOKEN (dir):\t%d (pid: %ld)\n", (dir_cfg->token_str != NULL)?dir_cfg->token_str:"NULL", (long)getpid());
            outlen = strlen(tmpstr);
            apr_file_write(fp, tmpstr, &outlen);
        }

        tmpstr  = apr_psprintf(pool, "-----------------------------\n\n");
        outlen  = strlen(tmpstr);
        apr_file_write(fp, tmpstr, &outlen);

        apr_file_close(fp);
    }
}

/*
 *   Check the context of a directive call.
 *   @return         TRUE iff context {Global, <VirtualHost>, <Limit>}
 *   @return         FALSE iff context {<Files>, <Directory>, <Location>}
 */
static int cmd_is_server_context(cmd_parms* cmd)
{
    const char* err;
    err = ap_check_cmd_context(cmd, NOT_IN_DIR_LOC_FILE);
    if(err) // if the directive was called with a directory-context...
        return 0;
    else
        return 1;
}

/*
 *   Validate a range string (make sure the string is of format: /[0-9]+(-[0-9])?+/, where both the lower bound and upper bound
 *   fall between the_lowest_acceptable and the_highest_acceptable, numerically)
 *   @return             TRUE iff (1) the string is a range consisting of [int1][-][int2] or simply [int1], (2) int1 <= int2 (3) int1 >= the_lowest_acceptable, (4) int2 >= the_highest_acceptable
 *   @param  the_string  the string to test for a valid range
 *   @param  the_lowest_acceptable   the highest acceptable integer value for either of the integers in the range
 *   @param  the_highest_acceptable  the lowest acceptable integer value for either of the integers in the range
 */
static int string_is_valid_int_range(apr_pool_t* p, const char* the_string, int the_lowest_acceptable, int the_highest_acceptable)
{
    char*   delims              = "-";

    if(!the_string)
        return 0; // valid range (NULL string)
    else if(strspn(the_string, "0987654321-") >= 0 &&
            strspn(the_string, "0987654321-") < strlen(the_string))
        return 0; // not a valid integer range (contains a character other than [-0-9])
    else
    {
        char*   lb      = NULL;
        char*   ub      = NULL;
        char*   running = (char*)apr_pstrdup(p, the_string);

        lb              = strsep(&running, delims);
        ub              = strsep(&running, delims);

        if(!ub || httpbl_string_matches(ub, ""))    // if the "range" is a single integer...
            ub  = lb;                               // duplicate the 

        // make sure there are no more ranges (more than one hyphen in the string)
        if (strsep(&running, delims))
            return 0; // string contains more than one hyphen... improperly formatted

        // check the integer values for validity
        if(atoi(lb) >= the_lowest_acceptable  &&
           atoi(lb) <= the_highest_acceptable &&
           atoi(ub) >= the_lowest_acceptable  &&
           atoi(ub) <= the_highest_acceptable &&
           atoi(lb) <= atoi(ub))
            return 1; // properly formatted and bounds are within range
        else
            return 0; // properly formatted but at least one bound is out of the acceptable range
    }
}

/*
 * Returns a string identical to the kvp_string parameter but with append_text prepended to every key name
 * (kvp means "key-value pair")
 * Example: pass_kvps_to_honeypot(..., "key1=value1&key2=value2", "post|") should return "post|key1=value1&post|key2=value2"
 * kvp_string should be formatted like the query_string of a uri_components struct:
 * - kvps are seperated by a '&' character
 * - keys are seperated from values by a '=' character
 * - all other occurrances of "special URL" characters (including '/', '?', '&', '=', and '#') should be URL-escaped
 */
static char* pass_kvps_to_honeypot(request_rec* r, const char* the_kvp_string, char* append_text, char* counter_key)
{
    char*   delimeter_str           = "&";
    char    delimeter_char          = '&';
    char*   return_string           = NULL;
    char*   return_string_cursor    = NULL;
    char*   kvp_string              = NULL;
    char*   kvp_string_cursor       = NULL;
    int     possible_num_of_kvps    = 0;
    int     actual_num_of_kvps      = 0;
    int     append_text_length      = strlen(append_text);
    int     allocation_length       = 0;
    char*   tmpstr                  = NULL;

    if(!the_kvp_string)
        return NULL;

    possible_num_of_kvps            = charcnt(the_kvp_string, '&')+1;   // if there are 5 '&' characters, there are 6 possible kvps
    allocation_length               = strlen(the_kvp_string) + strlen(counter_key) + 6 + (possible_num_of_kvps*(strlen(append_text)));

    if(strlen(the_kvp_string) > MAX_POST_LENGTH*1024*sizeof(char))  // MAX_POST_LENGTH (in kiloBytes) to try and process
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, 0, NULL, "HTTPBL received a request which contained parameters too large to process.  Dropping the parameters.");
        return NULL;
    }

    return_string                   = apr_pcalloc(r->pool,(allocation_length+1)*sizeof(char));
    if(!return_string)
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, 0, NULL, "HTTPBL was not granted enough memory to process a large parameter string.  Dropping the parameters.");
        return NULL;
    }

    *return_string                  = '\0';                         // we could run into problems if we don't terminate this string
    return_string_cursor            = return_string;
    kvp_string                      = apr_pstrdup(r->pool, the_kvp_string);    // make a working copy of the_kvp_string

    if(kvp_string[0] == '?')                        // handle the special case where the kvp_string starts with a '?'
        kvp_string++;                               // move the kvp_string to the next character

    int i   = 0;

    do
    {
        tmpstr                          = strsep(&kvp_string, delimeter_str);
        if(tmpstr)                                  // if this kvp is NULL, we have reached the end of kvp_string
        {
            int this_kvp_len                = strlen(tmpstr);
            if(this_kvp_len > 0)
            {
                // copy the string to prepend to the kvp
                memcpy(return_string_cursor, append_text, append_text_length);
                return_string_cursor += append_text_length;
//                *(return_string_cursor+1)= '\0';

                // copy this kvp
                memcpy(return_string_cursor, tmpstr, this_kvp_len);
                return_string_cursor += this_kvp_len;
//                *(return_string_cursor+1)= '\0';

                actual_num_of_kvps++;
                // copy the delimeter (if there is another kvp in the string)
                if(kvp_string)
                {
                    *return_string_cursor           = delimeter_char;
                    return_string_cursor           += strlen(delimeter_str);
                    *return_string_cursor           = '\0';
                }
            }
        }
        i++;
    }
    while(kvp_string && strlen(kvp_string) > 0);

    if(actual_num_of_kvps > 0)
    {
        char*   counter_str             = apr_psprintf(r->pool, "&%s=%d", counter_key, actual_num_of_kvps);
        int     counter_str_len         = strlen(counter_str);

        memcpy(return_string_cursor, counter_str, counter_str_len);
        return_string_cursor           += counter_str_len;
        *return_string_cursor           = '\0';
    }

    return return_string;
}

/*
 *   Parse a string (of format: /[0-9]+-[0-9]+/) and load the_range_lb and the_range_ub with the values before and after the hyphen, respectively.
 *   Assumes the_range_string is a valid int range (use string_is_valid_int_range(...)).
 *   @return         TRUE if successful (the_range_lb and the_range_ub are populated); FALSE otherwise
 */
static int parse_int_range(apr_pool_t* p, const char* the_range_string, int* the_range_lb, int* the_range_ub)
{
    char*   delims  = "-";
    char*   lb      = NULL;
    char*   ub      = NULL;
    char*   running = (char*)apr_pstrdup(p, the_range_string);

    lb              = strsep(&running, delims);
    ub              = strsep(&running, delims);
    if(lb && ub)
    {
        *the_range_lb   = atoi(lb);
        *the_range_ub   = atoi(ub);
        return 1;
    }
    else if(lb)
    {
        *the_range_lb   = atoi(lb);
        *the_range_ub   = atoi(lb);
    }
    else
        return 0;
}


/*
 *   Function for setting the initial hash table size from the hash table size Directive
 *   @return         a string explaining the problem with parsing and/or processing; NULL if no problem
 */
static const char* directive_set_hash_tbl_size(cmd_parms* cmd, void* dconfig, const char* value)
{
    long n = strtol(value, NULL, 0);

    if(n<=0)
        reccomended_hash_table_size = DEFAULT_HASH_TBL_SIZE;
    else
        reccomended_hash_table_size = (unsigned long)n;

    return NULL;
}

/*
 *   Function for setting the blocking period from the blocking period Directive
 *   @return         a string explaining the problem with parsing and/or processing; NULL if no problem
 */
static const char* directive_set_blocking_period(cmd_parms* cmd, void* dconfig, const char* value)
{
    long n = strtol(value, NULL, 0);
    if(n<=0)
        blocking_period = DEFAULT_BLOCKING_PERIOD;
    else
        blocking_period = n;

    return NULL;
}

/*
 *   Add a given IP address to the whitelist (as a directive)
 *   @param  ip      an IP address correctly formatted in decimal-dot notation (i.e. "127.0.0.1")
 *   @return         a string describing any errors while processing this directive (to be shown in stderr when the directive kills ApacheD startup)
 */
static const char* directive_add_to_whitelist(cmd_parms* cmd, void* dconfig, const char* ip)
{
    char* entry;
    entry   = apr_pstrdup(cmd->pool, ip); // allocate pooled memory for the storage of the whitelist key (IP string)
    whitelist_insert(cmd->temp_pool, entry);

    return NULL;
}

/*
 *   Set a directory to be exempt from HTTPBL filtering (effectively: all IPS are whitelisted for Exempted Directories)
 *   This should be used for captcha challenge directories/files.
 *   @return         a string explaining the problem with parsing and/or processing; NULL if no problem
 */
static const char* directive_set_httpbl_exempt(cmd_parms* cmd, void* dconfig, const int f)
{
    char*   the_return_string       = NULL;
    int     g                       = (f==0)?0:1;
    if(cmd_is_server_context(cmd)) // server-context
    {
        httpbl_dir_cfg* svr         = (httpbl_dir_cfg*)ap_get_module_config(cmd->server->module_config, &httpbl_module);
        svr->is_exempt              = g;
//            the_return_string           = apr_psprintf(cmd->pool, "Directory exempted (svr): %d", g);
    }
    else // if the directive was called with a directory-context...
    {
        httpbl_dir_cfg* the_config  = (httpbl_dir_cfg*)dconfig;
        the_config->is_exempt       = g;
//            the_return_string           = apr_psprintf(cmd->pool, "Directory exempted (dir): %d; dir=\"%s\"", g, cmd->path);
    }

    return the_return_string;
}


/*
 *   Set the domain to use as an RBL lookup
 */
static const char* directive_set_rbl_access_key(cmd_parms* cmd, void* dconfig, const char* access_key)
{
    char*   the_return_string   = NULL;

    if(strcspn(apr_str_tolower(cmd->temp_pool, access_key), VALID_ALPHA_CHARS)>0)
    {
        the_return_string   = apr_psprintf(cmd->temp_pool, "The "DIRECTIVE_TEXT_ACCESS_KEY" you entered contains invalid alphabetic characters (character %d).  Please use a valid RBL access key.", strspn(access_key, VALID_ALPHA_CHARS)); // XXX - describe where to get the access_key
    }
    else if(strlen(access_key) != 12)
    {
        the_return_string   = apr_psprintf(cmd->temp_pool, "The "DIRECTIVE_TEXT_ACCESS_KEY" you entered is an invalid length (your access key value length: %d characters; required length: 12 characters).  Please use a valid RBL access key.", strlen(access_key)); // XXX - describe where to get the access_key
    }
    else
    {
        if(cmd_is_server_context(cmd)) // server-context
        {
            g_an_access_key_was_set     = 1;
            httpbl_dir_cfg* svr         = (httpbl_dir_cfg*)ap_get_module_config(cmd->server->module_config, &httpbl_module);
            svr->access_key             = apr_pstrdup(cmd->pool, apr_str_tolower(cmd->temp_pool, access_key));
        }
    }

    return the_return_string;
}


/*
 *   Set the domain to use as an RBL lookup
 */
static const char* directive_set_rbl_domain(cmd_parms* cmd, void* dconfig, const char* rbl_domain)
{
    char*   the_return_string   = NULL;

    if(strcspn(apr_str_tolower(cmd->temp_pool, rbl_domain), VALID_DOMAIN_CHARS)>0)
    {
        the_return_string   = apr_psprintf(cmd->temp_pool, "The "DIRECTIVE_TEXT_RBL_DOMAIN" you entered contains invalid domain characters (character %d).  Please use a valid RBL server domain or remove this directive to use the default domain.", strspn(rbl_domain, VALID_DOMAIN_CHARS));
    }
    else
    {
        if(cmd_is_server_context(cmd)) // server-context
        {
            httpbl_dir_cfg* svr         = (httpbl_dir_cfg*)ap_get_module_config(cmd->server->module_config, &httpbl_module);
            svr->dns_lookup_domain      = apr_pstrdup(cmd->pool, rbl_domain);
        }
        else // if the directive was called with a directory-context...
        {
            httpbl_dir_cfg* the_config  = (httpbl_dir_cfg*)dconfig;
            the_config->dns_lookup_domain = apr_pstrdup(cmd->pool, rbl_domain);
        }
    }

    return the_return_string;
}


/*
 *   Accept the URL to be used to challenge visitors
 *   @return         a string explaining the problem with parsing and/or processing; NULL if no problem
 */
static const char* directive_set_honeypot_url(cmd_parms* cmd, void* dconfig, const char* honeypot_url)
{
    char*   the_return_string   = NULL;

    if(strcspn(apr_str_tolower(cmd->temp_pool, honeypot_url), VALID_URL_CHARS)>0 /*&&
        strspn(apr_str_tolower(cmd->temp_pool, honeypot_url), VALID_URL_CHARS)<strlen(honeypot_url)*/)
    {
        the_return_string   = apr_psprintf(cmd->temp_pool, "The "DIRECTIVE_TEXT_HONEYPOT_URL" you entered contains invalid URL characters (character %d).", strspn(honeypot_url, VALID_URL_CHARS));
    }
    else
    {
#ifdef SHOULD_REQUEST_HONEYPOTS
        if(cmd_is_server_context(cmd)) // server-context
        {
            httpbl_dir_cfg* svr         = (httpbl_dir_cfg*)ap_get_module_config(cmd->server->module_config, &httpbl_module);
            svr->honeypot_url           = apr_pstrdup(cmd->pool, honeypot_url);
        }
#endif
    }

    return the_return_string;
}


/*
 *   Accept the URL to be used to challenge visitors
 *   @return         a string explaining the problem with parsing and/or processing; NULL if no problem
 */
static const char* directive_set_g_httpbl_testing_url(cmd_parms* cmd, void* dconfig, const char* httpbl_diagnostic_url)
{
    char*   the_return_string           = NULL;
    // string-validation disabled due to errors with "apr_str_tolower", "strcspn" and "strspn" with some strings
    char*   httpbl_diagnostic_url_lower = /*apr_str_tolower(cmd->temp_pool, */g_httpbl_testing_url/*)*/;

/*
// this code was causing compile errors on my dev box
return apr_psprintf(cmd->temp_pool, "testing url: \"%s\" (%d), valid chars: \"%s\" (%d), strspn(...) = %d", httpbl_diagnostic_url, strlen(httpbl_diagnostic_url), VALID_URL_CHARS, strlen(VALID_URL_CHARS), strcspn(httpbl_diagnostic_url_lower, VALID_URL_CHARS));
*/

// this line temporarily disables the use of this directive.  Only the hard-coded value is usable for the time being.
return "The "" directive is temporarily disabled.  The default URL (\""DEFAULT_HTTPBL_TESTING_URL"\") is hardcoded and can not be altered (in this version).";


    if(strcspn(httpbl_diagnostic_url_lower, VALID_URL_CHARS)>0 &&
        strspn(httpbl_diagnostic_url_lower, VALID_URL_CHARS)<strlen(g_httpbl_testing_url))
    {
        the_return_string   = apr_psprintf(cmd->temp_pool, "The "DIRECTIVE_TEXT_DIAGNOSTICS_URL" you entered contains invalid URL characters (character %d).", strspn(httpbl_diagnostic_url_lower, VALID_URL_CHARS));
    }
    else
    {
        if(cmd_is_server_context(cmd)) // server-context
        {
            g_httpbl_testing_url          = apr_pstrdup(cmd->pool, httpbl_diagnostic_url_lower);
        }
        else
        {
            the_return_string             = apr_psprintf(cmd->temp_pool, "The "DIRECTIVE_TEXT_DIAGNOSTICS_URL" directive only works from a server context.");
        }
    }

    return the_return_string;
}


/*
 *   Accept the URL to be used to challenge visitors
 *   @return         a string explaining the problem with parsing and/or processing; NULL if no problem
 */
static const char* directive_set_challenge_url(cmd_parms* cmd, void* dconfig, const char* challenge_url)
{
    char*   the_return_string   = NULL;

    if(strcspn(apr_str_tolower(cmd->temp_pool, challenge_url), VALID_URL_CHARS)>0 /*&&
        strspn(apr_str_tolower(cmd->temp_pool, challenge_url), VALID_URL_CHARS)<strlen(challenge_url)*/)
    {
        the_return_string   = apr_psprintf(cmd->temp_pool, "The "DIRECTIVE_TEXT_CHALLENGE_URL" you entered contains invalid URL characters (character %d).  Please URLEncode all characters in this parameter.", strspn(challenge_url, VALID_URL_CHARS));
    }
    else
    {
#ifdef SHOULD_ALLOW_CHALLENGES
        if(cmd_is_server_context(cmd)) // server-context
        {
            httpbl_dir_cfg* svr         = (httpbl_dir_cfg*)ap_get_module_config(cmd->server->module_config, &httpbl_module);
            svr->challenge_url          = apr_pstrdup(cmd->pool, challenge_url);
        }
        else // if the directive was called with a directory-context...
        {
            httpbl_dir_cfg* the_config  = (httpbl_dir_cfg*)dconfig;
            the_config->challenge_url   = apr_pstrdup(cmd->pool, challenge_url);
        }
#endif
    }

    return the_return_string;
}

/*
 *   Accept the URL to be used to challenge visitors
 *   @return         a string explaining the problem with parsing and/or processing; NULL if no problem
 */
static const char* directive_set_replace_emails(cmd_parms* cmd, void* dconfig, const char* the_requested_replacement_scheme)
{
    char*   the_return_string       = NULL;
    int     the_replacement_scheme  = ERS_DEFAULT;

    if(!the_requested_replacement_scheme || // a NULL value should be the same as 'default'
       httpbl_string_matches(the_requested_replacement_scheme, "default") ||
       httpbl_string_matches(the_requested_replacement_scheme, "link"))
    {
        the_replacement_scheme  = ERS_DEFAULT;
    }
    else if(httpbl_string_matches(the_requested_replacement_scheme, "simple") ||
            httpbl_string_matches(the_requested_replacement_scheme, "text"))
    {
        the_replacement_scheme  = ERS_TEXT;
    }
    else if(httpbl_string_matches(the_requested_replacement_scheme, "none") ||
            httpbl_string_matches(the_requested_replacement_scheme, "off"))
    {
        the_replacement_scheme  = ERS_OFF;
    }
    else
    {
        the_return_string   = "Invalid choice for value of directive HTTPBLReplaceEmails.  Valid options are: {'default', 'simple', 'none'}";
    }

    if(!the_return_string) // the requested_replacement_scheme is a valid choice
    {
//        = the_replacement_scheme;
    }

    return the_return_string;
}

/*
 *   Accept the token to be used to whitelist an IP (must consist of only characters in 'valid_chars', case INsensitive)
 *   @return         a string explaining the problem with parsing and/or processing; NULL if no problem
 */
static const char* directive_set_challenge_token(cmd_parms* cmd, void* dconfig, const char* token_str)
{
    char*   valid_chars         = VALID_TOKEN_CHARS;
    char*   the_return_string   = NULL;

    if(strcspn(apr_str_tolower(cmd->pool, token_str), valid_chars)>0)
    {
        the_return_string   = "The "DIRECTIVE_TEXT_CHALLENGE_TOKEN" contains invalid characters.  Only A-Z and 0-9 are allowed.";
    }
    else
    {
#ifdef SHOULD_ALLOW_CHALLENGES
        if(cmd_is_server_context(cmd)) // server-context
        {
            httpbl_dir_cfg* svr         = (httpbl_dir_cfg*)ap_get_module_config(cmd->server->module_config, &httpbl_module);
            svr->token_str              = apr_pstrdup(cmd->pool, token_str);
            ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: \t"DIRECTIVE_TEXT_CHALLENGE_TOKEN" accepted (svr): %s", token_str);
        }
        else // if the directive was called with a directory-context...
        {
            httpbl_dir_cfg* the_config  = (httpbl_dir_cfg*)dconfig;
            the_config->token_str       = apr_pstrdup(cmd->pool, token_str);
            ap_log_error(APLOG_MARK, APLOG_INFO, 0, NULL, "HTTPBL: \t"DIRECTIVE_TEXT_CHALLENGE_TOKEN" accepted (svr): %s", token_str);
        }
#endif
    }
    return the_return_string;
}

/*
 *   Function for setting the repos file directory from the httpbl repos Directive
 *   @return         a string explaining the problem with parsing and/or processing; NULL if no problem
 */
static const char* directive_set_repos_dir(cmd_parms* cmd, void* dconfig, const char* value)
{
    if(value)
    {
        struct stat statbuf;
        if(!(lstat(value, &statbuf)))
        {
            if (value[strlen(value)-1] == '/') // check the last character to see if it is a '/' or other character
            {
                repos_dir                           = apr_pstrdup(cmd->pool, value);
            }
            else
            {
                repos_dir                           = apr_palloc(cmd->pool, (strlen(value)+1) * sizeof(char));
                repos_dir                           = apr_psprintf(cmd->pool, "%s/", value);
            }
    
            int reposfile_path_strlen           = strlen(repos_dir);
//return apr_psprintf(cmd->pool, "Repos directory set to: \"%s\" (length: %d)", repos_dir, strlen(repos_dir));
    
            // reset all of the repos_dir-dependent filepath variables
            restructure_repos_filepaths(cmd->pool);
        }
        else
            return apr_psprintf(cmd->pool, "Could not find a directory with path \"%s\".  Please ensure this directory exists and permissions allow the user running Apache to view and write to this directory.", value);
    }
    else
        return apr_psprintf(cmd->pool, ".", value);

    return NULL;
}

/*
 *   Function for setting the log file directory from the httpbl log Directive
 *   @return         a string explaining the problem with parsing and/or processing; NULL if no problem
 */
static const char* directive_set_log_dir(cmd_parms* cmd, void* dconfig, const char* value)
{
    if(value)
    {
        struct stat statbuf;
        if(!(lstat(value, &statbuf)))
        {
            if (value[strlen(value)-1] == '/') // check the last character to see if it is a '/' or other character
            {
                log_dir                             = apr_pstrdup(cmd->pool, value);
            }
            else
            {
                log_dir                             = apr_palloc(cmd->pool, (strlen(value)+1) * sizeof(char));
                log_dir                             = apr_psprintf(cmd->pool, "%s/", value);
            }
    
//return apr_psprintf(cmd->pool, "Repos directory set to: \"%s\" (length: %d)", log_dir, strlen(log_dir));
            int logfile_path_strlen             = strlen(log_dir);
    
            // reset all of the log_dir-dependent filepath variables
            g_httpbl_log_filepath                 = apr_psprintf(cmd->pool, "%s%s", log_dir, DEFAULT_HTTPBL_LOG_FILE);
            g_whitelist_testing_filepath          = apr_psprintf(cmd->pool, "%s%s", log_dir, DEFAULT_WHITELIST_TESTING_LOG_FILE);
#ifdef SHOULD_REQUEST_HONEYPOTS
            g_honeypot_testing_filepath           = apr_psprintf(cmd->pool, "%s%s", log_dir, DEFAULT_HONEYPOT_TESTING_LOG_FILE);
#endif
            g_httpbl_testing_filepath             = apr_psprintf(cmd->pool, "%s%s", log_dir, DEFAULT_HTTPBL_TESTING_LOG_FILE);
            g_rbl_toggle_log_filepath             = apr_psprintf(cmd->pool, "%s%s", log_dir, DEFAULT_RBL_TOGGLE_LOG_FILE);
            g_dir_cfg_log_filepath                = apr_psprintf(cmd->pool, "%s%s", log_dir, DEFAULT_DIR_CFG_LOG_FILE);
            g_httbl_merge_trace_log_filepath      = apr_psprintf(cmd->pool, "%s%s", log_dir, DEFAULT_HTTPBL_MERGE_TRACE_LOG_FILE);
            g_httpbl_replace_emails_log_filepath  = apr_psprintf(cmd->pool, "%s%s", log_dir, DEFAULT_REPLACE_EMAILS_LOG_FILE);
        }
        else
            return apr_psprintf(cmd->pool, "Could not find a directory with path \"%s\".  Please ensure this directory exists and permissions allow the user running Apache to view and write to this directory.", value);
    }
    else
        return apr_psprintf(cmd->pool, ".");

    return NULL;
}

/*
 *   Get the diretory-path of the template file that contains the 403 Forbidden Error Page contents (if 403s aren't enough)
 *   @return         a string explaining the problem with parsing and/or processing; NULL if no problem
 */
static const char* directive_set_forbidden_template_uri(cmd_parms* cmd, void* dconfig, const char* uri)
{
    if(uri && !httpbl_string_matches(uri, ""))
        forbidden_template  = apr_psprintf(cmd->pool, "%s", uri);
    return NULL;
}


/*
 *   Set the domain to use as an RBL lookup
 */
static const char* directive_set_email_links_rewrite(cmd_parms* cmd, void* dconfig, const char* rewrite_link)
{
    char*   the_return_string   = NULL;

    if(strcspn(apr_str_tolower(cmd->temp_pool, rewrite_link), VALID_URL_CHARS)>0)
    {
        the_return_string   = apr_psprintf(cmd->temp_pool, "The "DIRECTIVE_TEXT_REWRITE_EMAIL_TEXT" you entered contains invalid URL characters (character %d).  Please use a valid URL or remove this directive to use the default link.", strspn(rewrite_link, VALID_URL_CHARS));
    }
    else
    {
        if(cmd_is_server_context(cmd)) // server-context
        {
            httpbl_dir_cfg* svr             = (httpbl_dir_cfg*)ap_get_module_config(cmd->server->module_config, &httpbl_module);
            svr->email_rewrite_link         = apr_pstrdup(cmd->pool, rewrite_link);
        }
        else // if the directive was called with a directory-context...
        {
            httpbl_dir_cfg* the_config      = (httpbl_dir_cfg*)dconfig;
            the_config->email_rewrite_link  = apr_pstrdup(cmd->pool, rewrite_link);
        }
    }

    return the_return_string;
}


/*
 *   Set the domain to use as an RBL lookup
 */
static const char* directive_set_email_text_rewrite(cmd_parms* cmd, void* dconfig, const char* rewrite_text)
{
    char*   the_return_string   = NULL;

    if(cmd_is_server_context(cmd)) // server-context
    {
        httpbl_dir_cfg* svr             = (httpbl_dir_cfg*)ap_get_module_config(cmd->server->module_config, &httpbl_module);
        svr->email_rewrite_text         = apr_pstrdup(cmd->pool, rewrite_text);
    }
    else // if the directive was called with a directory-context...
    {
        httpbl_dir_cfg* the_config      = (httpbl_dir_cfg*)dconfig;
        the_config->email_rewrite_text  = apr_pstrdup(cmd->pool, rewrite_text);
    }

    return the_return_string;
}

/*
 *   Get a boolean value for whether or not the module should activate 404 caturing
 *   @return         a string explaining the problem with parsing and/or processing; NULL if no problem
 */
static const char* directive_set_404_capture_toggle(cmd_parms* cmd, void* dconfig, const int f)
{
    server_rec* svr             = cmd->server;
    httpbl_dir_cfg* dir_cfg     = (httpbl_dir_cfg*)dconfig;
    httpbl_dir_cfg* svr_cfg     = (httpbl_dir_cfg*)ap_get_module_config(svr->module_config, &httpbl_module);

    int newValue                = (f==0)?0:1; // value MUST be 1 (On) or 0 (Off).  If the directive is called, there can be no UNSET value assigned to this dir_cfg

    ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: \t404 Capture Toggle: %d\n", newValue);

    if(!cmd_is_server_context(cmd))
    {
        dir_cfg->is_404_recording_enabled           = newValue;
        ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: \t404 Capture Toggle (dir structure): %s\n", !is_set_int(dir_cfg->is_404_recording_enabled)?"UNSET":is_enabled_int(dir_cfg->is_404_recording_enabled)?"TRUE":"FALSE");
    }
    else
    {
        svr_cfg->is_404_recording_enabled           = newValue;
        ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: \t404 Capture Toggle (svr structure): %s\n", !is_set_int(svr_cfg->is_404_recording_enabled)?"UNSET":is_enabled_int(svr_cfg->is_404_recording_enabled)?"TRUE":"FALSE");
    }

    // also set the static global (until we replace it)
    g_FOF_enable_404_capture = newValue;
    ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: \t404 Capture Toggle (global static): %s\n", !is_set_int(g_FOF_enable_404_capture)?"UNSET":is_enabled_int(g_FOF_enable_404_capture)?"TRUE":"FALSE");
    return NULL;
}

/*
 *   Get a boolean value for whether or not the module should activate POST caturing
 *   @return         a string explaining the problem with parsing and/or processing; NULL if no problem
 */
static const char* directive_set_POST_capture_toggle(cmd_parms* cmd, void* dconfig, const int f)
{
    server_rec* svr             = cmd->server;
    httpbl_dir_cfg* dir_cfg     = (httpbl_dir_cfg*)dconfig;
    httpbl_dir_cfg* svr_cfg     = (httpbl_dir_cfg*)ap_get_module_config(svr->module_config, &httpbl_module);

    int newValue                = (f==0)?0:1; // value MUST be 1 (On) or 0 (Off).  If the directive is called, there can be no UNSET value assigned to this dir_cfg

    ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: \tPOST Capture Toggle: %d\n", newValue);

    if(!cmd_is_server_context(cmd))
    {
        dir_cfg->is_POST_recording_enabled          = newValue;
        ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: \tPOST Capture Toggle (dir structure): %s\n", !is_set_int(dir_cfg->is_POST_recording_enabled)?"UNSET":is_enabled_int(dir_cfg->is_POST_recording_enabled)?"TRUE":"FALSE");
    }
    else
    {
        svr_cfg->is_POST_recording_enabled          = newValue;
        ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: \tPOST Capture Toggle (svr structure): %s\n", !is_set_int(svr_cfg->is_POST_recording_enabled)?"UNSET":is_enabled_int(svr_cfg->is_POST_recording_enabled)?"TRUE":"FALSE");
    }

    // also set the static global (until we replace it)
    g_FOF_enable_404_capture = newValue;
    ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: \tPOST Capture Toggle (global static): %s\n", !is_set_int(g_FOF_enable_POST_capture)?"UNSET":is_enabled_int(g_FOF_enable_POST_capture)?"TRUE":"FALSE");
    return NULL;
}

/*
 *   ;
 */
static const char* directive_set_default_action(cmd_parms* cmd, void* dconfig, const char* the_action)
{
    char*   the_return_string   = NULL;
    server_rec* svr             = cmd->server;
    httpbl_dir_cfg* the_cfg     = NULL;

    if(!cmd_is_server_context(cmd))
        the_cfg                     = (httpbl_dir_cfg*)dconfig;
    else
        the_cfg                     = (httpbl_dir_cfg*)ap_get_module_config(svr->module_config, &httpbl_module);

    if(httpbl_string_matches(the_action, "allow"))
        the_cfg->default_action = HTTPBL_ACTION_ALLOW;
    else if(httpbl_string_matches(the_action, "deny"))
        the_cfg->default_action = HTTPBL_ACTION_DENY;
#ifdef SHOULD_ALLOW_CHALLENGES
    else if(httpbl_string_matches(the_action, "challenge"))
        the_cfg->default_action = HTTPBL_ACTION_CHALLENGE;
#endif
    else
#ifdef SHOULD_ALLOW_CHALLENGES
        the_return_string   = "An invalid value was encountered for "DIRECTIVE_TEXT_DEFAULT_ACTION".  The only acceptable actions are: \"allow\", \"deny\", and \"challenge\".";
#else
        the_return_string   = "An invalid value was encountered for "DIRECTIVE_TEXT_DEFAULT_ACTION".  The only acceptable actions are: \"allow\" and \"deny\".";
#endif

    return the_return_string;
}

/*
 *   Get the URI of the template file that contains the 403 Forbidden Error Page contents
 *   @return         a string explaining the problem with parsing and/or processing; NULL if no problem
 */
static const char* directive_set_rbl_lookup_toggle(cmd_parms* cmd, void* dconfig, const int f)
{
    ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: \tCommand Path: %s\n", (!(cmd->path))?"NULL":cmd->path);
    server_rec* svr             = cmd->server;
    httpbl_dir_cfg* dir_cfg     = (httpbl_dir_cfg*)dconfig;
    httpbl_dir_cfg* svr_cfg     = (httpbl_dir_cfg*)ap_get_module_config(svr->module_config, &httpbl_module);

    int newValue                = (f==0)?0:1;

    if(!cmd_is_server_context(cmd))
        dir_cfg->is_httpbl_enabled              = newValue;
    else
        svr_cfg->is_httpbl_enabled              = newValue;

    g_enable_rbl_lookup = newValue;

    return NULL;
}

/*
 *   Get the timeout (number of seconds) for the 404 POSTs
 *   @return         a string explaining the problem with parsing and/or processing; NULL if no problem
 */
static const char* directive_set_404_post_timeout(cmd_parms* cmd, void* dconfig, const char* timeout)
{
    if(timeout && !httpbl_string_matches(timeout, ""))
    {
        if(atoi(timeout)>0 && atoi(timeout)<9) // limit the timeout of each 404 POST to no more than 8 seconds
            g_FOF_max_timeout   = atoi(timeout);
        else
            return apr_psprintf(cmd->temp_pool, "Your value for "DIRECTIVE_TEXT_EXTERNAL_POST_TIMEOUT" was invalid.  Please enter a value between %d and %d.", 1, 8);
    }
    return NULL;
}

/*
 *   Get the number of 404 records to queue before sending 404 POSTs (can be overridden by 404 post interval)
 *   @return         a string explaining the problem with parsing and/or processing; NULL if no problem
 */
static const char* directive_set_404_post_record_count(cmd_parms* cmd, void* dconfig, const char* record_count)
{
    if(record_count && !httpbl_string_matches(record_count, ""))
    {
        if(atoi(record_count)>=MIN_404_RECORD_COUNT &&
           atoi(record_count)<=MAX_404_RECORD_COUNT) // limit the min number of 404 records in queue to between 1 and MAX_404_RECORD_COUNT-1 (inclusive)
            g_FOF_min_count   = atoi(record_count);
        else
            return apr_psprintf(cmd->temp_pool, "Your Minimum Record Count is too small.  Please increase the value to something between %d and %d.", MIN_404_RECORD_COUNT, MAX_404_RECORD_COUNT);
    }
    return NULL;
}

/*
 *   Get the max number of 404 POST retries before abandoning this queu of 404 records
 *   @return         a string explaining the problem with parsing and/or processing; NULL if no problem
 */
static const char* directive_set_404_post_max_retries(cmd_parms* cmd, void* dconfig, const char* max_retries)
{
    if(max_retries && !httpbl_string_matches(max_retries, ""))
        if(atoi(max_retries)>=0 && atoi(max_retries)<MAX_404_RETRIES) // limit the max number of retries to no more than MAX_404_RETRIES-1
            g_FOF_max_retries   = atoi(max_retries);
        else
            return apr_psprintf(cmd->temp_pool, "Your "DIRECTIVE_TEXT_404_SUBMISSION_MAX_RETRIES" value was invalid.  Please enter a number between %d and %d.", 0, MAX_404_RETRIES);
    return NULL;
}

/*
 *   Get the URI of the template file that contains the 403 Forbidden Error Page contents
 *   @return         a string explaining the problem with parsing and/or processing; NULL if no problem
 */
static const char* directive_set_404_post_url(cmd_parms* cmd, void* dconfig, const char* uri)
{
    if(uri && !httpbl_string_matches(uri, ""))
        g_FOF_post_url    = apr_psprintf(cmd->pool, "%s", uri);
    return NULL;
}

/*
 *   Get the URI of the template file that contains the 403 Forbidden Error Page contents
 *   @return         a string explaining the problem with parsing and/or processing; NULL if no problem
 */
static const char* directive_set_404_post_interval(cmd_parms* cmd, void* dconfig, const char* interval)
{
    if(interval && !httpbl_string_matches(interval, ""))
        if(atoi(interval)>=MIN_404_INTERVAL)
            g_FOF_min_interval   =  atoi(interval);
        else
            return apr_psprintf(cmd->temp_pool, "Your "DIRECTIVE_TEXT_404_SUBMISSION_INTERVAL" was not a valid value.  Please enter a value of at least %d.", MIN_404_INTERVAL);
    return NULL;
}

/*
 *   Get the URI of the template file that contains the 403 Forbidden Error Page contents
 *   @return         a string explaining the problem with parsing and/or processing; NULL if no problem
 */
static const char* directive_set_404_proxy_address(cmd_parms* cmd, void* dconfig, const char* proxy_address)
{
    ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: \tdirective_set_404_proxy_address function entered\n");
    if(proxy_address && !httpbl_string_matches(proxy_address, ""))
    {
        char*   tmp_ip      = apr_psprintf(cmd->pool, "%s", proxy_address);
        char*   tmp_port    = apr_psprintf(cmd->pool, "%s", strchr(proxy_address, ':'));
        if(tmp_port)
            tmp_port++;

        // only change the tmp_ip and tmp_port if both of the new strings are not NULL and at least 1 char long
        if(tmp_ip && strlen(tmp_ip)>0 &&
             tmp_port && strlen(tmp_port)>0 &&
             atoi(tmp_port)>0 && atoi(tmp_port)<65536 ) // and the port number is a valid 16 bit int
        {
            g_FOF_post_proxy_ip      = apr_psprintf(cmd->pool, "%s", tmp_ip);
            g_FOF_post_proxy_port    = apr_psprintf(cmd->pool, "%s", tmp_port);
        }
    }
    ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: \tdirective_set_404_proxy_address function exited\n");
    return NULL;
}

/*
 *   Get the URI of the template file that contains the 403 Forbidden Error Page contents
 *   @return         a string explaining the problem with parsing and/or processing; NULL if no problem
 */
static const char* directive_set_404_http_auth(cmd_parms* cmd, void* dconfig, const char* http_auth)
{
    if(http_auth && !httpbl_string_matches(http_auth, ""))
    {
        char*   tmp_un      = apr_psprintf(cmd->pool, "%s", http_auth);
        char*   tmp_pw      = apr_psprintf(cmd->pool, "%s", strchr(http_auth, ':'));
        if(tmp_pw)
            tmp_pw++;

        // only change the auth_un and auth_pw if both of the new strings are not NULL and at least 1 char long
        if(tmp_un && strlen(tmp_un)>0 &&
           tmp_pw && strlen(tmp_pw)>0)
        {
            g_FOF_post_httpauth_un   = apr_psprintf(cmd->pool, "%s", tmp_un);
            g_FOF_post_httpauth_pw   = apr_psprintf(cmd->pool, "%s", tmp_pw);
        }
    }
    else
    {
        return "No proper value could be read for directive "DIRECTIVE_TEXT_EXTERNAL_HTTPAUTH_INFO".";
    }

    return NULL;
}

/*
 *   Handle a rbl_directive and insert it into the appropriate config structure (dir or server, depending on the context of the call)
 *   @return         a string explaining the problem with parsing and/or processing; NULL if no problem
 */
static char* insert_rbl_directive_into_dir_cfg(apr_pool_t* svr_p, apr_pool_t* p, httpbl_dir_cfg* the_dir_cfg, const char* the_category_bs, const char* the_score, const char* the_days, const char* the_verb_bs, const char* the_behavior_string)
{
    ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: \tthe bitset:\t%s\n", the_category_bs);
    ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: \tthe verb bitset:\t%s\n", the_verb_bs);
    ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: \tthe days:\t%s\n", the_days);
    ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: \tthe score:\t%s\n", the_score);
    ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: \tthe behavior string:\t%s\n", the_behavior_string);

    if(!the_dir_cfg)
    {
        ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: \texited insert_rbl_directive_into_dir_cfg (4)\n");
        return NULL;
    }
    else if(
            (!the_category_bs || 
               (atoi(the_category_bs)>=0 && atoi(the_category_bs)<=255)) &&
               string_is_valid_int_range(p, the_score, 0, 255) &&
               string_is_valid_int_range(p, the_days, 0, 255) &&
               (!the_verb_bs || 
                 httpbl_string_matches(the_verb_bs, "ALL") || 
                 httpbl_string_matches(the_verb_bs, "All") || 
                 httpbl_string_matches(the_verb_bs, "all") || 
                  (atoi(the_verb_bs)>=0 && atoi(the_verb_bs)<=255))
            )
    {
        if(the_dir_cfg->num_of_rbl_handlers>=MAX_RBL_DIRECTIVES)
        {
            char*   tmpstr  = apr_psprintf(svr_p, "too many RBL handler directives.  There is a limit of %d directives.", MAX_RBL_DIRECTIVES);
            return tmpstr;
        }
        else if(!httpbl_string_matches(the_behavior_string, "allow") &&
                !httpbl_string_matches(the_behavior_string, "allow-xlate-emails") &&
                !httpbl_string_matches(the_behavior_string, "deny") &&
#ifdef SHOULD_ALLOW_CHALLENGES
                !httpbl_string_matches(the_behavior_string, "challenge") &&
#endif

#ifdef SHOULD_REQUEST_HONEYPOTS
                !httpbl_string_matches(the_behavior_string, "honeypot") &&
#endif
                1)
        {
            char*   tmpstr  = apr_psprintf(svr_p, "The behavior string ('%s') did not match any of the available behaviors. Behaviors include: 'allow', 'allow-xlate-emails', "
#ifdef SHOULD_ALLOW_CHALLENGES
                                                  "'challenge', "
#endif
#ifdef SHOULD_REQUEST_HONEYPOTS
                                                  "'honeypot', "
#endif
                                                  "or 'deny'", the_behavior_string);
            return tmpstr;
        }
        else
        {
            // parse ranges and load all values into a new rbl_handler struct
            rbl_handler* the_rbl_handler    = (rbl_handler*)apr_palloc(svr_p, sizeof(rbl_handler));
            parse_int_range(svr_p, the_score, &(the_rbl_handler->score_lb), &(the_rbl_handler->score_ub));
            parse_int_range(svr_p, the_days,  &(the_rbl_handler->days_lb),  &(the_rbl_handler->days_ub));
            the_rbl_handler->category_bs    = atoi(the_category_bs);
            if(!the_verb_bs || httpbl_string_matches(the_verb_bs, "ALL") || httpbl_string_matches(the_verb_bs, "All") || httpbl_string_matches(the_verb_bs, "all"))
                the_rbl_handler->verb_bs        = (unsigned long long)255; // 255 is the "catchall" for now for all HTTP "verbs"
            else
                the_rbl_handler->verb_bs        = (unsigned long long)atoi(the_verb_bs);
            the_rbl_handler->action_string  = apr_pstrdup(svr_p, the_behavior_string);
            the_dir_cfg->the_rbl_handlers[the_dir_cfg->num_of_rbl_handlers] = the_rbl_handler;
            the_dir_cfg->num_of_rbl_handlers++;

            ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: rbl_handler (%d) [%d]: { verb_bs: %"APR_UINT64_T_FMT"; days: %d-%d; score: %d-%d; cbs: %lu; action: %s }\n",
                                          the_dir_cfg->num_of_rbl_handlers,
                                               the_rbl_handler,
                                                       the_rbl_handler->verb_bs,
                                                           the_rbl_handler->days_lb,
                                                              the_rbl_handler->days_ub,
                                                                          the_rbl_handler->score_lb,
                                                                             the_rbl_handler->score_ub,
                                                                                   the_rbl_handler->category_bs,
                                                                                                               the_rbl_handler->action_string!=NULL?the_rbl_handler->action_string:"");
        }

        ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: \texited insert_rbl_directive_into_dir_cfg (1)\n");
        return NULL;
    }

    ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: \texited insert_rbl_directive_into_dir_cfg (0)\n");
    return "invalid directive parameters format.  Parameters should be: (1) [VERB bitset]:[DAYS int range]:[SCORE int range]:[TYPE bitset] (2) (allow|allow-xlate-emails"
#ifdef SHOULD_ALLOW_CHALLENGES
            "|challenge"
#endif
#ifdef SHOULD_REQUEST_HONEYPOTS
            "|honeypot"
#endif
            "|deny)";
}

/*
 *   Get the URI of the template file that contains the 403 Forbidden Error Page contents
 *   @return         a string explaining the problem with parsing and/or processing; NULL if no problem
 */
static const char* directive_set_rbl_req_handling_directive(cmd_parms* cmd, void* dconfig, const char* dilineated_str, const char* behavior_string)
{
    char*       delimeter   = ":";
    char*       token       = (char*)apr_palloc(cmd->temp_pool, 256*sizeof(char));
    char*       running1    = apr_pstrdup(cmd->pool, dilineated_str);
    char*       running2    = NULL;
    int         i           = 0;

    char*       type_bs     = NULL;
    char*       score_range = NULL;
    char*       days_range  = NULL;
    char*       verb_bs     = NULL;

    char*       ds_regex_pattern    = "^([0-9]{1,3}|ALL|All|all):([0-9]{1,3})(-([0-9]{1,3}))?:([0-9]{1,3})(-([0-9]{1,3}))?:([0-9]{1,3})$"; // dilineated_str regex to match (with backreferences)

    /*
    regex test to make sure the dilineated_str is formatted correctly
    */

    apr_pool_t*     p       = cmd->temp_pool;
    apr_size_t      len;
    pcre*           re;
    const char*     error;
    int             erroffset;
    int*            ovector;
    int             rc;

    if(dilineated_str && !httpbl_string_matches(dilineated_str, ""))
    {
        int     num_of_possible_matches = strlen(dilineated_str);
        int     size_of_ovec            = num_of_possible_matches*3*sizeof(int);
        ovector                         = apr_palloc(cmd->temp_pool, size_of_ovec);
        re = pcre_compile(
                            ds_regex_pattern,     // the pattern
                            0,                    // default options
                            &error,               // for error message
                            &erroffset,           // for error offset
                            NULL);                // use default character tables
        if (re == NULL)
        {
            return apr_psprintf(cmd->temp_pool, "mod_httpbl: PCRE compilation failed at offset %d: %s\n", erroffset, error);
        }
        rc = pcre_exec(
                            re,                   // the compiled pattern
                            NULL,                 // no extra data - we didn't study the pattern
                            dilineated_str,       // the subject string
                            (int)strlen(dilineated_str),// the length of the subject
                            0,                    // start at offset 0 in the subject
                            0,                    // default options
                            ovector,              // output vector for substring information
                            size_of_ovec);        // number of elements in the output vector

        if (rc < 0)
        {
            switch(rc)
            {
            case PCRE_ERROR_NOMATCH:
                        return "A "DIRECTIVE_TEXT_RBL_REQ_HANDLER" directive value was not formatted correctly.  Please see the mod_httpbl manual for instructions on how to create and format directives.";
                    break;
                default:
                    return apr_psprintf(cmd->temp_pool, "mod_httpbl: PCRE matching error %d\n", rc);
                    break;
            }
        }
    }
/*
 *   if we get this far, dilineated_str is formatted correctly
 */

    i           = 0;

    token       = strsep(&running1, delimeter);
    verb_bs     = token;
    token       = strsep(&running1, delimeter);
    days_range  = token;
    token       = strsep(&running1, delimeter);
    score_range = token;
    token       = strsep(&running1, delimeter);
    type_bs     = token;

    if(!verb_bs ||
        verb_bs == "ALL" || verb_bs == "All" || verb_bs == "all" ||
        ((apr_uint64_t)atol(verb_bs)<0 || (apr_uint64_t)atol(verb_bs)>255)) // if verb_bs == NULL, set it to a bitset full of '1's
        verb_bs = "ALL";

    if(cmd_is_server_context(cmd)) // directive was called with a global context
    {
        httpbl_dir_cfg* this_dir_cfg    = (httpbl_dir_cfg*)ap_get_module_config(cmd->server->module_config, &httpbl_module);
        return insert_rbl_directive_into_dir_cfg(cmd->pool, cmd->temp_pool, this_dir_cfg, type_bs, score_range, days_range, verb_bs, behavior_string);
    }
    else // directive was called with a directory-context...
    {
        httpbl_dir_cfg* this_dir_cfg    = dconfig;
        return insert_rbl_directive_into_dir_cfg(cmd->pool, cmd->temp_pool, this_dir_cfg, type_bs, score_range, days_range, verb_bs, behavior_string);
    }
    return NULL;
}

/*
 *   Initialize the g_FOF_cache_data_filepath global variable to the #define value (if the #define exists)
 *   @return         void
 */
static void init_404_cache_filename(apr_pool_t *p)
{
    g_FOF_cache_data_filepath   = apr_psprintf(p, DEFAULT_404_CACHE_DATAFILE_BASENAME);
    return;
}

/*
 *   Initialize the g_FOF_cache_data_filepath global variable to the #define value (if the #define exists)
 *   @return         void
 */
static void init_404_meta_filename(apr_pool_t *p)
{
    g_FOF_cache_meta_filepath   = apr_psprintf(p, DEFAULT_404_CACHE_METAFILE_BASENAME);
    return;
}

/*
 *   Initialize the g_FOF_cache_data_filepath global variable to the #define value (if the #define exists)
 *   @return         void
 */
static void init_404_post_url(apr_pool_t *p)
{
    g_FOF_post_url   = apr_pstrdup(p, DEFAULT_404_URL);
    return;
}

/*
 *   Serialize (from RAM to file) the 404 Cache Metadata file
 *   @return         1 if successful; -1 if file could not be opened; -2 if file could not be (written to/closed)
 */
static int serialize_404_cache_meta(apr_pool_t* p)
{
    apr_status_t    rv                  = APR_SUCCESS;
    apr_status_t    file_lock_acquired  = APR_SUCCESS;
    apr_file_t*     the_404_metafile    = NULL;
    char*           tmpstr              = NULL;
    apr_size_t      outlen              = 0;

    ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: serialize_404_cache_meta function entered\n");

    // record (the fact that we posted) to a logfile
    if(g_FOF_cache_data_filepath)
    {
        ap_log_error(APLOG_MARK, APLOG_INFO, 0, NULL, "HTTPBL: opening (and creating) file (%s)\n", g_FOF_cache_meta_filepath);
        rv = apr_file_open(&the_404_metafile, g_FOF_cache_meta_filepath, APR_READ | APR_WRITE | APR_TRUNCATE | APR_CREATE, APR_OS_DEFAULT, p);
        ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: 404 meta file opened [%sSUCCESSFULLY]\n", (rv==APR_SUCCESS)?"":"UN");
    }
    else
    {
        ap_log_error(APLOG_MARK, APLOG_INFO, 0, NULL, "HTTPBL: can't open 404 meta file, filename is NULL\n");
        return 0; // we can't do anything without the correct filename
    }

    if(rv == APR_SUCCESS)
    {
        file_lock_acquired  = apr_file_lock(the_404_metafile, APR_FLOCK_EXCLUSIVE);

        tmpstr  = apr_psprintf(p, "|%"APR_TIME_T_FMT"|%d|%d|", g_FOF_last_post_time, g_FOF_cur_tries, g_FOF_cur_count);
        outlen  = strlen(tmpstr);
        ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: writing data:\n");
        ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: - last post time: %"APR_TIME_T_FMT"\n", g_FOF_last_post_time);
        ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: - current # of tries: %u\n", g_FOF_cur_tries);
        ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: - current count: %u\n", g_FOF_cur_count);

        apr_file_write(the_404_metafile, tmpstr, &outlen);
        ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: wrote data:\n\"%s\", (%d)", tmpstr, outlen);

        if(file_lock_acquired)
            apr_file_unlock(the_404_metafile);

        apr_file_close(the_404_metafile);

        ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: serialize_404_cache_meta function exited (returning 1)\n");
        return 1;
    }
    else
    {
        ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: serialize_404_cache_meta: 404 cache meta file (%s) could not be opened.", g_FOF_cache_meta_filepath);
        return 0;
    }
}

/*
 *   Unserialize (from file to RAM) the 404 Cache Metadata file
 *   @return         1 if successful; -1 if file could not be opened; -2 if file could not be (written to/closed)
 */
static int unserialize_404_cache_meta(apr_pool_t* p)
{
    ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: unserialize_404_cache_meta function entered\n");
    apr_status_t    rv                      = APR_SUCCESS;
    apr_status_t    file_lock_acquired      = APR_SUCCESS;
    apr_file_t*     the_404_metafile        = 0;
    apr_size_t      outlen                  = 0;
    char*           tmpstr                  = NULL;

    ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: g_FOF_cache_meta_filepath = %s\n", g_FOF_cache_meta_filepath);

    // record the fact that we posted to a logfile
    if(g_FOF_cache_meta_filepath)
    {
        ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: Opening g_FOF_cache_meta_filepath\n");
        rv = apr_file_open(&the_404_metafile, g_FOF_cache_meta_filepath, APR_READ | APR_CREATE, APR_OS_DEFAULT, p);
        ap_log_error(APLOG_MARK, APLOG_INFO, 0, NULL, "HTTPBL: Opened g_FOF_cache_meta_filepath [%sSUCCESSFULLY]\n", (rv==APR_SUCCESS)?"":"UN");
    }
    else // if(g_FOF_cache_meta_filepath)
    {
        ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: g_FOF_cache_meta_filepath was NULL... exiting function\n");
        return 0; // we can't do anything without the correct filename
    }
    ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: tried to open file (%s); APR_SUCCESS (%d) == rv (%d) = %s\n", g_FOF_cache_meta_filepath, APR_SUCCESS, rv, (rv==APR_SUCCESS)?"TRUE":"FALSE");

    if(rv == APR_SUCCESS)
    {
        file_lock_acquired  = apr_file_lock(the_404_metafile, APR_FLOCK_SHARED);
        ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: 404 meta file (%s) locked? %s\n", g_FOF_cache_meta_filepath, (file_lock_acquired==APR_SUCCESS)?"TRUE":"FALSE");

        outlen = 4096;
        tmpstr  = apr_pcalloc(p, outlen*sizeof(char));
        // the contents fo the 404 meta file should be very small (way less than 4096 bytes)
        // It's not our fault if someone else edited the file... we only care about the first line
        apr_file_read(the_404_metafile, tmpstr, &outlen);

        char*   file_contents   = apr_ptrim(p, tmpstr);

#if VERBOSITY >= APLOG_DEBUG
        int     k           = 0;
        char*   out_string  = apr_pstrdup(p, "");
        char*   char_buffer = (char*)apr_pcalloc(p, 2*sizeof(char));
        char*   cur_char    = file_contents;
        for(k=0, cur_char=tmpstr; k<outlen && *cur_char!='\0'; k++,cur_char++)
        {
            apr_snprintf(char_buffer, 2, "%s", cur_char);
            out_string      = apr_psprintf(p, "%s%s'%s'", out_string, (k>0)?", ":"", char_buffer);
        }
        cur_char            = WHITESPACE_CHARS;
#endif

        if(file_lock_acquired==APR_SUCCESS)
        {
            file_lock_acquired  = apr_file_unlock(the_404_metafile);
            ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: 404 meta file (%s) unlocked? %s\n", g_FOF_cache_meta_filepath, (file_lock_acquired==APR_SUCCESS)?"TRUE":"FALSE");
        }

        rv  = apr_file_close(the_404_metafile);
        ap_log_error(APLOG_MARK, APLOG_INFO, 0, NULL, "HTTPBL: 404 meta file (%s) closed? %s\n", g_FOF_cache_meta_filepath, (rv==APR_SUCCESS)?"TRUE":"FALSE");

        if(outlen > 0 &&
           file_contents &&
           !httpbl_string_matches(file_contents, ""))
        {
            char* tok_cntx;

            char*   field0                      = apr_pcalloc(p, sizeof(char)*strlen(file_contents));
            char*   field1                      = apr_pcalloc(p, sizeof(char)*strlen(file_contents));
            char*   field2                      = apr_pcalloc(p, sizeof(char)*strlen(file_contents));
            char*   field3                      = apr_pcalloc(p, sizeof(char)*strlen(file_contents));
            char*   field4                      = apr_pcalloc(p, sizeof(char)*strlen(file_contents));
/*
            field1                              = apr_strtok(file_contents, " ", &tok_cntx);
            field2                              = apr_strtok(NULL, " ", &tok_cntx);
            field3                              = apr_strtok(NULL, " ", &tok_cntx);
*/
            //long    field1                      = atol(apr_strtok(file_contents, " ", &tok_cntx));
            apr_time_t  last_post_time          = apr_time_now();
            int     current_num_of_tries        = UNSET_INT;
            int     current_count               = UNSET_INT;

            char*   delims      = "|";
            field0              = strsep(&file_contents, delims);
            field1              = strsep(&file_contents, delims);
            field2              = strsep(&file_contents, delims);
            field3              = strsep(&file_contents, delims);
            field4              = strsep(&file_contents, delims);

//            sscanf(apr_ptrim(p, file_contents), "|%s|%s|%s|", field1, field2, field3);


            sscanf(apr_ptrim(p, field1), "%"APR_TIME_T_FMT, &last_post_time);
            sscanf(apr_ptrim(p, field2), "%u", &current_num_of_tries);
            sscanf(apr_ptrim(p, field3), "%u", &current_count);

            apr_time_t  time_now                = apr_time_now();
            char*   rfc822_timestamp            = apr_palloc(p, APR_RFC822_DATE_LEN*sizeof(char));
            char*   rfc822_timenow              = apr_palloc(p, APR_RFC822_DATE_LEN*sizeof(char));


#if VERBOSITY >= APLOG_DEBUG
            apr_rfc822_date(rfc822_timestamp, last_post_time);
            apr_rfc822_date(rfc822_timenow, time_now);
            ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: \t.*** reading from 404 meta file: *** \n");
            ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: \tParsed Field1: \"%s\"; integer value: %"APR_TIME_T_FMT"\n", field1, last_post_time);
            ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: \tParsed Field2: \"%s\"; integer value: %u\n", field2, current_num_of_tries);
            ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: \tParsed Field3: \"%s\"; integer value: %u\n", field3, current_count);
            ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: \t.*** done reading from 404 meta file. *** \n");
#endif
    
            if(last_post_time       > apr_time_make(0, 0) &&
               last_post_time       < time_now &&
               current_num_of_tries >= 0 && 
               current_count        >= 0)
            {
                ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: valid values, updating process memory...\n");
                g_FOF_last_post_time      = last_post_time;
                g_FOF_cur_tries           = current_num_of_tries;
                g_FOF_cur_count           = current_count;
            }
            else
            {
                ap_log_error(APLOG_MARK, APLOG_INFO, 0, NULL, "HTTPBL: invalid value found in 404 meta data... not syncing from file.\n");
            }
        } // non-null and non-empty data read in

        ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: unserialize_404_cache_meta function exited (returning 1)\n");
        return 1;
    }
    else // if (rv == APR_SUCCESS)
    {
        ap_log_error(APLOG_MARK, APLOG_INFO, 0, NULL, "HTTPBL: unserialize_404_cache_meta: 404 cache meta file (%s) could not be opened.", g_FOF_cache_meta_filepath);
        ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: unserialize_404_cache_meta function exited (returning %d)\n", 0);
        return 0;
    }
}

/*
 *   Simple comparison function to test if enough 404 records are cached to fire a 404 POST
 *   @return         TRUE iff the current number of 404 records >= the minimum number of 404 records required to make a POST (defined by HTTPBL404POSTWhenRecordsReaches)
 */
static int areEnough404sToPost()
{
    return(g_FOF_min_count <= g_FOF_cur_count);
}

/*
 *   Simple comparison function to test if enough time has passed since the last post (or since server startup) to allow a new 404 POST
 *   @return TRUE iff the time since last POST (or server startup) is at least the threshold set by HTTPBL404POSTInterval
 */
static int enoughTimeHasPassedToPost(apr_pool_t* p)
{
    apr_time_t  timestamp_now       = apr_time_now();
    return(g_FOF_min_interval <= (apr_time_sec(timestamp_now) - apr_time_sec(g_FOF_last_post_time)));
}

/*
 *   Simple comparison function to test if the 404 cache file has exceeded the reccomended max size threshold for a 404 POST
 *   @return         TRUE iff the filesize of the 404 cache file is larger than *** (future directive)
 */
static int cacheFileIsSmallEnoughToPost(apr_pool_t* p)
{
    apr_status_t    rv      = 1;
    apr_finfo_t     finfo;
    apr_off_t       file_size;

#ifndef SHOULD_CACHE
    return (g_FOF_cache_string_l <(50*1024));
#else
    rv                  = apr_stat(&finfo, g_FOF_cache_meta_filepath, APR_FINFO_NORM, p);
    file_size           = finfo.size;
    return(file_size <(50*1024)); // 50kB filesize max
#endif
}

/*
 *   Simple comparison function to test if the max number of 404 POST retries has been crossed
 *   @return         TRUE iff the current number of 404 retries is greater than HTTPBL404POSTMaxRetries
 */
static int enoughPostTriesHaveElapsed()
{
    return(g_FOF_cur_tries > g_FOF_max_retries);
}


/*
 *
 *    
 *
 */
static long get_response_code_from_repsonse_data(apr_pool_t* pool, char* response_data)
{
    long    respcode    = 0;
    char*   part        = NULL;

    if(!response_data)
        return respcode;

    part = strstr(response_data, "HTTP/1."); // really HTTP/1.x_
    if(part && strlen(part) > strlen("HTTP/1.x_"))
        respcode    = atol(apr_pstrndup(pool, (part + strlen("HTTP/1.x_")), 3));

    return respcode;
}


/*
 *   Set up Server config structure with default values
 *   @return         a new server configuration structure (of type httpbl_dir_cfg*)
 */
static void* httpbl_create_svr_conf(apr_pool_t* pool, server_rec* svr)
{
    int   i                                     = 0;
    httpbl_dir_cfg* svr_cfg                     = (httpbl_dir_cfg*) apr_pcalloc(pool, sizeof(httpbl_dir_cfg));

    // Set up the default values for fields of svr
    svr_cfg->is_httpbl_enabled                  = UNSET_INT;
    svr_cfg->is_404_recording_enabled           = UNSET_INT;
    svr_cfg->is_POST_recording_enabled          = UNSET_INT;
    svr_cfg->default_action                     = DEFAULT_ACTION_UNSET;
    svr_cfg->is_exempt                          = UNSET_INT;
    svr_cfg->the_rbl_handlers                   = (rbl_handler**)apr_pcalloc(pool, (MAX_RBL_DIRECTIVES)*sizeof(rbl_handler*));
    svr_cfg->num_of_rbl_handlers                = 0;
    svr_cfg->token_str                          = NULL;
    svr_cfg->access_key                         = NULL;
#ifdef SHOULD_ALLOW_CHALLENGES
    svr_cfg->challenge_url                      = apr_pstrdup(pool, DEFAULT_CHALLENGE_URL);
#endif
    svr_cfg->dns_lookup_domain                  = NULL; //apr_pstrdup(pool, DEFAULT_RBL_SERVER_DOMAIN);
    svr_cfg->email_rewrite_link                 = NULL;
    svr_cfg->email_rewrite_text                 = NULL;
    svr_cfg->ers                                = ERS_NONE;
    svr_cfg->honeypot_url                       = NULL;
    svr_cfg->dirpath                            = (svr->server_hostname)?apr_pstrdup(pool, svr->server_hostname):apr_pstrdup(pool, "<SERVER WITH NO HOSTNAME>");

    return svr_cfg;
}

/*
 *   Set up Directory config structure with default values
 *   @return         a new server configuration structure (of type httpbl_dir_cfg*)
 */
static void* httpbl_create_dir_conf(apr_pool_t* pool, char* x_dir_path)
{
    int   i                                     = 0;
    httpbl_dir_cfg* dir_cfg                     = apr_palloc(pool, sizeof(httpbl_dir_cfg));
    
    // Set up the default values for fields of dir
    dir_cfg->is_httpbl_enabled                  = UNSET_INT;
    dir_cfg->is_404_recording_enabled           = UNSET_INT;
    dir_cfg->is_POST_recording_enabled          = UNSET_INT;
    dir_cfg->default_action                     = DEFAULT_ACTION_UNSET;
    dir_cfg->is_exempt                          = 0;
    dir_cfg->the_rbl_handlers                   = (rbl_handler **)apr_pcalloc(pool, (MAX_RBL_DIRECTIVES)*sizeof(rbl_handler*));
    dir_cfg->num_of_rbl_handlers                = 0;
    dir_cfg->token_str                          = NULL;
    dir_cfg->access_key                         = NULL;
#ifdef SHOULD_ALLOW_CHALLENGES
    dir_cfg->challenge_url                      = apr_pstrdup(pool, DEFAULT_CHALLENGE_URL);
#endif
    dir_cfg->dns_lookup_domain                  = NULL; //apr_pstrdup(pool, DEFAULT_RBL_SERVER_DOMAIN);
    dir_cfg->email_rewrite_link                 = NULL;
    dir_cfg->email_rewrite_text                 = NULL;
    dir_cfg->ers                                = ERS_NONE;
    dir_cfg->honeypot_url                       = NULL;
    dir_cfg->dirpath                            = (x_dir_path)?apr_pstrdup(pool, x_dir_path):apr_pstrdup(pool, "<DIR WITH NO PATHNAME>");

    dump_dir_cfg_to_file(pool, dir_cfg);

    return dir_cfg;
}

/*
 *   Merge a directory config structure with it's parent directory config structure(s)
 *   @return         a new server configuration structure (of type httpbl_dir_cfg*) inheriting properties from it's merged structures
 */
static void* httpbl_merge_dir_conf(apr_pool_t* pool, void* parent_dirv, void* subdirv)
{
    httpbl_dir_cfg* parent_dir              = (httpbl_dir_cfg*)parent_dirv;
    httpbl_dir_cfg* subdir                  = (httpbl_dir_cfg*)subdirv;
    httpbl_dir_cfg* new_dir_cfg             = (httpbl_dir_cfg*)apr_pcalloc(pool, sizeof(httpbl_dir_cfg));
    rbl_handler*    new_rbh                 = NULL;
    int             i                       = 0; // i = the num_of_rbl_handlers already written in the new_dir_cfg array
    int             j                       = 0; // j = the num_of_rbl_handlers read from from_rbl_handler
    apr_status_t    rv                      = 1;
    apr_status_t    file_lock_acquired      = 1;
    apr_file_t*     fp                      = NULL;
    char*           tmpstr                  = NULL;
    apr_size_t      outlen                  = 0;
    int             merge_logfile_opened    = 0;

    new_dir_cfg->is_httpbl_enabled          = (is_set_action(subdir->is_httpbl_enabled))?       subdir->is_httpbl_enabled:                  parent_dir->is_httpbl_enabled;
    new_dir_cfg->is_exempt                  = (is_set_int(subdir->is_exempt))?                  subdir->is_exempt:                          parent_dir->is_exempt;
    new_dir_cfg->is_404_recording_enabled   = (is_set_int(subdir->is_404_recording_enabled))?   subdir->is_404_recording_enabled:           parent_dir->is_404_recording_enabled;
    new_dir_cfg->is_POST_recording_enabled  = (is_set_int(subdir->is_POST_recording_enabled))?  subdir->is_POST_recording_enabled:          parent_dir->is_POST_recording_enabled;
    new_dir_cfg->default_action             = (is_set_action(subdir->default_action))?          subdir->default_action:                     parent_dir->default_action;
    new_dir_cfg->dns_lookup_domain          = (subdir->dns_lookup_domain)?                      subdir->dns_lookup_domain:                  parent_dir->dns_lookup_domain;
    new_dir_cfg->is_httpbl_enabled          = (is_set_int(subdir->is_httpbl_enabled))?          subdir->is_httpbl_enabled:                  parent_dir->is_httpbl_enabled;
    new_dir_cfg->token_str                  = (subdir->token_str)?                              apr_pstrdup(pool, subdir->token_str):       (parent_dir->token_str)?apr_pstrdup(pool, parent_dir->token_str):NULL;
#ifdef SHOULD_REQUEST_HONEYPOTS
    new_dir_cfg->honeypot_url               = (subdir->honeypot_url)?                           apr_pstrdup(pool, subdir->honeypot_url):    (parent_dir->honeypot_url)?apr_pstrdup(pool, parent_dir->honeypot_url):NULL;
#endif
#ifdef SHOULD_ALLOW_CHALLENGES
    new_dir_cfg->challenge_url              = (subdir->challenge_url)?                          apr_pstrdup(pool, subdir->challenge_url):   (parent_dir->challenge_url)?apr_pstrdup(pool, parent_dir->challenge_url):NULL;
#endif
    new_dir_cfg->access_key                 = (subdir->access_key)?                             apr_pstrdup(pool, subdir->access_key):      (parent_dir->access_key)?apr_pstrdup(pool, parent_dir->access_key):NULL;
    new_dir_cfg->email_rewrite_text         = (subdir->email_rewrite_text)?                     apr_pstrdup(pool, subdir->email_rewrite_text):(parent_dir->token_str)?apr_pstrdup(pool, parent_dir->email_rewrite_text):NULL;
    new_dir_cfg->email_rewrite_link         = (subdir->email_rewrite_link)?                     apr_pstrdup(pool, subdir->email_rewrite_link):(parent_dir->email_rewrite_link)?apr_pstrdup(pool, parent_dir->email_rewrite_link):NULL;
    new_dir_cfg->the_rbl_handlers           = (rbl_handler**)apr_pcalloc(pool, (MAX_RBL_DIRECTIVES)*sizeof(rbl_handler*));
    new_dir_cfg->num_of_rbl_handlers        = 0;
    new_dir_cfg->dirpath                    = NULL;

#if VERBOSITY >= APLOG_DEBUG
    ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: httpbl_merge_dir_conf(0.0.-.0.0): logfile name: (%s)", g_httbl_merge_trace_log_filepath);

    // record the fact that we posted to a logfile
    if(g_httbl_merge_trace_log_filepath)
    {
        rv = apr_file_open(&fp, g_httbl_merge_trace_log_filepath, APR_APPEND | APR_WRITE | APR_CREATE, APR_OS_DEFAULT, pool);
        ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: httpbl_merge_dir_conf(0.0.3.0.0): file (%s) opened [%sSUCCESSFULLY]", g_httbl_merge_trace_log_filepath, (rv == APR_SUCCESS)?"":"UN");

        if(rv == APR_SUCCESS)
        {
            merge_logfile_opened = 1;
            file_lock_acquired  = apr_file_lock(fp, APR_FLOCK_EXCLUSIVE);
            ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: httpbl_merge_dir_conf(0.0.3.0.1): file (%s) locked [%sSUCCESSFULLY]", g_httbl_merge_trace_log_filepath, (file_lock_acquired == APR_SUCCESS)?"":"UN");

            char* is_httpbl_enabled_str = NULL;
            char* is_404_enabled_str    = NULL;
            char* tmpstr2               = NULL;
        
            tmpstr  = apr_psprintf(pool, "-----------------------------\n\nDIR CONFIG STRUCT:\n");
            outlen = strlen(tmpstr);
            apr_file_write(fp, tmpstr, &outlen);
        
            if (parent_dir)
            {
                tmpstr  = apr_psprintf(pool, "MERGE\tstarted RBL Handler Merge Loops\n");
                outlen = strlen(tmpstr);
                apr_file_write(fp, tmpstr, &outlen);
        
                tmpstr  = apr_psprintf(pool, "MERGE\tparent [%s]\n\t{\n", (parent_dir->dirpath)?(parent_dir->dirpath):"NULL");
                outlen = strlen(tmpstr);
                apr_file_write(fp, tmpstr, &outlen);
        
                tmpstr  = apr_psprintf(pool, "\tMERGE\t\tdefault_action           = %s\n", get_action_printable_string(parent_dir->default_action));
                outlen = strlen(tmpstr);
                apr_file_write(fp, tmpstr, &outlen);
        
                tmpstr  = apr_psprintf(pool, "\tMERGE\t\tis_404_recording_enabled = %s\n", !is_set_int(parent_dir->is_404_recording_enabled)?"UNSET":is_enabled_int(parent_dir->is_404_recording_enabled)?"TRUE":"FALSE");
                outlen = strlen(tmpstr);
                apr_file_write(fp, tmpstr, &outlen);
        
                tmpstr  = apr_psprintf(pool, "\tMERGE\t\tis_POST_recording_enabled= %s\n", !is_set_int(parent_dir->is_POST_recording_enabled)?"UNSET":is_enabled_int(parent_dir->is_POST_recording_enabled)?"TRUE":"FALSE");
                outlen = strlen(tmpstr);
                apr_file_write(fp, tmpstr, &outlen);
        
                tmpstr  = apr_psprintf(pool, "\tMERGE\t\tis_exempt                = %s\n", !is_set_int(parent_dir->is_exempt)?"UNSET":is_enabled_int(parent_dir->is_exempt)?"TRUE":"FALSE");
                outlen = strlen(tmpstr);
                apr_file_write(fp, tmpstr, &outlen);
        
                tmpstr  = apr_psprintf(pool, "\tMERGE\t\tis_httpbl_enabled        = %s\n", !is_set_int(parent_dir->is_httpbl_enabled)?"UNSET":is_enabled_int(parent_dir->is_httpbl_enabled)?"TRUE":"FALSE");
                outlen = strlen(tmpstr);
                apr_file_write(fp, tmpstr, &outlen);
        
                tmpstr  = apr_psprintf(pool, "\tMERGE\t\ttoken_str                = %s\n", (parent_dir->token_str)?(parent_dir->token_str):"NULL");
                outlen = strlen(tmpstr);
                apr_file_write(fp, tmpstr, &outlen);
        
                tmpstr  = apr_psprintf(pool, "\tMERGE\t\taccess_key               = %s\n", (parent_dir->access_key)?(parent_dir->access_key):"NULL");
                outlen = strlen(tmpstr);
                apr_file_write(fp, tmpstr, &outlen);
        
                tmpstr  = apr_psprintf(pool, "\tMERGE\t\tthe_rbl_handlers         = %s\n", (parent_dir->the_rbl_handlers)?"NOT_NULL":"NULL");
                outlen = strlen(tmpstr);
                apr_file_write(fp, tmpstr, &outlen);
        
                tmpstr  = apr_psprintf(pool, "\tMERGE\t\tnum_of_rbl_handlers      = %d\n", parent_dir->num_of_rbl_handlers);
                outlen = strlen(tmpstr);
                apr_file_write(fp, tmpstr, &outlen);
        
                tmpstr  = apr_psprintf(pool, "\t}\n");
                outlen = strlen(tmpstr);
                apr_file_write(fp, tmpstr, &outlen);
        
                tmpstr  = apr_psprintf(pool, "MERGE\tsubdir [%s]\n\t{\n", (subdir->dirpath)?(subdir->dirpath):"NULL");
                outlen = strlen(tmpstr);
                apr_file_write(fp, tmpstr, &outlen);
        
                tmpstr  = apr_psprintf(pool, "\tMERGE\t\tdefault_action           = %s\n", get_action_printable_string(subdir->default_action));
                outlen = strlen(tmpstr);
                apr_file_write(fp, tmpstr, &outlen);
        
                tmpstr  = apr_psprintf(pool, "\tMERGE\t\tis_404_recording_enabled = %s\n", !is_set_int(subdir->is_404_recording_enabled)?"UNSET":is_enabled_int(subdir->is_404_recording_enabled)?"TRUE":"FALSE");
                outlen = strlen(tmpstr);
                apr_file_write(fp, tmpstr, &outlen);
        
                tmpstr  = apr_psprintf(pool, "\tMERGE\t\tis_POST_recording_enabled= %s\n", !is_set_int(subdir->is_POST_recording_enabled)?"UNSET":is_enabled_int(subdir->is_POST_recording_enabled)?"TRUE":"FALSE");
                outlen = strlen(tmpstr);
                apr_file_write(fp, tmpstr, &outlen);
        
                tmpstr  = apr_psprintf(pool, "\tMERGE\t\tis_exempt                = %s\n", !is_set_int(subdir->is_exempt)?"UNSET":is_enabled_int(subdir->is_exempt)?"TRUE":"FALSE");
                outlen = strlen(tmpstr);
                apr_file_write(fp, tmpstr, &outlen);
        
                tmpstr  = apr_psprintf(pool, "\tMERGE\t\tis_httpbl_enabled        = %s\n", !is_set_int(subdir->is_httpbl_enabled)?"UNSET":is_enabled_int(subdir->is_httpbl_enabled)?"TRUE":"FALSE");
                outlen = strlen(tmpstr);
                apr_file_write(fp, tmpstr, &outlen);
        
                tmpstr  = apr_psprintf(pool, "\tMERGE\t\ttoken_str                = %s\n", (subdir->token_str)?(subdir->token_str):"NULL");
                outlen = strlen(tmpstr);
                apr_file_write(fp, tmpstr, &outlen);
        
                tmpstr  = apr_psprintf(pool, "\tMERGE\t\taccess_key               = %s\n", (subdir->access_key)?(subdir->access_key):"NULL");
                outlen = strlen(tmpstr);
                apr_file_write(fp, tmpstr, &outlen);
        
                tmpstr  = apr_psprintf(pool, "\tMERGE\t\tthe_rbl_handlers         = %s\n", (subdir->the_rbl_handlers)?"NOT_NULL":"NULL");
                outlen = strlen(tmpstr);
                apr_file_write(fp, tmpstr, &outlen);
        
                tmpstr  = apr_psprintf(pool, "\tMERGE\t\tnum_of_rbl_handlers      = %d\n", subdir->num_of_rbl_handlers);
                outlen = strlen(tmpstr);
                apr_file_write(fp, tmpstr, &outlen);
        
                tmpstr  = apr_psprintf(pool, "\t}\n");
                outlen = strlen(tmpstr);
                apr_file_write(fp, tmpstr, &outlen);
        
                tmpstr  = apr_psprintf(pool, "MERGE\tBEFORE MERGE\n");
                outlen = strlen(tmpstr);
                apr_file_write(fp, tmpstr, &outlen);
        
                tmpstr  = apr_psprintf(pool, "MERGE\tparent num_of_rbl_handlers   = %d\n", parent_dir->num_of_rbl_handlers);
                outlen = strlen(tmpstr);
                apr_file_write(fp, tmpstr, &outlen);
        
                tmpstr  = apr_psprintf(pool, "MERGE\tsubdir num_of_rbl_handlers   = %d\n", subdir->num_of_rbl_handlers);
                outlen = strlen(tmpstr);
                apr_file_write(fp, tmpstr, &outlen);
        
                tmpstr  = apr_psprintf(pool, "MERGE\tnewnum_of_rbl_handlers       = %d\n", new_dir_cfg->num_of_rbl_handlers);
                outlen = strlen(tmpstr);
                apr_file_write(fp, tmpstr, &outlen);
        
            }
        }
    }
#endif

    rbl_handler*    from_rbl_handler;
    rbl_handler*    to_rbl_handler;
    // fill in rbl directives starting with the parent, then the child (returning an error string if more than MAX_RBL_DIRECTIVES rbl_handlers are created/attempted
    for(i=0; i<subdir->num_of_rbl_handlers && new_dir_cfg->num_of_rbl_handlers<=MAX_RBL_DIRECTIVES; i++)
    {
        from_rbl_handler    = subdir->the_rbl_handlers[i];

#if VERBOSITY >= APLOG_DEBUG
        if(merge_logfile_opened)
        {
            tmpstr  = apr_psprintf(pool, "MERGE\t- i = %d; i <= num_of_rbl_handlers (%s) && num_of_rbl_handlers < MAX_RBL_DIRECTIVES (%s); i++ (%d)\n", i, (i < parent_dir->num_of_rbl_handlers+1)?"TRUE":"FALSE", (new_dir_cfg->num_of_rbl_handlers < MAX_RBL_DIRECTIVES)?"TRUE":"FALSE", i+1);
            outlen = strlen(tmpstr);
            apr_file_write(fp, tmpstr, &outlen);
    
            tmpstr  = apr_psprintf(pool, "MERGE\t- sizeof(rbl_handler)        = %d\n", sizeof(rbl_handler));
            outlen = strlen(tmpstr);
            apr_file_write(fp, tmpstr, &outlen);
    
            tmpstr  = apr_psprintf(pool, "MERGE\t- new_dir_cfg->num_of_rbl_handlers = %d\n", new_dir_cfg->num_of_rbl_handlers);
            outlen = strlen(tmpstr);
            apr_file_write(fp, tmpstr, &outlen);
        }
#endif

        if(from_rbl_handler)
        {

#if VERBOSITY >= APLOG_DEBUG
            if(merge_logfile_opened)
            {
                tmpstr  = apr_psprintf(pool, "MERGE\t- copy from RBL Handler [%d]\t{ verb: %"APR_UINT64_T_FMT", days: %d-%d, score: %d-%d, cat: %d, action: \"%s\" }\n", i, from_rbl_handler->verb_bs, from_rbl_handler->days_lb, from_rbl_handler->days_ub, from_rbl_handler->score_lb, from_rbl_handler->score_ub, from_rbl_handler->category_bs, (from_rbl_handler->action_string)?from_rbl_handler->action_string:"NULL");
                outlen = strlen(tmpstr);
                apr_file_write(fp, tmpstr, &outlen);
            }
#endif

            new_dir_cfg->the_rbl_handlers[i]    = apr_palloc(pool, sizeof(rbl_handler));
            to_rbl_handler                      = new_dir_cfg->the_rbl_handlers[i];
            memcpy(to_rbl_handler, from_rbl_handler, sizeof(rbl_handler)); // try a direct memory copy
            (new_dir_cfg->num_of_rbl_handlers)++;

#if VERBOSITY >= APLOG_DEBUG
            if(merge_logfile_opened)
            {
                tmpstr  = apr_psprintf(pool, "MERGE\t- new RBL Handler [%d]\t\t{ verb: %"APR_UINT64_T_FMT", days: %d-%d, score: %d-%d, cat: %d, action: \"%s\" }\n", i, to_rbl_handler->verb_bs, to_rbl_handler->days_lb, to_rbl_handler->days_ub, to_rbl_handler->score_lb, to_rbl_handler->score_ub, to_rbl_handler->category_bs, (to_rbl_handler->action_string)?to_rbl_handler->action_string:"NULL");
                outlen = strlen(tmpstr);
                apr_file_write(fp, tmpstr, &outlen);
        
                tmpstr  = apr_psprintf(pool, "MERGE\t}\n\n");
                outlen = strlen(tmpstr);
                apr_file_write(fp, tmpstr, &outlen);
            }
#endif
        }
    }

    for(j=0; j<parent_dir->num_of_rbl_handlers && new_dir_cfg->num_of_rbl_handlers<MAX_RBL_DIRECTIVES; j++)
    {
        from_rbl_handler    = parent_dir->the_rbl_handlers[j];

        if(from_rbl_handler)
        {

#if VERBOSITY >= APLOG_DEBUG
            if(merge_logfile_opened)
            {
                tmpstr  = apr_psprintf(pool, "MERGE\t- copy from RBL Handler [%d]\t{ verb: %"APR_UINT64_T_FMT", days: %d-%d, score: %d-%d, cat: %d, action: \"%s\" }\n", j, from_rbl_handler->verb_bs, from_rbl_handler->days_lb, from_rbl_handler->days_ub, from_rbl_handler->score_lb, from_rbl_handler->score_ub, from_rbl_handler->category_bs, (from_rbl_handler->action_string)?from_rbl_handler->action_string:"NULL");
                outlen = strlen(tmpstr);
                apr_file_write(fp, tmpstr, &outlen);
            }
#endif

            new_dir_cfg->the_rbl_handlers[i]    = apr_palloc(pool, sizeof(rbl_handler));
            to_rbl_handler                      = new_dir_cfg->the_rbl_handlers[i];
            memcpy(to_rbl_handler, from_rbl_handler, sizeof(rbl_handler)); // try a direct memory copy
            (new_dir_cfg->num_of_rbl_handlers)++;

#if VERBOSITY >= APLOG_DEBUG
            if(merge_logfile_opened)
            {
                tmpstr  = apr_psprintf(pool, "MERGE\t- to RBL Handler [%d]\t\t{ verb: %"APR_UINT64_T_FMT", days: %d-%d, score: %d-%d, cat: %d, action: \"%s\" }\n", i, to_rbl_handler->verb_bs, to_rbl_handler->days_lb, to_rbl_handler->days_ub, to_rbl_handler->score_lb, to_rbl_handler->score_ub, to_rbl_handler->category_bs, (to_rbl_handler->action_string)?to_rbl_handler->action_string:"NULL");
                outlen = strlen(tmpstr);
                apr_file_write(fp, tmpstr, &outlen);
            }
#endif

            i++;
        }
    }

    // done with copies; review the new RBL Handler array
    int k = 0;
#if VERBOSITY >= APLOG_DEBUG
    if(merge_logfile_opened)
    {
        tmpstr  = apr_psprintf(pool, "MERGE\tAFTER MERGE\n");
        outlen = strlen(tmpstr);
        apr_file_write(fp, tmpstr, &outlen);

        tmpstr  = apr_psprintf(pool, "MERGE\tparent num_of_rbl_handlers   = %d\n", parent_dir->num_of_rbl_handlers);
        outlen = strlen(tmpstr);
        apr_file_write(fp, tmpstr, &outlen);

        tmpstr  = apr_psprintf(pool, "MERGE\tsubdir num_of_rbl_handlers   = %d\n", subdir->num_of_rbl_handlers);
        outlen = strlen(tmpstr);
        apr_file_write(fp, tmpstr, &outlen);

        tmpstr  = apr_psprintf(pool, "MERGE\tnewnum_of_rbl_handlers       = %d\n", new_dir_cfg->num_of_rbl_handlers);
        outlen = strlen(tmpstr);
        apr_file_write(fp, tmpstr, &outlen);

        tmpstr  = apr_psprintf(pool, "MERGE\t- Recap of new RBL Req Handlers:\n");
        outlen = strlen(tmpstr);
        apr_file_write(fp, tmpstr, &outlen);
    }
#endif

    for(k=0; k<new_dir_cfg->num_of_rbl_handlers; k++)
    {
        to_rbl_handler      = new_dir_cfg->the_rbl_handlers[k];

        if(to_rbl_handler)
        {
#if VERBOSITY >= APLOG_DEBUG
            if(merge_logfile_opened)
            {
                tmpstr  = apr_psprintf(pool, "MERGE\t- new RBL Handler [%d]\t\t{ verb: %"APR_UINT64_T_FMT", days: %d-%d, score: %d-%d, cat: %d, action: \"%s\" }\n", k, to_rbl_handler->verb_bs, to_rbl_handler->days_lb, to_rbl_handler->days_ub, to_rbl_handler->score_lb, to_rbl_handler->score_ub, to_rbl_handler->category_bs, (to_rbl_handler->action_string)?to_rbl_handler->action_string:"NULL");
                outlen = strlen(tmpstr);
                apr_file_write(fp, tmpstr, &outlen);
            }
#endif
        }
        else
        {
#if VERBOSITY >= APLOG_DEBUG
            if(merge_logfile_opened)
            {
                tmpstr  = apr_psprintf(pool, "MERGE\t- NULL RBL Handler\n");
                outlen = strlen(tmpstr);
                apr_file_write(fp, tmpstr, &outlen);
            }
#endif
        }
    }

#if VERBOSITY >= APLOG_DEBUG
    if(merge_logfile_opened)
    {
        tmpstr  = apr_psprintf(pool, "MERGE\tfinished RBL Handler Merge Loops\n");
        outlen = strlen(tmpstr);
        apr_file_write(fp, tmpstr, &outlen);

        tmpstr  = apr_psprintf(pool, "-----------------------------\n\n");
        outlen  = strlen(tmpstr);
        apr_file_write(fp, tmpstr, &outlen);

        if(file_lock_acquired)
        {
            apr_status_t    merge_file_unlock_status    = apr_file_unlock(fp);
            ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: httpbl_merge_dir_conf(0.0.3.0.2): file (%s) unlocked [%sSUCCESSFULLY]", g_httbl_merge_trace_log_filepath, (merge_file_unlock_status == APR_SUCCESS)?"":"UN");
        }

        apr_status_t    merge_file_close_status         = apr_file_close(fp);
        ap_log_error(APLOG_MARK, APLOG_INFO, 0, NULL, "HTTPBL: httpbl_merge_dir_conf(0.0.3.0.3): file (%s) closed [%sSUCCESSFULLY]", g_httbl_merge_trace_log_filepath, (merge_file_close_status == APR_SUCCESS)?"":"UN");
    }
#endif

    dump_dir_cfg_to_file(pool, new_dir_cfg);

    return (void*)new_dir_cfg;
}
















/*
 *
 */
static int make_simple_http_request(apr_pool_t* hpot_pool, struct ext_connection_params* conx_params, apr_file_t* hpot_logfile)
{
    char*           tmpstr                  = NULL;
    apr_size_t      outlen                  = 0;
    char*           remote_host             = NULL;
    apr_port_t      remote_port             = 80;
    char*           remote_url_path         = NULL;
    char*           request                 = NULL;
    struct ab_connection*   
                    hpot_conn               = NULL;
    apr_pool_t*     mp                      = NULL;
    apr_socket_t*   s                       = NULL;
    apr_socket_t**  sock                    = &s;
    apr_sockaddr_t* sa                      = NULL;
    apr_status_t    rv                      = 1;

    hpot_conn                               = (struct ab_connection*)apr_pcalloc(hpot_pool, 1*sizeof(struct ab_connection));

    ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: make_simple_http_request: attempting to call parse_url_into_conx(...).");

    // set up the derivative-value fields of conx_params
    if (parse_url_into_conx(hpot_pool, conx_params, hpot_logfile) != APR_SUCCESS)
    {
#if VERBOSITY >= APLOG_DEBUG
        if(hpot_logfile)
        {
            tmpstr  = apr_psprintf(hpot_pool, "ERROR: Parsing of the URL failed.  Aborting http request...\n\n");
            outlen  = strlen(tmpstr);
            apr_file_write(hpot_logfile, tmpstr, &outlen);
        }
#endif

        return 0;
    }

    if(conx_params->proxy_ip) // we are using a proxy
    {
#if VERBOSITY >= APLOG_DEBUG
        if(hpot_logfile)
        {
            tmpstr  = apr_psprintf(hpot_pool, "DEBUG: Proxy is set... using proxy_ip as remote_host...\n\n");
            outlen  = strlen(tmpstr);
            apr_file_write(hpot_logfile, tmpstr, &outlen);
        }
#endif
        // set the final remote_host
        remote_host         = apr_psprintf(hpot_pool, "%s", conx_params->proxy_ip);
    
        // set the final proxy_port
        if(conx_params->proxy_port > 0)
            remote_port         = conx_params->proxy_port;
        else
            remote_port         = 443;  // default HTTP port

        // set the final remote_url_path; the full URL for proxy requests
        if(conx_params->full_url)
            remote_url_path     = apr_psprintf(hpot_pool, "%s", conx_params->full_url);
        else
            return 0;
    }
    else // not using a proxy
    {
#if VERBOSITY >= APLOG_DEBUG
        if(hpot_logfile)
        {
            tmpstr  = apr_psprintf(hpot_pool, "DEBUG: Proxy is not set... using hostname as remote_host...\n\n");
            outlen  = strlen(tmpstr);
            apr_file_write(hpot_logfile, tmpstr, &outlen);
        }
#endif
        // set the final remote_host
        if(conx_params->parsed_uri->hostname)
            remote_host         = apr_psprintf(hpot_pool, "%s", conx_params->parsed_uri->hostname);
        else
        {
#if VERBOSITY >= APLOG_DEBUG
        if(hpot_logfile)
        {
            tmpstr  = apr_psprintf(hpot_pool, "ERROR: Could not find a valid hostname to use as remote_host...\n\n");
            outlen  = strlen(tmpstr);
            apr_file_write(hpot_logfile, tmpstr, &outlen);
        }
#endif
            return 0;
        }
    
        // set the final host_port
        remote_port         = conx_params->parsed_uri->port;

        // set the final remote_url_path; everything after the TLD of the domain
        if(conx_params->parsed_uri)
            remote_url_path     = apr_psprintf(hpot_pool, "%s", conx_params->parsed_uri->path);
        else
            return 0;
    }


#if VERBOSITY >= APLOG_DEBUG
    if(hpot_logfile)
    {
        tmpstr  = apr_psprintf(hpot_pool,
                                "\n"
                                "%s->%s\n"
                                "%s->%s\n"
                                "%s->%s\n"
                                "%s->%s\n"
                                "%s->%ld\n"
                                "%s->%s\n"
                                "%s->%s\n"
                                "%s->%s\n"
                                "%s->%ld\n"
                                "%s->%s\n"
                                "%s->%s\n"
                                "%s->%s\n"
                                "%s->%s\n"
                                "%s->%s\n"
                                "\n",



                                "full_url",         conx_params->full_url,
                                "post_url",         conx_params->post_url,
                                "debug_logfile",    conx_params->debug_logfile,
                                "hostname",         conx_params->hostname,
                                "hostport",         conx_params->hostport,
                                "http_auth_user",   conx_params->http_auth_user,
                                "http_auth_pass",   conx_params->http_auth_pass,
                                "proxy_ip",         conx_params->proxy_ip,
                                "proxy_port",       conx_params->proxy_port,
                                "request_data",     conx_params->request_data,
                                "response_data",    conx_params->response_data,
                                "response_content", conx_params->response_content,
                                "method",           conx_params->method,
                                "content_type",     conx_params->content_type);


        tmpstr  = apr_psprintf(hpot_pool, "ext_connection_params:\n---------------------------\n%s---------------------------\n", tmpstr);
        outlen  = strlen(tmpstr);
        apr_file_write(hpot_logfile, tmpstr, &outlen);
    }
#endif


    request = apr_psprintf(hpot_pool,
                           "%s %s HTTP/1.0"CRLF_STR
                           "User-Agent: Apache mod_httpbl/%s"CRLF_STR
                           "%s"
                           "Host: %s%s"CRLF_STR
                           "Accept: */*"CRLF_STR
                           "Content-length: %" APR_SIZE_T_FMT CRLF_STR
                           "Content-type: %s\r\n"
                           "%s"
                           CRLF_STR"%s",

                           (conx_params->method)?conx_params->method:"POST", remote_url_path,
                           AP_MOD_HTTPBL_BASEREVISION,
                           /*(auth)?auth:*/"",
                           remote_host, apr_psprintf(hpot_pool, ":%d", (remote_port)?remote_port:80),
                           (conx_params->request_data)?strlen(conx_params->request_data):0,
                           (conx_params->content_type)?conx_params->content_type:"application/x-www-form-urlencoded",
                           /*(hdrs)?hdrs:*/"",
                           conx_params->request_data);

#if VERBOSITY >= APLOG_DEBUG
    if(hpot_logfile)
    {
        tmpstr  = apr_psprintf(hpot_pool, "request contents:\n---------------------------\n%s\n---------------------------\n", request);
        outlen  = strlen(tmpstr);
        apr_file_write(hpot_logfile, tmpstr, &outlen);

        tmpstr  = apr_psprintf(hpot_pool, "contacting server: %s; port: %ld;\n", remote_host, remote_port);
        outlen  = strlen(tmpstr);
        apr_file_write(hpot_logfile, tmpstr, &outlen);
    }
#endif

    apr_pool_create(&mp, hpot_pool);

// BEGIN CONNECT CODE

    rv = apr_sockaddr_info_get(&sa, remote_host, APR_INET, remote_port, 0, mp);
    if (rv == APR_SUCCESS) // apr_sockaddr_info_get(...)
    {
        // fork the apr_socket_create for APR >= 1.0; the call requires differing parameters
#if (APR_MAJOR_VERSION < 1)
        rv = apr_socket_create_ex(&s, sa->family, SOCK_STREAM, APR_PROTO_TCP, mp);
#else
        rv = apr_socket_create(&s, sa->family, SOCK_STREAM, APR_PROTO_TCP, mp);
#endif

        if (rv == APR_SUCCESS) // apr_socket_create(...)
        {
            // it is a good idea to specify socket options explicitly.
            // in this case, we make a blocking socket with timeout.
            apr_socket_opt_set(s, APR_SO_NONBLOCK, 1);
            apr_socket_timeout_set(s, DEF_SOCK_TIMEOUT);
        
            rv = apr_socket_connect(s, sa);
            if (rv == APR_SUCCESS) // apr_socket_connect(...)
            {
                int l_useproxy  = conx_params->proxy_ip?1:0;
                // see the tutorial about the reason why we have to specify options again
                apr_socket_opt_set(s, APR_SO_NONBLOCK, 0);
                apr_socket_timeout_set(s, DEF_SOCK_TIMEOUT);
            
                *sock = s;

#if VERBOSITY >= APLOG_DEBUG
                if(hpot_logfile)
                {
                    tmpstr  = apr_psprintf(hpot_pool, "\nSending content:\n---------------------------\n%s\n---------------------------\n", request);
                    outlen  = strlen(tmpstr);
                    apr_file_write(hpot_logfile, tmpstr, &outlen);
                }
#endif

                apr_size_t len = strlen(request);
                rv = apr_socket_send(s, request, &len);
                if (rv == APR_SUCCESS) // apr_socket_send(...)
                {
                    while (1)
                    {
                        char*           buf     = apr_palloc(hpot_pool, BUFSIZE * sizeof(char));
                        apr_size_t      len     = BUFSIZE;
                        apr_status_t    rv      = apr_socket_recv(s, buf, &len);
                        if (rv == APR_EOF || len == 0)
                            break;
                        else
                            if(len < BUFSIZE)
                                buf[len] = '\0'; // make sure we only read valid characters from the buffer string

#if VERBOSITY >= APLOG_DEBUG
                        if(hpot_logfile)
                        {
                            tmpstr  = apr_psprintf(hpot_pool, "\nRecieved content:\n---------------------------\n%s\n---------------------------\n", buf);
                            outlen  = strlen(tmpstr);
                            apr_file_write(hpot_logfile, tmpstr, &outlen);
                        }
#endif

                        if(conx_params->response_data) // we are continuing the response recording
                            conx_params->response_data  = apr_psprintf(hpot_pool, "%s%s", conx_params->response_data, buf);
                        else // conx_params->response_data is NULL (this is the beginning of the response)
                            conx_params->response_data  = apr_pstrdup(hpot_pool, buf);
                    }

                    // find the end of the header and set the ->..._content to that location
                    if(conx_params->response_data)
                        conx_params->response_content  = strstr(conx_params->response_data, CRLF_STR""CRLF_STR);
                }
                else // apr_socket_send(...) != APR_SUCCESS
                {
#if VERBOSITY >= APLOG_DEBUG
                    if(hpot_logfile)
                    {
                        tmpstr  = apr_psprintf(hpot_pool, "Error during apr_socket_send(...).");
                        outlen  = strlen(tmpstr);
                        apr_file_write(hpot_logfile, tmpstr, &outlen);
                    }
#endif
                    return 0;
                }
            }
            else // apr_socket_connect(...) != APR_SUCCESS
            {
#if VERBOSITY >= APLOG_DEBUG
                if(hpot_logfile)
                {
                    tmpstr  = apr_psprintf(hpot_pool, "Error during apr_socket_connect(...).");
                    outlen  = strlen(tmpstr);
                    apr_file_write(hpot_logfile, tmpstr, &outlen);
                }
#endif
                return 0;
            }
        }
        else // apr_socket_create(...) != APR_SUCCESS
        {
#if VERBOSITY >= APLOG_DEBUG
            if(hpot_logfile)
            {
                tmpstr  = apr_psprintf(hpot_pool, "Error during apr_socket_create(...).");
                outlen  = strlen(tmpstr);
                apr_file_write(hpot_logfile, tmpstr, &outlen);
            }
#endif
            return 0;
        }
// END SEND/RECEIVE CODE
    }
    else // apr_sockaddr_info_get(...) != APR_SUCCESS
    {
#if VERBOSITY >= APLOG_DEBUG
            if(hpot_logfile)
            {
                tmpstr  = apr_psprintf(hpot_pool, "Error during apr_sockaddr_info_get(...).");
                outlen  = strlen(tmpstr);
                apr_file_write(hpot_logfile, tmpstr, &outlen);
            }
#endif
        return 0;
    }

    if(conx_params->response_data)
        return (int)get_response_code_from_repsonse_data(hpot_pool, conx_params->response_data);
    else
        return 0;
}

/*
    split URL into parts
*/
static int parse_url_into_conx(apr_pool_t* pool, struct ext_connection_params* conx_params, apr_file_t* logfile)
{
    char*           cp          = NULL;
    char*           h           = NULL;
    char*           scope_id    = NULL;
    apr_status_t    rv          = 1;
    char*           url         = apr_pstrdup(pool, conx_params->full_url);
    char*           l_fullurl   = apr_pstrdup(pool, conx_params->full_url);
    char*           tmpstr      = NULL;
    apr_size_t      outlen      = 0;

    if(!conx_params->full_url)
    {
#if VERBOSITY >= APLOG_DEBUG
        if(logfile)
        {
            tmpstr  = apr_psprintf(pool, "ERROR: full_url is NULL; nothing to parse.\n\n");
            outlen  = strlen(tmpstr);
            apr_file_write(logfile, tmpstr, &outlen);
        }
#endif
        return 1; // return an error immediately if we can't see the full_url to parse
    }

#if VERBOSITY >= APLOG_DEBUG
    if(logfile)
    {
        tmpstr  = apr_psprintf(pool, "TEST: calling \"apr_uri_parse(...)\"...\n");
        outlen  = strlen(tmpstr);
        apr_file_write(logfile, tmpstr, &outlen);
    }
#endif

    conx_params->response_data  = NULL;
    conx_params->parsed_uri     = apr_pcalloc(pool, sizeof(apr_uri_t));
    rv  = apr_uri_parse(pool, conx_params->full_url, conx_params->parsed_uri);
    if(rv != APR_SUCCESS)
    {
#if VERBOSITY >= APLOG_DEBUG
        if(logfile)
        {
            tmpstr  = apr_psprintf(pool, "ERROR: apr_uri_parse(...) failed.\n\n");
            outlen  = strlen(tmpstr);
            apr_file_write(logfile, tmpstr, &outlen);
        }
#endif
        return 2;
    }
    else
    {
        if(!conx_params->parsed_uri->port_str)                                                          // if the port isn't set
            conx_params->parsed_uri->port   = apr_uri_port_of_scheme(conx_params->parsed_uri->scheme);  // use the default port for this scheme
#if VERBOSITY >= APLOG_DEBUG
        if(logfile)
        {
            tmpstr  = apr_psprintf(pool, "TEST: called \"apr_parse_addr_port(...)\"...\n");
            outlen  = strlen(tmpstr);
            apr_file_write(logfile, tmpstr, &outlen);
    
            tmpstr  = apr_psprintf(pool, "TEST: calling \"hostname\": \"%s\"...\n", conx_params->parsed_uri->hostname);
            outlen  = strlen(tmpstr);
            apr_file_write(logfile, tmpstr, &outlen);
    
            tmpstr  = apr_psprintf(pool, "TEST: received \"hostport\": \"%ld\"...\n", conx_params->parsed_uri->port);
            outlen  = strlen(tmpstr);
            apr_file_write(logfile, tmpstr, &outlen);
        }
#endif
    }

    conx_params->post_url   = apr_psprintf(pool, "%s%s%s",
                                                  conx_params->parsed_uri->path,
                                                    (conx_params->parsed_uri->query)?apr_psprintf(pool, "?%s", conx_params->parsed_uri->query):"",
                                                      (conx_params->parsed_uri->fragment)?apr_psprintf(pool, "#%s", conx_params->parsed_uri->fragment):"");

    return 0;
}


/*
 *   Return 1 if the verb_bs accepts the method of the current request
 */
static int is_method_accepted_by_rbl_handler(const char* the_method, const unsigned long the_verb_bs)
{
    return  ((httpbl_string_matches(the_method, "GET"    ) && (the_verb_bs & 0x0000001ul)) ||
             (httpbl_string_matches(the_method, "POST"   ) && (the_verb_bs & 0x0000010ul)) ||
             (httpbl_string_matches(the_method, "HEAD"   ) && (the_verb_bs & 0x0000100ul)) ||
             (httpbl_string_matches(the_method, "PUT"    ) && (the_verb_bs & 0x0001000ul)) ||
             (httpbl_string_matches(the_method, "DELETE" ) && (the_verb_bs & 0x0010000ul)) ||
             (httpbl_string_matches(the_method, "OPTIONS") && (the_verb_bs & 0x0100000ul)) ||
             (httpbl_string_matches(the_method, "TRACE"  ) && (the_verb_bs & 0x1000000ul)));
}

/*
 *   Do a bitset test against a value.
 *   @return 1 iff every bit which is set in bitset is set in value
 *   @return 0 iff any bit which is set in bitset is not set in value
 */
static int does_bitset_accept_value(unsigned int bitset, unsigned int value)
{
    return ((bitset&value) == bitset);
}

/*
 *   Function to get the token string (if one exists) from the appropriate (dir or svr) config struct.
 *   @return the token string (if found); otherwise NULL
 *   @param  r   the request_rec pointer for this page request
 */
static char* get_any_whitelist_tokens(request_rec* r)
{
    httpbl_dir_cfg* this_cfg    = (httpbl_dir_cfg*)ap_get_module_config(r->per_dir_config, &httpbl_module);

    if(!this_cfg || !(this_cfg->token_str))
        return NULL;
    else
        return this_cfg->token_str;
}





/*
 *  ;
 *
 */
static apr_status_t prepare_honeypot_request_postdata(request_rec* r, apr_pool_t* p, char* access_key, struct ext_connection_params* conx_params, apr_file_t* hpot_logfile)
{
    apr_status_t    file_opened_status          = 1;
    apr_status_t    file_locked_status          = 1;
    apr_status_t    file_unlocked_status        = 1;
    apr_status_t    file_closed_status          = 1;
    apr_file_t*     hpot_params_cachefile       = NULL;
    char*           this_tag1                   = NULL;
    char*           this_tag2                   = NULL;
    char*           this_tag3                   = NULL;
    char*           this_ip                     = NULL;
    char*           this_svrn                   = NULL;
    char*           this_svp                    = NULL;
    char*           this_svip                   = NULL;
    char*           this_rquri                  = NULL;
    char*           this_phpself                = NULL;
    char*           this_version                = NULL;
    char*           this_sn                     = NULL;
    char*           this_ref                    = NULL;

    char*           this_ua                     = NULL;
    char*           file_write_buffer           = NULL;
    apr_size_t      outlen;

    if(!access_key) // no access_key, no honeypot requests
    {
#if VERBOSITY >= APLOG_DEBUG
        if(hpot_logfile)
        {
            file_write_buffer   = apr_psprintf(p, "ERROR: No access key defined.  Aborting honeypot request.\n\n");
            outlen              = strlen(file_write_buffer);
            apr_file_write(hpot_logfile, file_write_buffer, &outlen);
        }
#endif
        return -5000; // return an APR_FAILURE code
    }

    this_tag1                                   = apr_pstrdup(p, "e217e1664d3b7db40a52fb2c5abdafa5");
    this_tag2                                   = apr_pstrdup(p, "f609af9d0d820f29f584f009165e6e78");
    this_tag3                                   = apr_pstrdup(p, "34aa2473d1aa4705f92165addfe297ff");
    this_ip                                     = apr_pstrdup(p, r->connection->remote_ip);
//    this_svrn                                   = apr_pstrdup(p, r->connection->local_host);    // not always available
    this_svrn                                   = apr_pstrdup(p, r->server->server_hostname);
    this_svp                                    = apr_pstrdup(p, r->parsed_uri.port_str);
//    this_svp                                    = apr_psprintf(p, "%u", r->server->port);       // not always available
    this_svip                                   = apr_pstrdup(p, r->connection->local_ip);
    this_rquri                                  = apr_pstrdup(p, r->unparsed_uri);
    this_phpself                                = apr_pstrdup(p, r->unparsed_uri);
    this_version                                = apr_pstrdup(p, ap_get_server_version());
    this_sn                                     = apr_pstrdup(p, r->unparsed_uri);
    this_ref                                    = apr_pstrdup(p, apr_table_get(r->headers_in, "Referer"));
    this_ua                                     = apr_pstrdup(p, apr_table_get(r->headers_in, "User-Agent"));

    file_write_buffer   = apr_psprintf(p,
                                        "hbl_access_key=%s"LINE_SEPARATOR""FIELD_SEPARATOR
                                            "tag1=%s"LINE_SEPARATOR""FIELD_SEPARATOR
                                                "tag2=%s"LINE_SEPARATOR""FIELD_SEPARATOR
                                                    "tag3=%s"LINE_SEPARATOR""FIELD_SEPARATOR
                                                        "ip=%s"LINE_SEPARATOR""FIELD_SEPARATOR
                                                            "svrn=%s"LINE_SEPARATOR""FIELD_SEPARATOR
                                                                "svp=%s"LINE_SEPARATOR""FIELD_SEPARATOR
                                                                    "svip=%s"LINE_SEPARATOR""FIELD_SEPARATOR
                                                                        "rquri=%s"LINE_SEPARATOR""FIELD_SEPARATOR
                                                                            "phpself=%s"LINE_SEPARATOR""FIELD_SEPARATOR
                                                                                "version=%s"LINE_SEPARATOR""FIELD_SEPARATOR
                                                                                    "sn=%s"LINE_SEPARATOR""FIELD_SEPARATOR
                                                                                        "ref=%s"LINE_SEPARATOR""FIELD_SEPARATOR
                                                                                            "uagnt=%s"LINE_SEPARATOR""FIELD_SEPARATOR
                                                                                                "auto_transcribe=1",
                                        (access_key)?access_key:"",
                                            (this_tag1)?this_tag1:"",
                                                (this_tag2)?this_tag2:"",
                                                    (this_tag3)?this_tag3:"",
                                                        (this_ip)?yahoo_urlencode(p, this_ip):"",
                                                            (this_svrn)?yahoo_urlencode(p, this_svrn):"",
                                                                (this_svp)?yahoo_urlencode(p, this_svp):"",
                                                                    (this_svip)?yahoo_urlencode(p, this_svip):"",
                                                                        (this_rquri)?yahoo_urlencode(p, this_rquri):"",
                                                                            (this_phpself)?yahoo_urlencode(p, this_phpself):"",
                                                                                (this_version)?yahoo_urlencode(p, this_version):"",
                                                                                    (this_sn)?yahoo_urlencode(p, this_sn):"",
                                                                                        (this_ref)?yahoo_urlencode(p, this_ref):"",
                                                                                            (this_ua)?yahoo_urlencode(p, this_ua):"");

    conx_params->full_url       = apr_pstrdup(p, DEFAULT_HONEYPOT_REQUEST_URL);
    conx_params->request_data   = apr_pstrdup(p, file_write_buffer);
    conx_params->method         = "POST";
    conx_params->content_type   = "application/x-www-form-urlencoded";
    conx_params->debug_logfile  = hpot_logfile;
    conx_params->http_auth_user = (g_FOF_post_httpauth_un && !httpbl_string_matches(g_FOF_post_httpauth_un, ""))?g_FOF_post_httpauth_un:NULL;
    conx_params->http_auth_pass = (g_FOF_post_httpauth_pw && !httpbl_string_matches(g_FOF_post_httpauth_pw, ""))?g_FOF_post_httpauth_pw:NULL;
//    conx_params->proxy_ip       = g_FOF_post_proxy_ip;
//    if(g_FOF_post_proxy_port)
//        conx_params->proxy_port     = atol(g_FOF_post_proxy_port);
    conx_params->proxy_ip       = NULL;
    conx_params->proxy_port     = 0;

    return APR_SUCCESS;
}





/*
 *  ;
 *
 */
static apr_status_t prepare_404_submission_postdata(request_rec* r, apr_pool_t* p, char* access_key, struct ext_connection_params* conx_params, apr_file_t* FOF_logfile)
{
    apr_status_t    file_opened_status          = 1;
    apr_status_t    file_locked_status          = 1;
    apr_status_t    file_unlocked_status        = 1;
    apr_status_t    file_closed_status          = 1;
    apr_file_t*     request_params_cachefile    = NULL;
    apr_finfo_t     finfo;
    apr_status_t    rv                          = 1;
    apr_size_t      postlen                     = 0;
    char*           postdata                    = NULL;

    char*           file_write_buffer           = NULL;
    apr_size_t      outlen;

    if(!access_key) // no access_key, no honeypot requests
    {
#if VERBOSITY >= APLOG_DEBUG
        if(FOF_logfile)
        {
            file_write_buffer   = apr_psprintf(p, "ERROR: No access key defined.  Aborting honeypot request.\n\n");
            outlen              = strlen(file_write_buffer);
            apr_file_write(FOF_logfile, file_write_buffer, &outlen);
        }
#endif
        return -5000; // return an APR_FAILURE code
    }

#ifndef SHOULD_CACHE
    postdata    = apr_pstrdup(r->pool, g_FOF_cache_string);
    outlen      = g_FOF_cache_string_l;
#else
    // attempt to open the 404 cache datafiel for reading...
    if ((rv = apr_file_open(&request_params_cachefile, g_FOF_cache_data_filepath, APR_READ, APR_OS_DEFAULT, p)) == APR_SUCCESS)
    {
        apr_file_info_get(&finfo, APR_FINFO_NORM, request_params_cachefile);
        postlen     = (apr_size_t)finfo.size;
        postdata    = apr_palloc(p, postlen);
        if (!postdata)
        {
#if VERBOSITY >= APLOG_DEBUG
            if(FOF_logfile)
            {
                file_write_buffer   = apr_psprintf(p, "Could not allocate POST data buffer\n");
                outlen              = strlen(file_write_buffer);
                apr_file_write(FOF_logfile, file_write_buffer, &outlen);
            }
#endif
            ap_log_error(APLOG_MARK, APLOG_INFO, 0, NULL, "HTTPBL: prepare_404_submission_postdata: Could not allocate POST data buffer\n");
            return APR_ENOMEM;
        }
        rv = apr_file_read_full(request_params_cachefile, postdata, postlen, NULL);
        if (rv != APR_SUCCESS)
        {
#if VERBOSITY >= APLOG_DEBUG
            if(FOF_logfile)
            {
                file_write_buffer   = apr_psprintf(p, "Could not read POST data file.  Aborting 404 Submission.\n");
                outlen              = strlen(file_write_buffer);
                apr_file_write(FOF_logfile, file_write_buffer, &outlen);
            }
#endif
            return rv;
        }
        apr_file_close(request_params_cachefile);
    }
    else
    {
#if VERBOSITY >= APLOG_INFO
        if(FOF_logfile)
        {
            file_write_buffer   = apr_psprintf(p, "ERROR: prepare_404_submission_postdata: 404 Cache file (%s) could not be opened for reading.\n  Aborting 404 submission.\n\n", g_FOF_cache_data_filepath);
            outlen              = strlen(file_write_buffer);
            apr_file_write(FOF_logfile, file_write_buffer, &outlen);
        }
#endif
        ;
    }
#endif

    conx_params->request_data   = postdata;
    conx_params->full_url       = g_FOF_post_url;
    conx_params->method         = "POST";
    conx_params->content_type   = "application/x-www-form-urlencoded";
    conx_params->debug_logfile  = FOF_logfile;
    conx_params->http_auth_user = (g_FOF_post_httpauth_un && !httpbl_string_matches(g_FOF_post_httpauth_un, ""))?g_FOF_post_httpauth_un:NULL;
    conx_params->http_auth_pass = (g_FOF_post_httpauth_pw && !httpbl_string_matches(g_FOF_post_httpauth_pw, ""))?g_FOF_post_httpauth_pw:NULL;
//    conx_params->proxy_ip       = g_FOF_post_proxy_ip;
//    if(g_FOF_post_proxy_port)
//        conx_params->proxy_port     = atol(g_FOF_post_proxy_port);
    conx_params->proxy_ip       = NULL;
    conx_params->proxy_port     = 0;

    return APR_SUCCESS;
}



/*
 *  ;
 *   httpbl_test_not_tried   = 0,
 *   httpbl_test_passed      = 1,
 *   httpbl_test_failed      = 2
 */
static char* get_test_status_bullet_string(request_rec* r, int test_status)
{
    char*   the_return_string       = NULL;
    char*   the_test_status_char    = NULL;

    switch(test_status)
    {
    case httpbl_test_passed:
        the_test_status_char    = "<td style=\"color: #000000; background-color: #00FF00;\">[ + ]</td>";
        break;
    case httpbl_test_failed:
        the_test_status_char    = "<td style=\"color: #FFFFFF; background-color: #FF0000;\">[ - ]</td>";
        break;
    case httpbl_test_not_tried:
    default:
        the_test_status_char    = "<td style=\"color: #FFFFFF; background-color: #000099;\">[ * ]</td>";
        break;
    }

    the_return_string   = apr_pstrdup(r->pool, the_test_status_char);

    return the_return_string;
}


/*
 *   Create a directory and any necessary ancestor directories.
 *   If the path already exists, simply return 1.
 *   @return 1 if the requested path existed or was created.
 *   @return 0 if the path was unable to be created
 */
static int create_dirpath_if_none_exists(apr_pool_t* p, char* path/*, perms*/)
{
    apr_finfo_t*        finfo;

    apr_status_t        apr_stat_status = apr_stat(finfo, path, APR_FINFO_LINK | APR_FINFO_TYPE, p);
    if(apr_stat_status != APR_SUCCESS && apr_stat_status != APR_INCOMPLETE) // if we can't lstat, the path/file doesn't exist
    {
        if(finfo->filetype == APR_NOFILE) // if the repos directory does not exist...
        {
            // create the directory
            apr_fileperms_t permissions = APR_UREAD | APR_UWRITE | APR_UEXECUTE | APR_GREAD | APR_GEXECUTE | APR_WREAD | APR_WEXECUTE;
            if(apr_dir_make_recursive(path, permissions, p) != APR_SUCCESS)
            {
//                ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "apr_dir_make_recursive failed; tried creating dir: \"%s\"", path);
                return 0;
            }
            else
                return 1;
        }
        else if(finfo->filetype != APR_DIR)
        {
            return 0;
        }
//            ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "log directory cannot be created because a non-directory file already exists with that name.");
    }
    else
    {
//        ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "log directory exists");
        return 1;
    }
}


/*
    Create a file and any necessary ancestor directories.
    If the path and the file already exist, simply return 1.
    @return 1 if the requested path and the requested file exist or were created.
    @return 0 if the path or file was unable to be created
*/
static int create_file_if_none_exists(apr_pool_t* p, char* path, char* filename/*, perms*/)
{
    int                 return_value        = 1;
    apr_finfo_t*        finfo               = NULL;
    apr_status_t        rv                  = 1;
    apr_file_t*         the_file            = NULL;

    return_value = create_dirpath_if_none_exists(p, path/*, perms*/);

    if (return_value == 1) // if the directory path exists
    {
        char*               the_filename    = apr_psprintf(p, "%s/%s", get_log_dir(), "httpbl_diagnostics_test.txt");
        if(the_filename)
            rv              = apr_file_open(&the_file, the_filename, APR_READ | APR_CREATE, APR_OS_DEFAULT, p);
        else
            return_value = 0; // we can't do anything without a correct filename

        if(rv != APR_SUCCESS) // if the file creation/opening was not successful
            return_value = 0;
        else
            apr_file_close(the_file);
    }

    return return_value;
}



/*
    ;
*/
static int test_httpbl_logfile_writes(request_rec* r)
{
    int                 the_return_value    = httpbl_test_not_tried;
    apr_finfo_t*        finfo               = NULL;
    apr_status_t        rv                  = 1;
    apr_file_t*         the_file            = NULL;

    char*               the_filename    = apr_psprintf(r->pool, "%s/%s", get_log_dir(), "httpbl_diagnostics_test.txt");
    if(the_filename)
    {
        rv              = apr_file_open(&the_file, the_filename, APR_READ | APR_CREATE, APR_OS_DEFAULT, r->pool);

        if(rv == APR_SUCCESS) // if the file creation/opening was not successful
        {
            apr_file_close(the_file);
    //        apr_file_remove(apr_psprintf(r->pool, "%s/%s", get_log_dir(), "httpbl_diagnostics_test.txt"), r->pool);
            the_return_value    = httpbl_test_passed;
        }
        else
        {
            ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "failed to create a temp file in the log_dir (\"%s\")", get_log_dir());
            the_return_value    = httpbl_test_failed;
        }
    }

    return the_return_value;
}



/*
    ;
*/
static int test_httpbl_reposfile_writes(request_rec* r)
{
    int                 the_return_value    = httpbl_test_not_tried;
    apr_finfo_t*        finfo               = NULL;
    apr_status_t        rv                  = 1;
    apr_file_t*         the_file            = NULL;

    char*               the_filename    = apr_psprintf(r->pool, "%s/%s", get_repos_dir(), "httpbl_diagnostics_test.txt");
    if(the_filename)
    {
        rv              = apr_file_open(&the_file, the_filename, APR_READ | APR_CREATE, APR_OS_DEFAULT, r->pool);

        if(rv == APR_SUCCESS) // if the file creation/opening was not successful
        {
            apr_file_close(the_file);
    //        apr_file_remove(apr_psprintf(r->pool, "%s/%s", get_log_dir(), "httpbl_diagnostics_test.txt"), r->pool);
            the_return_value    = httpbl_test_passed;
        }
        else
        {
            ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "failed to create a temp file in the log_dir (\"%s\")", get_log_dir());
            the_return_value    = httpbl_test_failed;
        }
    }

    return the_return_value;
}


/*
    ;
*/
static int test_httpbl_domain_resolution_request(request_rec* r)
{
    int                 the_return_value    = httpbl_test_not_tried;
    struct hostent*     returned1           = apr_palloc(r->pool, sizeof(struct hostent));
    char*               theRBLReturned1     = (char*)apr_palloc(r->pool, 16*sizeof(char));
    struct in_addr      h_addr; // internet address
    char*               ip_string           = NULL;

    returned1                               = (struct hostent*)gethostbyname(TEST_INTERNET_DOMAIN);

    if(returned1)
    {
        theRBLReturned1                         = apr_psprintf( r->pool, "%s", inet_ntop(returned1->h_addrtype, returned1->h_addr_list[0], theRBLReturned1, 15*sizeof(char)) );
        ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "getbyhostname(\"%s\") = %s", TEST_INTERNET_DOMAIN, theRBLReturned1);
        the_return_value                        = httpbl_test_passed;
    }
    else
    {
        theRBLReturned1                         = NULL;
        ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "getbyhostname(\"%s\") = %s", TEST_INTERNET_DOMAIN, theRBLReturned1);
        the_return_value                        = httpbl_test_failed;
    }

    return the_return_value;
}


/*
    ;
*/
static int test_httpbl_hostent_request(request_rec* r)
{
    int                 the_return_value    = httpbl_test_not_tried;
    struct hostent*     returned2           = apr_palloc(r->pool, sizeof(struct hostent));
    char*               theRBLReturned2     = (char*)apr_palloc(r->pool, 16*sizeof(char));
    struct in_addr      h_addr; // internet address
    char*               ip_string           = NULL;

    returned2                               = (struct hostent*)gethostbyname(TEST_RBL_DOMAIN);

    if(returned2)
    {
        theRBLReturned2                         = apr_psprintf( r->pool, "%s", inet_ntop(returned2->h_addrtype, returned2->h_addr_list[0], theRBLReturned2, 15*sizeof(char)) );
        ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "getbyhostname(\"%s\") = %s", TEST_RBL_DOMAIN, theRBLReturned2); // shows the same returned value as the previous line (bug)
    }
    else
    {
        ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "getbyhostname(\"%s\") = %s", TEST_RBL_DOMAIN, theRBLReturned2); // shows the same returned value as the previous line (bug)
    }

    if(httpbl_string_matches(theRBLReturned2, TEST_RBL_RESPONSE))
        the_return_value    = httpbl_test_passed;
    else
        the_return_value    = httpbl_test_failed;

    return the_return_value;
}


/*
    ;
*/
static int test_httpbl_rbl_server_domain_resolution_request(request_rec* r)
{
    int                 the_return_value    = httpbl_test_not_tried;
    struct hostent*     returned1           = apr_palloc(r->pool, sizeof(struct hostent));
    char*               theRBLReturned1     = (char*)apr_palloc(r->pool, 16*sizeof(char));
    struct in_addr      h_addr; // internet address
    char*               ip_string           = NULL;

    returned1                               = (struct hostent*)gethostbyname(TEST_DOMAIN3);


    if(returned1)
    {
        theRBLReturned1                 = apr_psprintf(r->pool, "%s", inet_ntop(returned1->h_addrtype, returned1->h_addr_list[0], theRBLReturned1, 15*sizeof(char)));
        ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "getbyhostname(\"%s\") = %s", TEST_DOMAIN3, theRBLReturned1); // shows the same returned value as the previous line
        the_return_value    = httpbl_test_passed;
    }
    else
    {
        theRBLReturned1                 = NULL;
        ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "getbyhostname(\"%s\") = NULL", TEST_DOMAIN3); // shows the same returned value as the previous line
        the_return_value    = httpbl_test_passed;

    }

    return the_return_value;
}


/*
    handlePageAccessAction
    This function returns the appropriate return_code (DECLINED, HTTP_FORBIDDEN, HTTP_MOVED_TEMPORARILY, etc.) and, if necessary,
    sets the Location field (as in a redirect).
*/
static int handlePageAccessAction(request_rec* r, int access_value, const char* the_rbl_string, httpbl_dir_cfg* this_dir_cfg)
{
    int     the_return_value    = DEFAULT_ACTION;
    char*   redir_to            = NULL;
    char*   full_url            = apr_psprintf(r->pool, "%s://%s%s%s%s", (r->parsed_uri.scheme)?r->parsed_uri.scheme:"http", r->hostname, (r->parsed_uri.port_str)?":":"", (r->parsed_uri.port_str)?(r->parsed_uri.port_str):"", r->unparsed_uri);

    switch(access_value)
    {
    case HTTPBL_ACTION_DENY:
        ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: - (0.5.1.1) page access: \"DENY\".\nReturning %d.", HTTP_FORBIDDEN);
        the_return_value = HTTP_FORBIDDEN;
        ap_log_rerror(APLOG_MARK, APLOG_NOTICE, 0, r, "HTTPBL: IP/URL combination denied by mod_httpbl configuration; IP: {%s}; RBL-value:{%s}; URL: {%s}", r->connection->remote_ip, the_rbl_string, full_url);
        break;
    case HTTPBL_ACTION_ALLOW_XLATE_EMAILS:
        ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: - (0.5.1.2.1) page access: \"ALLOW_XLATE_EMAILS\".\nReturning %d.", DECLINED);
        ap_add_output_filter("REPLACE_EMAIL_LINKS", NULL, r, r->connection); // set the replace-emails output filter to run... first pass replaces link URLs
        ap_add_output_filter("REPLACE_EMAIL_TEXT", NULL, r, r->connection); // set the replace-emails output filter to run... second pass replaces all other email text
        the_return_value = DECLINED;
        ap_log_rerror(APLOG_MARK, APLOG_NOTICE, 0, r, "HTTPBL: IP/URL combination allowed with translated emails by mod_httpbl configuration; IP: {%s}; RBL-value:{%s}; URL: {%s}", r->connection->remote_ip, the_rbl_string, full_url);
        break;
    case HTTPBL_ACTION_ALLOW:
        ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: - (0.5.1.2.0) page access: \"ALLOW\".\nReturning %d.", DECLINED);
        the_return_value = DECLINED;
        break;
#ifdef SHOULD_ALLOW_CHALLENGES
    case HTTPBL_ACTION_CHALLENGE:
        ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: - (0.5.1.3) page access: \"CHALLENGE\".\nReturning %d.", HTTP_MOVED_TEMPORARILY);
        redir_to            = apr_psprintf(r->pool, "%s?ref=%s", this_dir_cfg->challenge_url, yahoo_urlencode(r->pool, r->uri));
        apr_table_setn(r->headers_out, "Location", redir_to);   // this is a non-internal redirect; it would be nice to be able to cloak the challenge/whitelist token from logs by doing an internal redirect
        the_return_value    = HTTP_MOVED_TEMPORARILY;
        ap_log_rerror(APLOG_MARK, APLOG_NOTICE, 0, r, "HTTPBL: IP/URL combination challenged by mod_httpbl configuration; IP: {%s}; RBL-value:{%s}; URL: {%s}", r->connection->remote_ip, the_rbl_string, full_url);
        break;
#endif
#ifdef SHOULD_REQUEST_HONEYPOTS
    case HTTPBL_ACTION_HONEYPOT:
        the_return_value    = handle_honeypot_request(r);
        ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: - (*.*.*.*) *********************** AP HOOK HANDLER CALLED ***********************");
        ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: - (0.5.1.4) page access: \"HONEYPOT\".\nReturning %s.", (the_return_value==OK)?"OK":(the_return_value==DECLINED)?"DECLINED":"UNKNOWN");
        if(the_return_value == OK)
            the_return_value = DONE;
        ap_log_rerror(APLOG_MARK, APLOG_NOTICE, 0, r, "HTTPBL: IP/URL combination 'honey pot'ed by mod_httpbl configuration; IP: {%s}; RBL-value:{%s}; URL: {%s}", r->connection->remote_ip, the_rbl_string, full_url);
        break;
#endif
    default: // handle the DEFAULT ACTION (as defined by the precompiler constant)
        if (DEFAULT_ACTION != HTTPBL_ACTION_ALLOW &&
            DEFAULT_ACTION != HTTPBL_ACTION_DENY &&
#ifdef SHOULD_ALLOW_CHALLENGES
            DEFAULT_ACTION != HTTPBL_ACTION_CHALLENGE &&
#endif
#ifdef SHOULD_REQUEST_HONEYPOTS
            DEFAULT_ACTION != HTTPBL_ACTION_HONEYPOT &&
#endif
            1)
            the_return_value    = handlePageAccessAction(r, HTTPBL_ACTION_ALLOW, the_rbl_string, this_dir_cfg);    // recurse one iteration, but set the action
        else
            the_return_value    = handlePageAccessAction(r, DEFAULT_ACTION, the_rbl_string, this_dir_cfg);          // recurse one iteration, but set the action
        break;
    }

    return the_return_value;
}



/*
    Apache handler function to check if the request is a diagnostics page before any other handlers take action.
    If a honeypot request is encountered, add the requesting IP to the whitelist and internally redirect the request to the (originally) referring URI.
    @return         the status code if the function completes the request or DECLINED if the request requires further processing by this module
    @param  r       the request_rec of the Apache request to be handled
*/
static int handle_any_httpbl_tests(request_rec* r)
{
    int status  = DECLINED;
    if(is_request_a_httpbl_testing_token(r))
        ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: URI (%s) IS a httpbl_testing token", r->unparsed_uri);
    else
        ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: URI (%s) IS NOT a httpbl_testing token", r->unparsed_uri);

    if(is_request_a_httpbl_testing_token(r) && httpbl_string_matches(r->method, "GET"))
    {
        int             status_code = OK;

        status_code     = 200;

        if (status_code == 200)
        {
            int bytes_sent      = 0;

            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "HTTPBL: running test 1...");
            int passed_test1    = test_httpbl_logfile_writes(r);
            ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: test 1 %s", (passed_test1==httpbl_test_passed)?"PASSED":(passed_test1==httpbl_test_not_tried?"NOT TRIED":"FAILED"));

            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "HTTPBL: running test 2...");
            int passed_test2    = test_httpbl_reposfile_writes(r);
            ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: test 2 %s", (passed_test2==httpbl_test_passed)?"PASSED":(passed_test2==httpbl_test_not_tried?"NOT TRIED":"FAILED"));

            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "HTTPBL: running test 3...");
            int passed_test3    = test_httpbl_domain_resolution_request(r);
            ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: test 3 %s", (passed_test3==httpbl_test_passed)?"PASSED":(passed_test3==httpbl_test_not_tried?"NOT TRIED":"FAILED"));

            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "HTTPBL: running test 4...");
            int passed_test4    = test_httpbl_hostent_request(r);
            ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: test 4 %s", (passed_test4==httpbl_test_passed)?"PASSED":(passed_test4==httpbl_test_not_tried?"NOT TRIED":"FAILED"));

            int passed_test5    = 0;
            int passed_test6    = 0;
            int passed_test7    = 0;
#ifdef SHOULD_REQUEST_HONEYPOTS
            int passed_test8    = 0;
#endif

            // write from the subrequest results to the SuperRequest
            ap_set_content_type(r, "text/html");
            bytes_sent      = ap_rprintf(r, "<html>\n\t<head>\n<title>HTTPBL Internal Diagnostics Testing</title>\n\t</head>\n\t<body>\n\t\t<h1>HTTPBL Internal Diagnostics Testing</h1>");
            bytes_sent      = ap_rprintf(r, "\n\t\t<table cellpadding=\"4\" cellspacing=\"4\" border=\"1\" style=\"background-color: #FFFFFF;\">");
            bytes_sent      = ap_rprintf(r, "\n\t\t\t<tr><td colspan=\"2\">Test Results</td></tr>");
            bytes_sent      = ap_rprintf(r, "\n\t\t\t<tr>%s<td><b>Write to the log directory</b> (%s)</td></tr>",                   get_test_status_bullet_string(r, passed_test1), get_log_dir());
            bytes_sent      = ap_rprintf(r, "\n\t\t\t<tr>%s<td><b>Write to the repos directory</b> (%s)</td></tr>",                 get_test_status_bullet_string(r, passed_test2), get_repos_dir());
            bytes_sent      = ap_rprintf(r, "\n\t\t\t<tr>%s<td><b>Resolve and reach the RBL domain</b> (%s)</td></tr>",             get_test_status_bullet_string(r, passed_test3), TEST_INTERNET_DOMAIN);
/*
//            bytes_sent      = ap_rprintf(r, "\n\t\t\t<tr>%s<td><b>Query the Project Honey Pot RBL</b> (%s)</td></tr>",              get_test_status_bullet_string(r, passed_test4), );
            bytes_sent      = ap_rprintf(r, "\n\t\t\t<tr>%s<td><b>Authenticate with the Project Honey Pot RBL</b> (%s)</td></tr>",  get_test_status_bullet_string(r, passed_test5), "*.dave.httpbl.org");
            bytes_sent      = ap_rprintf(r, "\n\t\t\t<tr>%s<td><b>Submit 404 data to the server</b> (%s)</td></tr>",                get_test_status_bullet_string(r, passed_test6), DEFAULT_404_URL);
            bytes_sent      = ap_rprintf(r, "\n\t\t\t<tr>%s<td><b>Submit POST data to the server</b> (%s)</td></tr>",               get_test_status_bullet_string(r, passed_test7), DEFAULT_POST_URL);
#ifdef SHOULD_REQUEST_HONEYPOTS
            bytes_sent      = ap_rprintf(r, "\n\t\t\t<tr>%s<td><b>Query for honeypots</b> (%s)</td></tr>",                          get_test_status_bullet_string(r, passed_test8), DEFAULT_HONEYPOT_REQUEST_URL);
#endif
*/
            bytes_sent      = ap_rprintf(r, "\n\t\t\t<tr><td colspan=\"2\">&nbsp;</td></tr>");
            bytes_sent      = ap_rprintf(r, "\n\t\t\t<tr><td colspan=\"2\">Test Key</td></tr>");
            bytes_sent      = ap_rprintf(r, "\n\t\t\t<tr style=\"background-color: #F0F0F0;\">%s<td>Passed Test</td></tr>",                 get_test_status_bullet_string(r, httpbl_test_passed));
            bytes_sent      = ap_rprintf(r, "\n\t\t\t<tr style=\"background-color: #F0F0F0;\">%s<td>Failed Test</td></tr>",                 get_test_status_bullet_string(r, httpbl_test_failed));
            bytes_sent      = ap_rprintf(r, "\n\t\t\t<tr style=\"background-color: #F0F0F0;\">%s<td>Test Was Not Performed</td></tr>",      get_test_status_bullet_string(r, httpbl_test_not_tried));
            bytes_sent      = ap_rprintf(r, "\n\t\t</table>");
            bytes_sent      = ap_rprintf(r, "\n\t\t\t\t<br /><br />");
            bytes_sent      = ap_rprintf(r, "\n\t</body>\n</html>");
        }
// done processing content

        status = OK;
    } // if(is_request_a_httpbl_testing_token(r) && httpbl_string_matches(r->method, "GET"))

    return status;
}




/*
 *   Apache handler function to check if the request is a whitelist token before any other handlers take action.
 *   If a whitelist token is encountered, add the requesting IP to the whitelist and internally redirect the request to the (originally) referring URI.
 *   @return         the status code if the function completes the request or DECLINED if the request requires further processing by this module
 *   @param  r       the request_rec of the Apache request to be handled
 */
static int handle_any_whitelist_tokens(request_rec* r)
{
    httpbl_dir_cfg* this_cfg    = (httpbl_dir_cfg*)ap_get_module_config(r->server->module_config, &httpbl_module);
    if (!this_cfg || !(this_cfg->token_str))    // if there is no valid svr_cfg
        return DECLINED;                        // don't process this request as a whitelist token

    if(is_request_a_whitelist_token(r))
    {
        whitelist_insert(r->pool, r->connection->remote_ip);

        if(strstr(r->uri, this_cfg->token_str) != NULL)
        {
            if(strstr(r->unparsed_uri, "ref=") != NULL) // a referrer is set
            {
                char*   urlenc_referring_url   = NULL;
                char*   urldec_referring_url   = NULL;
                char*   running = (char*)apr_pstrdup(r->pool, r->unparsed_uri);

                urlenc_referring_url    = strstr( running, "ref=" )+4;
                urldec_referring_url    = yahoo_urldecode(r->pool, urlenc_referring_url);
                apr_table_set(r->headers_out, "Location", urldec_referring_url);
            }
            else
                apr_table_set(r->headers_out, "Location", "/"); // default to the server-root page

            if(ap_is_HTTP_REDIRECT(r->status))
            {
                int n = r->status;
                r->status   = HTTP_OK;
    
                return n;
            }
            else
            {
                ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: HTTPBL Token handled.  Redirecting to: \"%s\"", apr_table_get(r->headers_out, "Location"));
                return HTTP_MOVED_TEMPORARILY;
            }
        }
        else
            return DECLINED; // maybe display a custom token-error message here?
    }
    else // don't process this as a "token request"... 
    {
        ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: - request is not a whitelist token.");
        return DECLINED; // continue module processing of this request
    }
}


/*
 *
 */
static int handle_honeypot_request(request_rec* r)
{
    apr_file_t*     hpot_logfile                    = NULL; // logfile for debugging only but needs to be passed as NULL if logfile is never opened 
#if VERBOSITY >= APLOG_DEBUG
    apr_status_t    hpot_logfile_opened             = 1;
    apr_status_t    hpot_logfile_locked             = 1;
    apr_status_t    hpot_logfile_unlocked           = 1;
    apr_status_t    hpot_logfile_closed             = 1;
#endif
    apr_status_t    hpot_cachefile_removed          = 1;
    int             status                          = DECLINED;
    apr_pool_t*     hpot_pool                       = NULL;
    int             status_code                     = OK;
    char*           this_access_key                 = NULL;
    char*           testing_url                     = NULL;
    char*           full_testing_url_string         = NULL;
    char*           this_ip                         = NULL;
    char*           hpot_logfile_filename           = NULL;
    char*           tmpstr                          = NULL;
    apr_size_t      outlen                          = 0;
    apr_time_t      timestamp_now                   = apr_time_now();
    struct ext_connection_params*   conx_params     = NULL;

    char*           DEFAULT_HPOT_GOOD_RESPONSE      = "<html>\n"
                                                        "\t<head>\n"
                                                        "<title>MY FIRST HONEYPOT</title>\n"
                                                        "\t</head>\n"
                                                        "\t<body>\n"
                                                        "\t\t<h1>My First Honeypot's Content:</h1>\n"
                                                        "\t\t<pre>%s</pre>\n"
                                                        "\t</body>\n"
                                                        "</html>";
    char*           DEFAULT_HPOT_NO_KEY_RESPONSE    = "<html>\n"
                                                        "\t<head>\n"
                                                        "<title>An Error Occurred</title>\n"
                                                        "\t</head>\n"
                                                        "\t<body>\n"
                                                        "\t\t<h1>An error occurred request a Honey Pot from Project Honey Pot</h1>\n"
                                                        "\t\t<p>Please ensure you have set the <i>"DIRECTIVE_TEXT_ACCESS_KEY"</i> directive and that the value matches the Access Key you were issued by Project Honey Pot.<br />\n"
                                                        "\t\t\tIf you do not have a HTTPBL RBL Access Key, visit <a href=\"http://www.ProjectHoneyPot.org/\">Project Honey Pot</a> and sign into your account to receive one.<br />"
                                                        "\t\t\tFor help on using the <i>"DIRECTIVE_TEXT_ACCESS_KEY"</i> directive, please refer to the mod_httpbl documentation.<br />"
                                                        "\t\t\t%s</p>\n"
                                                        "\t</body>\n"
                                                        "</html>";
    char*           DEFAULT_HPOT_ERROR_RESPONSE     = "<html>\n"
                                                        "\t<head>\n"
                                                        "<title>An Error Occurred</title>\n"
                                                        "\t</head>\n"
                                                        "\t<body>\n"
                                                        "\t\t<h1>An error occurred request a Honey Pot from Project Honey Pot</h1>\n"
                                                        "\t\t<p>Please ensure you have set the <i>"DIRECTIVE_TEXT_REPOS_DIR"</i> directive and that directory has write permissions for the user which runs Apache.<br />\n"
                                                        "\t\t\tPlease also ensure you have set the <i>"DIRECTIVE_TEXT_ACCESS_KEY"</i> directive and that the value matches the Access Key you were issued by Project Honey Pot.<br />\n"
                                                        "\t\t\tIf you do not have a HTTPBL RBL Access Key, visit <a href=\"http://www.ProjectHoneyPot.org/\">Project Honey Pot</a> and sign into your account to receive one.<br />"
                                                        "\t\t\tFor help on using the <i>"DIRECTIVE_TEXT_REPOS_DIR"</i> and <i>"DIRECTIVE_TEXT_ACCESS_KEY"</i> directives, please refer to the mod_httpbl documentation.<br />"
                                                        "\t\t\t%s</p>\n"
                                                        "\t</body>\n"
                                                        "</html>";

    
    apr_pool_create(&hpot_pool, r->pool);

    // a calloc should clear the memory contents (set all zeros), which should NULL out all pointers within the struct
    conx_params                             = (struct ext_connection_params*)apr_pcalloc(hpot_pool, 1 * sizeof(struct ext_connection_params));

    ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: Responding to URI (%s) with a honeypot", r->unparsed_uri);

    testing_url                             = get_this_requests_g_httpbl_testing_url(r);
    full_testing_url_string                 = apr_psprintf(hpot_pool, "Your diagnostics URL: <a href=\"%s\">Diagnostics Page</a>", testing_url);
    this_ip                                 = apr_pstrdup(hpot_pool, r->connection->remote_ip);                                                        
    this_access_key                         = get_access_key_for_this_request(r, NULL, NULL);
    hpot_logfile_filename                   = apr_psprintf(hpot_pool, "%shttpbl_honeypot_params_%s_%"APR_TIME_T_FMT"%s", get_log_dir(), this_ip, timestamp_now, ".log");

ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: handling honeypot.  Got here (1)");

#if VERBOSITY >= APLOG_DEBUG
        hpot_logfile_opened     = apr_file_open(&hpot_logfile, hpot_logfile_filename, APR_WRITE | APR_TRUNCATE | APR_CREATE, APR_OS_DEFAULT, hpot_pool);
        if(hpot_logfile_opened == APR_SUCCESS)
            hpot_logfile_locked     = apr_file_lock(hpot_logfile, APR_FLOCK_EXCLUSIVE);
        else
            hpot_logfile            = NULL;
#endif

ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: handling honeypot.  Got here (2)");

    // write the honeypot POST params to a file
    if(prepare_honeypot_request_postdata(r, hpot_pool, this_access_key, conx_params, hpot_logfile) == APR_SUCCESS)
    {
        int     post_was_successful = 0;
        char*   post_vars_to_send   = NULL;
        char*   get_vars_to_send    = NULL;

ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: handling honeypot.  Got here (3)");

        parse_url_into_conx(hpot_pool, conx_params, hpot_logfile);

ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: handling honeypot.  Got here (4)");
        
        if((post_vars_to_send = get_post_kvp_string(r, MAX_POST_LENGTH))) // if the post parameter string is not NULL...
            post_vars_to_send           = pass_kvps_to_honeypot(r, post_vars_to_send, "post|", "has_post");
        
        if(r->parsed_uri.query)
            get_vars_to_send            = pass_kvps_to_honeypot(r, r->parsed_uri.query, "get|", "has_get");
        
        if(get_vars_to_send && post_vars_to_send)
            conx_params->request_data   = apr_pstrcat(r->pool, "&", conx_params->request_data, "&", get_vars_to_send, "&", post_vars_to_send, NULL);
        else if(get_vars_to_send)
            conx_params->request_data   = apr_pstrcat(r->pool, "&", conx_params->request_data, "&", get_vars_to_send, NULL);
        else if(post_vars_to_send)
            conx_params->request_data   = apr_pstrcat(r->pool, "&", conx_params->request_data, "&", post_vars_to_send, NULL);

ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: handling honeypot.  Got here (5)");

        // send the honeypot POST and get the pointer to the response
        post_was_successful = make_simple_http_request(hpot_pool, conx_params, hpot_logfile);

ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: handling honeypot.  Got here (6)");

        ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "HTTPBL: handle_honeypot_request: make_simple_http_request %s.", (post_was_successful)?"was successful":"failed");

/*
if(conx_params->parsed_uri)
{
    ap_rprintf(r, "<b>ext_connection_data parsed uri:</b><br />\n"
                    "<table>\n"
                    "<tr><td>%s</td><td>%s</td></tr>\n"
                    "<tr><td>%s</td><td>%s</td></tr>\n"
                    "<tr><td>%s</td><td>%s</td></tr>\n"
                    "<tr><td>%s</td><td>%s</td></tr>\n"
                    "<tr><td>%s</td><td>%s</td></tr>\n"
                    "<tr><td>%s</td><td>%s</td></tr>\n"
                    "<tr><td>%s</td><td>%s</td></tr>\n"
                    "<tr><td>%s</td><td>%s</td></tr>\n"
                    "<tr><td>%s</td><td>%s</td></tr>\n"
                    "<tr><td>%s</td><td>%u</td></tr>\n"
                    "<tr><td>%s</td><td>%u</td></tr>\n"
                    "<tr><td>%s</td><td>%u</td></tr>\n"
                    "<tr><td>%s</td><td>%u</td></tr>\n"
                    "</table>\n",

                    "scheme",           conx_params->parsed_uri->scheme,
                    "hostinfo",         conx_params->parsed_uri->hostinfo,
                    "user",             conx_params->parsed_uri->user,
                    "password",         conx_params->parsed_uri->password,
                    "hostname",         conx_params->parsed_uri->hostname,
                    "port_str",         conx_params->parsed_uri->port_str,
                    "path",             conx_params->parsed_uri->path,
                    "query",            conx_params->parsed_uri->query,
                    "fragment",         conx_params->parsed_uri->fragment,
                    "port",             conx_params->parsed_uri->port,
                    "is_initialized",   conx_params->parsed_uri->is_initialized,
                    "dns_looked_up",    conx_params->parsed_uri->dns_looked_up,
                    "dns_resolved",     conx_params->parsed_uri->dns_resolved
               );
}
*/

ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: handling honeypot.  Got here (7)");

        // writing repsonse content to logfile
#if VERBOSITY >= APLOG_DEBUG
        if(hpot_logfile)
        {
            tmpstr  = apr_psprintf(hpot_pool,
                                    "\n"
                                    "%s->%s\n"
                                    "%s->%s\n"
                                    "%s->%s\n"
                                    "%s->%ld\n"
                                    "%s->%s\n"
                                    "%s->%s\n"
                                    "%s->%s\n"
                                    "%s->%ld\n"
                                    "%s->%s\n"
                                    "%s->%s\n"
                                    "%s->%s\n"
                                    "\n",
    
                                    "full_url",         conx_params->full_url,
                                    "post_url",         conx_params->post_url,
                                    "hostname",         (conx_params->parsed_uri)?(conx_params->parsed_uri->hostname):"(null)",
                                    "hostport",         (conx_params->parsed_uri)?(conx_params->parsed_uri->port):(-1),
                                    "http_auth_user",   conx_params->http_auth_user,
                                    "http_auth_pass",   conx_params->http_auth_pass,
                                    "proxy_ip",         conx_params->proxy_ip,
                                    "proxy_port",       conx_params->proxy_port,
                                    "request_data",     conx_params->request_data,
                                    "response_data",    conx_params->response_data,
                                    "response_content", conx_params->response_content);
    
            tmpstr  = apr_psprintf(hpot_pool, "ext_connection_params:\n---------------------------\n%s---------------------------\n", tmpstr);
            outlen  = strlen(tmpstr);
            apr_file_write(hpot_logfile, tmpstr, &outlen);

            tmpstr  = apr_psprintf(hpot_pool, "Content received from POST response:\n\n----------------------\n%s\n----------------------\n\n", conx_params->response_content);
            outlen  = strlen(tmpstr);
            apr_file_write(hpot_logfile, tmpstr, &outlen);
        }
#endif

ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: handling honeypot.  Got here (8)");

        if(post_was_successful &&
           conx_params->response_content &&
           !httpbl_string_matches(conx_params->response_content, ""))
        {
ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: handling honeypot.  Got here (9)");
            // direct dump from the Honey Pot server to the response to the client
            ap_set_content_type(r, "text/html");
            int bytes_sent  = ap_rprintf(r, "%s", conx_params->response_content);
            status = OK;
ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: handle_honeypot_request: returning %s.", (status==OK)?"OK":(status==DECLINED)?"DECLINED":"UNKNOWN");
            apr_pool_destroy(hpot_pool);
            return status;
            // done processing content
        }
        else // if(conx_params->response_content)
        {
ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: handling honeypot.  Got here (10)");
            ap_set_content_type(r, "text/html");
            int bytes_sent  = ap_rprintf(r, DEFAULT_HPOT_ERROR_RESPONSE, (testing_url)?full_testing_url_string:"");
            status = OK;
ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: handle_honeypot_request: returning %s.", (status==OK)?"OK":(status==DECLINED)?"DECLINED":"UNKNOWN");
            apr_pool_destroy(hpot_pool);
            return status;
            // done processing content
        }
ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: handling honeypot.  Got here (11)");


#if VERBOSITY >= APLOG_DEBUG
        if(hpot_logfile_locked == APR_SUCCESS)
            hpot_logfile_unlocked  = apr_file_unlock(hpot_logfile);

        if (hpot_logfile_opened == APR_SUCCESS)
            hpot_logfile_closed    = apr_file_close(hpot_logfile);
#endif
    }
    else // if (prepare_honeypot_request_postdata(r, hpot_pool, this_access_key, conx_params, hpot_logfile) == APR_SUCCESS)
    {
ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: handling honeypot.  Got here (12)");
        ap_set_content_type(r, "text/html");
        int bytes_sent  = ap_rprintf(r, DEFAULT_HPOT_NO_KEY_RESPONSE, (testing_url)?full_testing_url_string:"");
        status = OK;
ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: handle_honeypot_request: returning %s.", (status==OK)?"OK":(status==DECLINED)?"DECLINED":"UNKNOWN");
        apr_pool_destroy(hpot_pool);
        return status;
        // done processing content
    }
ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: handling honeypot.  Got here (13)");

ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: handle_honeypot_request: returning %s.", (status==OK)?"OK":(status==DECLINED)?"DECLINED":"UNKNOWN");

    apr_pool_destroy(hpot_pool);
    return status;
}


/*
 *
 */
static char* get_post_kvp_string (request_rec* r, long max_buffer_length)
{
    char*   read_buffer     = NULL;
    char*   output_buffer   = NULL;
    long    cur_alloc_size  = 0;
    int     alloc_increment = 8192;
    int     bytes_just_read = 0;

    if(r->method_number != M_POST || r->clength > MAX_POST_LENGTH*1024*sizeof(char))              // we can't read this request if it is: (1) not a POST, or (2) too large to pass on
        return 0;

    output_buffer           = apr_pstrdup(r->pool, "");
    read_buffer             = apr_palloc(r->pool, (alloc_increment+1)*sizeof(char));

    ap_setup_client_block(r, REQUEST_CHUNKED_DECHUNK);
    if(ap_should_client_block(r) == 1)
    {
        while((bytes_just_read = ap_get_client_block(r, read_buffer, alloc_increment)) > 0)
        {
/*
            ap_rputs("Reading in buffer...<br>", r);
            ap_rputs(buffer, r);
*/
            cur_alloc_size += bytes_just_read;
            output_buffer   = apr_psprintf(r->pool, "%s%s", output_buffer, read_buffer);
        }
        return output_buffer;
    }
    else
    {
/*
        ap_rputs("Nothing to read...<br>", r);
*/
        return NULL;
    }
}



/*
 *   Apache handler function to check if the request is a honeypot before any other handlers take action.
 *   If a honeypot request is encountered, add the requesting IP to the whitelist and internally redirect the request to the (originally) referring URI.
 *   @return         the status code if the function completes the request or DECLINED if the request requires further processing by this module
 *   @param  r       the request_rec of the Apache request to be handled
 */
static int handle_any_honeypots(request_rec* r)
{
    int             status                          = DECLINED;

#ifdef SHOULD_REQUEST_HONEYPOTS
    ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: entered handle_any_honeypots(...).");

    if(is_request_a_honeypot_token(r))
    {
        ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: is_request_a_honeypot_token(r) returned TRUE.");
        status = handle_honeypot_request(r);
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "HTTPBL: handle_honeypot_request(...) returned %s.", (status==OK)?"OK":(status==DECLINED)?"DECLINED":"UNKNOWN");
    }
    else
    {
        ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: is_request_a_honeypot_token(r) returned FALSE.");
    }
#endif

    return status;
}




/*
 *   is_access_allowed
 *   does the_dir_cfg allow access to the_rbl_hostent?
 *   returns:
 *       -  1 if hostent is NULL (assumed a clean RBL address)
 *       -  HTTPBL_ACTION_ALLOW     if the IP address of the_rbl_hostent matches an entry in the_dir_cfg AND the_dir_cfg entry sets access as "allow"
 *       -  HTTPBL_ACTION_CHALLENGE if the IP address of the_rbl_hostent matches an entry in the_dir_cfg AND the_dir_cfg entry sets access as "challenge"
 *       -  HTTPBL_ACTION_DENY      if the IP address of the_rbl_hostent matches an entry in the_dir_cfg AND the_dir_cfg entry sets access as "deny"
 *       - -1 if the_dir_cfg is NULL
 */
static int is_access_allowed(apr_pool_t* p, httpbl_dir_cfg* the_dir_cfg, httpbl_dir_cfg* the_svr_cfg, char* the_rbl_value, request_rec *r)
{
    if(!the_rbl_value ||                                    // a NULL RBL hostent is clean (not flagged by the RBL); also error-checking
       httpbl_string_matches(the_rbl_value, "") ||
       string_matches_whitelist_rbl_value(the_rbl_value) || // a NULL RBL hostent is whitelisted
       string_matches_cleanlist_rbl_value(the_rbl_value))   // a NULL RBL hostent is cleanlisted
        return HTTPBL_ACTION_ALLOW;

    // init the_return_value with the correct DEFAULT_ACTION for this request
    int the_return_value    = UNSET_INT;
    int i                   = 0;
    int octet4              = getOctetIntFromIPString(p, the_rbl_value, 4);
    int octet3              = getOctetIntFromIPString(p, the_rbl_value, 3);
    int octet2              = getOctetIntFromIPString(p, the_rbl_value, 2);
    int octet1              = getOctetIntFromIPString(p, the_rbl_value, 1);

    for(i=0; i<the_dir_cfg->num_of_rbl_handlers; i++)
    {
        rbl_handler* this_rbl_handler   = the_dir_cfg->the_rbl_handlers[i];
        if (this_rbl_handler) // this rbl_handler != NULL
        {
#if VERBOSITY >= APLOG_DEBUG
            ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: - (9.0.3.5) Testing: [%u] & [%u] = %s",
                                                                                       ((unsigned int)this_rbl_handler->category_bs),
                                                                                              ((unsigned int)octet4),
                                                                                                    (does_bitset_accept_value((unsigned int)this_rbl_handler->category_bs, (unsigned int)octet4))?"YES":"NO");

            ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: - (9.0.4) Iteration #%d;\tTesting: octets: [%d].[%d].[%d].[%d],\tverbs bs: [%"APR_UINT64_T_FMT"] = %s,\tdays: [%d-%d] = %s,\tscore: [%d-%d] = %s,\tcategories bs: [%d] = %s,\ttest = %s",
                                                                                      i,
                                                                                                             octet1,
                                                                                                                  octet2,
                                                                                                                       octet3,
                                                                                                                            octet4,
                                                                                                                                             this_rbl_handler->verb_bs,
                                                                                                                                                   (is_method_accepted_by_rbl_handler(r->method, this_rbl_handler->verb_bs))?"TRUE":"FALSE",
                                                                                                                                                              this_rbl_handler->days_lb,
                                                                                                                                                                 this_rbl_handler->days_ub,
                                                                                                                                                                       (this_rbl_handler->days_lb <= octet2 && this_rbl_handler->days_ub >= octet2)?"TRUE":"FALSE",
                                                                                                                                                                                    this_rbl_handler->score_lb,
                                                                                                                                                                                       this_rbl_handler->score_ub,
                                                                                                                                                                                             (this_rbl_handler->score_lb <= octet3 && this_rbl_handler->score_ub >= octet3)?"TRUE":"FALSE",
                                                                                                                                                                                                                  this_rbl_handler->category_bs,
                                                                                                                                                                                                                       does_bitset_accept_value((unsigned int)this_rbl_handler->category_bs, (unsigned int)octet4)?"TRUE":"FALSE",
                                                                                                                                                                                                                                                         (this_rbl_handler->days_lb <= octet2 && this_rbl_handler->days_ub >= octet2 &&
                                                                                                                                                                                                                                                          this_rbl_handler->score_lb <= octet3 && this_rbl_handler->score_ub >= octet3 &&
                                                                                                                                                                                                                                                         does_bitset_accept_value((unsigned int)this_rbl_handler->category_bs, (unsigned int)octet4)?"TRUE":"FALSE"));
#endif                                                                                                                                                                                                   
            if(is_method_accepted_by_rbl_handler(r->method, this_rbl_handler->verb_bs))             // if the rbl_handler accepts the current request's method...
            {
                if(this_rbl_handler->days_lb<=octet2 && this_rbl_handler->days_ub>=octet2)          // all octet 2 criteria match
                {
                    if(this_rbl_handler->score_lb<=octet3 && this_rbl_handler->score_ub>=octet3)    // all octet 3 criteria match
                    {
                        if((this_rbl_handler->category_bs == 0 && octet4 == 0) ||                   // search engine case (category bitset == 0)
                           does_bitset_accept_value((unsigned int)this_rbl_handler->category_bs, (unsigned int)octet4)) // all octet 4 criteria match
                        {
                            if(httpbl_string_matches(this_rbl_handler->action_string, "deny"))
                                the_return_value    = HTTPBL_ACTION_DENY;
                            else if(httpbl_string_matches(this_rbl_handler->action_string, "allow-xlate-emails"))
                                the_return_value    = HTTPBL_ACTION_ALLOW_XLATE_EMAILS;
                            else if(httpbl_string_matches(this_rbl_handler->action_string, "allow"))
                                the_return_value    = HTTPBL_ACTION_ALLOW;
#ifdef SHOULD_ALLOW_CHALLENGES
                            else if(httpbl_string_matches(this_rbl_handler->action_string, "challenge"))
                                the_return_value    = HTTPBL_ACTION_CHALLENGE;
#endif
#ifdef SHOULD_REQUEST_HONEYPOTS
                            else if(httpbl_string_matches(this_rbl_handler->action_string, "honeypot"))
                                the_return_value    = HTTPBL_ACTION_HONEYPOT;
#endif
                            break; // end the loop, we've found a match
                        } // all octet 4 criteria match
                    }
                }
            }
        } // this_rbl_handler != NULL
    } // for loop

    if(is_set_int(the_return_value))
        return the_return_value;

    for(i=0; i<the_svr_cfg->num_of_rbl_handlers; i++)
    {
        rbl_handler* this_rbl_handler   = the_svr_cfg->the_rbl_handlers[i];
        if (this_rbl_handler) // this rbl_handler != NULL
        {
#if VERBOSITY >= APLOG_DEBUG
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "HTTPBL: - (9.0.4) Iteration #%d;\tTesting: octets: [%d].[%d].[%d].[%d],\tdays: [%d-%d] = %s,\tscore: [%d-%d] = %s,\tcategories bs: [%d] = %s,\tverbs bs: [%"APR_UINT64_T_FMT"] = %s,\ttest = %s",
                                                                                       i,
                                                                                                              octet1,
                                                                                                                   octet2,
                                                                                                                        octet3,
                                                                                                                             octet4,
                                                                                                                                          this_rbl_handler->verb_bs,
                                                                                                                                                (is_method_accepted_by_rbl_handler(r->method, this_rbl_handler->verb_bs))?"TRUE":"FALSE",
                                                                                                                                                                this_rbl_handler->days_lb,
                                                                                                                                                                   this_rbl_handler->days_ub,
                                                                                                                                                                         (this_rbl_handler->days_lb <= octet2 && this_rbl_handler->days_ub >= octet2)?"TRUE":"FALSE",
                                                                                                                                                                                      this_rbl_handler->score_lb,
                                                                                                                                                                                         this_rbl_handler->score_ub,
                                                                                                                                                                                               (this_rbl_handler->score_lb <= octet3 && this_rbl_handler->score_ub >= octet3)?"TRUE":"FALSE",
                                                                                                                                                                                                                    this_rbl_handler->category_bs,
                                                                                                                                                                                                                         does_bitset_accept_value((unsigned int)this_rbl_handler->category_bs, (unsigned int)octet4)?"TRUE":"FALSE",
                                                                                                                                                                                                                                                           (this_rbl_handler->days_lb <= octet2 && this_rbl_handler->days_ub >= octet2 &&
                                                                                                                                                                                                                                                            this_rbl_handler->score_lb <= octet3 && this_rbl_handler->score_ub >= octet3 &&
                                                                                                                                                                                                                                                          does_bitset_accept_value((unsigned int)this_rbl_handler->category_bs, (unsigned int)octet4)?"TRUE":"FALSE"));
#endif                                                                                                                                                                                                    
            if(is_method_accepted_by_rbl_handler(r->method, this_rbl_handler->verb_bs))             // if the rbl_handler accepts the current request's method...
            {
                if(this_rbl_handler->days_lb<=octet2 && this_rbl_handler->days_ub>=octet2)          // all octet 2 criteria match
                {
                    if(this_rbl_handler->score_lb<=octet3 && this_rbl_handler->score_ub>=octet3)    // all octet 3 criteria match
                    {
                        if((this_rbl_handler->category_bs == 0 && octet4 == 0) ||                   // search engine case (category bitset == 0)
                           does_bitset_accept_value((unsigned int)this_rbl_handler->category_bs, (unsigned int)octet4)) // all octet 4 criteria match
                        {
                            if(httpbl_string_matches(this_rbl_handler->action_string, "deny"))
                                the_return_value    = HTTPBL_ACTION_DENY;
                            else if(httpbl_string_matches(this_rbl_handler->action_string, "allow-xlate-emails"))
                                the_return_value    = HTTPBL_ACTION_ALLOW_XLATE_EMAILS;
                            else if(httpbl_string_matches(this_rbl_handler->action_string, "allow"))
                                the_return_value    = HTTPBL_ACTION_ALLOW;
#ifdef SHOULD_ALLOW_CHALLENGES
                            else if(httpbl_string_matches(this_rbl_handler->action_string, "challenge"))
                                the_return_value    = HTTPBL_ACTION_CHALLENGE;
#endif
#ifdef SHOULD_REQUEST_HONEYPOTS
                            else if(httpbl_string_matches(this_rbl_handler->action_string, "honeypot"))
                                the_return_value    = HTTPBL_ACTION_HONEYPOT;
#endif
                            break; // end the loop, we've found a match
                        } // all octet 4 criteria match
                    }
                }
            }
        } // this_rbl_handler != NULL
    } // for loop

    if(is_set_int(the_return_value))
        return the_return_value;
    return get_default_action_from_request(r);
}


/*
    Access Checker - main handler of HTTPBL protected pages.
    This function is called first and determines if an IP should be allowed to view a page.
*/
static int access_checker(request_rec* r)
{
    char*               the_rbl_string  = NULL;
    struct ntt_node*    n               = NULL;

    ap_log_error(APLOG_MARK, APLOG_INFO, 0, NULL, "HTTPBL: ***** NEW REQUEST (\"%s\")... Beginning HTTPBL code *****", r->uri);
    ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: ***** NEW REQUEST (\"%s\")... Beginning HTTPBL code *****", r->uri);
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "HTTPBL: Hash Table Size: '%lu'; Hash Table Size: '%ld'; Current Hash Table Elements Used: '%ld'.", actual_hash_table_size, hit_list->size, hit_list->items);

    int             ret                 = OK;       // set default value for the return status code
    httpbl_dir_cfg* this_dir_cfg        = (httpbl_dir_cfg*)ap_get_module_config(r->per_dir_config, &httpbl_module);
    httpbl_dir_cfg* this_svr_cfg        = (httpbl_dir_cfg*)ap_get_module_config(r->server->module_config, &httpbl_module);

    if(this_dir_cfg || this_svr_cfg)
    {
        // return immediately if httpbl is not enabled by the cfg structs
        ret = is_httpbl_rbl_enabled_for_this_request(r, this_dir_cfg, this_svr_cfg);
        if(ret != OK)
        {
            ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: ***** mod_httpbl not enabled... exiting mod_httpbl (uri %s) *****", r->uri);
            return ret;
        }

        // return immediately if the cfg struct(s) direct(s) us to exempt this request
        ret = is_httpbl_rbl_exempted_for_this_request(r, this_dir_cfg, this_svr_cfg);
        if(ret != OK)
        {
            ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: ***** Request exempted... exiting mod_httpbl (uri %s) *****", r->uri);
            return ret;
        }

        int tok_handled = handle_any_whitelist_tokens(r);   // parse URI for whitelist tokens
        if(tok_handled != DECLINED)                         // if whitelist tokens were found
        {
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "HTTPBL: ***** Whitelist token handled... exiting mod_httpbl (uri %s) *****", r->uri);
            return tok_handled;                             // return the appropriate status code (200 or 30x)
        }
    }

#ifdef SHOULD_CACHE
    // update whitelist from cache-file if necessary
    if(is_whitelist_outofdate())
        if(!unserialize_whitelist_from_file(r->pool)) // populate the existing whitelist from serialized-whitelist file
            ap_log_rerror(APLOG_MARK, APLOG_NOTICE, 0, r, "HTTPBL: Error loading whitelist from cachefile (%s)", whitelist_filepath);
    serialize_whitelist_to_file(r->pool);       // then resave the new, merged whitelist back to the cache-file

    // update hitlist from cache-file
    if(!unserialize_hitlist_from_file(r->pool))
        ap_log_rerror(APLOG_MARK, APLOG_NOTICE, 0, r, "HTTPBL: Error loading hitlist from cachefile (%s)", hitlist_filepath);
    serialize_hitlist_to_file(r->pool);         // then resave the new, merged hitlist back to the cache-file
#endif

    if(hit_list)
    {

        ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: Request -> URI: \"%s\"", r->uri);
        ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: Request -> FLN: \"%s\"", r->filename);

        // First see if the IP itself is in the hitlist
        n                                   = ntt_find(hit_list, r->connection->remote_ip);

        if(!n || (n && ntt_is_expired(n))) // if the requesting IP does not have an active record in the hitlist
        {
            char*   server_to_query             = get_rbl_domain(r, this_dir_cfg, this_svr_cfg, apr_pstrdup(r->pool, DEFAULT_RBL_SERVER_DOMAIN));
            ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: get_rbl_domain returned: %s", server_to_query);

            // do RBL lookup and get the C-string of the returned IP (i.e. "127.0.0.1")
            the_rbl_string                      = check_via(r, server_to_query);
            ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: check_via returned \"%s\"", the_rbl_string);

            if(the_rbl_string == NULL || httpbl_string_matches(the_rbl_string, ""))
                the_rbl_string                      = get_cleanlist_rbl_value(r->pool);
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "HTTPBL: RBL Value (in access checker): \"%s\"", the_rbl_string);

            // insert new RBL lookup object into the hitlist ntt
            hitlist_insert(r->pool, r->connection->remote_ip, the_rbl_string, r);
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "HTTPBL: called hitlist_insert (in access checker): \"%s\"", the_rbl_string);

            n   = ntt_find(hit_list, r->connection->remote_ip);

            if(n)
                ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: n found: \"%s\" => \"%s\" (%p).", n->key, n->rbl_value, n);
            else
                ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: n not found.");
        }

        if (!n) // if there was no hit_list insert
            return DECLINED; // exiting RBL part of mod_httpbl

        // if the rbl_value is NULL or an empty string... set the RBL value to be the sentinel value
        if(!(n->rbl_value) || httpbl_string_matches(n->rbl_value, ""))
            n->rbl_value    = get_cleanlist_rbl_value(r->pool);                  // set the rbl_value to our special whitelist value (for data integrity while storing)

        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "HTTPBL: - (0.1.1) Stored RBL value on IP (in NTT): %s => %s", r->connection->remote_ip, (n->rbl_value)?(string_matches_whitelist_rbl_value(n->rbl_value)?"WHITELISTED_VALUE":string_matches_cleanlist_rbl_value(n->rbl_value)?"CLEANLIST_VALUE":n->rbl_value):"NULL");
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "HTTPBL: - (0.1.1.0) httpbl_string_matches(\"%s\", WHITELIST_VALUE): %s", n->rbl_value, string_matches_whitelist_rbl_value(n->rbl_value)?"TRUE":"FALSE");
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "HTTPBL: - (0.1.1.1) httpbl_string_matches(\"%s\", CLEANLIST_VALUE): %s", n->rbl_value, string_matches_cleanlist_rbl_value(n->rbl_value)?"TRUE":"FALSE");

        if(string_matches_whitelist_rbl_value(n->rbl_value) || is_whitelisted(r->connection->remote_ip))
        {
            whitelist_insert(r->pool, r->connection->remote_ip);    // insert/update whitelist record for this requesting IP
            ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: - (0.1.1.2) IP %s is whitelisted... page request ALLOWed", r->connection->remote_ip);
            return DECLINED;
        }
        if(string_matches_cleanlist_rbl_value(n->rbl_value))
        {
            ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: - (0.1.1.3) IP %s is cleanlisted... page request ALLOWed", r->connection->remote_ip);
            return DECLINED;
        }
        else    // RBL value is something other than clean/whitelist-value
        {
            ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "Incoming request from: IP: '%s';      PID: '%ld';      RBL: '%s';      Found in resident memory", r->connection->remote_ip, (long)getpid(), n->rbl_value);
        }
        int access_value    = is_access_allowed(r->pool, this_dir_cfg, this_svr_cfg, n->rbl_value, r);
        ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: - (0.1.1.4) access_value: %d", access_value);
        ret                 = handlePageAccessAction(r, access_value, n->rbl_value, this_dir_cfg);
    } // if(hit_list)
    else
    {
        ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: hit_list is NULL");
    }
    // END HTTPBL Code

ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: %s", (ret==OK)?"OK":(ret==DECLINED)?"DECLINED":(ret==HTTP_FORBIDDEN)?"HTTP_FORBIDDEN":"UNKNOWN");

    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "HTTPBL: -------------");
    return ret;
}





/*
    Function to handle the processing before and after the RBL DNS lookup request.
    Mostly borrowed from mod_access_rbl.
    May be able to handle IPv6 addresses, but works fine for IPv4.
    Requires "gethostbyname(...)" [POSIX function?  Cross-platform supported?]
*/
static char* check_via(request_rec *r, const char* via_list)
{
    char hb[100];
    char* ha, *s, *sb, *sc;
    httpbl_dir_cfg* this_dir_cfg        = (httpbl_dir_cfg*)ap_get_module_config(r->per_dir_config, &httpbl_module);
    httpbl_dir_cfg* this_svr_cfg        = (httpbl_dir_cfg*)ap_get_module_config(r->server->module_config, &httpbl_module);
    char*           access_key          = get_access_key_for_this_request(r, this_dir_cfg, this_svr_cfg);

    if(!via_list)
    {
        ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: check_via(...) returning NULL; \"domain to use\" is NULL");
        return NULL;
    }

    ha = r->connection->remote_ip; // get the requesting IP from the request_rec

    // take the network address, convert to ascii, reverse the order of
    // the numbers, tack on the rbl-style list to search, add a period
    // at the end if there isn't one already, and see if it's listed
    // perhaps caching results would be a good idea

    s       = ha + strlen(ha);
    sb      = hb;
    while(--s!=ha)
    {
        if(*s=='.')
        {
            sc      = s;
            while(*++sc!='.' && *sc)
                *sb++   = *sc;
            *sb++   = '.';
        }
    }
    sc      = s;
    while(*sc!='.' && *sc)
        *sb++   = *sc++;
    *sb++   = '.';
    sc      = (char*)via_list;
    while( (*sb++=*sc++) )
        ;
    if(sb[-2]!='.')
    {
        sb[-1]  = '.';
        *sb     = '\0';
    }

    char*           full_rbl_req    = NULL;

    if(access_key)
        full_rbl_req                    = apr_psprintf( r->pool, "%s.%s", access_key, hb );
    else
        return NULL;

    struct hostent* returned        = apr_palloc( r->pool, sizeof(struct hostent));
    returned                        = (struct hostent*)gethostbyname(full_rbl_req);
    char*           theReturnValue  = (char*)apr_palloc( r->pool, 16*sizeof(char) );

    if(returned)
    {
        struct in_addr  h_addr;                 // internet address
        char*           ip_string       = NULL;
        theReturnValue                  = apr_psprintf( r->pool, "%s", inet_ntop(returned->h_addrtype, returned->h_addr_list[0], theReturnValue, 15*sizeof(char)) );

        ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: looked up: '%s' @ '%s'; returned: \"%s\" (in check_via)", ha, returned->h_name, (theReturnValue != NULL)?theReturnValue:"NULL");
    }
    else
    {
        ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: RBL look-up returned NULL (in check_via)");
        theReturnValue  = NULL;
    }

    return theReturnValue;
}





/*
    intercept 404 error coded responses and record them to file
    @return         Apache request handler return code (i.e. HTTP_FORBIDDEN, DECLINED, OK, etc.)
*/
static int handle_404_recording(request_rec *r)
{
    struct ext_connection_params*   conx_params             = NULL;
    apr_status_t                    datafile_lock_acquired  = 1;
    apr_status_t                    rv                      = 1;
    apr_file_t*                     fp                      = NULL;
    apr_file_t*                     FOF_logfile             = NULL;
    char*                           this_access_key         = NULL;
    int                             is_a_404_req            = 0;
    int                             is_a_post_req           = 0;

    char*                           tmpstr                  = NULL;
    apr_size_t                      outlen;

    ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: Beginning HTTPBL 404 code...");
    ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: is 404 capture enabled? ... %s", (is_enabled_int(g_FOF_enable_404_capture))?"TRUE":"FALSE");
    ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: is POST  capture enabled? ... %s", (is_enabled_int(g_FOF_enable_POST_capture))?"TRUE":"FALSE");
    ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: 404 cache filepath? ... %s", g_FOF_cache_data_filepath);

    is_a_404_req    = (r->status == 404)?1:0;
    is_a_post_req   = (r->method_number == M_POST)?1:0;

    // immediately exit if 404 capturing is explicitly disabled
    if(is_set_int(g_FOF_enable_404_capture) && !is_enabled_int(g_FOF_enable_404_capture))
        return DECLINED;

    // Only record GETs for 404s and POSTs (if the g_FOF_enable_POST_capture is enabled)
    // any other combination of Method should be ignored and not processed
    if(r->method_number==M_POST && !is_enabled_int(g_FOF_enable_POST_capture))
        return DECLINED;
    else if(r->method_number!=M_GET)
        return DECLINED;

    // We've already got a file of some kind or another
    if(r->proxyreq || (r->finfo.filetype != 0))
        return DECLINED;

    // This is a sub request - don't mess with it
    if(!ap_is_initial_req(r))
        return DECLINED;

    ap_log_rerror(APLOG_MARK, APLOG_NOTICE, 0, r, "HTTPBL: Recording 404/POST request...");

    char* req_meth          = apr_palloc(r->pool,  256*sizeof(char));
    char* req_timestamp     = apr_palloc(r->pool,  256*sizeof(char));
    char* req_host          = apr_palloc(r->pool,  256*sizeof(char));
    char* req_uri           = apr_palloc(r->pool,  256*sizeof(char));
    char* req_ip            = apr_palloc(r->pool,  256*sizeof(char));
    char* req_ua            = apr_palloc(r->pool,  256*sizeof(char));
    char* req_ref           = apr_palloc(r->pool,  256*sizeof(char));
    char* req_sc            = apr_palloc(r->pool,    4*sizeof(char));
    char* req_full_req      = apr_palloc(r->pool, 4096*sizeof(char));

    req_full_req    = apr_psprintf(r->pool,
                                   "%s: %s ... %s: %s ... %s: %s ... %s: %s ... %s: %s ... %s: %s ... %s: %s ... %s: %s ... %s: %s ... %s: %s ... %s: %d.",
                                    "METHOD", r->method,
                                               "SCHEME", r->parsed_uri.scheme,
                                                          "HOSTINFO", r->parsed_uri.hostinfo,
                                                                     "USER", r->parsed_uri.user,
                                                                                "PASSWORD", r->parsed_uri.password,
                                                                                           "USERNAME", r->parsed_uri.hostname,
                                                                                                      "PORT", r->parsed_uri.port_str,
                                                                                                                 "PATH", r->parsed_uri.path,
                                                                                                                            "QUERY", r->parsed_uri.query,
                                                                                                                                       "FRAGMENT", r->parsed_uri.fragment,
                                                                                                                                                  "STATUS", r->status); 

    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "HTTPBL: (8.0.0.0).");
    apr_time_t  time_now                = apr_time_now();

    // convert the request timestamp to a string
    apr_time_t  rawtime     = apr_time_now();
    apr_rfc822_date(req_timestamp, rawtime);

#ifdef IS_TEST_MODE
    apr_cpystrn(req_meth,       r->method!=NULL?r->method:"",                                                                     254);   // Request Method (i.e. GET, POST, HEAD, etc.) String
    apr_cpystrn(req_host,       r->server->server_hostname!=NULL?r->server->server_hostname:"",                                   254);   // Request Host String
    apr_cpystrn(req_uri,        r->the_request!=NULL?r->the_request:"",                                                           254);   // Request UI String
    apr_cpystrn(req_ip,         r->connection->remote_ip,                                                                         254);   // Requesting IP
    apr_cpystrn(req_ua,         apr_table_get(r->headers_in, "User-Agent")!=NULL?apr_table_get(r->headers_in, "User-Agent"):"",   254);   // Get the User Agent string
    apr_cpystrn(req_ref,        apr_table_get(r->headers_in, "referer")!=NULL?apr_table_get(r->headers_in, "referer"):"",         254);   // Get the Referer string
    apr_cpystrn(req_timestamp,  req_timestamp?req_timestamp:"",                                                                   64);    // Request UI String
    apr_cpystrn(req_sc,         (0 <= req_sc && req_sc <= 1000)?apr_ltoa(r->pool, req_sc):"",                                     4)
#else
    apr_cpystrn(req_meth,       yahoo_urlencode(r->pool, r->method!=NULL?r->method:""),                                                                     254);   // Request Method (i.e. GET, POST, HEAD, etc.) String
    apr_cpystrn(req_host,       yahoo_urlencode(r->pool, r->server->server_hostname!=NULL?r->server->server_hostname:""),                                   254);   // Request Host String
    apr_cpystrn(req_uri,        yahoo_urlencode(r->pool, r->the_request!=NULL?r->the_request:""),                                                           254);   // Request UI String
    apr_cpystrn(req_ip,         yahoo_urlencode(r->pool, r->connection->remote_ip),                                                                         254);   // Requesting IP
    apr_cpystrn(req_ua,         yahoo_urlencode(r->pool, apr_table_get(r->headers_in, "User-Agent")!=NULL?apr_table_get(r->headers_in, "User-Agent"):""),   254);   // Get the User Agent string
    apr_cpystrn(req_ref,        yahoo_urlencode(r->pool, apr_table_get(r->headers_in, "referer")!=NULL?apr_table_get(r->headers_in, "referer"):""),         254);   // Get the Referer string
    apr_cpystrn(req_timestamp,  yahoo_urlencode(r->pool, req_timestamp?req_timestamp:""),                                                                   64);    // Request UI String
    apr_cpystrn(req_sc,         (0 <= r->status && r->status <= 1000)?yahoo_urlencode(r->pool, apr_ltoa(r->pool, r->status)):"",                            4);
#endif


#ifdef SHOULD_CACHE
    if(unserialize_404_cache_meta(r->pool)<1) // update this child process's memory from the 404 cache metafile
        ap_log_rerror(APLOG_MARK, APLOG_NOTICE, 0, r, "HTTPBL: handle_404_recording: 404 cache meta file (%s) could not be opened.", g_FOF_cache_meta_filepath);

    // write the 404 info to the 404 cache file
    if(g_FOF_cache_data_filepath)
    {
        rv = apr_file_open(&fp, g_FOF_cache_data_filepath, APR_WRITE | APR_APPEND | APR_CREATE, APR_OS_DEFAULT, r->pool);
        if(rv == APR_SUCCESS)
        {
            apr_status_t    file_closed_status   = 1;
                ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: 404 cachefile opened [%sSUCCESSFULLY].", (rv==APR_SUCCESS)?"":"UN");

            datafile_lock_acquired  = apr_file_lock(fp, APR_FLOCK_EXCLUSIVE);
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "HTTPBL: 404 cachefile locked [%sSUCCESSFULLY].", (datafile_lock_acquired==APR_SUCCESS)?"":"UN");
#endif

            tmpstr  = apr_psprintf(r->pool,
                                        LINE_SEPARATOR  FIELD_SEPARATOR "method%u=%s" FIELD_SEPARATOR "host%u=%s" FIELD_SEPARATOR "uri%u=%s" FIELD_SEPARATOR "ip%u=%s" FIELD_SEPARATOR "ua%u=%s" FIELD_SEPARATOR "ref%u=%s" FIELD_SEPARATOR "sc%u=%s" FIELD_SEPARATOR "time%u=%s",
                                        g_FOF_cur_count+1,
                                        req_meth,
                                        g_FOF_cur_count+1,
                                        req_host,
                                        g_FOF_cur_count+1,
                                        req_uri,
                                        g_FOF_cur_count+1,
                                        r->connection->remote_ip,
                                        g_FOF_cur_count+1,
                                        req_ua,
                                        g_FOF_cur_count+1,
                                        req_ref,
                                        g_FOF_cur_count+1,
                                        req_sc,
                                        g_FOF_cur_count+1,
                                        req_timestamp);
            outlen = strlen(tmpstr);
#ifndef SHOULD_CACHE
            if(g_FOF_cache_string)
            {
                g_FOF_cache_string      = realloc(g_FOF_cache_string, (g_FOF_cache_string_l + outlen)*sizeof(char));
                g_FOF_cache_string_l   += outlen;
            }
            else
            {
                g_FOF_cache_string      = malloc(outlen * sizeof(char));
                g_FOF_cache_string_l    = outlen;
            }
            g_FOF_cur_count++; // increment the current counter
#else
            apr_file_write(fp, tmpstr, &outlen);

            g_FOF_cur_count++; // increment the current counter

            if(datafile_lock_acquired==APR_SUCCESS)
            {
                datafile_lock_acquired  = apr_file_unlock(fp);
                ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "HTTPBL: handle_404_recording: 404 cachefile unlocked [%sSUCCESSFULLY].", (datafile_lock_acquired==APR_SUCCESS)?"":"UN");
            }
    
            // if rv != APR_SUCCESS, writes failed
            file_closed_status  = apr_file_close(fp);
            ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: handle_404_recording: 404 cachefile closed [%sSUCCESSFULLY].", (file_closed_status==APR_SUCCESS)?"":"UN");

            if(serialize_404_cache_meta(r->pool)<1) // update this child process's memory from the 404 cache metafile
                ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: handle_404_recording: 404 cache meta file (%s) could not be opened for serialization.", g_FOF_cache_meta_filepath);
        }
        else // rv == APR_SUCCESS
        {
            ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: handle_404_recording: 404 cache data file (%s) could not be opened for serialization.", g_FOF_cache_data_filepath);
        }
    } // if(g_FOF_cache_data_filepath)
#endif

#if VERBOSITY >= APLOG_DEBUG
    char*       todays_date         = apr_palloc(r->pool,  APR_RFC822_DATE_LEN*sizeof(char));
    apr_time_t  the_time_now        = apr_time_now();
    apr_time_t  last_post_time_secs = apr_time_sec(g_FOF_last_post_time);
    apr_time_t  cur_time_secs       = apr_time_sec(apr_time_now());
    apr_rfc822_date(todays_date, the_time_now);

    ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: FOF_config values: min_count: %d; cur_count: %d, last post timestamp: %"APR_TIME_T_FMT", cur timestamp: %"APR_TIME_T_FMT"; secs elapsed since last POST: %"APR_TIME_T_FMT".",
                                                                                       g_FOF_min_count,
                                                                                                      g_FOF_cur_count,
                                                                                                                               g_FOF_last_post_time,
                                                                                                                                                                 the_time_now,
                                                                                                                                                                                                                 (apr_time_sec(the_time_now)-apr_time_sec(g_FOF_last_post_time)));
#endif

    if(!cacheFileIsSmallEnoughToPost(r->pool))  // if the 404 cache is too large to be sent, delete the cache file, the meta file, and reset the variables in RAM
        clear_httpbl_404_cache(r->pool);

    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "HTTPBL: 404 test values:");
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "HTTPBL: 404 areEnough404sToPost()       (...): %s", (areEnough404sToPost())?"TRUE":"FALSE");
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "HTTPBL: 404 enoughTimeHasPassedToPost   (...): %s", (enoughTimeHasPassedToPost(r->pool))?"TRUE":"FALSE");
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "HTTPBL: 404 cacheFileIsSmallEnoughToPost(...): %s", (cacheFileIsSmallEnoughToPost(r->pool))?"TRUE":"FALSE");

    // if there are enough 404s cached or if sufficient time has passed, do a post
    if(areEnough404sToPost()                &&
       enoughTimeHasPassedToPost(r->pool)   &&
       cacheFileIsSmallEnoughToPost(r->pool))
    {
        apr_pool_t*         FOF_pool            = NULL;
        conx_params                             = (struct ext_connection_params*)apr_pcalloc(r->pool, 1 * sizeof(struct ext_connection_params));                                                     
        this_access_key                         = get_access_key_for_this_request(r, NULL, NULL);

        apr_pool_create(&FOF_pool, r->pool);

        int post_response   = /*do_post(r)*/0;


        // record the fact that we posted to a logfile
        if(g_FOF_cache_data_filepath)
        {
            char*   FOF_logfile_filepath    = apr_psprintf(FOF_pool, "%shttpbl_404_submissions.log", get_log_dir());
            rv = apr_file_open(&FOF_logfile, FOF_logfile_filepath, APR_WRITE | APR_APPEND | APR_CREATE, APR_OS_DEFAULT, FOF_pool);

            if(rv == APR_SUCCESS)
            {
                datafile_lock_acquired  = apr_file_lock(FOF_logfile, APR_FLOCK_EXCLUSIVE);

                tmpstr  = apr_psprintf(FOF_pool, "\n^^^^^^^^^^ %s (%d) ^^^^^^^^^^\n", g_FOF_post_url, post_response);
                outlen = strlen(tmpstr);
                apr_file_write(FOF_logfile, tmpstr, &outlen);


                if (prepare_404_submission_postdata(r, r->pool, this_access_key, conx_params, FOF_logfile) == APR_SUCCESS)
                {
                    int post_was_successful = 0;

                    conx_params->full_url                           = NULL;
                    conx_params->debug_logfile                      = FOF_logfile;
        //            conx_params->proxy_ip                           = g_FOF_post_proxy_ip;
        //            if(g_FOF_post_proxy_port)
        //                conx_params->proxy_port                         = atol(g_FOF_post_proxy_port);
                    conx_params->http_auth_user                     = g_FOF_post_httpauth_un;
                    conx_params->http_auth_pass                     = g_FOF_post_httpauth_pw;
        
                    conx_params->full_url                           = apr_pstrdup(r->pool, g_FOF_post_url);
        
                    // send the cache data as a POST and get the response text
                    post_was_successful = make_simple_http_request(FOF_pool, conx_params, FOF_logfile);
        
#if VERBOSITY >= APLOG_DEBUG
                    if(FOF_logfile)
                    {
                        tmpstr  = apr_psprintf(FOF_pool,
                                                "\n"
                                                "%s->%s\n"
                                                "%s->%s\n"
                                                "%s->%s\n"
                                                "%s->%ld\n"
                                                "%s->%s\n"
                                                "%s->%s\n"
                                                "%s->%s\n"
                                                "%s->%ld\n"
                                                "%s->%s\n"
                                                "%s->%s\n"
                                                "%s->%s\n"
                                                "\n",
                
                                                "full_url",         conx_params->full_url,
                                                "post_url",         conx_params->post_url,
                                                "hostname",         conx_params->hostname,
                                                "hostport",         conx_params->hostport,
                                                "http_auth_user",   conx_params->http_auth_user,
                                                "http_auth_pass",   conx_params->http_auth_pass,
                                                "proxy_ip",         conx_params->proxy_ip,
                                                "proxy_port",       conx_params->proxy_port,
                                                "request_data",     conx_params->request_data,
                                                "response_data",    conx_params->response_data,
                                                "response_content", conx_params->response_content);
                
                
                        tmpstr  = apr_psprintf(FOF_pool, "ext_connection_params:\n---------------------------\n%s---------------------------\n", tmpstr);
                        outlen  = strlen(tmpstr);
                        apr_file_write(FOF_logfile, tmpstr, &outlen);
                    }
#endif
        
                    if(post_was_successful)
                    {
                        ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: 404 cache data file (%s) was posted to (%s).", g_FOF_cache_data_filepath, g_FOF_post_url);

#if VERBOSITY >= APLOG_DEBUG
                        if(FOF_logfile)
                        {
                            tmpstr  = apr_psprintf(FOF_pool, "\n^^^^^^^^^^ 404 submission successful ^^^^^^^^^^\n\n");
                            outlen  = strlen(tmpstr);
                            apr_file_write(FOF_logfile, tmpstr, &outlen);
                        }
#endif
                    }
                    else
                    {
#if VERBOSITY >= APLOG_DEBUG
                        if(FOF_logfile)
                        {
                            tmpstr  = apr_psprintf(FOF_pool, "\n^^^^^^^^^^ 404 submission failed ^^^^^^^^^^\n\n");
                            outlen = strlen(tmpstr);
                            apr_file_write(FOF_logfile, tmpstr, &outlen);
                        }
#endif
                    }

                }
        
                if(post_response > 0) // if the post was successful
                {
                    ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: handle_404_recording: 404 POST successful.  Clearing 404 cache...");
                    clear_httpbl_404_cache(r->pool);
                }
                else // unsuccessful post; otherwise increment the tries counter and only clear the cache if cur_tries is more than max_tries
                {
                    g_FOF_cur_tries++;
                    if(enoughPostTriesHaveElapsed())
                    {
                        ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: handle_404_recording: 404 POST failed after max_retries (%d) attempts.  Clearing 404 cache...", g_FOF_max_retries);
                        clear_httpbl_404_cache(r->pool);
                    }
                }
        
#ifdef SHOULD_CACHE
                if(serialize_404_cache_meta(r->pool)<1) // update the 404 cache metafile from this child process's RAM
                {
                    ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: handle_404_recording: 404 cache meta file (%s) could not be opened.", g_FOF_cache_meta_filepath);
                }
                else
                {
                    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "HTTPBL: handle_404_recording: clearing 404 cache (1)...");
                    clear_httpbl_404_cache(r->pool);
                }
#endif

                // close down the logfile
                if(datafile_lock_acquired)
                    apr_file_unlock(FOF_logfile);
                apr_file_close(FOF_logfile);
            }
            else // if(apr_file_open(...) == APR_SUCCESS)
            {
                ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: handle_404_recording: 404 cache data file (%s) could not be opened.", g_FOF_cache_data_filepath);
            }
        }

        apr_pool_destroy(FOF_pool);
    }
    else // if (/*should POST criteria passed*/)
    {
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "HTTPBL: handle_404_recording: skipping 404 POST.");
#ifdef SHOULD_CACHE
        int unserialize_returned    = unserialize_404_cache_meta(r->pool);
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "HTTPBL: handle_404_recording: 404 cache meta file unserialization returned: %d.", unserialize_returned);

        if(unserialize_returned == 1) // update this child process's RAM from the 404 cache metafile
        {
            ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: handle_404_recording: 404 cache meta file (%s) could not be opened.", g_FOF_cache_meta_filepath);
        }
        else
        {
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "HTTPBL: handle_404_recording: clearing 404 cache file (2)...");
            clear_httpbl_404_cache(r->pool);
        }

        int serialize_returned  = serialize_404_cache_meta(r->pool);
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "HTTPBL: handle_404_recording: 404 cache meta file serialization returned: %d.", serialize_returned);

        if(serialize_returned == 1) // update the 404 cache metafile from this child process's RAM
        {
            ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "HTTPBL: handle_404_recording: 404 cache meta file (%s) could not be opened.", g_FOF_cache_meta_filepath);
        }
        else
        {
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "HTTPBL: handle_404_recording: clearing 404 cache file (3)....");
            clear_httpbl_404_cache(r->pool);
        }
#endif
        return DECLINED;
    }

    return DECLINED; // allow Apache to continue handling this request
}


/*
    Assert - @param at_sign must point to a part of @param whole_string before the first occurance of a '\0'
*/
static int is_inside_html_tag(apr_pool_t* pool, const char* whole_string, const char* at_sign)
{
    char*   searchable_str  = apr_pstrndup(pool, whole_string, at_sign-whole_string);
    char*   last_open_tag   = strrchr(searchable_str, '<');
    char*   last_close_tag  = strrchr(searchable_str, '>');

    if(last_open_tag || last_close_tag)
    {
        if(!last_open_tag)
            return 0;
        else if(!last_close_tag)
            return 1;

        if(last_open_tag > last_close_tag)
            return 1;
        else
            return 0;
    }
    else
    {
        return -1;
    }
}

static int charcnt(const char* subject, char char_to_look_for)
{
    int     the_count       = 0;
    char*   next_string     = (char*)subject;

    while(next_string && strchr(next_string, char_to_look_for))
    {
        the_count++;
        next_string = strchr(next_string, char_to_look_for)+1;
    }

    return the_count;
}

static apr_status_t replace_email_filter(ap_filter_t* f, apr_bucket_brigade* bb, int inside_tags, char* with_text, char* matching_regex)
{
    pcre*           re                  = NULL;
    apr_file_t*     fp                  = NULL;
    const char*     error;
    int*            ovector             = NULL;
    int             erroffset           = 0;
    int             rc                  = 1;
    int             i                   = 0;
    int             logfile_is_open     = 0;
    apr_status_t    file_lock_acquired  = 1;
    apr_status_t    rv                  = 1;
    apr_size_t      outlen              = 0;
    char*           tmpstr              = NULL;

    tmpstr                              = (char*)calloc(4096, sizeof(char));

    if(httpbl_string_matches(f->r->content_type, "text/html")) // we are only going to filter HTML documents
        return DECLINED;

    // record the fact that we posted to a logfile
    if(g_httpbl_replace_emails_log_filepath)
    {
        rv = apr_file_open(&fp, g_httpbl_replace_emails_log_filepath, APR_WRITE | APR_APPEND | APR_CREATE, APR_OS_DEFAULT, f->r->pool);

        if(rv == APR_SUCCESS)
        {
            file_lock_acquired  = apr_file_lock(fp, APR_FLOCK_EXCLUSIVE);

            logfile_is_open = 1;

            tmpstr  = apr_psprintf(f->r->pool, "\n^^^^^^^^^^ Filter run (%s) ^^^^^^^^^^\n", "replace_emails");
            outlen = strlen(tmpstr);
            apr_file_write(fp, tmpstr, &outlen);
    
            tmpstr  = apr_psprintf(f->r->pool, "\n- Handling request: \"%s\"\n", f->r->unparsed_uri);
            outlen = strlen(tmpstr);
            apr_file_write(fp, tmpstr, &outlen);
        }
    }

    er_struct*    ctx     = f->ctx;
    apr_bucket*     e;
    apr_pool_t*     p       = f->r->pool;
    char*           word1   = NULL,
        *           word2   = NULL;
    if(ctx == NULL)
    {
        f->ctx      = ctx = apr_pcalloc(f->r->pool, sizeof(*ctx));
        ctx->bb     = apr_brigade_create(f->r->pool, bb->bucket_alloc);
    }

    for(e = APR_BRIGADE_FIRST(bb); e != APR_BRIGADE_SENTINEL(bb); e = APR_BUCKET_NEXT(e))
    {
        const char* str;
        apr_size_t  len;

        if(APR_BUCKET_IS_EOS(e) || APR_BUCKET_IS_FLUSH(e)) // bucket is the last of the brigade
        {
            APR_BUCKET_REMOVE(e);
            APR_BRIGADE_INSERT_TAIL(ctx->bb, e);
            ap_pass_brigade(f->next, ctx->bb);

            if(rv == APR_SUCCESS)
            {
                if(file_lock_acquired)
                    apr_file_unlock(fp);
                apr_file_close(fp);
            }

            return APR_SUCCESS;
        }
        apr_bucket_read(e, &str, &len, APR_NONBLOCK_READ);

        if(str && !httpbl_string_matches(str, ""))
        {
            char*   next_str                = (char*)str;                      // a pointer to keep track of the beginning of the string to search
            
            while(next_str)
            {
                if(!strcmp(next_str, ""))
                    break; // nothing to process; break out of the "while(next_str)" loop

                int     num_of_possible_matches = charcnt(next_str, '@');
                int     size_of_ovec            = num_of_possible_matches*3*sizeof(int);
                ovector                         = apr_palloc(f->r->pool, size_of_ovec);

                re = pcre_compile(
                                    matching_regex, // the pattern
                                    0,                    // default options
                                    &error,               // for error message
                                    &erroffset,           // for error offset
                                    NULL);                // use default character tables

                if (re == NULL)
                {
                    tmpstr  = apr_psprintf(f->r->pool, "mod_httpbl: PCRE compilation failed at offset %d: %s\n", erroffset, error);
                    outlen = strlen(tmpstr);
                    apr_file_write(fp, tmpstr, &outlen);

                    if(file_lock_acquired)
                        apr_file_unlock(fp);
                    apr_file_close(fp);

                    return 1;
                }

                rc = pcre_exec(
                                    re,                   // the compiled pattern
                                    NULL,                 // no extra data - we didn't study the pattern
                                    next_str,             // the subject string
                                    (int)strlen(next_str),// the length of the subject
                                    0,                    // start at offset 0 in the subject
                                    0,                    // default options
                                    ovector,              // output vector for substring information
                                    size_of_ovec);        // number of elements in the output vector
                if (rc < 0)
                {
                    switch(rc)
                    {
                    case PCRE_ERROR_NOMATCH:
                            // we probably don't need to log an error if there is no match
                            break;
                        default:
                            tmpstr  = apr_psprintf(f->r->pool, "Matching error %d\n", rc);
                            outlen = strlen(tmpstr);
                            apr_file_write(fp, tmpstr, &outlen);
                            break;
                    }
                    ap_fprintf(f->next, ctx->bb, "%s", next_str);
                    next_str    = NULL;
                }
                else if (rc == 0)
                {
                    int effective_size  = size_of_ovec/3;
                    tmpstr  = apr_psprintf(f->r->pool, "mod_httpbl (in function 'replace_email_filter'): ovector only has room for %d captured substrings\n", effective_size - 1);
                    outlen = strlen(tmpstr);
                    apr_file_write(fp, tmpstr, &outlen);
                }
                else
                {
                    char*   substring_start             = NULL;
                    int     substring_length            = -1;
                    char*   beginning_of_next_result    = NULL;
                    char*   substring_before_match      = NULL;
                    char*   substring_after_match       = NULL;

                    for (i=0; i<rc; ++i)
                    {
                        if(i == 0) // we only care about the large match (not any backreferences within the large match)
                        {
                            substring_start             = next_str + ovector[2*i];
                            substring_length            = ovector[2*i+1] - ovector[2*i];
                            substring_before_match      = apr_pstrndup(f->r->pool, next_str, substring_start-next_str);
                            substring_after_match       = substring_start+substring_length;
    
                            beginning_of_next_result    = substring_after_match;
    
                            if (inside_tags == 1 && is_inside_html_tag(f->r->pool, next_str, strchr(substring_start, '@')))
                            {
                                ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, f->r, "REPLACE_EMAIL: 0.0.1.5.2.5: Replacing \"%s\" with \"%s\".\n", apr_pstrndup(f->r->pool, substring_start, substring_length), with_text);
                                if(logfile_is_open)
                                {
                                    tmpstr  = apr_psprintf(f->r->pool, "Replacing \"%s\" with \"%s\".\n", apr_pstrndup(f->r->pool, substring_start, substring_length), with_text);
                                    outlen = strlen(tmpstr);
                                    apr_file_write(fp, tmpstr, &outlen);
                                }

                                ap_fprintf(f->next, ctx->bb, "%s%s%s", substring_before_match, with_text, substring_after_match);
                            }
                            else if (inside_tags == 0 && !is_inside_html_tag(f->r->pool, next_str, strchr(substring_start, '@')))
                            {
                                ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, f->r, "REPLACE_EMAIL: 0.0.1.5.6.5: Replacing \"%s\" with \"%s\".\n", apr_pstrndup(f->r->pool, substring_start, substring_length), with_text);
                                if(logfile_is_open)
                                {
                                    tmpstr  = apr_psprintf(f->r->pool, "Replacing \"%s\" with \"%s\".\n", apr_pstrndup(f->r->pool, substring_start, substring_length), with_text);
                                    outlen = strlen(tmpstr);
                                    apr_file_write(fp, tmpstr, &outlen);
                                }

                                ap_fprintf(f->next, ctx->bb, "%s%s%s", substring_before_match, with_text, substring_after_match);
                            }
                            else
                            {
                                int print_len   = strlen(substring_before_match) + substring_length;
                                ap_fprintf(f->next, ctx->bb, "%s", apr_pstrndup(f->r->pool, next_str, print_len));
                            }
                        } // if(i == 0)
                    } // for (i=0; i<rc; ++i)

                    if (strlen(beginning_of_next_result) > 0)
                        next_str    = beginning_of_next_result;
                    else
                        next_str    = NULL;
                } // else
            } // while (next_str)
            str = NULL;
        } // if(str && !httpbl_string_matches(str, ""))
    } 

    if(logfile_is_open)
    {
        if(file_lock_acquired)
            apr_file_unlock(fp);
        apr_file_close(fp);
    }

    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, f->r, "REPLACE_EMAILS: filter returning : %s", "APR_SUCCESS");

    return APR_SUCCESS; 
}


/*
    ;
*/
static apr_status_t replace_email_links_filter(ap_filter_t* f, apr_bucket_brigade* bb)
{
    char*   replace_with_link       = get_email_rewrite_link(f->r);
//    char*   mailto_address_regex    = "mailto:[-_.+A-Za-z0-9]+(@|&#64;)[-.A-Za-z0-9]+\\.[A-Za-z]{2,4}"; // try to get (@|&#64;) to work (without creating backreferences)
    char*   mailto_address_regex    = "[-_.:he+A-Za-z0-9]+(@|&#64;)[-.A-Za-z0-9]+\\.[A-Za-z]{2,4}"; // try to get (@|&#64;) to work (without creating backreferences)

    return replace_email_filter(f, bb, 1, replace_with_link, mailto_address_regex);
}


/*
    ;
*/
static apr_status_t replace_email_text_filter(ap_filter_t* f, apr_bucket_brigade* bb)
{
    char*   replace_with_text       = get_email_rewrite_text(f->r);
    char*   email_address_regex     = "[-_.+A-Za-z0-9]+(@|&#64;)[-.A-Za-z0-9]+\\.[A-Za-z]{2,4}"; // try to get (@|&#64;) to work (without creating backreferences)

    return replace_email_filter(f, bb, 0, replace_with_text, email_address_regex);
}


/*
    httpbl_post_config
    This function does checks to make sure specific required directives are loaded.  If they are not loaded, we want to kill server startup with an explanation.
*/
static apr_status_t httpbl_post_config(apr_pool_t* pconf, apr_pool_t* plog, apr_pool_t* ptemp, server_rec* s)
{
    if(!get_repos_dir() ||
       httpbl_string_matches((const char*)get_repos_dir(), DEFAULT_REPOS_DIR))
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, 0, s, "Your "DIRECTIVE_TEXT_REPOS_DIR" directive was not set.  The module can not run without this directive.");
        return HTTP_INTERNAL_SERVER_ERROR;
    }

#if VERBOSITY >= APLOG_DEBUG
    if(!get_log_dir() ||
       httpbl_string_matches((const char*)get_log_dir(), DEFAULT_LOG_DIR))
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, 0, s, "Your "DIRECTIVE_TEXT_LOG_DIR" directive was not set.  The module can not run without this directive.");
        return HTTP_INTERNAL_SERVER_ERROR;
    }
#endif

    if(!g_an_access_key_was_set)
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, 0, s, "Your "DIRECTIVE_TEXT_ACCESS_KEY" directive was never set.  The module can not make DNSBL lookups without an access key.  Visit http://www.projecthoneypot.org/ to get a DNSBL access key.");
        return HTTP_INTERNAL_SERVER_ERROR;
    }

    // if we get this far, we are okay to continue loading the module
    return OK;
}


/*
    Register hooks function - register these hook functions with Apache
    @return         void
*/
static void register_hooks (apr_pool_t *p)
{
    char*   temp_dir   = NULL;
//    apr_temp_dir_get((const char**)&temp_dir, p);
//    repos_dir   = apr_pstrdup(p, temp_dir);
//    log_dir     = apr_pstrdup(p, temp_dir);

    /* start requests to get a temp directory */
    temp_dir    = ap_server_root_relative(p, "logs/");
    if(!temp_dir)
        apr_temp_dir_get((const char**)&temp_dir, p);
/*
    temp_dir    = ap_server_root_relative(p, "logs/");
    if(!temp_dir)
        apr_temp_dir_get((const char**)&temp_dir, p);
*/
    /* end requests to get a temp directory */

    repos_dir   = apr_pstrdup(p, temp_dir);
    log_dir     = apr_pstrdup(p, temp_dir);

    // Create a new hit list for this child process
    if(g_enable_rbl_lookup)
    {
        create_white_list(p);
        create_hit_list(p);
    }
    init_default_httpbl_404_config(p);
    clear_httpbl_404_cache(p); // only adjusts the 404 cache variables (in memory); does not write to cache data file(s) or cache-meta file(s)

#ifdef SHOULD_CACHE
    init_404_cache_filename(p);
    init_404_meta_filename(p);
#endif
    init_404_post_url(p);

    ap_hook_access_checker      (access_checker,            NULL,                       NULL,                       APR_HOOK_FIRST);        // file request handler - call "access_checker" first (before other handler functions) on all requests
    ap_hook_handler             (handle_any_httpbl_tests,   NULL,                       NULL,                       APR_HOOK_FIRST);
#ifdef SHOULD_REQUEST_HONEYPOTS
    ap_hook_handler             (handle_any_honeypots,      NULL,                       NULL,                       APR_HOOK_FIRST);
#endif
#ifdef SHOULD_SUBMIT_404s
    ap_hook_fixups              (handle_404_recording,      NULL,                       NULL,                       APR_HOOK_LAST);         // file request handler - intercept 404 errors after all other handlers have been called
#endif
    ap_register_output_filter   ("REPLACE_EMAIL_LINKS",     replace_email_links_filter, NULL,                       AP_FTYPE_CONTENT_SET);
    ap_register_output_filter   ("REPLACE_EMAIL_TEXT",      replace_email_text_filter,  NULL,                       AP_FTYPE_CONTENT_SET);  // 
    apr_pool_cleanup_register   (p,                         NULL,                       apr_pool_cleanup_null,      destroy_hit_list);      // call "destroy_hit_list" when the apr_pool is performing cleanup

    ap_hook_post_config         (httpbl_post_config,        NULL,                       NULL,                       APR_HOOK_LAST);
}



/*
    Directives registration - this structure assigns handler functions for when Apache encounters
    directives with these specific strings "HTTPBL[A-Za-z]+"
*/
static const command_rec access_cmds[] = {
    AP_INIT_FLAG    (DIRECTIVE_TEXT_RBL_TOGGLE,                 directive_set_rbl_lookup_toggle,            NULL, RSRC_CONF | OR_LIMIT,     "Have HTTPBL query a RBL server to determine if a visitor is harmful"),
#ifdef SHOULD_ALLOW_CHALLENGES
    AP_INIT_TAKE1   (DIRECTIVE_TEXT_CHALLENGE_TOKEN,            directive_set_challenge_token,              NULL, RSRC_CONF | ACCESS_CONF,  "Token to set the challenge-passed flag and whitelist the challenged user"),
    AP_INIT_TAKE1   (DIRECTIVE_TEXT_CHALLENGE_URL,              directive_set_challenge_url,                NULL, RSRC_CONF,                "Set the URL of the challenge (captcha) webpage"),
#endif
    AP_INIT_TAKE1   (DIRECTIVE_TEXT_DIAGNOSTICS_URL,            directive_set_g_httpbl_testing_url,         NULL, RSRC_CONF,                "Set the URL of the HTTPBL diagnostics testing webpage"),
#ifdef SHOULD_REQUEST_HONEYPOTS
    AP_INIT_TAKE1   (DIRECTIVE_TEXT_HONEYPOT_URL,               directive_set_honeypot_url,                 NULL, RSRC_CONF | ACCESS_CONF,  "Set the base-URL for honeypots"),
#endif
    AP_INIT_TAKE1   (DIRECTIVE_TEXT_ACCESS_KEY,                 directive_set_rbl_access_key,               NULL, RSRC_CONF,                "Set the access key for RBL lookups"),
    AP_INIT_FLAG    (DIRECTIVE_TEXT_EXEMPT_TOGGLE,              directive_set_httpbl_exempt,                NULL, RSRC_CONF | OR_LIMIT,     "Exempt these files/directories from HTTPBL filetering (meant to be used for captcha-filtering pages)"),
    AP_INIT_TAKE1   (DIRECTIVE_TEXT_HASH_TABLE_SIZE,            directive_set_hash_tbl_size,                NULL, RSRC_CONF,                "Set size of hash table"),
    AP_INIT_TAKE1   (DIRECTIVE_TEXT_BLOCKING_PERIOD,            directive_set_blocking_period,              NULL, RSRC_CONF,                "Set blocking period for caching HTTPBL IPs"),
    AP_INIT_TAKE1   (DIRECTIVE_TEXT_LOG_DIR,                    directive_set_log_dir,                      NULL, RSRC_CONF,                "Set the path to the log directory"),
    AP_INIT_TAKE1   (DIRECTIVE_TEXT_REPOS_DIR,                  directive_set_repos_dir,                    NULL, RSRC_CONF,                "Set the path to the repos directory (used for all cache files)"),
    AP_INIT_ITERATE (DIRECTIVE_TEXT_WHITELIST_AN_IPADDR,        directive_add_to_whitelist,                 NULL, RSRC_CONF | OR_LIMIT,     "IP-addresses wildcards to whitelist"),
#ifdef SHOULD_SUBMIT_404s
    AP_INIT_FLAG    (DIRECTIVE_TEXT_404_TOGGLE,                 directive_set_404_capture_toggle,           NULL, RSRC_CONF | OR_LIMIT,     "Have HTTPBL record 404 information and periodically POST it to httpbl.org (to identify spam/exploit crawlers)"),
    AP_INIT_FLAG    (DIRECTIVE_TEXT_POST_TOGGLE,                directive_set_POST_capture_toggle,          NULL, RSRC_CONF | OR_LIMIT,     "Have HTTPBL record POST information and periodically POST it to httpbl.org (to identify comment spammers)"),
    AP_INIT_TAKE1   (DIRECTIVE_TEXT_404_SUBMISSION_URL,         directive_set_404_post_url,                 NULL, RSRC_CONF,                "Assign a URL to tell HTTBL where to POST 404 information"),
    AP_INIT_TAKE1   (DIRECTIVE_TEXT_404_SUBMISSION_INTERVAL,    directive_set_404_post_interval,            NULL, RSRC_CONF,                "Set the interval (in seconds) between POSTs of 404 information"),
    AP_INIT_TAKE1   (DIRECTIVE_TEXT_404_SUBMISSION_MAX_RETRIES, directive_set_404_post_max_retries,         NULL, RSRC_CONF,                "Set a max number of retries to use with HTTPBL 404 POSTs"),
    AP_INIT_TAKE1   (DIRECTIVE_TEXT_404_SUBMISSION_MIN_RECORDS, directive_set_404_post_record_count,        NULL, RSRC_CONF,                "Set a minimum number of 404 records to accumulate before the next HTTPBL 404 POST"),
#endif
    AP_INIT_TAKE1   (DIRECTIVE_TEXT_EXTERNAL_PROXY_INFO,        directive_set_404_proxy_address,            NULL, RSRC_CONF,                "Set a HTTP Proxy server to use with HTTPBL 404 POSTs"),
    AP_INIT_TAKE1   (DIRECTIVE_TEXT_EXTERNAL_HTTPAUTH_INFO,     directive_set_404_http_auth,                NULL, RSRC_CONF,                "Set HTTP authentication to use with HTTPBL 404 POSTs (format: 'user:pass')"),
    AP_INIT_TAKE1   (DIRECTIVE_TEXT_EXTERNAL_POST_TIMEOUT,      directive_set_404_post_timeout,             NULL, RSRC_CONF,                "Set a timeout (in seconds) to use with HTTPBL 404 POSTs"),
    AP_INIT_TAKE1   (DIRECTIVE_TEXT_DEFAULT_ACTION,             directive_set_default_action,               NULL, RSRC_CONF | OR_LIMIT,     "Set the default action (allow|deny"
#ifdef SHOULD_ALLOW_CHALLENGES
                                                                                                                                            "|challenge"
#endif
                                                                                                                                            ")."),
    AP_INIT_TAKE1   (DIRECTIVE_TEXT_RBL_DOMAIN,                 directive_set_rbl_domain,                   NULL, RSRC_CONF | OR_LIMIT,     "Set the domain of the RBL server to use for RBL lookups"),
    AP_INIT_TAKE1   (DIRECTIVE_TEXT_REWRITE_EMAIL_TEXT,         directive_set_email_links_rewrite,          NULL, RSRC_CONF | OR_LIMIT,     "Set the email replacement link content"),
    AP_INIT_TAKE1   (DIRECTIVE_TEXT_REWRITE_EMAIL_LINKS,        directive_set_email_text_rewrite,           NULL, RSRC_CONF | OR_LIMIT,     "Set the email replacement text content"),
    AP_INIT_TAKE2   (DIRECTIVE_TEXT_RBL_REQ_HANDLER,            directive_set_rbl_req_handling_directive,   NULL, RSRC_CONF | OR_LIMIT,     "Takes 2 parameters: [VERB bitset]:[SCORE range]:[DATE range]:[TYPE bitset] [ BEHAVIOR string ]"),
//    AP_INIT_TAKE1  ("DIRECTIVE_TEXT_FORBIDDEN_TEMPLATE_PATH,    directive_set_forbidden_template_uri,       NULL, RSRC_CONF,                "Set URI of a custom forbidden page template"),
    { NULL }
};

/*
    Dispatch list for API hooks - standard Apache function registration
    See Apache Developer Documentation for details
*/
module AP_MODULE_DECLARE_DATA httpbl_module = {
    STANDARD20_MODULE_STUFF,
    httpbl_create_dir_conf,         // Create config rec for Directory
    httpbl_merge_dir_conf,          // Merge config rec for Directory
    httpbl_create_svr_conf,         // init_httpbl_structures
    httpbl_merge_dir_conf,          //
    access_cmds,
    register_hooks
};
