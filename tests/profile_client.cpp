// make profile_client && valgrind --tool=callgrind ./tests/profile_client && qcachegrind callgrind.out.9109
#include <time.h>
#include "Common.h"
#include "Client.h"
#include "test_common.h"

#ifndef __MACH__
#include <unistd.h>
#else
#include <mach/clock.h>
#include <mach/mach.h>
#endif

using douban::mc::Client;
using douban::mc::tests::newClient;
using douban::mc::tests::gen_random;


static double getCPUTime() {
  timespec ts;
#ifdef __MACH__ // OS X does not have clock_gettime, use clock_get_time
  clock_serv_t cclock;
  mach_timespec_t mts;
  host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
  clock_get_time(cclock, &mts);
  mach_port_deallocate(mach_task_self(), cclock);
  ts.tv_sec = mts.tv_sec;
  ts.tv_nsec = mts.tv_nsec;
#else
  clock_gettime(CLOCK_REALTIME, &ts);
#endif
  return static_cast<double>(ts.tv_sec) + 1e-9 * static_cast<double>(ts.tv_nsec);
}

void set_(Client* client, const char* const* keys, const size_t* key_lens,
          const flags_t* flags, const char* const* vals, const size_t* val_len, size_t n = 1) {

  const exptime_t exptime = 0;
  const int noreply = 0;

  message_result_t **m_results = NULL;
  size_t nResults = 0;

  client->set(keys, key_lens, flags, exptime, NULL, noreply, vals, val_len,
              n, &m_results, &nResults);
  client->destroyMessageResult();
}


void get_(Client* client, const char* const * keys, const size_t* key_lens, size_t n = 1) {
  retrieval_result_t** r_results = NULL;
  size_t nResults = 0;
  client->get(keys, key_lens, n, &r_results, &nResults);
  client->destroyRetrievalResult();
}

static const int N_LOOP = 10000;


#define DEFINE_PROFILE_SET_GET(M, N) \
void profile_set_get_##M(Client* client, const char* key, const size_t key_len, const char* val) { \
  size_t mm = (M); \
  flags_t flags = 0; \
  set_(client, &key, &key_len, &flags, &val, &mm, 1); \
  for (int i = 0; i < (N); i++) { \
    get_(client, &key, &key_len, 1); \
  } \
}

DEFINE_PROFILE_SET_GET(1000, N_LOOP)
DEFINE_PROFILE_SET_GET(4000, N_LOOP)
DEFINE_PROFILE_SET_GET(8000, N_LOOP)
DEFINE_PROFILE_SET_GET(20000, N_LOOP)
DEFINE_PROFILE_SET_GET(512000, 1)
DEFINE_PROFILE_SET_GET(1000000, 1)


// FIXME: ##NITEM##VAL_LEN
#define DEFINE_PROFILE_SET_GET_MULTI(NITEM, VAL_LEN, N) \
void profile_set_get_multi_##NITEM##VAL_LEN(Client* client, const char* const * keys, \
                                            const size_t* key_lens, const char* const * vals) { \
  size_t val_lens[(NITEM)]; \
  flags_t flags[(NITEM)]; \
  for (int i = 0; i < (NITEM); i++) { \
    val_lens[i] = (VAL_LEN); \
    flags[i] = 0; \
  } \
  \
  set_(client, keys, key_lens, flags, vals, val_lens, (NITEM)); \
  for (int i = 0; i < (N); i++) { \
    get_(client, keys, key_lens, (NITEM)); \
  } \
}

DEFINE_PROFILE_SET_GET_MULTI(10, 100, 1000)
DEFINE_PROFILE_SET_GET_MULTI(10, 1000, 1000)
DEFINE_PROFILE_SET_GET_MULTI(100, 100, 1000)


static const int N_ITEMS = 1000;
static const int KEY_MAX = 200;
static const int VAL_MAX = 1000000;
static const int VAL_MAX2 = 1000;

int main() {
  Client* client = newClient(20);

  const char key[] = "test_foo";
  const size_t key_len = 8;
  char val_[VAL_MAX + 1];
  gen_random(val_, VAL_MAX);
  const char* val = val_;

  char **keys = new char*[N_ITEMS];
  size_t* key_lens = new size_t[N_ITEMS];
  char **vals = new char*[N_ITEMS];
  size_t* val_lens = new size_t[N_ITEMS];

  for (int i = 0; i < N_ITEMS; i++) {
    keys[i] = new char[KEY_MAX + 1];
    key_lens[i] = snprintf(keys[i], KEY_MAX, "test_profile_key_%d", i);
    vals[i] = new char[VAL_MAX2 + 1];
    gen_random(vals[i], VAL_MAX2);
    val_lens[i] = VAL_MAX2;
  }

#define TIMEIT(REPR) do { \
  double t0 = getCPUTime(); \
  (REPR); \
  double t1 = getCPUTime(); \
  (void) t0; \
  (void) t1; \
  log_info(#REPR" in %.3f s", t1 - t0); \
} while (0)

  TIMEIT(profile_set_get_1000(client, key, key_len, val));
  TIMEIT(profile_set_get_4000(client, key, key_len, val));
  TIMEIT(profile_set_get_8000(client, key, key_len, val));
  TIMEIT(profile_set_get_20000(client, key, key_len, val));
  TIMEIT(profile_set_get_512000(client, key, key_len, val));
  TIMEIT(profile_set_get_1000000(client, key, key_len, val));
  TIMEIT(profile_set_get_multi_10100(client, keys, key_lens, vals));
  TIMEIT(profile_set_get_multi_101000(client, keys, key_lens, vals));
  TIMEIT(profile_set_get_multi_100100(client, keys, key_lens, vals));

  for (int i = 0; i < N_ITEMS; i++) {
    delete[] keys[i];
    delete[] vals[i];
  }
  delete[] keys;
  delete[] key_lens;
  delete[] vals;
  delete[] val_lens;

  delete client;
  return 0;
}
