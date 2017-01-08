# SFC
Modified code of Segment Frequent Compression Cache

***************************************************
This reposity is used to store the code which I write/modified for my final project of Computer Architecture.

This project is based on the following publications:

Alameldeen A R, Wood D A. Adaptive cache compression for high-performance processors[C]//Computer Architecture, 2004. Proceedings. 31st Annual International Symposium on. IEEE, 2004: 212-223.

Franaszek P, Robinson J, Thomas J. Parallel compression with cooperative dictionary construction[C]//Data Compression Conference, 1996. DCC’96. Pro-ceedings. IEEE, 1996: 200-209.

Ahn E, Yoo S M, Kang S M S. Effective algorithms for cache-level compression[C]//Proceedings of the 11th Great Lakes symposium on VLSI. ACM, 2001:89-92.
****************************************************
And the tools I used is Pin tools of Intel Inc.

I read several pin tools to know how to implement my own cache hierarchy and simulate it with Pin platform.
 

allCache.cpp

Code to configure the cache is in allCache.cpp. Including the cache level, cache size and cache block size is defined in this file. And Pin-cache.H is included.

Pin_cache.H

Details to implement the cache hierarchy is included in Pin_cache.H. It defines the cache size in details, including how an "access" operation is implemented. And in this file, the replacement method is updated to LRU from Direct-Mapped.


To simulate the SFC, we need to copy the Pin_cache.H to /source/include/pin/ , and copy allCache.cpp to /source/tools/Memory/ , then "make" in the /source/tools/Memory/, and use normal pin command to run the program with allCache tool. 

And ZhengbiaoDeng_Final_project.pdf is my project report.