#pragma once

#include "Result.h"

namespace douban {
namespace mc {
namespace keywords {

static const char kCR = '\r';
static const char kLF = '\n';
static const char kCRLF[] = "\r\n";
static const char kSPACE[] = " ";

static const char kGET[] = "get";
static const char kGETS[] = "gets";

static const char kSET_[] = "set ";
static const char kADD_[] = "add ";
static const char kREPLACE_[] = "replace ";
static const char kAPPEND_[] = "append ";
static const char kPREPEND_[] = "prepend ";
static const char kCAS_[] = "cas ";

static const char kDELETE_[] = "delete ";
static const char kTOUCH_[] = "touch ";

static const char kINCR_[] = "incr ";
static const char kDECR_[] = "decr ";

static const char k_NOREPLY[] = " noreply";

static const char kVERSION[] = "version";
static const char kSTATS[] = "stats";

static const char kQUIT[] = "quit";


// error messages for humman
static const char kSEND_ERROR[] = "send_error";
static const char kRECV_ERROR[] = "recv_error";
static const char kSERVER_ERROR[] = "server_error";
static const char kCONN_POLL_ERROR[] = "conn_poll_error";
static const char kPOLL_TIMEOUT[] = "poll_timeout";
static const char kPOLL_ERROR[] = "poll_error";
static const char kPROGRAMMING_ERROR[] = "programming_error";

} // namespace keywords
} // namespace mc
} // namespace douban
