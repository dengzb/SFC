# SFC
Modified code of Segment Frequent Compression Cache

***************************************************
This reposity is used to store the code which I write/modified for my final project of Computer Architecture.

This project is based on the following publications:


And the tools I used is Pin tools of Intel Inc.

I read several pin tools to know how to implement my own cache hierarchy and simulate it with Pin platform.
 

allCache.cpp

Code to configure the cache is in allCache.cpp. Including the cache level, cache size and cache block size is defined in this file. And Pin-cache.H is included.

Pin_cache.H

Details to implement the cache hierarchy is included in Pin_cache.H. It defines the cache size in details, including how an "access" operation is implemented. And in this file, the replacement method is updated to LRU from Direct-Mapped.
