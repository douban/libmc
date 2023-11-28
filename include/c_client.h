#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "Export.h"

#ifdef __cplusplus
extern "C" {
#endif

  void* client_create();
  void client_init(void* client, const char* const * hosts, const uint32_t* ports,
                   size_t n, const char* const * aliases, const int failover);
  void client_config(void* client, config_options_t opt, int val);
  void client_destroy(void* client);

  const char* client_get_server_address_by_key(void* client, const char* key, const size_t key_len);
  const char* client_get_realtime_server_address_by_key(void* client, const char* key,
                                                        const size_t key_len);

  err_code_t client_version(void* client, broadcast_result_t** results, size_t* n_hosts);
  void client_destroy_broadcast_result(void* client);

#define DECL_RETRIEVAL_CMD(M) \
  err_code_t client_##M(void* client, const char* const* keys, const size_t* key_lens, \
                 size_t nKeys, retrieval_result_t*** results, size_t* n_results)
  DECL_RETRIEVAL_CMD(get);
  DECL_RETRIEVAL_CMD(gets);
#undef DECL_RETRIEVAL_CMD

  void client_destroy_retrieval_result(void* client);

#define DECL_STORAGE_CMD(M) \
  err_code_t client_##M(void* client, const char* const* keys, const size_t* key_lens, \
               const flags_t* flags, const exptime_t exptime, \
               const cas_unique_t* cas_uniques, const bool noreply, \
               const char* const* vals, const size_t* val_lens, \
               size_t nItems, message_result_t*** results, size_t* n_results)
  DECL_STORAGE_CMD(set);
  DECL_STORAGE_CMD(add);
  DECL_STORAGE_CMD(replace);
  DECL_STORAGE_CMD(append);
  DECL_STORAGE_CMD(prepend);
  DECL_STORAGE_CMD(cas);
#undef DECL_STORAGE_CMD

  err_code_t client_touch(void* client, const char* const* keys, const size_t* key_lens,
                   const exptime_t exptime, const bool noreply, size_t n_items,
                   message_result_t*** results, size_t* n_results);
  void client_destroy_message_result(void* client);

  err_code_t client_delete(void*client, const char* const* keys, const size_t* key_lens,
                    const bool noreply, size_t n_items,
                    message_result_t*** results, size_t* n_results);

  err_code_t client_incr(void* client, const char* key, const size_t keyLen,
                  const uint64_t delta, const bool noreply,
                  unsigned_result_t** results, size_t* n_results);
  err_code_t client_decr(void* client, const char* key, const size_t keyLen,
                  const uint64_t delta, const bool noreply,
                  unsigned_result_t** results, size_t* n_results);
  void client_destroy_unsigned_result(void* client);

  err_code_t client_stats(void* client, broadcast_result_t** results, size_t* n_servers);
  void client_toggle_flush_all_feature(void* client, bool enabled);
  err_code_t client_flush_all(void* client, broadcast_result_t** results, size_t* n_servers);
  err_code_t client_quit(void* client);

  const char* err_code_to_string(err_code_t err);
  ServerSpec splitServerString(char* input);
#ifdef __cplusplus
}
#endif
