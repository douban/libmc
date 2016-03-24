# Benchmark for golibmc

(compared with [bradfitz/gomemcache](https://github.com/bradfitz/gomemcache/))

## How to run the benchmark

    go get github.com/bradfitz/gomemcache/memcache
    go get github.com/douban/libmc/golibmc
    cd misc/gomcbench
    go test -bench .

## Results

result based on a MacBook Pro (Retina, 13-inch, Late 2013) w/ 8GB Mem, and a 2.4 GHz Intel Core i5 CPU.

    BenchmarkGolibmcGetSingleSmallValue-4   	   30000	     46638 ns/op
    BenchmarkGomemcacheGetSingleSmallValue-4	   20000	     87720 ns/op

    BenchmarkGolibmcGetSingleLargeValue-4   	   20000	     83470 ns/op
    BenchmarkGomemcacheGetSingleLargeValue-4	   10000	    110609 ns/op

    BenchmarkGolibmcGet10SmallValues-4      	   10000	    137134 ns/op
    BenchmarkGomemcacheGet10SmallValues-4   	    5000	    329235 ns/op

    BenchmarkGolibmcGet100SmallValues-4     	    3000	    382400 ns/op
    BenchmarkGomemcacheGet100SmallValues-4  	    2000	    894691 ns/op

    BenchmarkGolibmcGet1000SmallValues-4    	     500	   2212252 ns/op
    BenchmarkGomemcacheGet1000SmallValues-4 	     200	   6924752 ns/op

    BenchmarkGolibmcGet10LargeValues-4      	    5000	    284501 ns/op
    BenchmarkGomemcacheGet10LargeValues-4   	    3000	    546686 ns/op

    BenchmarkGolibmcGet100LargeValues-4     	     500	   2892901 ns/op
    BenchmarkGomemcacheGet100LargeValues-4  	     500	   3534152 ns/op

    BenchmarkGolibmcGet1000LargeValues-4    	      50	  21692412 ns/op
    BenchmarkGomemcacheGet1000LargeValues-4 	      50	  22605197 ns/op
