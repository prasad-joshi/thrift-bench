namespace cpp example

struct LeafBenchmarkData {
	1: i64 request_id; /* ever increasing ID */

	/* latency calculations */
	2: i64 leaf_latency;
	3: i64 io_latency;
	4: i64 cpu_consumed;

	/* flags to bypass nodes */
	5: bool bypass_io;

	/* payload management */
	6: string data;
	7: i64    data_size;
	8: bool   empty_data_on_resp;
}

service LeafBenchmark {
	LeafBenchmarkData key_put(1: string key, 2: LeafBenchmarkData ldb);
}
