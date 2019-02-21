#include <crypto/base64.hpp>

#include <cryptopp/base64.h>

#include <crypto/exception.hpp>

#ifdef CRYPTOPP_NO_GLOBAL_BYTE
using CryptoPP::byte;
#endif

namespace crypto::base64 {

std::string Base64Encode(const std::string& data, Pad pad) {
  std::string response;
  try {
    CryptoPP::Base64Encoder encoder(new CryptoPP::StringSink(response));
    CryptoPP::AlgorithmParameters params =
        CryptoPP::MakeParameters(CryptoPP::Name::Pad(), Pad::kWith == pad)(
            CryptoPP::Name::InsertLineBreaks(), false);
    encoder.IsolatedInitialize(params);
    encoder.PutMessageEnd(reinterpret_cast<const byte*>(data.c_str()),
                          data.size());
  } catch (const CryptoPP::Exception& exc) {
    throw crypto::CryptoException(exc.what());
  }
  return response;
}

std::string Base64Decode(const std::string& data) {
  std::string response;
  try {
    CryptoPP::Base64Decoder decoder(new CryptoPP::StringSink(response));
    decoder.PutMessageEnd(reinterpret_cast<const byte*>(data.c_str()),
                          data.size());
  } catch (const CryptoPP::Exception& exc) {
    throw crypto::CryptoException(exc.what());
  }
  return response;
}

}  // namespace crypto::base64
