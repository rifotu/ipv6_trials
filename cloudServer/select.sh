#!/bin/bash

# --insecure = -k : disable https certificate validation
# --verbose = -v : detailed information

curl --verbose --insecure --data-urlencode q@select.sql "https://rifo.cartodb.com/api/v2/sql/?api_key=bdfd89e9d8828cdf0a229a17d60382d5100eea3a"
