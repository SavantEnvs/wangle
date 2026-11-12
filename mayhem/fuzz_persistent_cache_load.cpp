// Loads an arbitrary file as the persisted state of a FilePersistentCache<string, SSLSessionCacheData>
// (JSON -> folly::dynamic -> convertTo), then reads every entry back.
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include <folly/FileUtil.h>
#include <folly/executors/ManualExecutor.h>
#include <wangle/client/persistence/FilePersistentCache.h>
#include <wangle/client/ssl/SSLSessionCacheData.h>

#include "scratch_dir.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  ScratchDir dir;
  const std::string path = dir.file("cache.json");
  if (!folly::writeFile(
          folly::StringPiece(reinterpret_cast<const char*>(data), size), path.c_str())) {
    std::abort();
  }
  auto executor = std::make_shared<folly::ManualExecutor>();
  auto config = wangle::PersistentCacheConfig::Builder()
                    .setCapacity(16)
                    .setExecutor(executor)
                    .setInlinePersistenceLoading(true)
                    .build();
  {
    wangle::FilePersistentCache<std::string, wangle::SSLSessionCacheData> cache(
        path, std::move(config));
    executor->drain();
    volatile size_t n = cache.size();
    (void)n;
    cache.get("a");
  }
  return 0;
}
