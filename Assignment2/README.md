## Dependencies
### Requires Boost Regex library.
To get started on Boost Regex, follow the instructions [here](http://www.boost.org/doc/libs/1_61_0/more/getting_started/unix-variants.html) for Unix variants or [here](http://www.boost.org/doc/libs/1_54_0/more/getting_started/windows.html) for Windows.



## Exeucting
To run, enter the following commands in order in terminal:

```
 $ rm output.txt   #This is to ensure that output.txt does not exist beforehand
 $ sudo g++ -I /path/to/boost_1_61_0 HazaliCrawler.cpp -o crawler \-L /path/to/lib/ -lboost_regex-mt -std=c++11;
 $ ./crawler;
```

Replace `path/to/boost_1_60_0` (only the `path/to` section, leave the boost_1_61_0) and (`path/to/lib`) with the path in your machine
