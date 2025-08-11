# Encryption Patterns

## Overview

Comprehensive encryption patterns for userver applications, covering TLS/SSL configuration, data encryption at rest and in transit, key management strategies, secure communication protocols, and certificate management.

## Core Encryption Components

### TLS/SSL Implementation
- [`engine::io::TlsWrapper`](https://userver.tech/df/d00/classengine_1_1io_1_1TlsWrapper.html) - TLS encryption for sockets
- [`crypto::Certificate`](https://userver.tech/d6/d32/classcrypto_1_1Certificate.html) - Certificate management
- [`crypto::PrivateKey`](https://userver.tech/d7/d32/classcrypto_1_1PrivateKey.html) - Private key handling
- [`server::http::HttpRequest`](https://userver.tech/d3/d44/classserver_1_1http_1_1HttpRequest.html) - HTTPS request handling

### Data Encryption
- [`crypto::aes::Aes256`](https://userver.tech/d8/d32/classcrypto_1_1aes_1_1Aes256.html) - AES-256 encryption
- [`crypto::hmac::HmacSha256`](https://userver.tech/d9/d32/classcrypto_1_1hmac_1_1HmacSha256.html) - HMAC-SHA256 for data integrity
- [`crypto::hash::Sha256`](https://userver.tech/da/d32/classcrypto_1_1hash_1_1Sha256.html) - SHA-256 hashing
- [`crypto::random`](https://userver.tech/db/d32/namespacecrypto_1_1random.html) - Cryptographically secure random number generation

### Key Management
- [`crypto::Key`](https://userver.tech/dc/d32/classcrypto_1_1Key.html) - Key management and storage
- [`crypto::kms`](https://userver.tech/dd/d32/namespacecrypto_1_1kms.html) - Key Management Service integration
- [`concurrent::Variable`](https://userver.tech/d8/dcc/namespaceconcurrent.html) - Thread-safe key storage
- [`components::ComponentBase`](https://userver.tech/d0/d9c/classcomponents_1_1ComponentBase.html) - Key management component base

## TLS/SSL Implementation

### HTTPS Server Configuration
```cpp
#include <userver/server/component.hpp>
#include <userver/engine/io/tls_wrapper.hpp>
#include <userver/crypto/certificate.hpp>
#include <userver/crypto/private_key.hpp>

class SecureHttpServer : public components::ComponentBase {
public:
  static constexpr std::string_view kName = "secure-http-server";
  
  SecureHttpServer(const components::ComponentConfig& config,
                   const components::ComponentContext& context)
    : ComponentBase(config, context) {
    
    // Load TLS certificates
    auto cert_path = config["tls"]["cert-file"].As<std::string>();
    auto key_path = config["tls"]["key-file"].As<std::string>();
    auto ca_path = config["tls"]["ca-file"].As<std::string>("");
    
    certificate_ = crypto::Certificate::LoadFromFile(cert_path);
    private_key_ = crypto::PrivateKey::LoadFromFile(key_path);
    
    if (!ca_path.empty()) {
      ca_certificate_ = crypto::Certificate::LoadFromFile(ca_path);
    }
    
    // Configure TLS settings
    tls_config_.cipher_suites = config["tls"]["cipher-suites"].As<std::string>(
      "ECDHE-RSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256"
    );
    tls_config_.verify_client = config["tls"]["verify-client"].As<bool>(false);
    tls_config_.min_version = config["tls"]["min-version"].As<std::string>("TLSv1.2");
    tls_config_.max_version = config["tls"]["max-version"].As<std::string>("TLSv1.3");
    
    // Configure session resumption
    tls_config_.session_cache_size = config["tls"]["session-cache-size"].As<int>(1000);
    tls_config_.session_timeout = config["tls"]["session-timeout"].As<int>(300);
  }

private:
  struct TlsConfig {
    std::string cipher_suites;
    bool verify_client = false;
    std::string min_version = "TLSv1.2";
    std::string max_version = "TLSv1.3";
    int session_cache_size = 1000;
    int session_timeout = 300;
  };
  
  crypto::Certificate certificate_;
  crypto::PrivateKey private_key_;
  std::optional<crypto::Certificate> ca_certificate_;
  TlsConfig tls_config_;
};
```

### TLS Client Implementation
```cpp
#include <userver/clients/http/client.hpp>
#include <userver/engine/io/tls_wrapper.hpp>
#include <userver/crypto/certificate.hpp>

class SecureTcpClient {
public:
  struct TlsConfig {
    std::string ca_file;
    std::string cert_file;
    std::string key_file;
    bool verify_server = true;
    std::string server_name; // For SNI
    std::string cipher_suites;
    std::string min_version = "TLSv1.2";
    std::string max_version = "TLSv1.3";
    bool enable_session_resumption = true;
  };
  
  SecureTcpClient(const engine::io::Sockaddr& server_addr, TlsConfig tls_config)
    : server_addr_(server_addr), tls_config_(std::move(tls_config)) {}
  
  void Connect() {
    // Create TCP socket
    socket_ = engine::io::Socket::Create(
      server_addr_.Domain(),
      engine::io::SocketType::kStream
    );
    
    auto deadline = engine::Deadline::FromDuration(std::chrono::seconds(10));
    socket_.Connect(server_addr_, deadline);
    
    // Wrap with TLS
    tls_wrapper_ = engine::io::TlsWrapper::StartTlsClient(
      std::move(socket_),
      tls_config_.ca_file,
      tls_config_.cert_file,
      tls_config_.key_file
    );
    
    if (tls_config_.verify_server) {
      tls_wrapper_->SetVerifyMode(engine::io::TlsWrapper::VerifyMode::kPeer);
    }
    
    if (!tls_config_.server_name.empty()) {
      tls_wrapper_->SetServerName(tls_config_.server_name);
    }
    
    // Configure TLS parameters
    if (!tls_config_.cipher_suites.empty()) {
      tls_wrapper_->SetCipherSuites(tls_config_.cipher_suites);
    }
    
    tls_wrapper_->SetMinVersion(tls_config_.min_version);
    tls_wrapper_->SetMaxVersion(tls_config_.max_version);
    
    if (tls_config_.enable_session_resumption) {
      tls_wrapper_->EnableSessionResumption(true);
    }
    
    // Perform TLS handshake
    tls_wrapper_->DoHandshake(deadline);
  }
  
  std::string SendSecureRequest(const std::string& request) {
    auto deadline = engine::Deadline::FromDuration(std::chrono::seconds(30));
    
    tls_wrapper_->SendAll(request.data(), request.size(), deadline);
    
    std::array<char, 4096> buffer;
    auto bytes_received = tls_wrapper_->RecvSome(buffer.data(), buffer.size(), deadline);
    
    return std::string(buffer.data(), bytes_received);
  }
  
  void Disconnect() {
    if (tls_wrapper_ && tls_wrapper_->IsValid()) {
      tls_wrapper_->Close();
    }
  }

private:
  engine::io::Sockaddr server_addr_;
  TlsConfig tls_config_;
  engine::io::Socket socket_;
  std::optional<engine::io::TlsWrapper> tls_wrapper_;
};
```

### Mutual TLS (mTLS) Configuration
```cpp
#include <userver/server/component.hpp>
#include <userver/engine/io/tls_wrapper.hpp>
#include <userver/crypto/certificate.hpp>

class MutualTlsServer : public components::ComponentBase {
public:
  static constexpr std::string_view kName = "mutual-tls-server";
  
  MutualTlsServer(const components::ComponentConfig& config,
                  const components::ComponentContext& context)
    : ComponentBase(config, context) {
    
    // Load server certificates
    server_cert_ = crypto::Certificate::LoadFromFile(
      config["server-cert"].As<std::string>()
    );
    server_key_ = crypto::PrivateKey::LoadFromFile(
      config["server-key"].As<std::string>()
    );
    
    // Load CA certificate for client verification
    ca_cert_ = crypto::Certificate::LoadFromFile(
      config["ca-cert"].As<std::string>()
    );
    
    // Load Certificate Revocation List (optional)
    auto crl_path = config["crl-file"].As<std::string>("");
    if (!crl_path.empty()) {
      crl_ = crypto::CertificateRevocationList::LoadFromFile(crl_path);
    }
    
    // Configure mutual TLS settings
    mtls_config_.require_client_cert = config["require-client-cert"].As<bool>(true);
    mtls_config_.verify_depth = config["verify-depth"].As<int>(2);
    mtls_config_.check_crl = config["check-crl"].As<bool>(false);
  }
  
  bool VerifyClientCertificate(const crypto::Certificate& client_cert) const {
    try {
      // Verify certificate against CA
      client_cert.Verify(ca_cert_);
      
      // Check certificate validity period
      if (!client_cert.IsValidAt(std::chrono::system_clock::now())) {
        LOG_WARNING() << "Client certificate is not valid";
        return false;
      }
      
      // Check against CRL if configured
      if (crl_ && mtls_config_.check_crl) {
        if (crl_->IsRevoked(client_cert)) {
          LOG_WARNING() << "Client certificate is revoked";
          return false;
        }
      }
      
      return true;
      
    } catch (const std::exception& e) {
      LOG_WARNING() << "Client certificate verification failed: " << e.what();
      return false;
    }
  }

private:
  struct MutualTlsConfig {
    bool require_client_cert = true;
    int verify_depth = 2;
    bool check_crl = false;
  };
  
  crypto::Certificate server_cert_;
  crypto::PrivateKey server_key_;
  crypto::Certificate ca_cert_;
  std::optional<crypto::CertificateRevocationList> crl_;
  MutualTlsConfig mtls_config_;
};
```

## Data Encryption Implementation

### AES-256 Encryption Service
```cpp
#include <userver/crypto/aes.hpp>
#include <userver/crypto/random.hpp>
#include <userver/concurrent/variable.hpp>

class DataEncryptionService {
public:
  struct EncryptedData {
    std::string ciphertext;
    std::string iv; // Initialization Vector
    std::string auth_tag; // For AEAD modes
    std::chrono::system_clock::time_point created_at;
  };
  
  DataEncryptionService(const components::ComponentConfig& config,
                       const components::ComponentContext& context)
    : key_rotation_interval_(config["key-rotation-interval"].As<int>(86400)) // 24 hours
    , encryption_mode_(ParseEncryptionMode(config["mode"].As<std::string>("AES-256-GCM"))) {
    
    // Initialize master key
    LoadMasterKey(config);
    
    // Start key rotation task
    key_rotation_task_ = utils::Async("key-rotation", [this]() {
      KeyRotationLoop();
    });
  }
  
  EncryptedData Encrypt(const std::string& plaintext) {
    // Get current encryption key
    auto key_info = GetCurrentKey();
    
    // Generate random IV
    std::string iv = crypto::random::GenerateRandomString(16); // 128 bits for AES
    
    EncryptedData result;
    result.iv = iv;
    result.created_at = std::chrono::system_clock::now();
    
    switch (encryption_mode_) {
      case EncryptionMode::kAes256Gcm: {
        auto aes = crypto::aes::Aes256Gcm::Create(key_info.key);
        auto encrypted = aes.Encrypt(plaintext, iv);
        result.ciphertext = encrypted.ciphertext;
        result.auth_tag = encrypted.auth_tag;
        break;
      }
      
      case EncryptionMode::kAes256Cbc: {
        // Add PKCS#7 padding
        std::string padded_plaintext = AddPkcs7Padding(plaintext, 16);
        auto aes = crypto::aes::Aes256Cbc::Create(key_info.key);
        result.ciphertext = aes.Encrypt(padded_plaintext, iv);
        break;
      }
      
      default:
        throw std::runtime_error("Unsupported encryption mode");
    }
    
    return result;
  }
  
  std::string Decrypt(const EncryptedData& encrypted_data) {
    // Get appropriate key for decryption
    auto key_info = GetKeyForTimestamp(encrypted_data.created_at);
    
    std::string plaintext;
    
    switch (encryption_mode_) {
      case EncryptionMode::kAes256Gcm: {
        auto aes = crypto::aes::Aes256Gcm::Create(key_info.key);
        crypto::aes::Aes256Gcm::EncryptedData data;
        data.ciphertext = encrypted_data.ciphertext;
        data.auth_tag = encrypted_data.auth_tag;
        plaintext = aes.Decrypt(data, encrypted_data.iv);
        break;
      }
      
      case EncryptionMode::kAes256Cbc: {
        auto aes = crypto::aes::Aes256Cbc::Create(key_info.key);
        std::string padded_plaintext = aes.Decrypt(encrypted_data.ciphertext, encrypted_data.iv);
        plaintext = RemovePkcs7Padding(padded_plaintext);
        break;
      }
      
      default:
        throw std::runtime_error("Unsupported encryption mode");
    }
    
    return plaintext;
  }
  
  std::string Hash(const std::string& data) {
    return crypto::hash::Sha256::Hash(data);
  }
  
  std::string CreateHmac(const std::string& data, const std::string& key_id = "current") {
    auto key_info = GetKey(key_id);
    return crypto::hmac::HmacSha256(key_info.key, data);
  }

private:
  enum class EncryptionMode {
    kAes256Gcm,
    kAes256Cbc
  };
  
  struct KeyInfo {
    std::string key; // 256-bit key (32 bytes)
    std::chrono::system_clock::time_point created_at;
    std::string id;
  };
  
  EncryptionMode ParseEncryptionMode(const std::string& mode_str) {
    if (mode_str == "AES-256-GCM") return EncryptionMode::kAes256Gcm;
    if (mode_str == "AES-256-CBC") return EncryptionMode::kAes256Cbc;
    throw std::invalid_argument("Unknown encryption mode: " + mode_str);
  }
  
  void LoadMasterKey(const components::ComponentConfig& config) {
    std::string master_key;
    
    // Try to load from configuration
    if (config.HasMember("master-key")) {
      master_key = config["master-key"].As<std::string>();
    } else {
      // Generate new master key
      master_key = crypto::random::GenerateRandomString(32); // 256 bits
      LOG_INFO() << "Generated new master key";
    }
    
    // Initialize current key
    KeyInfo key_info;
    key_info.key = master_key;
    key_info.created_at = std::chrono::system_clock::now();
    key_info.id = "master";
    
    auto keys = encryption_keys_.Lock();
    (*keys)["current"] = key_info;
    (*keys)["master"] = key_info;
  }
  
  KeyInfo GetCurrentKey() {
    auto keys = encryption_keys_.Lock();
    auto it = keys->find("current");
    if (it != keys->end()) {
      return it->second;
    }
    
    throw std::runtime_error("No current encryption key available");
  }
  
  KeyInfo GetKey(const std::string& key_id) {
    auto keys = encryption_keys_.Lock();
    auto it = keys->find(key_id);
    if (it != keys->end()) {
      return it->second;
    }
    
    throw std::runtime_error("Encryption key not found: " + key_id);
  }
  
  KeyInfo GetKeyForTimestamp(const std::chrono::system_clock::time_point& timestamp) {
    // In a real implementation, this would find the appropriate key
    // based on when the data was encrypted
    return GetCurrentKey();
  }
  
  void KeyRotationLoop() {
    while (!should_stop_) {
      engine::SleepFor(key_rotation_interval_);
      
      try {
        RotateKey();
      } catch (const std::exception& e) {
        LOG_ERROR() << "Key rotation failed: " << e.what();
      }
    }
  }
  
  void RotateKey() {
    LOG_INFO() << "Rotating encryption key";
    
    // Generate new key
    std::string new_key = crypto::random::GenerateRandomString(32); // 256 bits
    
    KeyInfo new_key_info;
    new_key_info.key = new_key;
    new_key_info.created_at = std::chrono::system_clock::now();
    new_key_info.id = "key-" + std::to_string(
      std::chrono::duration_cast<std::chrono::seconds>(
        new_key_info.created_at.time_since_epoch()
      ).count()
    );
    
    // Update current key
    auto keys = encryption_keys_.Lock();
    (*keys)["previous"] = (*keys)["current"];
    (*keys)["current"] = new_key_info;
    
    LOG_INFO() << "Key rotation completed, new key ID: " << new_key_info.id;
  }
  
  std::string AddPkcs7Padding(const std::string& data, size_t block_size) {
    size_t padding_length = block_size - (data.length() % block_size);
    std::string padded_data = data;
    padded_data.append(padding_length, static_cast<char>(padding_length));
    return padded_data;
  }
  
  std::string RemovePkcs7Padding(const std::string& padded_data) {
    if (padded_data.empty()) return padded_data;
    
    unsigned char padding_length = padded_data.back();
    if (padding_length > 0 && padding_length <= 16) {
      return padded_data.substr(0, padded_data.length() - padding_length);
    }
    
    return padded_data; // No padding or invalid padding
  }
  
  concurrent::Variable<std::unordered_map<std::string, KeyInfo>> encryption_keys_;
  std::chrono::seconds key_rotation_interval_;
  EncryptionMode encryption_mode_;
  engine::TaskWithResult<void> key_rotation_task_;
  std::atomic<bool> should_stop_{false};
};

// Encryption service component
class EncryptionServiceComponent : public components::ComponentBase {
public:
  static constexpr std::string_view kName = "encryption-service";
  
  EncryptionServiceComponent(const components::ComponentConfig& config,
                            const components::ComponentContext& context)
    : ComponentBase(config, context)
    , encryption_service_(std::make_unique<DataEncryptionService>(config, context)) {}
  
  DataEncryptionService& GetEncryptionService() {
    return *encryption_service_;
  }

private:
  std::unique_ptr<DataEncryptionService> encryption_service_;
};
```

## Key Management Implementation

### Key Management Service
```cpp
#include <userver/components/component.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/crypto/kms.hpp>

class KeyManagementService {
public:
  struct KeyMetadata {
    std::string id;
    std::string name;
    std::string algorithm;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point last_used;
    bool enabled = true;
    std::string description;
    std::vector<std::string> tags;
  };
  
  KeyManagementService(const components::ComponentConfig& config,
                      const components::ComponentContext& context)
    : default_key_ttl_(config["default-key-ttl"].As<int>(2592000)) // 30 days
    , enable_key_rotation_(config["enable-key-rotation"].As<bool>(true)) {
    
    // Initialize KMS client if configured
    if (config.HasMember("kms-provider")) {
      kms_provider_ = config["kms-provider"].As<std::string>();
      InitializeKmsClient(config);
    }
    
    // Load existing keys
    LoadKeys(config);
  }
  
  std::string GenerateKey(const std::string& name,
                         const std::string& algorithm = "AES-256",
                         int ttl_seconds = 0) {
    if (ttl_seconds <= 0) {
      ttl_seconds = default_key_ttl_.count();
    }
    
    std::string key_id = GenerateKeyId();
    std::string key_material;
    
    if (kms_provider_.has_value()) {
      // Generate key in KMS
      key_material = GenerateKeyInKms(key_id, algorithm);
    } else {
      // Generate local key
      if (algorithm == "AES-256") {
        key_material = crypto::random::GenerateRandomString(32);
      } else {
        throw std::runtime_error("Unsupported algorithm: " + algorithm);
      }
    }
    
    // Store key metadata
    KeyMetadata metadata;
    metadata.id = key_id;
    metadata.name = name;
    metadata.algorithm = algorithm;
    metadata.created_at = std::chrono::system_clock::now();
    metadata.last_used = metadata.created_at;
    metadata.enabled = true;
    
    auto keys = key_store_.Lock();
    (*keys)[key_id] = std::make_pair(key_material, metadata);
    
    LOG_INFO() << "Generated new key: " << key_id << " (" << name << ")";
    
    // Schedule key rotation if enabled
    if (enable_key_rotation_) {
      ScheduleKeyRotation(key_id, std::chrono::seconds(ttl_seconds));
    }
    
    return key_id;
  }
  
  std::string GetKey(const std::string& key_id) {
    auto keys = key_store_.Lock();
    auto it = keys->find(key_id);
    
    if (it == keys->end()) {
      throw std::runtime_error("Key not found: " + key_id);
    }
    
    auto& [key_material, metadata] = it->second;
    
    if (!metadata.enabled) {
      throw std::runtime_error("Key is disabled: " + key_id);
    }
    
    // Update last used timestamp
    metadata.last_used = std::chrono::system_clock::now();
    
    if (kms_provider_.has_value()) {
      // Decrypt key material from KMS if needed
      return DecryptKeyFromKms(key_id, key_material);
    }
    
    return key_material;
  }
  
  void DisableKey(const std::string& key_id) {
    auto keys = key_store_.Lock();
    auto it = keys->find(key_id);
    
    if (it != keys->end()) {
      it->second.second.enabled = false;
      LOG_INFO() << "Disabled key: " << key_id;
    }
  }
  
  void EnableKey(const std::string& key_id) {
    auto keys = key_store_.Lock();
    auto it = keys->find(key_id);
    
    if (it != keys->end()) {
      it->second.second.enabled = true;
      LOG_INFO() << "Enabled key: " << key_id;
    }
  }
  
  std::vector<KeyMetadata> ListKeys() {
    std::vector<KeyMetadata> result;
    
    auto keys = key_store_.Lock();
    for (const auto& [key_id, key_pair] : *keys) {
      result.push_back(key_pair.second);
    }
    
    return result;
  }
  
  void RotateKey(const std::string& key_id) {
    auto keys = key_store_.Lock();
    auto it = keys->find(key_id);
    
    if (it == keys->end()) {
      throw std::runtime_error("Key not found: " + key_id);
    }
    
    auto& [old_key_material, metadata] = it->second;
    
    // Generate new key material
    std::string new_key_material;
    if (kms_provider_.has_value()) {
      new_key_material = GenerateKeyInKms(key_id + "-rotated", metadata.algorithm);
    } else {
      if (metadata.algorithm == "AES-256") {
        new_key_material = crypto::random::GenerateRandomString(32);
      } else {
        throw std::runtime_error("Unsupported algorithm: " + metadata.algorithm);
      }
    }
    
    // Update key material
    old_key_material = new_key_material;
    metadata.created_at = std::chrono::system_clock::now();
    
    LOG_INFO() << "Rotated key: " << key_id;
  }

private:
  void InitializeKmsClient(const components::ComponentConfig& config) {
    // Initialize KMS client based on provider
    // This is a simplified example
    LOG_INFO() << "Initialized KMS client for provider: " << kms_provider_.value();
  }
  
  std::string GenerateKeyInKms(const std::string& key_id, const std::string& algorithm) {
    // Generate key in KMS and return key identifier or encrypted key material
    // This is a simplified example
    return "kms://" + kms_provider_.value() + "/" + key_id;
  }
  
  std::string DecryptKeyFromKms(const std::string& key_id, const std::string& encrypted_key) {
    // Decrypt key material from KMS
    // This is a simplified example
    return crypto::random::GenerateRandomString(32);
  }
  
  void LoadKeys(const components::ComponentConfig& config) {
    // Load existing keys from storage
    // This is a simplified example
    LOG_INFO() << "Loaded key store";
  }
  
  std::string GenerateKeyId() {
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    return "key-" + std::to_string(now) + "-" + 
           crypto::random::GenerateRandomString(8);
  }
  
  void ScheduleKeyRotation(const std::string& key_id, std::chrono::seconds ttl) {
    // Schedule automatic key rotation
    // This is a simplified example
    LOG_INFO() << "Scheduled key rotation for " << key_id << " in " << ttl.count() << " seconds";
  }
  
  concurrent::Variable<std::unordered_map<std::string, std::pair<std::string, KeyMetadata>>> key_store_;
  std::optional<std::string> kms_provider_;
  std::chrono::seconds default_key_ttl_;
  bool enable_key_rotation_;
};

// Key management component
class KeyManagementComponent : public components::ComponentBase {
public:
  static constexpr std::string_view kName = "key-management";
  
  KeyManagementComponent(const components::ComponentConfig& config,
                        const components::ComponentContext& context)
    : ComponentBase(config, context)
    , key_manager_(std::make_unique<KeyManagementService>(config, context)) {}
  
  KeyManagementService& GetKeyManager() {
    return *key_manager_;
  }

private:
  std::unique_ptr<KeyManagementService> key_manager_;
};
```

## Certificate Management Implementation

### Certificate Manager
```cpp
#include <userver/components/component.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/crypto/certificate.hpp>

class CertificateManager {
public:
  struct CertificateInfo {
    std::string id;
    std::string name;
    std::string type; // server, client, ca
    std::chrono::system_clock::time_point not_before;
    std::chrono::system_clock::time_point not_after;
    std::chrono::system_clock::time_point last_updated;
    bool is_valid = true;
    std::vector<std::string> domains; // For server certificates
    std::string issuer;
    std::string subject;
  };
  
  CertificateManager(const components::ComponentConfig& config,
                    const components::ComponentContext& context)
    : cert_check_interval_(config["cert-check-interval"].As<int>(3600)) // 1 hour
    , auto_renew_threshold_(config["auto-renew-threshold"].As<int>(604800)) // 7 days
    , storage_path_(config["storage-path"].As<std::string>("")) {
    
    // Load existing certificates
    LoadCertificates(config);
    
    // Start certificate monitoring task
    if (cert_check_interval_.count() > 0) {
      cert_monitoring_task_ = utils::Async("cert-monitoring", [this]() {
        CertificateMonitoringLoop();
      });
    }
  }
  
  void AddCertificate(const std::string& name,
                     const std::string& cert_path,
                     const std::string& key_path,
                     const std::string& type = "server") {
    auto cert = crypto::Certificate::LoadFromFile(cert_path);
    auto key = crypto::PrivateKey::LoadFromFile(key_path);
    
    // Validate certificate
    ValidateCertificate(cert, key);
    
    CertificateInfo cert_info;
    cert_info.id = GenerateCertId();
    cert_info.name = name;
    cert_info.type = type;
    cert_info.not_before = cert.GetNotBefore();
    cert_info.not_after = cert.GetNotAfter();
    cert_info.last_updated = std::chrono::system_clock::now();
    cert_info.is_valid = true;
    cert_info.issuer = cert.GetIssuer();
    cert_info.subject = cert.GetSubject();
    
    // Extract domains for server certificates
    if (type == "server") {
      cert_info.domains = cert.GetSubjectAltNames();
      if (cert_info.domains.empty()) {
        auto common_name = cert.GetSubjectCommonName();
        if (!common_name.empty()) {
          cert_info.domains.push_back(common_name);
        }
      }
    }
    
    // Store certificate
    auto certs = certificates_.Lock();
    (*certs)[cert_info.id] = std::make_pair(cert, cert_info);
    
    LOG_INFO() << "Added certificate: " << cert_info.id << " (" << name << ")";
  }
  
  crypto::Certificate GetCertificate(const std::string& cert_id) {
    auto certs = certificates_.Lock();
    auto it = certs->find(cert_id);
    
    if (it == certs->end()) {
      throw std::runtime_error("Certificate not found: " + cert_id);
    }
    
    return it->second.first;
  }
  
  CertificateInfo GetCertificateInfo(const std::string& cert_id) {
    auto certs = certificates_.Lock();
    auto it = certs->find(cert_id);
    
    if (it == certs->end()) {
      throw std::runtime_error("Certificate not found: " + cert_id);
    }
    
    return it->second.second;
  }
  
  std::vector<CertificateInfo> ListCertificates() {
    std::vector<CertificateInfo> result;
    
    auto certs = certificates_.Lock();
    for (const auto& [cert_id, cert_pair] : *certs) {
      result.push_back(cert_pair.second);
    }
    
    return result;
  }
  
  bool IsCertificateValid(const std::string& cert_id) {
    try {
      auto cert_info = GetCertificateInfo(cert_id);
      auto now = std::chrono::system_clock::now();
      
      return cert_info.is_valid && 
             now >= cert_info.not_before && 
             now <= cert_info.not_after;
    } catch (...) {
      return false;
    }
  }
  
  std::chrono::system_clock::time_point GetCertificateExpiry(const std::string& cert_id) {
    auto cert_info = GetCertificateInfo(cert_id);
    return cert_info.not_after;
  }
  
  void RenewCertificate(const std::string& cert_id) {
    // Implementation for certificate renewal
    // This would typically involve calling a CA or certificate management service
    LOG_INFO() << "Renewing certificate: " << cert_id;
  }

private:
  void LoadCertificates(const components::ComponentConfig& config) {
    // Load existing certificates from storage
    // This is a simplified example
    LOG_INFO() << "Loaded certificate store";
  }
  
  void ValidateCertificate(const crypto::Certificate& cert,
                          const crypto::PrivateKey& key) {
    // Validate certificate structure
    if (!cert.IsValid()) {
      throw std::runtime_error("Invalid certificate");
    }
    
    // Validate certificate validity period
    auto now = std::chrono::system_clock::now();
    if (now < cert.GetNotBefore() || now > cert.GetNotAfter()) {
      throw std::runtime_error("Certificate is not valid at current time");
    }
    
    // Validate key pair match
    if (!cert.MatchesPrivateKey(key)) {
      throw std::runtime_error("Certificate does not match private key");
    }
  }
  
  void CertificateMonitoringLoop() {
    while (!should_stop_) {
      engine::SleepFor(cert_check_interval_);
      
      try {
        CheckCertificates();
      } catch (const std::exception& e) {
        LOG_ERROR() << "Certificate monitoring failed: " << e.what();
      }
    }
  }
  
  void CheckCertificates() {
    LOG_DEBUG() << "Checking certificates for expiration";
    
    auto now = std::chrono::system_clock::now();
    auto renewal_threshold = now + std::chrono::seconds(auto_renew_threshold_);
    
    auto certs = certificates_.Lock();
    for (auto& [cert_id, cert_pair] : *certs) {
      auto& [cert, cert_info] = cert_pair;
      
      // Check if certificate is expired
      if (now > cert_info.not_after) {
        LOG_WARNING() << "Certificate expired: " << cert_info.name << " (" << cert_id << ")";
        cert_info.is_valid = false;
        continue;
      }
      
      // Check if certificate needs renewal
      if (now > cert_info.not_after - std::chrono::seconds(auto_renew_threshold_)) {
        LOG_WARNING() << "Certificate needs renewal: " << cert_info.name << " (" << cert_id << ")";
        // Trigger renewal process
        if (auto_renew_enabled_) {
          // In a real implementation, this would trigger the renewal process
          LOG_INFO() << "Auto-renewal triggered for certificate: " << cert_id;
        }
      }
    }
  }
  
  std::string GenerateCertId() {
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    return "cert-" + std::to_string(now);
  }
  
  concurrent::Variable<std::unordered_map<std::string, std::pair<crypto::Certificate, CertificateInfo>>> certificates_;
  std::chrono::seconds cert_check_interval_;
  std::chrono::seconds auto_renew_threshold_;
  std::string storage_path_;
  bool auto_renew_enabled_ = true;
  engine::TaskWithResult<void> cert_monitoring_task_;
  std::atomic<bool> should_stop_{false};
};

// Certificate management component
class CertificateManagementComponent : public components::ComponentBase {
public:
  static constexpr std::string_view kName = "certificate-management";
  
  CertificateManagementComponent(const components::ComponentConfig& config,
                                const components::ComponentContext& context)
    : ComponentBase(config, context)
    , cert_manager_(std::make_unique<CertificateManager>(config, context)) {}
  
  CertificateManager& GetCertificateManager() {
    return *cert_manager_;
  }

private:
  std::unique_ptr<CertificateManager> cert_manager_;
};
```

## Encryption Configuration

### Static Configuration
```yaml
# Encryption service configuration
encryption-service:
  mode: "AES-256-GCM"
  key-rotation-interval: 86400  # 24 hours
  master-key: ""  # If empty, generates new key
  enable-key-rotation: true

# Key management configuration
key-management:
  default-key-ttl: 2592000  # 30 days
  enable-key-rotation: true
  kms-provider: ""  # If empty, uses local key storage
  key-storage-path: "/var/lib/userver/keys"

# Certificate management configuration
certificate-management:
  cert-check-interval: 3600  # 1 hour
  auto-renew-threshold: 604800  # 7 days
  storage-path: "/etc/userver/certs"
  auto-renew-enabled: true

# TLS server configuration
tls-server:
  cert-file: "/etc/ssl/certs/server.crt"
  key-file: "/etc/ssl/private/server.key"
  ca-file: "/etc/ssl/certs/ca.crt"
  cipher-suites: "ECDHE-RSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256"
  min-version: "TLSv1.2"
  max-version: "TLSv1.3"
  verify-client: false
  session-cache-size: 1000
  session-timeout: 300

# Mutual TLS configuration
mutual-tls:
  server-cert: "/etc/ssl/certs/server.crt"
  server-key: "/etc/ssl/private/server.key"
  ca-cert: "/etc/ssl/certs/ca.crt"
  crl-file: "/etc/ssl/certs/crl.pem"
  require-client-cert: true
  verify-depth: 2
  check-crl: false
```

### Client TLS Configuration
```yaml
# TLS client configuration
tls-client:
  ca-file: "/etc/ssl/certs/ca.crt"
  cert-file: "/etc/ssl/certs/client.crt"
  key-file: "/etc/ssl/private/client.key"
  verify-server: true
  server-name: "api.example.com"
  cipher-suites: "ECDHE-RSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256"
  min-version: "TLSv1.2"
  max-version: "TLSv1.3"
  enable-session-resumption: true
```

## Cross-References

### Related Framework Components
- [Network Security Patterns](../../networking/network-security.md) - TLS/SSL integration, security middleware
- [Authentication Patterns](authentication.md) - JWT signing, secure token handling
- [Component System](../../../memory-bank/main/component-system.md) - Encryption component integration
- [Framework Core](../../../memory-bank/main/framework-core.md) - Core cryptographic primitives

### Security Best Practices
- [Security Best Practices](security-best-practices.md) - Secure key management, certificate handling
- [Vulnerability Prevention](vulnerability-prevention.md) - Encryption-related vulnerability prevention
- [Authorization Patterns](authorization.md) - Secure data access with encryption

### Implementation References
- [`engine::io::TlsWrapper`](https://userver.tech/df/d00/classengine_1_1io_1_1TlsWrapper.html)
- [`crypto::Certificate`](https://userver.tech/d6/d32/classcrypto_1_1Certificate.html)
- [`crypto::aes::Aes256`](https://userver.tech/d8/d32/classcrypto_1_1aes_1_1Aes256.html)
- [`crypto::Key`](https://userver.tech/dc/d32/classcrypto_1_1Key.html)
- [`concurrent::Variable`](https://userver.tech/d8/dcc/namespaceconcurrent.html)