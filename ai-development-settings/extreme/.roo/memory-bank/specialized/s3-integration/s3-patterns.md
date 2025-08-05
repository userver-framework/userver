# S3 and Object Storage Integration Patterns in Userver

## Overview

This document covers patterns and best practices for integrating with S3-compatible object storage services in userver-based applications. These patterns help build robust, scalable file storage solutions.

## S3 Client Implementation

### Basic S3 Client

#### HTTP-based S3 Client
```cpp
#include <userver/clients/http/client.hpp>
#include <userver/crypto/signers.hpp>
#include <userver/formats/json.hpp>

class S3Client {
public:
    struct Config {
        std::string endpoint;
        std::string access_key;
        std::string secret_key;
        std::string region;
        std::string bucket;
    };
    
    S3Client(const Config& config, clients::http::Client& http_client)
        : config_(config), http_client_(http_client) {}
    
    std::string PutObject(const std::string& key,
                         const std::string& data,
                         const std::string& content_type = "application/octet-stream") {
        
        auto url = BuildUrl(key);
        auto date = GetCurrentDate();
        auto signature = SignRequest("PUT", url, date, data, content_type);
        
        auto response = http_client_.CreateRequest()
            .method(clients::http::HttpMethod::kPut)
            .url(url)
            .header("Authorization", signature)
            .header("x-amz-date", date)
            .header("Content-Type", content_type)
            .body(data)
            .timeout(std::chrono::seconds(30))
            .retry(3)
            .perform();
        
        if (!response->IsOk()) {
            throw std::runtime_error("Failed to upload object: " + 
                                   std::to_string(response->GetStatusCode()));
        }
        
        return response->GetHeader("ETag");
    }
    
    std::string GetObject(const std::string& key) {
        auto url = BuildUrl(key);
        auto date = GetCurrentDate();
        auto signature = SignRequest("GET", url, date, "", "");
        
        auto response = http_client_.CreateRequest()
            .method(clients::http::HttpMethod::kGet)
            .url(url)
            .header("Authorization", signature)
            .header("x-amz-date", date)
            .timeout(std::chrono::seconds(30))
            .retry(3)
            .perform();
        
        if (!response->IsOk()) {
            throw std::runtime_error("Failed to download object: " + 
                                   std::to_string(response->GetStatusCode()));
        }
        
        return response->body();
    }
    
    void DeleteObject(const std::string& key) {
        auto url = BuildUrl(key);
        auto date = GetCurrentDate();
        auto signature = SignRequest("DELETE", url, date, "", "");
        
        auto response = http_client_.CreateRequest()
            .method(clients::http::HttpMethod::kDelete)
            .url(url)
            .header("Authorization", signature)
            .header("x-amz-date", date)
            .timeout(std::chrono::seconds(30))
            .retry(3)
            .perform();
        
        if (!response->IsOk() && response->GetStatusCode() != 204) {
            throw std::runtime_error("Failed to delete object: " + 
                                   std::to_string(response->GetStatusCode()));
        }
    }
    
private:
    std::string BuildUrl(const std::string& key) {
        return config_.endpoint + "/" + config_.bucket + "/" + key;
    }
    
    std::string GetCurrentDate() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&time_t), "%Y%m%dT%H%M%SZ");
        return ss.str();
    }
    
    std::string SignRequest(const std::string& method,
                           const std::string& url,
                           const std::string& date,
                           const std::string& payload,
                           const std::string& content_type) {
        // Simplified AWS Signature Version 4 implementation
        // In practice, use a proper AWS SDK or library
        
        std::string canonical_request = method + "\n" +
                                       GetPathFromUrl(url) + "\n" +
                                       GetQueryFromUrl(url) + "\n" +
                                       "host:" + GetHostFromUrl(url) + "\n" +
                                       "x-amz-date:" + date + "\n" +
                                       "\n" +
                                       "host;x-amz-date\n" +
                                       crypto::SHA256(payload);
        
        std::string string_to_sign = "AWS4-HMAC-SHA256\n" +
                                    date + "\n" +
                                    GetDatePart(date) + "/" + config_.region + "/s3/aws4_request\n" +
                                    crypto::SHA256(canonical_request);
        
        auto signature = crypto::HMACSHA256(
            crypto::HMACSHA256(
                crypto::HMACSHA256(
                    crypto::HMACSHA256(
                        "AWS4" + config_.secret_key,
                        GetDatePart(date)
                    ),
                    config_.region
                ),
                "s3"
            ),
            "aws4_request"
        );
        
        return "AWS4-HMAC-SHA256 Credential=" + config_.access_key + "/" +
               GetDatePart(date) + "/" + config_.region + "/s3/aws4_request, " +
               "SignedHeaders=host;x-amz-date, Signature=" +
               crypto::HMACSHA256(signature, string_to_sign);
    }
    
    std::string GetPathFromUrl(const std::string& url) {
        // Extract path from URL
        auto pos = url.find('/', 8); // Skip http:// or https://
        if (pos == std::string::npos) return "/";
        return url.substr(pos);
    }
    
    std::string GetQueryFromUrl(const std::string& url) {
        // Extract query from URL
        auto pos = url.find('?');
        if (pos == std::string::npos) return "";
        return url.substr(pos + 1);
    }
    
    std::string GetHostFromUrl(const std::string& url) {
        // Extract host from URL
        auto start = url.find("://") + 3;
        auto end = url.find('/', start);
        if (end == std::string::npos) return url.substr(start);
        return url.substr(start, end - start);
    }
    
    std::string GetDatePart(const std::string& date) {
        return date.substr(0, 8);
    }
    
    Config config_;
    clients::http::Client& http_client_;
};
```

