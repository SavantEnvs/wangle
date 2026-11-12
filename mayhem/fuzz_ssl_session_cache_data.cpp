// Treats the input as the DER bytes of a cached SSL_SESSION (SSLSessionCacheData::sessionData) and
// runs wangle's session conversion/clone/identity helpers over it.
#include <cstddef>
#include <cstdint>

#include <folly/ssl/OpenSSLPtrTypes.h>
#include <openssl/ssl.h>
#include <wangle/client/ssl/SSLSessionCacheData.h>
#include <wangle/client/ssl/SSLSessionCacheUtils.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  wangle::SSLSessionCacheData cacheData;
  cacheData.sessionData = folly::fbstring(reinterpret_cast<const char*>(data), size);

  folly::ssl::SSLSessionUniquePtr session(wangle::getSessionFromCacheData(cacheData));
  if (!session) {
    return 0;
  }
  folly::ssl::SSLSessionUniquePtr clone(wangle::cloneSSLSession(session.get()));
  (void)wangle::getSessionServiceIdentity(session.get());
  (void)wangle::getSessionPeerIdentities(session.get());
  auto back = wangle::getCacheDataForSession(session.get());
  if (back) {
    (void)folly::toDynamic(*back);
  }
  return 0;
}
