namespace cpp example

struct BenchmarkData {
	1: i64 request_id; /* ever increasing ID */

	/* latency calculations */
	2: i64 client_latency;
	3: i64 db_latency;
	4: i64 dict_latency;
	5: i64 leaf_latency;
	6: i64 io_latency;

	/* flags to bypass nodes */
	7:  bool bypass_dict;
	8:  bool bypass_cpu;
	9:  bool bypass_leaf;
	10: bool bypass_io;

	/* payload management */
	11: string data;
	12: i64    data_size;
	13: bool   empty_data_on_resp;
}

service Benchmark {
	BenchmarkData key_put(1: string key, 2: BenchmarkData bd);
}