### Connection Pooling for S3

#### S3 Connection Manager
```cpp
class S3ConnectionManager {
public:
    struct Connection {
        clients::http::ClientPtr client;
        std::chrono::steady_clock::time_point last_used;
        bool is_healthy{true};
    };
    
    S3ConnectionManager(clients::http::Client& http_client,
                       const S3Client::Config& config,
                       size_t max_connections = 10)
        : http_client_(http_client)
        , config_(config)
        , max_connections_(max_connections) {}
    
    std::shared_ptr<S3Client> AcquireClient() {
        std::lock_guard lock(mutex_);
        
        // Try to find an existing client
        for (auto it = available_clients_.begin(); it != available_clients_.end();) {
            if (IsClientHealthy(*it)) {
                auto client = std::move(*it);
                available_clients_.erase(it);
                return client;
            } else {
                it = available_clients_.erase(it);
            }
        }
        
        // Create new client if under limit
        if (total_clients_ < max_connections_) {
            auto client = std::make_shared<S3Client>(config_, http_client_);
            ++total_clients_;
            return client;
        }
        
        // Wait for available client (with timeout)
        throw std::runtime_error("No available S3 connections");
    }
    
    void ReleaseClient(std::shared_ptr<S3Client> client) {
        if (!IsClientHealthy(client)) {
            std::lock_guard lock(mutex_);
            --total_clients_;
            return; // Don't return broken clients to pool
        }
        
        std::lock_guard lock(mutex_);
        available_clients_.push_back(std::move(client));
    }
    
private:
    bool IsClientHealthy(const std::shared_ptr<S3Client>& client) {
        if (!client) return false;
        // Implement health check logic
        return true;
    }
    
    clients::http::Client& http_client_;
    S3Client::Config config_;
    size_t max_connections_;
    
    std::mutex mutex_;
    std::vector<std::shared_ptr<S3Client>> available_clients_;
    size_t total_clients_{0};
};
```

## Advanced S3 Patterns

### Multipart Upload

