PDS
===

A tiny, but heavily optimized utility for netflow aggregation and sorting - a school project to networking course

The application reads an input file (or a directory with input files) with captured netflow data and aggregates
them by source/destination IP address or source/destination port and optionally sorts them by packets or by bytes.

The key optimizations involved are:
- memory mapping to avoid copying file contents from page cache to a buffer in application
- to improve the performance of memory mapping even more, the program uses a special helper thread that reads
  a single byte of data from every page of the file. This strategy forces the operating system to preload
  next page of file data, while the current page is being processed in the application.
  For more information on this topic see: http://stackoverflow.com/questions/19402771/fastest-way-of-reading-a-file-in-linux
  and http://stackoverflow.com/questions/14822151/in-c-what-is-the-fastest-way-to-load-a-large-binary-1gb-4gb-file-into-memor
- aggregation of ports is done using a single linear array of 65536 elements instead of a hash table to avoid unnecessary overhead.
- since this array is indexed by a port field of each netflow record the precise order in which the elements of this array will
  be accessed cannot be predicted by HW. However this order can still be deduced since each port (position in the array) is known
  and is written in the input file. This results in using SW prefetching (special prefetching instructions of the processor)
  to improve the performance of port aggregation.
- heavy use of templates to push out various if-s (or other decision making code) to higher levels
  of the program and thus to free the hot spots from unnecessary computations
- the output of the aggregation and sorting is first printed to an internal buffer, which is exactly 256kB large to match
  the optimum buffer size as precisely as possible. This buffer is then printed to stdout in one call to POSIX write function.
  This scheme gives a very good performance and there seems to be no difference whether redirecting stdout to /dev/null or to regular file.
- the aggregation by IP addresses is parallelized based on the number of available processor cores.
  Each core aggregates data from a single input file and the results of all threads are merged together at the end of the program.
- before aggregating by IP addresses the number of buckets in std::unordered_map is preallocated based on the number of netflow records
  in the file.
