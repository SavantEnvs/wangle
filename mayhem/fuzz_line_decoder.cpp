// Feeds bytes to wangle's LineBasedFrameDecoder through a real Pipeline, split across several
// reads so the decoder's discarding/over-length state carries between calls.
#include <cstddef>
#include <cstdint>
#include <memory>

#include <folly/io/IOBuf.h>
#include <folly/io/IOBufQueue.h>
#include <wangle/channel/Pipeline.h>
#include <wangle/codec/LineBasedFrameDecoder.h>

namespace {
class Sink : public wangle::InboundHandler<std::unique_ptr<folly::IOBuf>> {
 public:
  void read(Context*, std::unique_ptr<folly::IOBuf> buf) override {
    frames_ += 1;
    bytes_ += buf->computeChainDataLength();
  }
  void readException(Context*, folly::exception_wrapper) override {
    errors_ += 1;
  }
  size_t frames_{0};
  size_t bytes_{0};
  size_t errors_{0};
};
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  // header: maxLength, flags (bit0 stripDelimiter, bits1.. terminator type), chunk size
  if (size < 3) {
    return -1;
  }
  const uint32_t maxLength = data[0];
  const bool strip = data[1] & 1;
  const auto terminator =
      static_cast<wangle::LineBasedFrameDecoder::TerminatorType>((data[1] >> 1) % 3);
  const size_t chunk = (data[2] % 32) + 1;
  data += 3;
  size -= 3;

  auto pipeline =
      wangle::Pipeline<folly::IOBufQueue&, std::unique_ptr<folly::IOBuf>>::create();
  auto sink = std::make_shared<Sink>();
  (*pipeline)
      .addBack(wangle::LineBasedFrameDecoder(maxLength, strip, terminator))
      .addBack(sink)
      .finalize();

  folly::IOBufQueue q(folly::IOBufQueue::cacheChainLength());
  for (size_t off = 0; off < size; off += chunk) {
    const size_t n = size - off < chunk ? size - off : chunk;
    q.append(folly::IOBuf::copyBuffer(data + off, n));
    pipeline->read(q);
  }
  return 0;
}
