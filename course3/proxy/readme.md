**download:**
```
git clone <url this repo>
```
**build:**
```
cd Operating-systems/course3/proxy/
make all
```

**run:**
```
./proxy -flag:
            -p <номер порта>
            -c <размер кэша в МБ>
            -h хелп
```
пример:
```
./proxy -p 8080
```


1 тест:
```
curl -x http://localhost:8080 -O http://ccfit.nsu.ru/~rzheutskiy/test_files/50mb.dat
```

2 тест:
```
wget -e use_proxy=yes -e http_proxy=http://localhost:8080 \
     -O client1.zip \
     http://xcal1.vodafone.co.uk/1GB.zip
```

ждем 5%
запускаем в другой сессии
```
wget -e use_proxy=yes -e http_proxy=http://localhost:8080 \
     -O client2.zip \
     http://xcal1.vodafone.co.uk/1GB.zip
```
