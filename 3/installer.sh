#/bin/sh

echo "I am an installer, using $(cat $1) keys"
cat $1 > /mytool.config