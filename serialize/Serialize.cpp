#include <iostream>
#include <string>
#include <numeric>

#include <benchmark/benchmark.h>

#include <thrift/lib/cpp/transport/TBufferTransports.h>
#include <thrift/lib/cpp/util/ThriftSerializer.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "gen-cpp2/StorageRpc_types.h"

using namespace folly;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::util;

static void BM_BinarySerialize(benchmark::State& state) {
	for (auto _ : state) {
		state.PauseTiming();
		pio_thrift::WriteRequest write;
		write.set_reqid(1000);
		write.set_size(16*1024);
		write.set_offset(1024*1024*1024);
		auto iobuf = folly::IOBuf::create(state.range(0));
		write.set_data(std::move(iobuf));
		state.ResumeTiming();

		for (int j = 0; j < state.range(1); ++j) {
			benchmark::DoNotOptimize(BinarySerializer::serialize<std::string>(write));
		}
	}
}

static void BM_CompactSerialize(benchmark::State& state) {
	for (auto _ : state) {
		state.PauseTiming();
		pio_thrift::WriteRequest write;
		write.set_reqid(1000);
		write.set_size(16*1024);
		write.set_offset(1024*1024*1024);
		auto iobuf = folly::IOBuf::create(state.range(0));
		write.set_data(std::move(iobuf));
		state.ResumeTiming();

		for (int j = 0; j < state.range(1); ++j) {
			benchmark::DoNotOptimize(CompactSerializer::serialize<std::string>(write));
		}
	}
}

static void BM_BinaryDeSerialize(benchmark::State& state) {
	for (auto _ : state) {
		state.PauseTiming();
		pio_thrift::WriteRequest write;
		write.set_reqid(1000);
		write.set_size(16*1024);
		write.set_offset(1024*1024*1024);
		auto iobuf = folly::IOBuf::create(state.range(0));
		write.set_data(std::move(iobuf));
		const auto serialized = BinarySerializer::serialize<std::string>(write);
		state.ResumeTiming();

		for (int j = 0; j < state.range(1); ++j) {
			benchmark::DoNotOptimize(BinarySerializer::deserialize<pio_thrift::WriteRequest>(serialized));
		}
	}
}

static void BM_CompactDeSerialize(benchmark::State& state) {
	for (auto _ : state) {
		state.PauseTiming();
		pio_thrift::WriteRequest write;
		write.set_reqid(1000);
		write.set_size(16*1024);
		write.set_offset(1024*1024*1024);
		auto iobuf = folly::IOBuf::create(state.range(0));
		write.set_data(std::move(iobuf));
		const auto serialized = CompactSerializer::serialize<std::string>(write);
		state.ResumeTiming();

		for (int j = 0; j < state.range(1); ++j) {
			benchmark::DoNotOptimize(CompactSerializer::deserialize<pio_thrift::WriteRequest>(serialized));
		}
	}
}

size_t constexpr kIterations = 1 << 14;

BENCHMARK(BM_BinarySerialize)->Ranges({{1<<10, 1<<20}, {kIterations, kIterations}});
BENCHMARK(BM_CompactSerialize)->Ranges({{1<<10, 1<<20}, {kIterations, kIterations}});
BENCHMARK(BM_BinaryDeSerialize)->Ranges({{1<<10, 1<<20}, {kIterations, kIterations}});
BENCHMARK(BM_CompactDeSerialize)->Ranges({{1<<10, 1<<20}, {kIterations, kIterations}});


BENCHMARK_MAIN();
