#!/bin/bash
# Save the historical last dmesg logs in a file with a limit size 
# the BLOCK tag is "LOG START" / "LOG END"

LOG_FILE="/fii-log/lastdmesg.log"
MAX_SIZE_MB=2
MAX_SIZE_BYTES=$((MAX_SIZE_MB * 1024 * 1024))

# Function: Clear all logs saved in the file
clear_log() {
    echo > "$LOG_FILE"
}

# Function: Delete the oldest log block
delete_oldest_block() {
    TMP_FILE=$(mktemp)
    awk '
    BEGIN {found=0}
    /^=== LOG START/ {
        if (!found) {
            found=1
            next
        }
    }
    found==1 && /^=== LOG END/ {
        found=2
        next
    }
    found==0 || found==2 {
        print
    }
    ' "$LOG_FILE" > "$TMP_FILE"
    mv "$TMP_FILE" "$LOG_FILE"
}

# Function: Save a new log block (with size estimation)
save_log() {
    if [ "$ALLOW_SAVE_LOG" != "true" ]; then
        echo "This function must be called by the OS automation layer."
        exit 1
    fi

    TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')
    TMP_LOG=$(mktemp)

    # Generate new log block into temp file
    {
        echo "=== LOG START $TIMESTAMP ==="
        fii-lastdmesg
        echo "=== LOG END $TIMESTAMP ==="
        echo ""
    } > "$TMP_LOG"

    NEW_SIZE=$(stat -c%s "$TMP_LOG")
    CURRENT_SIZE=$(stat -c%s "$LOG_FILE" 2>/dev/null || echo 0)

    # Delete old blocks until space is enough
    while [ $((CURRENT_SIZE + NEW_SIZE)) -gt "$MAX_SIZE_BYTES" ]; do
        echo "Log file will exceed ${MAX_SIZE_MB}MB. Removing oldest block..."
        delete_oldest_block
        CURRENT_SIZE=$(stat -c%s "$LOG_FILE" 2>/dev/null || echo 0)
    done

    # Append new block
    cat "$TMP_LOG" >> "$LOG_FILE"
    rm -f "$TMP_LOG"
}

# Function: List all log blocks
list_logs() {
    awk '
    /^=== LOG START/ {
        count++
        print "Block " count ": " $4, $5
    }
    ' "$LOG_FILE"
}

# Function: View log block by timestamp
view_by_timestamp() {
    SEARCH_TIME="$1"
    awk -v ts="$SEARCH_TIME" '
        $0 ~ "=== LOG START " ts " ===" {in_block=1; print; next}
        in_block && /^=== LOG END/ {print; exit}
        in_block {print}
    ' "$LOG_FILE"
}

# Function: View log block by index
view_by_index() {
    TARGET_INDEX="$1"
    awk -v target="$TARGET_INDEX" '
    BEGIN { in_block = 0 }

    /^=== LOG START/ {
        block++
        in_block = (block == target)
    }

    in_block {
        print
        if (/^=== LOG END/) {
            exit
        }
    }
    ' "$LOG_FILE"
}

# Function: Auto-detect input type and view accordingly
view_auto() {
    INPUT="$1"
    if [[ "$INPUT" =~ ^[0-9]+$ ]]; then
        view_by_index "$INPUT"
    elif [[ "$INPUT" =~ ^[0-9]{4}-[0-9]{2}-[0-9]{2}\ [0-9]{2}:[0-9]{2}:[0-9]{2}$ ]]; then
        view_by_timestamp "$INPUT"
    else
        echo "Invalid input. Use either a block number or a timestamp in format: YYYY-MM-DD HH:MM:SS"
        exit 1
    fi
}

# Main logic
case "$1" in
    save)
        save_log
        ;;
    list)
        list_logs
        ;;
    view)
        if [ -z "$2" ]; then
            echo "Usage: $0 view <block_number | \"timestamp\">"
            exit 1
        fi
        view_auto "$2"
        ;;
    clr | clear)
        clear_log
        ;;
    *)
        echo "Usage:"
        echo "  $0 save                 # Save new log block, it is called by the OS automaticlly, prohibit to call manually"
        echo "  $0 list                 # List all log blocks"
        echo "  $0 view <index|ts>      # View block by index or timestamp"
        echo "  $0 clear/clr            # Clear all logs saved in the $LOG_FILE"
        exit 1
        ;;
esac
