// Feeds bytes to wangle's LengthFieldBasedFrameDecoder through a real Pipeline, with the decoder's
// configuration taken from the input header and the payload delivered in several chunks.
#include <cstddef>
#include <cstdint>
#include <memory>

#include <folly/io/IOBuf.h>
#include <folly/io/IOBufQueue.h>
#include <wangle/channel/Pipeline.h>
#include <wangle/codec/LengthFieldBasedFrameDecoder.h>

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
  // header (8 bytes): lengthFieldLength selector + byte order, maxFrameLength (2),
  // lengthFieldOffset, lengthAdjustment, initialBytesToStrip, chunk size
  if (size < 8) {
    return -1;
  }
  static const uint32_t kLengthFieldLengths[] = {1, 2, 4, 8};
  const uint32_t lengthFieldLength = kLengthFieldLengths[data[0] & 3];
  const bool networkByteOrder = data[0] & 4;
  const uint32_t maxFrameLength = (uint32_t(data[1]) << 8) | data[2];
  const uint32_t lengthFieldOffset = data[3];
  const int32_t lengthAdjustment = static_cast<int8_t>(data[4]);
  const uint32_t initialBytesToStrip = data[5];
  const size_t chunk = (data[6] % 32) + 1;
  data += 8;
  size -= 8;

  // The constructor's own preconditions (WANGLE_CHECK in LengthFieldBasedFrameDecoder.cpp):
  // maxFrameLength > 0 and lengthFieldOffset <= maxFrameLength - lengthFieldLength.
  if (maxFrameLength == 0 || maxFrameLength < lengthFieldLength ||
      lengthFieldOffset > maxFrameLength - lengthFieldLength) {
    return -1;
  }

  auto pipeline =
      wangle::Pipeline<folly::IOBufQueue&, std::unique_ptr<folly::IOBuf>>::create();
  auto sink = std::make_shared<Sink>();
  (*pipeline)
      .addBack(wangle::LengthFieldBasedFrameDecoder(
          lengthFieldLength,
          maxFrameLength,
          lengthFieldOffset,
          lengthAdjustment,
          initialBytesToStrip,
          networkByteOrder))
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
