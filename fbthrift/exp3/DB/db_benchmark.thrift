namespace cpp example

struct DBBenchmarkData {
	1: i64 request_id;
	
	/* latency calculations */
	2: i64 leaf_latency;
	3: i64 io_latency;
	4: i64 cpu_consumed;

	5: string data;
}

service DBBenchmark {
	void write(1: string filename, 2: string data);
	DBBenchmarkData read(1: i64 request_id, 2: string filename, 3: i32 size);
}
