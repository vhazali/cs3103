To run, enter the following commands in order:
rm output.txt;
sudo g++ -I /usr/local/boost/boost_1_61_0 HazaliCrawler.cpp -o crawler \-L /usr/local/lib/ -lboost_regex-mt -std=c++11;
./crawler;