#### Large File Upload
```cpp
class MultipartS3Uploader {
public:
    struct UploadPart {
        int part_number;
        std::string etag;
    };
    
    MultipartS3Uploader(S3Client& s3_client,
                       const std::string& key,
                       size_t part_size = 5 * 1024 * 1024) // 5MB default
        : s3_client_(s3_client)
        , key_(key)
        , part_size_(part_size) {}
    
    void UploadFile(const std::string& file_path) {
        // Initiate multipart upload
        auto upload_id = InitiateMultipartUpload();
        
        try {
            std::vector<UploadPart> parts;
            
            // Read file in chunks and upload parts
            std::ifstream file(file_path, std::ios::binary);
            std::string buffer(part_size_, '\0');
            int part_number = 1;
            
            while (file.read(buffer.data(), part_size_) || file.gcount() > 0) {
                buffer.resize(file.gcount());
                
                auto etag = UploadPart(upload_id, part_number, buffer);
                parts.push_back({part_number, etag});
                
                buffer.resize(part_size_);
                ++part_number;
            }
            
            // Complete multipart upload
            CompleteMultipartUpload(upload_id, parts);
            
        } catch (const std::exception& ex) {
            // Abort multipart upload on error
            AbortMultipartUpload(upload_id);
            throw;
        }
    }
    
private:
    std::string InitiateMultipartUpload() {
        // Implementation for initiating multipart upload
        // This would involve making a POST request to S3
        return "upload-id-placeholder";
    }
    
    std::string UploadPart(const std::string& upload_id,
                          int part_number,
                          const std::string& data) {
        // Implementation for uploading a part
        // This would involve making a PUT request with upload ID and part number
        return "etag-placeholder";
    }
    
    void CompleteMultipartUpload(const std::string& upload_id,
                               const std::vector<UploadPart>& parts) {
        // Implementation for completing multipart upload
        // This would involve making a POST request with part information
    }
    
    void AbortMultipartUpload(const std::string& upload_id) {
        // Implementation for aborting multipart upload
        // This would involve making a DELETE request
    }
    
    S3Client& s3_client_;
    std::string key_;
    size_t part_size_;
};
```

### Streaming Upload/Download

#### Stream-based Operations
```cpp
class StreamingS3Client {
public:
    template<typename InputStream>
    std::string UploadStream(const std::string& key,
                            InputStream& input_stream,
                            size_t buffer_size = 64 * 1024) {
        // For small files, use direct upload
        if (GetSize(input_stream) < 5 * 1024 * 1024) {
            std::string data;
            char buffer[buffer_size];
            while (input_stream.read(buffer, buffer_size) || input_stream.gcount() > 0) {
                data.append(buffer, input_stream.gcount());
            }
            return s3_client_.PutObject(key, data);
        }
        
        // For large files, use multipart upload
        return UploadLargeStream(key, input_stream, buffer_size);
    }
    
    template<typename OutputStream>
    void DownloadStream(const std::string& key,
                       OutputStream& output_stream,
                       size_t buffer_size = 64 * 1024) {
        // Stream download implementation
        // This would involve range requests for large files
        auto data = s3_client_.GetObject(key);
        output_stream.write(data.data(), data.size());
    }
    
private:
    template<typename InputStream>
    std::string UploadLargeStream(const std::string& key,
                                 InputStream& input_stream,
                                 size_t buffer_size) {
        MultipartS3Uploader uploader(s3_client_, key);
        // Implementation would involve creating a temporary file
        // or using a more sophisticated streaming approach
        throw std::runtime_error("Streaming multipart upload not implemented");
    }
    
    template<typename InputStream>
    size_t GetSize(InputStream& stream) {
        auto current_pos = stream.tellg();
        stream.seekg(0, std::ios::end);
        auto size = stream.tellg();
        stream.seekg(current_pos);
        return static_cast<size_t>(size);
    }
    
    S3Client& s3_client_;
};
```

## Error Handling and Resilience

### Retry Logic

