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
    auto c = pool->_acquire();
    c->c.set(&keys, data_lens, flags, exptime, NULL, 0, &values, data_lens, 1, &m_results, &nResults);
    c->c.destroyMessageResult();
    c->c.get(&keys, data_lens, 1, &r_results, &nResults);
    ASSERT_N_STREQ(r_results[0]->data_block, values, data_size);
    c->c.destroyRetrievalResult();
    pool->_release(c);
  }

  delete pool;
}

TEST(test_client_pool, threaded_set_get) {
  unsigned int n_threads = 8;
  uint32_t ports[n_servers];
  const char* hosts[n_servers];
  for (unsigned int i = 0; i < n_servers; i++) {
    ports[i] = start_port + i;
    hosts[i] = host;
  }

  std::thread* threads = new std::thread[n_threads];
  ClientPool* pool = new ClientPool();
  pool->config(CFG_HASH_FUNCTION, OPT_HASH_FNV1A_32);
  pool->init(hosts, ports, n_servers);

  for (unsigned int i = 0; i < n_threads; i++) {
    threads[i] = std::thread([&pool]() {
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
        auto c = pool->_acquire();
        tprintf("acquired client %d\n", c->index);
        c->c.set(&keys, data_lens, flags, exptime, NULL, 0, &values, data_lens, 1, &m_results, &nResults);
        c->c.destroyMessageResult();
        c->c.get(&keys, data_lens, 1, &r_results, &nResults);
        ASSERT_N_STREQ(r_results[0]->data_block, values, data_size);
        c->c.destroyRetrievalResult();
        pool->_release(c);
      }
    });
  }
  for (unsigned int i = 0; i < n_threads; i++) {
    threads[i].join();
  }
  delete[] threads;
  delete pool;
}
