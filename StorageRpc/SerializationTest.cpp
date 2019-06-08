#include <iostream>
#include <string>
#include <numeric>

#include <thrift/lib/cpp/protocol/TJSONProtocol.h>
#include <thrift/lib/cpp/transport/TBufferTransports.h>
#include <thrift/lib/cpp/util/ThriftSerializer.h>
#include <thrift/lib/cpp2/protocol/JSONProtocol.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "gen-cpp2/StorageRpc.h"

using namespace folly;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::util;

using S2 = SimpleJSONSerializer;

int main(int agrc, char* argv[]) {
	pio_thrift::Vmdk vmdk;
	vmdk.vmdkid = 10;
	vmdk.vmid = 2;
	vmdk.path = "/a/b/c/d/e/f";
	vmdk.config = "{'block_size':4096, 'compression' : 'snappy', 'aeroclustid' : 2}";


	std::vector<int64_t> nos(10);
	std::iota(nos.begin(), nos.end(), 1);
	vmdk.flushed_checkpoints = nos;

	vmdk.unflushed_checkpoints.emplace_back(9);
	vmdk.unflushed_checkpoints.emplace_back(10);

	const auto serialized = S2::serialize<std::string>(vmdk);
	std::cout << "Serialized " << serialized << std::endl;

	{
		pio_thrift::Vmdk vmdk1;
		const auto size = S2::deserialize(serialized, vmdk1);

		assert(vmdk1.vmdkid == vmdk.vmdkid);
		assert(vmdk1.vmid == vmdk.vmid);
		assert(vmdk1.path == vmdk.path);
		assert(vmdk1.config == vmdk.config);
		assert(vmdk1.unflushed_checkpoints == vmdk.unflushed_checkpoints);
		assert(vmdk1.flushed_checkpoints == vmdk.flushed_checkpoints);
	}

	return 0;
}