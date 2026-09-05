#include "TestHarness.h"
#include "Infrastructure/Persistence/PostgresDb.h"
#include "Infrastructure/Persistence/PostgresCacheRepository.h"
#include "Infrastructure/Persistence/PostgresMessageQueueRepository.h"
#include "Infrastructure/Security/JwtService.h"
#include "Infrastructure/Security/OpenSslCrypto.h"
#include "Infrastructure/Security/RsaKeyManager.h"
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

static auto getPerfDb() {
    static auto db = std::make_shared<Infrastructure::Persistence::PostgresDb>("", 20);
    return db;
}

TEST_CASE("Performance::Benchmarks", "ConnectionPoolHighConcurrencyLeasing") {
    auto db = getPerfDb();
    const int numThreads = 8;
    const int leasesPerThread = 15;
    std::atomic<size_t> successCount{0};

    auto start = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> workers;
    workers.reserve(numThreads);
    for (int t = 0; t < numThreads; ++t) {
        workers.emplace_back([&]() {
            for (int i = 0; i < leasesPerThread; ++i) {
                auto conn = db->getConnection();
                if (conn && conn->is_open()) {
                    pqxx::work tx{*conn};
                    auto res = tx.exec("SELECT 1");
                    if (!res.empty()) {
                        successCount++;
                    }
                    tx.commit();
                }
            }
        });
    }

    for (auto& w : workers) {
        w.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    EXPECT_EQ(successCount.load(), numThreads * leasesPerThread);
    std::cout << "         [Benchmark] " << (numThreads * leasesPerThread) << " pooled DB queries completed in "
              << totalMs << " ms (" << (totalMs > 0 ? (successCount.load() * 1000 / totalMs) : 9999) << " ops/sec)\n";
}

TEST_CASE("Performance::Benchmarks", "JwtRs256SigningAndVerificationThroughput") {
    auto keyMgr = std::make_shared<Infrastructure::Security::RsaKeyManager>("perf-key-2026");
    Infrastructure::Security::JwtService jwtService(keyMgr, "perf-issuer", "perf-aud");

    const int tokenCount = 100;

    // 1. Benchmark Signing
    auto signStart = std::chrono::high_resolution_clock::now();
    std::vector<std::string> tokens;
    tokens.reserve(tokenCount);

    for (int i = 0; i < tokenCount; ++i) {
        tokens.push_back(jwtService.createAccessToken(
            "usr-perf-benchmark",
            "user",
            "sess-perf-benchmark",
            "jti-perf-" + std::to_string(i),
            std::chrono::hours(1)
        ));
    }
    auto signEnd = std::chrono::high_resolution_clock::now();
    auto signMs = std::chrono::duration_cast<std::chrono::milliseconds>(signEnd - signStart).count();

    EXPECT_EQ(tokens.size(), tokenCount);
    std::cout << "         [Benchmark] " << tokenCount << " RS256 JWTs signed in " << signMs << " ms ("
              << (signMs > 0 ? (tokenCount * 1000 / signMs) : 9999) << " tokens/sec)\n";

    // 2. Benchmark Verification
    auto verifyStart = std::chrono::high_resolution_clock::now();
    size_t verifiedCount = 0;
    for (const auto& token : tokens) {
        auto claimsRes = jwtService.validateAccessToken(token);
        if (claimsRes.isSuccess()) {
            verifiedCount++;
        }
    }
    auto verifyEnd = std::chrono::high_resolution_clock::now();
    auto verifyMs = std::chrono::duration_cast<std::chrono::milliseconds>(verifyEnd - verifyStart).count();

    EXPECT_EQ(verifiedCount, tokenCount);
    std::cout << "         [Benchmark] " << tokenCount << " RS256 JWTs verified in " << verifyMs << " ms ("
              << (verifyMs > 0 ? (tokenCount * 1000 / verifyMs) : 9999) << " verifications/sec)\n";
}

TEST_CASE("Performance::Benchmarks", "CacheKvHighThroughputBurst") {
    auto db = getPerfDb();
    Infrastructure::Persistence::PostgresCacheRepository cacheRepo(db);

    const int burstCount = 50;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < burstCount; ++i) {
        std::string key = "perf:cache:" + std::to_string(i);
        cacheRepo.set(key, "perf_payload_data_" + std::to_string(i), 300);
    }

    size_t retrievedCount = 0;
    for (int i = 0; i < burstCount; ++i) {
        std::string key = "perf:cache:" + std::to_string(i);
        auto val = cacheRepo.get(key);
        if (val.has_value()) {
            retrievedCount++;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    EXPECT_EQ(retrievedCount, burstCount);
    std::cout << "         [Benchmark] " << (burstCount * 2) << " Cache KV Set/Get ops in " << totalMs << " ms ("
              << (totalMs > 0 ? (burstCount * 2 * 1000 / totalMs) : 9999) << " ops/sec)\n";
}

TEST_CASE("Performance::Benchmarks", "QueueConcurrentWorkerDrainSkipLocked") {
    auto db = getPerfDb();
    Infrastructure::Persistence::PostgresMessageQueueRepository queueRepo(db);

    std::string topic = "perf.queue.topic." + Infrastructure::Security::OpenSslCrypto::generateSecureToken(6);
    const int messageCount = 20;

    for (int i = 0; i < messageCount; ++i) {
        queueRepo.publish(topic, "{\"task_id\":" + std::to_string(i) + "}");
    }

    std::atomic<size_t> drainedCount{0};
    const int workerCount = 4;
    std::vector<std::thread> workers;

    auto start = std::chrono::high_resolution_clock::now();

    for (int w = 0; w < workerCount; ++w) {
        workers.emplace_back([&]() {
            while (true) {
                auto msg = queueRepo.pollNext(topic);
                if (!msg.has_value()) {
                    break;
                }
                queueRepo.acknowledge(msg->id);
                drainedCount++;
            }
        });
    }

    for (auto& w : workers) {
        w.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    EXPECT_EQ(drainedCount.load(), messageCount);
    std::cout << "         [Benchmark] " << messageCount << " messages drained with " << workerCount
              << " concurrent SKIP LOCKED workers in " << totalMs << " ms\n";
}
