fakeroot create_shar() {
	cat << "EOF" > output_script.sh
#!/bin/bash

# NOTE: how to use the appended data in the tail of script, 
payload_offset=$(($(grep -na -m1 "^MARKER:$" $0|cut -d':' -f1) + 1))

printf "Extracting SDK..."
tail -n +$payload_offset $0| $SUDO_EXEC tar xJ -C $target_sdk_dir
echo "done"

# NOTE: todo something here until end of the script
exit 0

# make a tag at the end of script for appending data, whic also will be found to extract data
MARKER:
EOF
	# append the SDK tarball
	cat somedata_to_append.tar.gz >> output_script.sh
}