#### Exponential Backoff
```cpp
class ResilientS3Client {
public:
    ResilientS3Client(S3Client& s3_client,
                     int max_retries = 3,
                     std::chrono::milliseconds base_delay = std::chrono::milliseconds(100))
        : s3_client_(s3_client)
        , max_retries_(max_retries)
        , base_delay_(base_delay) {}
    
    template<typename Operation>
    auto ExecuteWithRetry(Operation&& operation) -> decltype(operation()) {
        for (int attempt = 0; attempt <= max_retries_; ++attempt) {
            try {
                return operation();
            } catch (const std::exception& ex) {
                if (attempt == max_retries_ || !IsRetryableError(ex)) {
                    throw; // Last attempt or non-retryable error
                }
                
                auto delay = CalculateDelay(attempt);
                LOG_WARNING() << "S3 operation failed (attempt " << (attempt + 1) 
                             << "), retrying in " << delay.count() << "ms: " << ex.what();
                
                engine::SleepFor(delay);
            }
        }
        
        throw std::runtime_error("Unreachable code");
    }
    
private:
    bool IsRetryableError(const std::exception& ex) {
        // Implement logic to determine if error is retryable
        // Common retryable errors: network timeouts, 5xx errors, throttling
        std::string error_message = ex.what();
        return error_message.find("timeout") != std::string::npos ||
               error_message.find("503") != std::string::npos ||
               error_message.find("throttling") != std::string::npos;
    }
    
    std::chrono::milliseconds CalculateDelay(int attempt) {
        // Exponential backoff with jitter
        auto base_delay = base_delay_ * (1 << attempt);
        auto jitter = std::chrono::milliseconds(rand() % 100);
        return base_delay + jitter;
    }
    
    S3Client& s3_client_;
    int max_retries_;
    std::chrono::milliseconds base_delay_;
};
```

### Circuit Breaker Pattern

#### S3 Circuit Breaker
```cpp
class S3CircuitBreaker {
public:
    enum class State {
        kClosed,
        kOpen,
        kHalfOpen
    };
    
    S3CircuitBreaker(S3Client& s3_client,
                    int failure_threshold = 5,
                    std::chrono::milliseconds timeout = std::chrono::minutes(1))
        : s3_client_(s3_client)
        , failure_threshold_(failure_threshold)
        , timeout_(timeout) {}
    
    template<typename Operation>
    auto Execute(Operation&& operation) -> decltype(operation()) {
        if (state_ == State::kOpen) {
            if (std::chrono::steady_clock::now() < next_retry_time_) {
                throw std::runtime_error("Circuit breaker is open");
            }
            state_ = State::kHalfOpen;
        }
        
        try {
            auto result = operation();
            OnSuccess();
            return result;
        } catch (const std::exception& ex) {
            OnFailure();
            throw;
        }
    }
    
private:
    void OnSuccess() {
        std::lock_guard lock(mutex_);
        failure_count_ = 0;
        state_ = State::kClosed;
    }
    
    void OnFailure() {
        std::lock_guard lock(mutex_);
        ++failure_count_;
        
        if (failure_count_ >= failure_threshold_) {
            state_ = State::kOpen;
            next_retry_time_ = std::chrono::steady_clock::now() + timeout_;
        }
    }
    
    S3Client& s3_client_;
    int failure_threshold_;
    std::chrono::milliseconds timeout_;
    
    std::mutex mutex_;
    State state_{State::kClosed};
    int failure_count_{0};
    std::chrono::steady_clock::time_point next_retry_time_;
};
```

## Performance Optimization

### Caching Strategies

#### S3 Object Caching
```cpp
class CachedS3Client {
public:
    CachedS3Client(S3Client& s3_client,
                  cache::LruCache<std::string, std::string>& cache)
        : s3_client_(s3_client)
        , cache_(cache) {}
    
    std::string GetObject(const std::string& key) {
        // Try cache first
        if (auto cached = cache_.Get(key)) {
            cache_hits_++;
            return *cached;
        }
        
        // Load from S3
        auto data = s3_client_.GetObject(key);
        
        // Store in cache
        cache_.Put(key, data);
        cache_misses_++;
        
        return data;
    }
    
    void PutObject(const std::string& key, const std::string& data) {
        // Store in S3
        s3_client_.PutObject(key, data);
        
        // Update cache
        cache_.Put(key, data);
    }
    
    void InvalidateCache(const std::string& key) {
        cache_.Invalidate(key);
    }
    
    formats::json::Value GetCacheStatistics() const {
        formats::json::ValueBuilder stats;
        stats["hits"] = cache_hits_;
        stats["misses"] = cache_misses_;
        stats["hit_rate"] = cache_hits_ > 0 ? 
            static_cast<double>(cache_hits_) / (cache_hits_ + cache_misses_) : 0.0;
        return stats.ExtractValue();
    }
    
private:
    S3Client& s3_client_;
    cache::LruCache<std::string, std::string>& cache_;
    
    std::atomic<size_t> cache_hits_{0};
    std::atomic<size_t> cache_misses_{0};
};
```

