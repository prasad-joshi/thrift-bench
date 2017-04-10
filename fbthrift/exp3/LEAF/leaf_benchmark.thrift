namespace cpp example

struct LeafBenchmarkData {
	/* latency calculations */
	2: i64 io_latency;
	3: i64 cpu_consumed;

	/* payload management */
	4: string data;
}

service LeafBenchmark {
	void write(1: string filename, 2: string data);
	LeafBenchmarkData read(1: string filename, 2: i32 size);
}
