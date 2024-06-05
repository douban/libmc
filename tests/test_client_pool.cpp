#include "ClientPool.h"
#include "test_common.h"

#include <thread>
#include "gtest/gtest.h"

using douban::mc::ClientPool;
using douban::mc::tests::gen_random;

const unsigned int n_ops = 5;
const unsigned int data_size = 10;
const unsigned int n_servers = 20;
const unsigned int start_port = 21211;
const char host[] = "127.0.0.1";
unsigned int n_threads = 8;

void inner_test_loop(ClientPool* pool) {
  retrieval_result_t **r_results = NULL;
  message_result_t **m_results = NULL;
  size_t nResults = 0;
  flags_t flags[] = {};
  size_t data_lens[] = {data_size};
  exptime_t exptime = 0;
  char key[data_size + 1];
  char value[data_size + 1];
  const char* keys = &key[0];
  const char* values = &value[0];

  for (unsigned int j = 0; j < n_ops; j++) {
    gen_random(key, data_size);
    gen_random(value, data_size);
    auto c = pool->acquire();
    c->set(&keys, data_lens, flags, exptime, NULL, 0, &values, data_lens, 1, &m_results, &nResults);
    c->destroyMessageResult();
    c->get(&keys, data_lens, 1, &r_results, &nResults);
    EXPECT_EQ(nResults, 1);
    ASSERT_N_STREQ(r_results[0]->data_block, values, data_size);
    c->destroyRetrievalResult();
    pool->release(c);
  }
}

bool check_availability(ClientPool* pool) {
  auto c = pool->acquire();
  broadcast_result_t* results;
  size_t nHosts;
  int ret = c->version(&results, &nHosts);
  c->destroyBroadcastResult();
  pool->release(c);
  return ret == 0;
}

TEST(test_client_pool, simple_set_get) {
  uint32_t ports[n_servers];
  const char* hosts[n_servers];
  for (unsigned int i = 0; i < n_servers; i++) {
    ports[i] = start_port + i;
    hosts[i] = host;
  }

  ClientPool* pool = new ClientPool();
  pool->config(CFG_HASH_FUNCTION, OPT_HASH_FNV1A_32);
  pool->init(hosts, ports, n_servers);
  ASSERT_TRUE(check_availability(pool));

  for (unsigned int j = 0; j < n_threads; j++) {
    inner_test_loop(pool);
  }

  delete pool;
}

TEST(test_client_pool, threaded_set_get) {
  uint32_t ports[n_servers];
  const char* hosts[n_servers];
  for (unsigned int i = 0; i < n_servers; i++) {
    ports[i] = start_port + i;
    hosts[i] = host;
  }

  std::thread* threads = new std::thread[n_threads];
  ClientPool* pool = new ClientPool();
  pool->config(CFG_HASH_FUNCTION, OPT_HASH_FNV1A_32);
  //pool->config(CFG_INITIAL_CLIENTS, 4);
  pool->init(hosts, ports, n_servers);
  ASSERT_TRUE(check_availability(pool));

  for (unsigned int i = 0; i < n_threads; i++) {
    threads[i] = std::thread([&pool] { inner_test_loop(pool); });
  }
  for (unsigned int i = 0; i < n_threads; i++) {
    threads[i].join();
  }
  delete[] threads;
  delete pool;
}
