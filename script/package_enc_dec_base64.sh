create_shar()
{
    local dir="$1"
    #package and encrypt
    cat > output_script.sh <<'EOF'
#!/bin/bash

set -e

payload_offset=$(( $(grep -na -m1 "^MARKER:$" "$0" | cut -d':' -f1) + 1 ))

echo "Extracting package..."

tail -n +$payload_offset "$0" | \
        openssl enc -d -aes-256-cbc -pbkdf2 | \
        tar xz

echo "done"

exit 0

MARKER:
EOF
    tar czf - "$dir" | \
        openssl enc -aes-256-cbc -pbkdf2 -salt >> output_script.sh

    chmod +x output_script.sh

    base64 -w 0 output_script.sh > output_script.sh.b64
    # base64 -d output_script.sh.b64 > output_script.sh
}

enc(){
        tar czf - "$dir" |
        openssl enc -aes-256-cbc -pbkdf2 -salt |
        base64 -w 0 > sdk.base64
}

dec(){
        base64 -d sdk.base64 |
        openssl enc -d -aes-256-cbc -pbkdf2 |
        tar xz
}
