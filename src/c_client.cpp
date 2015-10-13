#include "c_client.h"
#include "Client.h"


using douban::mc::Client;


void* client_create() {
  return new Client();
}


void client_init(void* client, const char* const * hosts, const uint32_t* ports,
                 size_t n, const char* const * aliases, const int failover) {
  douban::mc::Client* c = static_cast<Client*>(client);
  c->config(douban::mc::CFG_HASH_FUNCTION, douban::mc::OPT_HASH_CRC_32); // TODO
  c->init(hosts, ports, n, aliases);
  if (failover) {
    c->enableConsistentFailover();
  } else {
    c->disableConsistentFailover();
  }
}


void client_destroy(void* client) {
  douban::mc::Client* c = static_cast<Client*>(client);
  delete c;
}

const char* client_get_server_address_by_key(void* client, const char* key, size_t key_len) {
  douban::mc::Client* c = static_cast<Client*>(client);
  return c->getServerAddressByKey(key, key_len);
}

int client_version(void* client, broadcast_result_t** results, size_t* n_hosts) {
  douban::mc::Client* c = static_cast<Client*>(client);
  return c->version(results, n_hosts);
}


void client_destroy_broadcast_result(void* client) {
  douban::mc::Client* c = static_cast<Client*>(client);
  return c->destroyBroadcastResult();
}

#define IMPL_RETRIEVAL_CMD(M) \
int client_##M(void* client, const char* const* keys, const size_t* key_lens, \
               size_t n_keys, retrieval_result_t*** results, size_t* n_results) { \
  douban::mc::Client* c = static_cast<Client*>(client); \
  return c->M(keys, key_lens, n_keys, results, n_results); \
}
IMPL_RETRIEVAL_CMD(get)
IMPL_RETRIEVAL_CMD(gets)
#undef IMPL_RETRIEVAL_CMD

void client_destroy_retrieval_result(void* client) {
  douban::mc::Client* c = static_cast<Client*>(client);
  return c->destroyRetrievalResult();
}


#define IMPL_STORAGE_CMD(M) \
int client_##M(void* client, const char* const* keys, const size_t* key_lens, \
               const flags_t* flags, const exptime_t exptime, \
               const cas_unique_t* cas_uniques, const bool noreply, \
               const char* const* vals, const size_t* val_lens, \
               size_t nItems, message_result_t*** results, size_t* n_results) { \
  douban::mc::Client* c = static_cast<Client*>(client); \
  return c->M(keys, key_lens, flags, exptime, cas_uniques, \
                noreply, vals, val_lens, nItems, results, n_results); \
}

IMPL_STORAGE_CMD(set)
IMPL_STORAGE_CMD(add)
IMPL_STORAGE_CMD(replace)
IMPL_STORAGE_CMD(append)
IMPL_STORAGE_CMD(prepend)
IMPL_STORAGE_CMD(cas)
#undef IMPL_STORAGE_CMD


int client_touch(void* client, const char* const* keys, const size_t* key_lens,
                 const exptime_t exptime, const bool noreply, size_t n_items,
                 message_result_t*** results, size_t* n_results) {
  douban::mc::Client* c = static_cast<Client*>(client);
  return c->touch(keys, key_lens, exptime, noreply, n_items, results, n_results);
}

void client_destroy_message_result(void* client) {
  douban::mc::Client* c = static_cast<Client*>(client);
  return c->destroyMessageResult();
}


int client_delete(void*client, const char* const* keys, const size_t* key_lens,
                  const bool noreply, size_t n_items,
                  message_result_t*** results, size_t* n_results) {
  douban::mc::Client* c = static_cast<Client*>(client);
  return c->_delete(keys, key_lens, noreply, n_items, results, n_results);
}
