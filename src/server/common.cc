#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include "common.hh"

using namespace std;

const unsigned long WebGUI::SLEEP_ON_AUTH_FAILURE = 2 * 1000 * 1000; //microseconds

const unsigned long WebGUI::ID_MAX =   4294967295UL;
const unsigned long WebGUI::ID_START = 2147483648UL;
const unsigned long WebGUI::ID_RANGE = 2147483647UL;



const unsigned long WebGUI::SESSION_TIMEOUT_WINDOW = 1800UL;
//const unsigned long WebGUI::SESSION_TIMEOUT_WINDOW = (unsigned long)99999999;

const string WebGUI::VYATTA_TEMP_CONFIG_DIR = "/opt/vyatta/config/tmp/new_config_"; //VYATTA_TEMP_CONFIG_DIR=/opt/vyatta/config/tmp/new_config_22764
const string WebGUI::VYATTA_CHANGES_ONLY_DIR = "/tmp/changes_only_"; //VYATTA_CHANGES_ONLY_DIR=/tmp/changes_only_22764
const string WebGUI::VYATTA_ACTIVE_CONFIGURATION_DIR = "/opt/vyatta/config/active"; //VYATTA_ACTIVE_CONFIGURATION_DIR=/opt/vyatta/config/active


const string WebGUI::OP_COMMAND_DIR = "/opt/vyatta/share/vyatta-op/templates";
const string WebGUI::ACTIVE_CONFIG_DIR = "/opt/vyatta/config/active";
const string WebGUI::CONFIG_TMP_DIR = "/opt/vyatta/config/tmp/tmp_";
const string WebGUI::LOCAL_CHANGES_ONLY = "/tmp/changes_only_";
const string WebGUI::LOCAL_CONFIG_DIR = "/opt/vyatta/config/tmp/new_config_";
const string WebGUI::CFG_TEMPLATE_DIR = "/opt/vyatta/share/vyatta-cfg/templates";
const string WebGUI::OP_TEMPLATE_DIR = "/opt/vyatta/share/vyatta-op/templates";
const string WebGUI::COMMIT_LOCK_FILE = "/opt/vyatta/config/.lock";
const string WebGUI::VYATTA_MODIFY_DIR = "/opt/vyatta/config/tmp/";
const string WebGUI::VYATTA_MODIFY_FILE = WebGUI::VYATTA_MODIFY_DIR + ".vyattamodify_";


const string WebGUI::CHUNKER_RESP_TOK_DIR = "/opt/vyatta/tmp/webgui/";
const string WebGUI::CHUNKER_RESP_TOK_BASE = "multi_";
const string WebGUI::CHUNKER_RESP_PID = "/tmp"; //"/opt/vyatta/var/run";
const string WebGUI::CHUNKER_SOCKET = "/tmp/browser_pager";
const unsigned long WebGUI::CHUNKER_MAX_WAIT_TIME = 2; //seconds
const string WebGUI::CHUNKER_MSG_FORMAT = "<vyatta><chunker_command><token>%s</token><statement>%s</statement></chunker_command></vyatta>\0\0";
const string WebGUI::CHUNKER_UPDATE_FORMAT = "<vyatta><chunker_command><token>%s</token><statement></statement></chunker_command></vyatta>\0\0";

char const* WebGUI::ErrorDesc[10] = {" ",
				    "request cannot be parsed",
				    "authentication error",
				    "session is not valid",
				    "permission level is not valid",
				    "general server failure",
				    "command failed",
				    "commit is in progress",
				    "configuration has changed",
                                    "commit is missing mandatory nodes"};

/**
 *
 **/
std::string
WebGUI::generate_response(string &token, Error err)
{
    char buf[40];
    sprintf(buf, "%d", err);
    return ("<?xml version='1.0' encoding='utf-8'?><vyatta><error><token>"+token+"</token><code>"+string(buf)+"</code><msg>"+string(WebGUI::ErrorDesc[err])+"</msg></error></vyatta>");
}

/**
 *
 **/
int
WebGUI::execute(std::string &cmd, std::string &stdout, bool read)
{
  int err = 0;

  string dir = "w";
  if (read == true) {
    dir = "r";
  }
  //  cout << "WebGUI::execute(A): '" << cmd << "'" << endl;
  FILE *f = popen(cmd.c_str(), dir.c_str());
  if (f) {
    if (read == true) {
      fflush(f);
      char *buf = NULL;
      size_t len = 0;
      ssize_t read_len = 0;
      while ((read_len = getline(&buf, &len, f)) != -1) {
	//	cout << "WebGUI::execute(): " << string(buf) << ", " << len << ", " << read_len << endl;
	stdout += string(buf) + " ";
      }

      if (buf) {
	free(buf);
      }
    }
    err = pclose(f);
  }
  return err;
}

