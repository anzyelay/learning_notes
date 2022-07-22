#!/bin/bash

curl -s https://api.github.com/orgs/elementary/repos\?per_page\=100\&page\=1 | perl -ne 'print "$1\n" if (/"clone_url": "([^"]+)/)'
curl -s https://api.github.com/orgs/elementary/repos\?per_page\=100\&page\=2 | perl -ne 'print "$1\n" if (/"clone_url": "([^"]+)/)'