### Connection Management

#### S3 Connection Pool
```cpp
class S3ConnectionPool {
public:
    struct ConnectionConfig {
        std::string endpoint;
        std::string access_key;
        std::string secret_key;
        std::string region;
        int max_connections{10};
        std::chrono::milliseconds connection_timeout{std::chrono::seconds(30)};
        std::chrono::milliseconds request_timeout{std::chrono::seconds(60)};
    };
    
    S3ConnectionPool(const ConnectionConfig& config)
        : config_(config) {
        InitializePool();
    }
    
    std::shared_ptr<S3Client> AcquireConnection() {
        std::unique_lock lock(mutex_);
        
        // Wait for available connection (with timeout)
        if (!cv_.wait_for(lock, std::chrono::seconds(10), 
                         [this] { return !available_connections_.empty(); })) {
            throw std::runtime_error("Timeout waiting for S3 connection");
        }
        
        auto connection = std::move(available_connections_.back());
        available_connections_.pop_back();
        
        return connection;
    }
    
    void ReleaseConnection(std::shared_ptr<S3Client> connection) {
        if (!connection) return;
        
        std::lock_guard lock(mutex_);
        available_connections_.push_back(std::move(connection));
        cv_.notify_one();
    }
    
private:
    void InitializePool() {
        for (int i = 0; i < config_.max_connections; ++i) {
            auto http_client = CreateHttpClient();
            auto s3_client = std::make_shared<S3Client>(
                S3Client::Config{
                    config_.endpoint,
                    config_.access_key,
                    config_.secret_key,
                    config_.region,
                    "" // bucket is specified per operation
                },
                *http_client
            );
            
            http_clients_.push_back(std::move(http_client));
            available_connections_.push_back(std::move(s3_client));
        }
    }
    
    std::unique_ptr<clients::http::Client> CreateHttpClient() {
        // Configure HTTP client with appropriate settings for S3
        clients::http::Client::Config config;
        config.timeout = config_.request_timeout;
        config.connection_timeout = config_.connection_timeout;
        config.max_connections = 1; // Each S3 client gets one connection
        
        return std::make_unique<clients::http::Client>(config);
    }
    
    ConnectionConfig config_;
    
    std::mutex mutex_;
    std::condition_variable cv_;
    std::vector<std::unique_ptr<clients::http::Client>> http_clients_;
    std::vector<std::shared_ptr<S3Client>> available_connections_;
};
```

## Security Considerations

### Credential Management

#### Secure Credential Handling
```cpp
class SecureS3Credentials {
public:
    struct Credentials {
        std::string access_key;
        std::string secret_key;
        std::chrono::steady_clock::time_point expiry;
    };
    
    SecureS3Credentials(dynamic_config::Source config_source)
        : config_source_(config_source) {
        LoadCredentials();
    }
    
    Credentials GetCredentials() const {
        std::shared_lock lock(mutex_);
        if (std::chrono::steady_clock::now() > credentials_.expiry) {
            throw std::runtime_error("S3 credentials expired");
        }
        return credentials_;
    }
    
    void RefreshCredentials() {
        std::unique_lock lock(mutex_);
        LoadCredentials();
    }
    
private:
    void LoadCredentials() {
        // Load from secure configuration source
        auto config = config_source_();
        
        credentials_.access_key = config["S3_ACCESS_KEY"].As<std::string>();
        credentials_.secret_key = config["S3_SECRET_KEY"].As<std::string>();
        credentials_.expiry = std::chrono::steady_clock::now() + 
                             std::chrono::hours(config["S3_CREDENTIALS_TTL_HOURS"].As<int>());
        
        // Clear sensitive data from memory when possible
        // This is a simplified example - in practice, use secure memory management
    }
    
    dynamic_config::Source config_source_;
    mutable std::shared_mutex mutex_;
    Credentials credentials_;
};
```

These S3 integration patterns provide a comprehensive foundation for building robust object storage solutions using the userver framework. They cover connection management, error handling, performance optimization, and security considerations essential for production-grade S3 integrations.