/**
 *
 **/
std::string // replace all instances of victim with replacement
WebGUI::mass_replace(const std::string &source, const std::string &victim, const
	     std::string &replacement)
{
  std::string::size_type jump = replacement.length();
  std::string answer = source;
  std::string::size_type j = 0;
  std::string::size_type offset= 0;
  while ((j = answer.find(victim, j+offset)) != std::string::npos ) {
    offset = jump;
    answer.replace(j, victim.length(), replacement);
  }
  return answer;
}

/**
 *
 **/
std::string
WebGUI::trim_whitespace(const std::string &src)
{
  string str(src);
  size_t startpos = str.find_first_not_of(" \t"); 
  size_t endpos = str.find_last_not_of(" \t"); 
  if(( string::npos == startpos ) || ( string::npos == endpos)) {
    str = "";
  }
  else {
    str = str.substr( startpos, endpos-startpos+1 );
  }
  return str;
}

/**
 *
 **/
void
WebGUI::remove_session(string id)
{
  discard_session(id);
}

/**
 *
 **/
void
WebGUI::discard_session(string &id)
{
  string cmd,stdout;
  /* //OLD WAY...
  cmd = "sudo umount " + WebGUI::LOCAL_CONFIG_DIR + id;
  cmd += ";rm -fr " + WebGUI::LOCAL_CHANGES_ONLY + id;
  cmd += ";mkdir -p " + WebGUI::LOCAL_CHANGES_ONLY + id;
  execute(cmd,stdout);
  */

  cmd = "sudo umount " + WebGUI::VYATTA_TEMP_CONFIG_DIR + id;
  //  cmd += ";sudo rm -fr " + WebGUI::VYATTA_CHANGES_ONLY_DIR + id + "/ " + WebGUI::VYATTA_TEMP_CONFIG_DIR + id + "/";
  cmd += ";sudo rm -fr " + WebGUI::VYATTA_CHANGES_ONLY_DIR + id + "/{.*,*} >&/dev/null ; /bin/true";
  cmd += ";mkdir -p " + WebGUI::VYATTA_CHANGES_ONLY_DIR + id + "/";
  cmd += ";mkdir -p " + WebGUI::VYATTA_TEMP_CONFIG_DIR + id + "/";
  cmd += ";sudo mount -t "+WebGUI::unionfs()+" -o dirs=" + WebGUI::VYATTA_CHANGES_ONLY_DIR + id + "=rw:" + WebGUI::VYATTA_ACTIVE_CONFIGURATION_DIR + "=ro "+WebGUI::unionfs()+" " + WebGUI::VYATTA_TEMP_CONFIG_DIR + id;

  execute(cmd,stdout);
  /*
  cmd = "echo '" + cmd + "' > /tmp/foo";
  execute(cmd,stdout);
  */
}

/**
 * adapted from cli_new.c in vyatta-cfg
 **/
int 
WebGUI::mkdir_p(const char *path)
{
  if (mkdir(path, 0777) == 0)
    return 0;

  if (errno != ENOENT)
    return -1;

  char *tmp = strdup(path);
  if (tmp == NULL) {
    errno = ENOMEM;
    return -1;
  }

  char *slash = strrchr(tmp, '/');
  if (slash == NULL)
    return -1;
  *slash = '\0';

  /* recurse to make missing piece of path */
  int ret = mkdir_p(tmp);
  if (ret == 0)
    ret = mkdir(path, 0777);

  free(tmp);
  return ret;
}

// see if using unionfs or aufs
std::string 
WebGUI::unionfs(void)
{
    static const char *fs = NULL;
    FILE *f = fopen("/proc/cmdline", "r");
    char buf[1024];

    if (fs)
	return fs;

    if (f) {
	if (fgets(buf, sizeof buf, f) != NULL &&
	    strstr(buf, "union=aufs") != NULL)
	    fs = "aufs";
	fclose(f);
    }

    if (!fs && (f = fopen("/proc/filesystems", "r"))) {
	if (fgets(buf, sizeof buf, f) != NULL &&
	    strstr(buf, "\taufs\n") != NULL)
	    fs = "aufs\n";
	fclose(f);
    }
    if (!fs)
	fs = "unionfs";

    return fs;
}

